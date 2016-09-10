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

#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QUuid>
#include <QXmlStreamReader>

#include <QtConcurrent/QtConcurrentRun>

#include <KLocalizedString>
#include <KCompositeJob>

#include <interfaces/icore.h>
#include <interfaces/iproject.h>

MsvcImportJob::MsvcImportJob(MsvcProjectItem* dom) :
    m_dom(dom),
    m_file(dom->path()),
    m_futureWatcher(new QFutureWatcher<void>(this)),
    m_canceled(false)
{
    connect(m_futureWatcher, &QFutureWatcher<void>::finished,
            this, &MsvcImportJob::emitResult );
    
    setObjectName(i18n("Project Import: %1", m_dom->project()->name()));
    
    connect(KDevelop::ICore::self(), &KDevelop::ICore::aboutToShutdown,
            this, &MsvcImportJob::aboutToShutdown );
}

void MsvcImportJob::start()
{
    QFuture<void> future = QtConcurrent::run(this, &MsvcImportJob::run);
    m_futureWatcher->setFuture(future);
}

bool MsvcImportJob::doKill()
{
    m_futureWatcher->cancel();
    m_canceled = true;

    setError(1);
    setErrorText(i18n("Project import canceled."));

    m_futureWatcher->waitForFinished();
    
    return true;
}

void MsvcImportJob::aboutToShutdown()
{
    kill();
}

void MsvcImportJob::run()
{    
    QFile file( m_file.toLocalFile() );
    if (! file.open(QFile::ReadOnly) )
    {
        qCWarning(KDEV_MSVC) << "Failed to open " << m_file.toLocalFile();
        return;
    }
    
    QXmlStreamReader reader(&file);
    
    for ( ;reader.readNextStartElement(); reader.skipCurrentElement() )
    {
        if ( reader.name().compare("VisualStudioProject", Qt::CaseInsensitive) == 0 )
        {
            parseVcProj(reader);
        }
    }
    
    // Add the project file itself
    new KDevelop::ProjectFileItem( m_dom->project(), m_file, m_dom );
}

void MsvcImportJob::parseFileList(KDevelop::ProjectBaseItem* parent,
                                  QXmlStreamReader& reader) const
{
    while( reader.readNextStartElement() )
    {
        if ( m_canceled )
            return;
        
        if ( reader.name().compare("File", Qt::CaseInsensitive) == 0 )
        {
            QString relativePath = reader.attributes().value("RelativePath").toString().replace('\\', '/');

            KDevelop::Path path (m_file.parent(), relativePath );

            new KDevelop::ProjectFileItem(parent->project(), path, parent );
            
            reader.skipCurrentElement();
        }
        else if ( reader.name().compare("Filter", Qt::CaseInsensitive) == 0 )
        {
            MsvcFilterItem * filter = new MsvcFilterItem( parent->project(),
                                                          reader.attributes().value("Name").toString(),
                                                          parent );
            
            parseFileList( filter, reader );
        }
    }
}

void MsvcImportJob::parseVcProj(QXmlStreamReader& reader) const
{
    QString projectName = reader.attributes().value("Name").toString();
    QUuid projectUuid = reader.attributes().value("ProjectGUID").toString();
    
    m_dom->rename( projectName );
    m_dom->setUuid( projectUuid );
   
    while ( reader.readNextStartElement() )
    {
        if ( reader.name().compare("Files", Qt::CaseInsensitive) == 0 )
        {
            parseFileList( m_dom, reader );
        }
        else if ( reader.name() == "Configurations" )
        {
            while ( reader.readNextStartElement() )
            {
                if ( reader.name() == "Configuration" )
                {
                    m_dom->addConfiguration( parseConfig( reader ) );
                }
                else
                {
                    reader.skipCurrentElement();
                }
            }
        }
        else
        {
            reader.skipCurrentElement();
        }
    }
    
    MsvcProjectConfig config = m_dom->getCurrentConfig();
    switch ( config.configurationType )
    {
        case MsvcProjectConfig::Unknown:
        case MsvcProjectConfig::Generic:
            new KDevelop::ProjectTargetItem( m_dom->project(),
                                             projectName,
                                             m_dom );
        default:
        case MsvcProjectConfig::Application:
            new MsvcExecutableTargetItem( m_dom->project(),
                                          projectName,
                                          m_dom );
            break;
        case MsvcProjectConfig::DynamicLibrary:
        case MsvcProjectConfig::StaticLibrary:
            new KDevelop::ProjectLibraryTargetItem( m_dom->project(),
                                                    projectName,
                                                    m_dom );
            break;
    }
}

MsvcImportSolutionJob::MsvcImportSolutionJob(MsvcSolutionItem* dom) :
    m_dom(dom),
    m_solutionPath(dom->path()),
    m_futureWatcher(new QFutureWatcher<void>(this)),
    m_finished(false)
{
    connect(m_futureWatcher, &QFutureWatcher<void>::finished,
            this, &MsvcImportSolutionJob::reconsider );
    
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

void MsvcImportSolutionJob::addProject(const QString & relativePath)
{
    MsvcProjectItem * result = new MsvcProjectItem( m_dom->project(),
                                                    KDevelop::Path(m_solutionPath.parent(), relativePath),
                                                    m_dom );
    KJob * job = new MsvcImportJob( result );
    addSubjob(job);
    job->start();
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
    
    while ( line = file.readLine(), !file.atEnd() )
    {
        const QString projectStartTag = "Project";
        const QString projectEndTag = "EndProject";
        
        const QString globSectStart = "GlobalSection(ProjectConfigurationPlatforms) = postSolution";
        const QString globSectEnd = "EndGlobalSection";
        
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
            
            QString nextLine = file.readLine();
            
            if ( !result.isValid() || !nextLine.startsWith("EndProject") )
                continue;

//             QUuid _ = result.captured(1);
            QString projectName = result.captured(2);
            QString projectPath = result.captured(3);
//             QUuid projectUUID = result.captured(4);
           
            QMetaObject::invokeMethod(this,
                                      "addProject", 
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, projectPath.replace('\\','/') ) );
        }
        else if ( line.trimmed().startsWith(globSectStart) )
        {
            while ( line = file.readLine(), !(file.atEnd() || line.trimmed().startsWith(globSectEnd) ) )
            {
                //TODO implement
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
