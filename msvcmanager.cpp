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

#include "msvcmanager.h"
#include "msvcbuilder.h"
#include "msvcconfig.h"
#include "msvcbuilderpreferences.h"
#include "msvcimportjob.h"
#include "msvcmodelitems.h"
#include "debug.h"

#include <QDebug>
#include <QDir>
#include <QHash>
#include <QMessageBox>

#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>

#include <interfaces/iproject.h>
#include <project/projectmodel.h>

K_PLUGIN_FACTORY_WITH_JSON(MsvcSupportFactory, "kdevmsvcmanager.json", registerPlugin<MsvcProjectManager>();)

MsvcProjectManager::MsvcProjectManager(QObject * parent, const QVariantList &) :
    KDevelop::AbstractFileManagerPlugin("kdevmsvcmanager", parent),
    m_builder( new MsvcBuilder() )
{
    KDEV_USE_EXTENSION_INTERFACE(IBuildSystemManager)
}

KDevelop::ProjectFolderItem* MsvcProjectManager::import( KDevelop::IProject* project )
{
    KSharedConfigPtr cfg = project->projectConfiguration();
    
    KConfigGroup grp = cfg->group("Project");
    
    KDevelop::Path path( project->path(),
                         grp.readEntry("CreatedFrom", QString() ) );

    // Might happen on windows
    if ( ! path.lastPathSegment().endsWith(".sln", Qt::CaseInsensitive) )
    {
        if ( !path.isLocalFile() )
        {
            QMessageBox::warning(nullptr, "KDEV_MSVC", "Can not open: " + path.toLocalFile() + " because is not a local file");
            return nullptr;
        }

        QDir dir ( path.parent().toLocalFile() );

        QFileInfoList files = dir.entryInfoList( QStringList() << "*.sln", QDir::Files);

        qCWarning(KDEV_MSVC) << "Found: " << files.size() << " solution file in the folder";

        if ( files.empty() )
        {
            QMessageBox::warning(nullptr, "KDEV_MSVC", "No solution files found in: " + dir.path() );
            return nullptr;
        }

        // FIXME: we just pick the first file here.

        path.setLastPathSegment( files.front().fileName() );
    }

    MsvcConfig::guessCompilerIfNotConfigured( project );

    return new MsvcSolutionItem( project, path );
}

KJob* MsvcProjectManager::createImportJob( KDevelop::ProjectFolderItem* item )
{
    MsvcSolutionItem * solItem = dynamic_cast<MsvcSolutionItem*>(item);
    Q_ASSERT(solItem);
    
    return new MsvcImportSolutionJob(solItem);
}

KDevelop::IProjectBuilder* MsvcProjectManager::builder() const
{
    return m_builder;
}

KDevelop::Path::List MsvcProjectManager::includeDirectories(KDevelop::ProjectBaseItem * item) const
{
    KDevelop::Path::List result;

    if (!item)
    {
        return result;
    }
   
    KSharedConfigPtr cfg = item->project()->projectConfiguration();
    KConfigGroup grp = cfg->group(MsvcConfig::CONFIG_GROUP);
    
    KDevelop::Path msIncludePath( grp.readEntry(MsvcConfig::MSVC_INCLUDE, QString() ) );
    
    if ( msIncludePath.isValid() || msIncludePath.isEmpty() )
    {
        result.push_back( std::move(msIncludePath) );
    }
    
    KDevelop::Path winSdkIncludePath( grp.readEntry(MsvcConfig::WINSDK_INCLUDE, QString() ) );
    
    if ( winSdkIncludePath.isValid() || winSdkIncludePath.isEmpty() )
    {
        result.push_back( std::move(winSdkIncludePath) );
    }
   
    for ( KDevelop::ProjectBaseItem * p = item; p; p = p->parent() )
    {
        if ( MsvcProjectItem * projItem = dynamic_cast<MsvcProjectItem*>(p) )
        {
            const QUrl projectPath = projItem->path().parent().toUrl();
            QStringList includes = projItem->getCurrentConfig().additionalIncludeDirectories;
            
            MsvcVariableReplacer replacer; 
            
            for (QString const & s : replacer.replace(includes, projItem) )
            {
                QUrl url = QUrl::fromUserInput(s);
                
                // Relative paths are relative to the project path.
                if ( url.isRelative() )
                    url = projectPath.resolved(url);

                if ( url.isValid() )
                    result << KDevelop::Path(url);
                else
                {
                    qCWarning(KDEV_MSVC) << "Invalid include path:" << s;
                }
            }
            break;
        }
    }
   
    return result;
}

QHash<QString,QString> MsvcProjectManager::defines(KDevelop::ProjectBaseItem* item) const
{
    //TODO compiler-injected defines
    
    if (!item)
    {
        return {};
    }

    for ( KDevelop::ProjectBaseItem * p = item; p; p = p->parent() )
    {
        if ( MsvcProjectItem * projItem = dynamic_cast<MsvcProjectItem*>(p) )
        {
            return projItem->getCurrentConfig().preprocessorDefines;
        }
    }
    
    return {};
}

bool MsvcProjectManager::hasIncludesOrDefines(KDevelop::ProjectBaseItem* item) const
{
    return true;
}

int MsvcProjectManager::perProjectConfigPages() const
{
    return 1;
}

KDevelop::ConfigPage* MsvcProjectManager::perProjectConfigPage(int number, 
                                                               const KDevelop::ProjectConfigOptions& options, 
                                                               QWidget* parent)
{
    switch( number )
    {
    case 0:
        return new MsvcBuilderPreferences(this, options, parent);
    default:
        return nullptr;
    }
}


#include "msvcmanager.moc"
