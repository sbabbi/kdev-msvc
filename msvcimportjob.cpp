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

MsvcImportSolutionJob::MsvcImportSolutionJob(MsvcSolutionItem* dom) :
    m_dom(dom),
    m_solutionPath(dom->path()),
    m_futureWatcher(new QFutureWatcher<void>(this))
{
    connect(m_futureWatcher, &QFutureWatcher<void>::finished,
            this, [this]() { emitResult(); } );
    
    setCapabilities(KJob::Killable);
    setObjectName(i18n("Solution Import: %1", m_dom->project()->name()));
}

MsvcImportSolutionJob::~MsvcImportSolutionJob()
{
    m_futureWatcher->cancel();
    m_futureWatcher->waitForFinished();
}

void MsvcImportSolutionJob::start()
{
    QFuture<void> future = QtConcurrent::run(this, &MsvcImportSolutionJob::run);
    m_futureWatcher->setFuture(future);
}

bool MsvcImportSolutionJob::doKill()
{
    m_futureWatcher->cancel();
    m_futureWatcher->waitForFinished();
    
    return true;
}

void MsvcImportSolutionJob::parseProject(const QString & relativePath)
{
    const KDevelop::Path path (m_solutionPath.parent(), relativePath);
    QScopedPointer< MsvcProjectParser > parser( CreateProjectParser( path, m_dom->project() ) );
    
    // Although MsvcProjectParser can work asynchronously, I could not figure out
    // how to protect access to IProject. Run it synchrnously for now.
    parser->run();
    
    auto future =  parser->getFuture();
    
    if ( future.isResultReadyAt(0) )
    {
        m_dom->appendRow( future.result() );
    }
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

            parseProject( projectPath.replace('\\','/') );
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
}
