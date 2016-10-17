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

#include "msvcprojectparser.h"
#include "msvcmodelitems.h"
#include "debug.h"

#include <QFile>
#include <QXmlStreamReader>

namespace
{
    
// Not sure if we have something like this already..
template<class Predicate>
KDevelop::ProjectBaseItem * findItem( KDevelop::ProjectBaseItem * root, Predicate pred )
{
    QList<KDevelop::ProjectBaseItem*> children = root->children();
    
    auto it = std::find_if( children.begin(), children.end(), pred );
    
    if ( it != children.end() )
        return *it;
    
    for ( KDevelop::ProjectBaseItem * item : children )
    {
        if ( auto result = findItem(item, pred) )
            return result;
    }
    
    return nullptr;
}

}

void MsvcProjectParser::run()
{
    if (! projectPath().isLocalFile() )
    {
        qCWarning(KDEV_MSVC) << "Reading non-local file is not supported yet. (" << projectPath() << ")";
        m_promise.reportCanceled();
        m_promise.reportFinished();
        return;
    }
    
    QFile file( projectPath().toLocalFile() );
    if (! file.open(QFile::ReadOnly) )
    {
        qCDebug(KDEV_MSVC) << "Cannot to open " << projectPath().toLocalFile();
        m_promise.reportCanceled();
        m_promise.reportFinished();
        return;
    }
    
    qCDebug(KDEV_MSVC) << "Reading: " << file.fileName();
    
    QXmlStreamReader reader(&file);
    
    auto result = parse(reader);
    if (!result)
    {
        m_promise.reportCanceled();
        m_promise.reportFinished();
        return;
    }

    // Add the project file itself
    new KDevelop::ProjectFileItem( result->project(), projectPath(), result.get() );
    
    m_promise.reportResult( result.release() );
    m_promise.reportFinished();
}

std::unique_ptr< MsvcProjectItem > MsvcVcProjParser::parse(QXmlStreamReader & reader)
{
    std::unique_ptr< MsvcProjectItem > result( new MsvcProjectItem(project(), projectPath() ) );

    for ( ;reader.readNextStartElement(); reader.skipCurrentElement() )
    {
        if ( reader.name().compare("VisualStudioProject", Qt::CaseInsensitive) == 0 )
        {
            parseVisualStudioProject(reader, result.get());
        }
    }
    
    return result;
}

