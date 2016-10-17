/* KDevelop MSVC Support
 *
 * Copyright 2015 Ennio Barbaro <enniobarbaro@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "msvcimportjob.h"
#include "debug.h"
#include "msvcmodelitems.h"
#include "msvcprojectparser.h"

#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QThreadPool>
#include <QUuid>
#include <QXmlStreamReader>

#include <QtConcurrent/QtConcurrentRun>

#include <KLocalizedString>
#include <KCompositeJob>

#include <interfaces/icore.h>
#include <interfaces/iproject.h>

namespace
{
MsvcProjectParser * CreateProjectParser( KDevelop::Path const & path, KDevelop::IProject * project)
{
    if ( path.lastPathSegment().endsWith(".vcproj", Qt::CaseInsensitive) )
    {
        return new MsvcVcProjParser(path, project);
    }
    else if ( path.lastPathSegment().endsWith(".vcxproj", Qt::CaseInsensitive) )
    {
        return new MsvcVcxProjParser(path, project);
    }
    else
    {
        qCWarning(KDEV_MSVC) << "Unknown project file extension: (" << path << ")";
        return nullptr;
    }
}
}

MsvcImportJob::MsvcImportJob(KDevelop::Path const & path, KDevelop::IProject * project) :
    m_parser( CreateProjectParser(path, project) ),
    m_futureWatcher(new QFutureWatcher<MsvcProjectItem*>(this))
{
    connect(m_futureWatcher, &QFutureWatcher<void>::finished,
            this, &MsvcImportJob::emitResult );

    setCapabilities(KJob::Killable);
    setObjectName(i18n("Parsing: %1", path.lastPathSegment()) );
    
    connect(KDevelop::ICore::self(), &KDevelop::ICore::aboutToShutdown,
            this, &MsvcImportJob::aboutToShutdown );

    if (m_parser)
    {
        m_parser->setAutoDelete(false);
        m_futureWatcher->setFuture(m_parser->getFuture());
    }

}

MsvcImportJob::~MsvcImportJob()
{
    delete m_parser;
}

void MsvcImportJob::start()
{
    if ( m_parser )
    {
        QThreadPool::globalInstance()->start( m_parser );
    }
}

bool MsvcImportJob::doKill()
{
    m_futureWatcher->cancel();

    setError(1);
    setErrorText(i18n("Project import canceled."));

    m_futureWatcher->waitForFinished();
    
    return true;
}

void MsvcImportJob::aboutToShutdown()
{
    kill();
}

MsvcImportSolutionJob::MsvcImportSolutionJob(MsvcSolutionItem* dom) :
    m_dom(dom),
    m_solutionPath(dom->path()),
    m_futureWatcher(new QFutureWatcher<void>(this)),
    m_finished(false)
{
    connect(m_futureWatcher, &QFutureWatcher<void>::finished,
            this, &MsvcImportSolutionJob::reconsider );
    
    setCapabilities(KJob::Killable);
    setObjectName(i18n("Solution Import: %1", m_dom->project()->name()));
}

void MsvcImportSolutionJob::start()
{
    QFuture<void> future = QtConcurrent::run(this, &MsvcImportSolutionJob::run);
    m_futureWatcher->setFuture(future);
}

void MsvcImportSolutionJob::slotResult(KJob * job)
{
    KCompositeJob::slotResult(job);
    reconsider();
}

bool MsvcImportSolutionJob::doKill()
{
    QList<KJob*> jobs = subjobs();
    
    // Try to kill them all!
    auto zombies_start = std::partition( jobs.begin(), jobs.end(), [](KJob* j) { return j->kill(); } );
    
    // Remove the ones that we actually killed
    std::for_each( jobs.begin(), zombies_start, [&](KJob* j) { removeSubjob(j); } );
    
    // If no zombies left, report success.
    return zombies_start == jobs.end();
}

void MsvcImportSolutionJob::addProject(const QString & relativePath)
{
    MsvcImportJob * job = new MsvcImportJob( KDevelop::Path(m_solutionPath.parent(), relativePath), m_dom->project() );
    addSubjob(job);
    job->start();
    
    using WatcherType = QFutureWatcher<MsvcProjectItem*>;
    
    WatcherType * watcher = job->futureWatcher();
    
    connect( watcher, static_cast< void (QFutureWatcherBase::*)(int) > (&QFutureWatcherBase::resultReadyAt),
             this, [this, watcher](int index) { m_dom->appendRow( watcher->resultAt(index) ); } );
}

void MsvcImportSolutionJob::run()
{
    QFile file( m_solutionPath.toLocalFile() );
    if (! file.open(QFile::ReadOnly) )
    {
        qCWarning(KDEV_MSVC) << "Failed to open " << m_solutionPath;
        return;
    }
    
    QString line; 
    
    // TODO split this mess into multiple functions in a class.
    while ( line = file.readLine(), !file.atEnd() )
    {
        const QString projectStartTag = "Project";
        const QString projectEndTag = "EndProject";
        
        const QString globStart = "Global";
        const QString globEnd = "EndGlobal";
        
        if ( line.trimmed().startsWith(projectStartTag) )
        {
            // For now greedy parsing using regexs
            static QRegularExpression regex(
                "Project"
                "\\("
                    R"X("({[A-F0-9\-]+})")X"
                "\\)"
                "\\s*"
                "="
                "\\s*"
                    R"z("([A-Z0-9_a-z]+)")z"
                "\\s*,\\s*"
                    R"z("([A-Z0-9_a-z\\\./]+)")z"
                "\\s*,\\s*"
                    R"X("({[A-F0-9\-]+})")X"
                "\\s*");

            QRegularExpressionMatch result = regex.match( line );

            if (! result.isValid() )
            {
                continue; // Ignore stuff we do not know about
            }

            QString nextLine;

            // Skip dependency description and other amenities..
            do
            {
                nextLine = file.readLine();
            }
            while ( !(file.error() || file.atEnd() || nextLine.startsWith("EndProject") ) );

//             QUuid _ = result.captured(1);
            QString projectName = result.captured(2);
            QString projectPath = result.captured(3);
//             QUuid projectUUID = result.captured(4);

            qCDebug(KDEV_MSVC) << "About to parse project file: " << projectPath;

            QMetaObject::invokeMethod(this,
                                      "addProject", 
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, projectPath.replace('\\','/') ) );
        }
        else if ( line.trimmed().startsWith(globStart) )
        {
            while ( line = file.readLine(), !(file.error() || file.atEnd() || line.trimmed().startsWith(globEnd) ) )
            {
                const QRegularExpression globalSectionStart (R"(GlobalSection\s*\(([a-zA-Z]+)\)\s*=\s*([a-zA-Z]+))");
                const QString globalSectionEnd = "EndGlobalSection";

                QRegularExpressionMatch result = globalSectionStart.match( line.trimmed() );

                if ( !result.isValid() )
                    continue;

                if ( result.captured(1) == "SolutionConfigurationPlatforms" && 
                     result.captured(2) == "preSolution" )
                {
                    while ( line = file.readLine(), !(file.error() || file.atEnd() || line.startsWith(globalSectionEnd) ) )
                    {
                        const QRegularExpression configRegex (R"(([a-zA-Z0-9_]+\|[a-zA-Z0-9_]+)\s*=\s*[a-zA-Z0-9_]+\|[a-zA-Z0-9_]+)");

                        QRegularExpressionMatch cfgMatch = configRegex.match( line.trimmed() );

                        if ( cfgMatch.isValid() )
                        {
                            m_dom->addConfiguration( cfgMatch.captured(1) );
                        }
                    }
                }
            }
        }
        // else skip line
    }
    
    m_finished = true;
}

void MsvcImportSolutionJob::reconsider()
{
    if ( m_finished && subjobs().isEmpty() )
    {
        m_finished = false;
        emitResult();
    }
}
