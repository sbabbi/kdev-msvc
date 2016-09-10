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

#ifndef MSVCPROJECTCONFIG_H
#define MSVCPROJECTCONFIG_H

#include <QHash>
#include <QString>

class QXmlStreamReader;

struct MsvcProjectConfig
{    
    enum TargetType
    {
        Unknown,
        Application,
        DynamicLibrary,
        StaticLibrary,
        Generic
    };
   
    enum CharacterSet
    {
        CharSetNotSet,
        CharSetUnicode,
        CharSetMBCS
    };
    
    enum RuntimeLibrary
    {
        MultiThreaded,
        MultiThreadedDebug,
        MultiThreadedDll,
        MultiThreadedDebugDll,
    };
    
    enum SubSystem
    {
        SubSystemNotSet,
        SubSystemConsole,
        SubSystemWindows,
        SubSystemNative
    };
    
    // General config
    QString         configurationName;
    QString         targetArchitecture;
    QString         outputDirectory;
    TargetType      configurationType;
    CharacterSet    characterSet;
    bool            wholeProgramOptimization;
    
    //VCCLCompilerTool
    int                     optimizationLevel;
    bool                    intrinsicInstructions;
    QStringList             additionalIncludeDirectories;
    QHash<QString,QString>  preprocessorDefines;
    RuntimeLibrary          rtLibrary;
    bool                    usepch;
    int                     warningLevel;
    
    //VCLinkerTool
    bool                    linkIncremental;
    SubSystem               subSystem;
    QString                 outputFile;
};

MsvcProjectConfig parseConfig( QXmlStreamReader & );

#endif //MSVCPROJECTCONFIG_H
