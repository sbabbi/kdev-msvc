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


#include "devenvjob.h"
#include "debug.h"
#include "msvcconfig.h"

#include <QFileInfo>

#include <KConfigGroup>
#include <KLocalizedString>

#include <interfaces/iproject.h>
#include <project/interfaces/ibuildsystemmanager.h>
#include <outputview/ioutputview.h>

DevEnvJob::DevEnvJob(QObject* parent, KDevelop::ProjectBuildFolderItem * item, CommandType command ) :
    KDevelop::OutputExecuteJob(parent),
    m_item(item),
    m_command(command)
{
    auto bsm = m_item->project()->buildSystemManager();
    auto buildDir = bsm->buildDirectory(item);

    setCapabilities( Killable );
    setProperties( PortableMessages | DisplayStderr | IsBuilderHint );

    setJobName( i18n("Build (%1)", item->text() ) );
    setToolTitle( i18n("DevEnv") );
}

void DevEnvJob::start()
{
    setStandardToolView(KDevelop::IOutputView::BuildView);
    setBehaviours(KDevelop::IOutputView::AllowUserClose | KDevelop::IOutputView::AutoScroll);

    OutputExecuteJob::start();
}

QStringList DevEnvJob::commandLine() const
{
    if ( !m_item )
        return QStringList();
    
    KSharedConfigPtr configPtr = m_item->project()->projectConfiguration();
    
    qCWarning(KDEV_MSVC) << configPtr->group("Project").groupList();
    
    KConfigGroup builderGroup( configPtr, MsvcConfig::CONFIG_GROUP );
    
    QString builder = builderGroup.readEntry( MsvcConfig::DEVENV_BINARY, QString() );
    
    QFileInfo fi(builder);
    if (!fi.exists() || !fi.isExecutable())
    {
        qCWarning(KDEV_MSVC) << "Badly configured devenv.exe";
        return {};
    }
    
    // Note: this command line seems to work with both .sln and .vcproj
    QStringList result;
    result << builder;
    
    result << m_item->path().toLocalFile();
    
    switch ( m_command )
    {
    case BuildCommand:
    default:
        result << "/Build";
        break;
    case CleanCommand:
        result << "/Clean";
        break;
    }
   
    result << builderGroup.readEntry( MsvcConfig::ACTIVE_CONFIGURATION, "Debug");
    
    return result;
}
