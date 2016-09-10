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

#ifndef MSVCCONFIG_H
#define MSVCCONFIG_H

#include <QList>
#include <QUrl>

namespace KDevelop {
class IProject;
}

class MsvcConfig
{
public:
    static const char *CONFIG_GROUP;

    static const char *DEVENV_BINARY,
                      *MSVC_INCLUDE,
                      *WINSDK_INCLUDE,
                      *ACTIVE_CONFIGURATION,
                      *ACTIVE_ARCHITECTURE;

    /**
     * Returns true when the given project is sufficiently configured.
     */
    static bool isConfigured(const KDevelop::IProject* project);
    
    /**
     * Tries to find visual studio installation directory
     */
    static QList<QUrl> findMSVC();
    
    /**
     * Tries to find a devenv.exe for the given MSVC installation.
     */
    static QUrl findDevEnvBinary(QUrl const & msvc);
    
    /**
     * Tries to find the MSVC include directory for the given MSVC installation.
     */
    static QUrl findMSVCIncludeDir(QUrl const & msvc);
    
    /**
     * Tries to find visual studio SDK installation directory
     */
    static QList<QUrl> findWinSDK();
    
    /**
     * Tries to find the SDK C++ include directory
     */
    static QList<QUrl> findWinSDKInclude(QUrl const & winSdk);
    
};

#endif //MSVCCONFIG_H

