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


#include "msvcprojectconfig.h"

#include <QXmlStreamReader>

namespace
{

QString getDefaultOutputExtension( MsvcProjectConfig::TargetType type )
{
    switch( type )
    {
    case MsvcProjectConfig::Unknown:
    case MsvcProjectConfig::Generic:
    default:
        return QString();
    case MsvcProjectConfig::Application:
        return ".exe";
    case MsvcProjectConfig::DynamicLibrary:
        return ".dll";
    case MsvcProjectConfig::StaticLibrary:
        return ".lib";
    }
}

void parseConfigGeneric(MsvcProjectConfig & result, QXmlStreamReader & reader)
{
    QStringList nameAndArch = reader.attributes().value("Name").toString().split('|');

    //
    // Generic configuration
    result.configurationName = nameAndArch.value(0);
    result.targetArchitecture = nameAndArch.value(1);
        
    int configurationType = reader.attributes().value("ConfigurationType").toInt();
    
    result.configurationType = (configurationType > 0 && configurationType <= 4) ? 
                                MsvcProjectConfig::TargetType(configurationType) :
                                MsvcProjectConfig::Unknown;
                                
    result.outputDirectory  = reader.attributes().value("OutputDirectory").toString();
                               
    int characterSet = reader.attributes().value("CharacterSet").toInt();
    
    result.characterSet = (characterSet > 0 && characterSet <= 2) ?
                          MsvcProjectConfig::CharacterSet(characterSet) :
                          MsvcProjectConfig::CharSetNotSet;
    
    result.wholeProgramOptimization = reader.attributes().value("WholeProgramOptimization").toInt() != 0;
}

void parseConfigCompilerTool(MsvcProjectConfig & result, QXmlStreamReader & reader)
{
    //
    //VCCLCompilerTool
    
    result.optimizationLevel = reader.attributes().value("Optimization").toInt();
    result.intrinsicInstructions = 
        reader.attributes().value("EnableIntrinsicFunctions").compare("true", Qt::CaseInsensitive) == 0;
    
    QStringList preprocessorDef = reader.attributes().value("PreprocessorDefinitions").toString().split(';');
    
    for ( const QString & s : preprocessorDef )
    {
        QStringList nameAndValue = s.split('=');
        result.preprocessorDefines.insert( nameAndValue.value(0), nameAndValue.value(1) );
    }
    
    result.additionalIncludeDirectories = 
        reader.attributes().value("AdditionalIncludeDirectories").toString().split(';');
    
    int runtimeLibrary = reader.attributes().value("RuntimeLibrary").toInt();
    result.rtLibrary = ( runtimeLibrary >= 0 && runtimeLibrary < 4) ?
                        MsvcProjectConfig::RuntimeLibrary( runtimeLibrary ) :
                        MsvcProjectConfig::MultiThreaded;
    
    result.usepch = reader.attributes().value("UsePrecompiledHeader").toInt() != 0;
    result.warningLevel = reader.attributes().value("WarningLevel").toInt();
}

void parseConfigLinkerTool(MsvcProjectConfig & result, QXmlStreamReader & reader)
{
    result.linkIncremental = reader.attributes().value("LinkIncremental").toInt() != 0;
    
    int subSystem = reader.attributes().value("SubSystem").toInt();
    
    result.subSystem = (subSystem > 0 && subSystem <= 3 ) ?
                        MsvcProjectConfig::SubSystem(subSystem) :
                        MsvcProjectConfig::SubSystemNotSet;
                        
    result.outputFile = reader.attributes().hasAttribute("OutputFile") ?
                        reader.attributes().value("OutputFile").toString() :
                        "$(OutDir)\\$(ProjectName)" + getDefaultOutputExtension(result.configurationType );
}

}

MsvcProjectConfig parseConfig(QXmlStreamReader& reader)
{
    Q_ASSERT( reader.name() == "Configuration" );
   
    MsvcProjectConfig result = {};
    
    parseConfigGeneric(result, reader);

    for ( ;reader.readNextStartElement(); reader.skipCurrentElement() )
    {
        if ( reader.name() != "Tool" )
        {
            continue;
        }
        
        QString toolName = reader.attributes().value("Name").toString();
        
        if ( toolName == "VCCLCompilerTool" )
        {
            parseConfigCompilerTool(result, reader);
        }
        else if ( toolName == "VCLinkerTool" )
        {
            parseConfigLinkerTool(result, reader);
        }
    }
   
    return result;
}