void MsvcVcProjParser::parseFileList(KDevelop::ProjectBaseItem* parent, QXmlStreamReader& reader) const
{
    while( reader.readNextStartElement() )
    {
        if ( isCanceled() )
            return;
        
        if ( reader.name().compare("File", Qt::CaseInsensitive) == 0 )
        {
            QString relativePath = reader.attributes().value("RelativePath").toString().replace('\\', '/');

            const KDevelop::Path path (projectPath().parent(), relativePath );

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

void MsvcVcProjParser::parseVisualStudioProject(QXmlStreamReader& reader, MsvcProjectItem * proj)
{
    QString projectName = reader.attributes().value("Name").toString();
    QUuid projectUuid = reader.attributes().value("ProjectGUID").toString();
    
    proj->rename( projectName );
    proj->setUuid( projectUuid );
   
    while ( reader.readNextStartElement() )
    {
        if ( reader.name().compare("Files", Qt::CaseInsensitive) == 0 )
        {
            parseFileList( proj, reader );
        }
        else if ( reader.name() == "Configurations" )
        {
            while ( reader.readNextStartElement() )
            {
                if ( reader.name() == "Configuration" )
                {
                    proj->addConfiguration( parseConfig( reader ) );
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
    
    MsvcProjectConfig config = proj->getCurrentConfig();
    switch ( config.configurationType )
    {
        case MsvcProjectConfig::Unknown:
        case MsvcProjectConfig::Generic:
            new KDevelop::ProjectTargetItem( proj->project(),
                                             projectName,
                                             proj );
        default:
        case MsvcProjectConfig::Application:
            new MsvcExecutableTargetItem( proj->project(),
                                          projectName,
                                          proj );
            break;
        case MsvcProjectConfig::DynamicLibrary:
        case MsvcProjectConfig::StaticLibrary:
            new KDevelop::ProjectLibraryTargetItem( proj->project(),
                                                    projectName,
                                                    proj );
            break;
    }
}

std::unique_ptr< MsvcProjectItem > MsvcVcxProjParser::parse( QXmlStreamReader & )
{
    std::unique_ptr< MsvcProjectItem > result( new MsvcProjectItem(project(), projectPath() ) );

    // For now, completely ignore the vcxproj, focus on the filter file
    KDevelop::Path filterFileName = projectPath();
    filterFileName.setLastPathSegment( filterFileName.lastPathSegment() + ".filters" );
    
    if ( !filterFileName.isLocalFile() )
    {
        qCWarning(KDEV_MSVC) << "Cannot parse non-local file: (" << filterFileName << ")";
        return false;
    }
    
    QFile filterFile( filterFileName.toLocalFile() );
    if ( !filterFile.open(QFile::ReadOnly) )
    {
        qCWarning(KDEV_MSVC) << "Cannot open: " << filterFile.fileName();
        return false;
    }
    
    qCDebug(KDEV_MSVC) << "Parsing filter file: " << filterFile.fileName();
    QXmlStreamReader filterReader( &filterFile );
    parseFilterFile( filterReader, result.get() );
    
    return result;
}

void MsvcVcxProjParser::parseFilterFile(QXmlStreamReader & reader, MsvcProjectItem * result)
{
    while ( reader.readNextStartElement() )
    {
        if ( reader.name() == "Project" )
        {
            while ( reader.readNextStartElement() )
            {
                if ( reader.name() == "ItemGroup" )
                {
                    parseItemGroup( reader, result );
                }
            }
        }
        else
        {
            reader.skipCurrentElement();
        }
    }
}

void MsvcVcxProjParser::parseItemGroup(QXmlStreamReader & reader, MsvcProjectItem * proj)
{
    while ( reader.readNextStartElement() )
    {
        if ( reader.name() == "Filter" )
        {
            new MsvcFilterItem( proj->project(),
                                reader.attributes().value("Include").toString(),
                                proj );
            
            reader.skipCurrentElement();
        }
        else if ( reader.name() == "ClInclude" || 
                  reader.name() == "ClCompile" || 
                  reader.name() == "ResourceCompile" || 
                  reader.name() == "Text" )
        {
            QString relativePath = reader.attributes().value("Include").toString().replace('\\', '/');
            
            KDevelop::ProjectBaseItem * parent = proj;
            
            // Try to see if it has an associated filter
            while ( reader.readNextStartElement() )
            {
                if ( reader.name() == "Filter" )
                {
                    QString filterName = reader.readElementText(QXmlStreamReader::SkipChildElements);
                    
                    qCDebug(KDEV_MSVC) << "Filter for item: " << relativePath << filterName;

                    auto findFiltPred = [&filterName](KDevelop::ProjectBaseItem * item)
                                        {
                                            return dynamic_cast<MsvcFilterItem*>(item) &&
                                                    item->baseName() == filterName;
                                        };
                    
                    if ( KDevelop::ProjectBaseItem * filterItem = findItem( proj, findFiltPred ) )
                    {                                                                       
                        qCDebug(KDEV_MSVC) << "Found filter: " << filterItem->baseName() << "for" << relativePath;
                 
                        parent = filterItem;
                    }
                }
                else
                {
                    reader.skipCurrentElement();
                }
            }

            const KDevelop::Path path (projectPath().parent(), relativePath );

            new KDevelop::ProjectFileItem(proj->project(), path, parent );
        }
        else
        {
            reader.skipCurrentElement();
        }
    }
}

