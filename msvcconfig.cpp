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


#include "msvcconfig.h"

#include <QFileInfo>
#include <QUrl>
#include <Qtglobal>

#include <KConfigGroup>

#include <interfaces/iproject.h>

const char* MsvcConfig::CONFIG_GROUP = "MsvcBuilder";

const char* MsvcConfig::DEVENV_BINARY = "DevenvExecutable";
const char* MsvcConfig::MSVC_INCLUDE = "MsvcIncludePath";
const char* MsvcConfig::WINSDK_INCLUDE = "WinSdkIncludePath";
const char* MsvcConfig::ACTIVE_CONFIGURATION = "Config";
const char* MsvcConfig::ACTIVE_ARCHITECTURE  = "Arch";

bool MsvcConfig::isConfigured(const KDevelop::IProject* project)
{
    KConfigGroup cg(project->projectConfiguration(), CONFIG_GROUP);
    return cg.exists() && cg.hasKey(DEVENV_BINARY) && cg.hasKey(MSVC_INCLUDE) && cg.hasKey(ACTIVE_CONFIGURATION);
}

QList<MsvcConfig::CompilerPath> MsvcConfig::findMSVC()
{
    // Environment variables to look for
    static const QPair<int, const char *> msvcToolsEnv[] =
    {
        { 14, "VS140COMNTOOLS" },
        { 13, "VS130COMNTOOLS" },
        { 12, "VS120COMNTOOLS" },
        { 11, "VS110COMNTOOLS" },
        { 10, "VS100COMNTOOLS" },
        { 9, "VS90COMNTOOLS" }
    };
    
    QList<CompilerPath> result;
    for ( const auto & p : msvcToolsEnv )
    {
        QByteArray e = qgetenv( p.second );
        if ( !e.isEmpty() )
        {
            KDevelop::Path common7Path = KDevelop::Path(e.constData()).parent();

            QFileInfo info(common7Path.toLocalFile() );
            if ( info.exists() )
            {
                result.append( findCompilerPath( common7Path, p.first) );
            }
        }
    }
    
    return result;
}

QList< MsvcConfig::CompilerPath > MsvcConfig::findCompilerPath( const KDevelop::Path & common7path, int version )
{
    static const QPair<QString, QString> devenvCandidateSubPaths[] =
    {
        {"IDE/devenv.com", "" },
        {"IDE/VCExpress.exe", " (Express)" }
    };

    QList< MsvcConfig::CompilerPath > result;
    for ( const auto & s : devenvCandidateSubPaths )
    {
        KDevelop::Path fullPath = KDevelop::Path(common7path, s.first);

        QFileInfo i(fullPath.toLocalFile());

        if ( i.exists() && i.isExecutable() )
        {
            QString compilerName = "Microsoft Visual Studio " + QString::number( version ) + s.second;
            result.push_back( CompilerPath{ version, fullPath, compilerName } );
        }
    }
    return result;
 
}
