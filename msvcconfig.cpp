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

QList<QUrl> MsvcConfig::findMSVC()
{
    // For now hard-coded reasonable defaults
    static const QUrl msvcCandidatePaths[] =
    {
        QUrl(R"(C:/Program Files/Microsoft Visual Studio 9.0)")
    };
    
    QList<QUrl> result;
    for ( const auto & p : msvcCandidatePaths )
    {
        QString localPath = p.toLocalFile();
        QFileInfo info(localPath);
        
        if ( info.exists() && info.isDir() )
            result.push_back(p);
    }
    
    return result;
}

QUrl MsvcConfig::findDevEnvBinary(const QUrl& msvc)
{
    static const QString devenvCandidateSubPaths[] = 
    {
        "Common7/IDE/devenv.exe",
        "Common7/IDE/VCExpress.exe"
    };
    for ( const QString & s : devenvCandidateSubPaths )
    {
        QUrl fullPath = msvc.resolved(s);
        QFileInfo i(fullPath.toLocalFile());
        
        if ( i.exists() && i.isExecutable() )
            return fullPath;
    }
    return QUrl();
}
