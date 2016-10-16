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

#include "msvcmodelitems.h"
#include "debug.h"

#include <QRegularExpression>

#include <project/projectmodel.h>

bool MsvcFilterItem::lessThan(const KDevelop::ProjectBaseItem* item) const
{
    if ( item->type() > CustomProjectItemType )
    {
        // Compare filter vs filter.
        return text() < item->text();
    }
    
    return KDevelop::ProjectBaseItem::lessThan(item);
}

QString MsvcFilterItem::iconName() const
{
    return QStringLiteral("filter");
}

QUrl MsvcExecutableTargetItem::builtUrl() const
{
    // Walk up and find our project parent (should be the first one)
    KDevelop::ProjectBaseItem * p = parent();
    while ( p && !dynamic_cast<MsvcProjectItem*>(p) )
        p = p->parent();
   
    if (!p)
    {
        // Bail out
        return QUrl();
    }
    
    Q_ASSERT( dynamic_cast<MsvcProjectItem*>(p) );
   
    MsvcProjectItem * proj = static_cast<MsvcProjectItem*>(p);
    
    MsvcVariableReplacer replacer;
    QString outputFilePath = replacer.replace( proj->getCurrentConfig().outputFile,
                                               proj ).replace('\\', '/');

    return QUrl( outputFilePath );
}

MsvcProjectItem::MsvcProjectItem( KDevelop::IProject* project,
                                   const KDevelop::Path& path,
                                   KDevelop::ProjectBaseItem* parent ) :
    KDevelop::ProjectBuildFolderItem( project, path, parent )
{
    setText( path.lastPathSegment().section('.', 0, -2) );
}

bool MsvcProjectItem::lessThan(const KDevelop::ProjectBaseItem* item) const
{
    if ( item->type() > CustomProjectItemType )
    {
        // Compare filter vs filter.
        return text() < item->text();
    }
    
    return KDevelop::ProjectBaseItem::lessThan(item);
}

void MsvcProjectItem::addConfiguration(const MsvcProjectConfig & config)
{
    QString configFullName = config.configurationName + "|" + config.targetArchitecture;
    configurations_.insert( configFullName, config );
    
    if ( current_config_.isEmpty() )
    {
        current_config_ = configFullName;
    }
}

bool MsvcProjectItem::setCurrentConfiguration(const QString& configFullName)
{
    if ( configurations_.contains( configFullName ) )
    {
        current_config_ = configFullName;
        return true;
    }
    return false;
}

MsvcProjectConfig MsvcProjectItem::getCurrentConfig() const
{
    return configurations_.value( current_config_ );
}

MsvcSolutionItem::MsvcSolutionItem(KDevelop::IProject* project,
                                   const KDevelop::Path& path,
                                   KDevelop::ProjectBaseItem* parent ) :
    KDevelop::ProjectBuildFolderItem( project, path, parent )
{
}

bool MsvcSolutionItem::lessThan(const KDevelop::ProjectBaseItem* item) const
{
    if ( item->type() > CustomProjectItemType )
    {
        // Compare filter vs filter.
        return text() < item->text();
    }
    
    return KDevelop::ProjectBaseItem::lessThan(item);
}

void MsvcSolutionItem::setCurrentConfig(const QString & config)
{
    const auto & cfg = config_map_[config];
    
    for ( const auto & key : cfg.keys() )
    {
        if ( MsvcProjectItem * proj = findProjectByUuid(key) )
        {
            proj->setCurrentConfiguration( cfg.value(key) );
        }
    }
}

MsvcProjectItem* MsvcSolutionItem::findProjectByUuid(const QUuid & uuid) const
{
    for (const auto & x : children() )
    {
        if ( auto * proj = dynamic_cast<MsvcProjectItem*>(x) )
        {
            if ( proj->uuid() == uuid )
                return proj;
        }
    }
    return nullptr;
}

QString MsvcVariableReplacer::replace( QString s, KDevelop::ProjectBaseItem const * item )
{
    static QRegularExpression regex(R"(\$\(([a-zA-Z0-9]+)\))");
    
    //FIXME this is not very efficient
    QRegularExpressionMatchIterator i;
    while ( i = regex.globalMatch(s, 0), i.hasNext() )
    {
        QRegularExpressionMatch m = i.next();
        s.replace( m.capturedStart(),
                   m.capturedLength(),
                   getReplacement(m.captured(1), item) );
    }
    
    return s;
}

QString MsvcVariableReplacer::getReplacement( QString const & key, KDevelop::ProjectBaseItem const * item )
{
    if ( !item )
        return {};
    
    if (const MsvcProjectItem * projItem = dynamic_cast<const MsvcProjectItem*>(item) )
    {
        return visit(key, projItem);
    }
    else if (const MsvcSolutionItem * solItem= dynamic_cast<const MsvcSolutionItem*>(item) )
    {
        return visit(key, solItem);
    }
    else
    {
        return visit(key, item);
    }
}

QString MsvcVariableReplacer::visit(QString const & key, KDevelop::ProjectBaseItem const  * item)
{
    if ( key == "InputDir" )
    {
        return item->path().parent().toLocalFile() + '\\';
    }
    else if ( key == "InputPath" )
    {
        return item->path().toLocalFile();
    }
    else if ( key == "InputName" )
    {
        return item->baseName();
    }
    else if ( key == "InputFileName" )
    {
        return item->path().lastPathSegment();
    }
    else if ( key == "InputExt" )
    {
        return "." + item->path().lastPathSegment().section('.', -2, -1);
    }
    else if ( key == "ParentName" )
    {
        KDevelop::ProjectBaseItem * parent = item->parent();
        return parent ? parent->text() : QString();
    }
    
    // If everything else fails, forward to parent
    return getReplacement(key, item->parent() );
}

QString MsvcVariableReplacer::visit( QString const & key, MsvcProjectItem const * item )
{
    if ( key == "ProjectDir" )
    {
        return visit( "InputDir", static_cast<KDevelop::ProjectBaseItem const *>(item) );
    }
    else if ( key == "ProjectPath" )
    {
        return visit( "InputPath", static_cast<KDevelop::ProjectBaseItem const *>(item) );
    }
    else if ( key == "ProjectName" )
    {
        return visit( "InputName", static_cast<KDevelop::ProjectBaseItem const *>(item) );
    }
    else if ( key == "ProjectFileName" )
    {
        return visit( "InputFileName", static_cast<KDevelop::ProjectBaseItem const *>(item) );
    }
    else if ( key == "ProjectExt" )
    {
        return visit( "InputExt", static_cast<KDevelop::ProjectBaseItem const *>(item) );
    }
    else if ( key == "TargetDir" )
    {
        recursionChecker checker( replace_target_dir_guard_ );
        
        if ( checker.stop() )
            return QString();

        return replace( item->getCurrentConfig().outputDirectory + "\\", item );
    }
    else if ( key == "TargetPath" )
    {
        recursionChecker checker( replace_target_path_guard_ );
        
        if ( checker.stop() )
            return QString();

        return visit("TargetDir", item) + 
                replace( item->getCurrentConfig().outputFile, item );
    }
    else if ( key == "TargetName" )
    {
        recursionChecker checker( replace_target_name_guard_ );
        
        if ( checker.stop() )
            return QString();

        return replace( item->getCurrentConfig().outputFile, item ).section('.', 0, -2);
    }
    else if ( key == "TargetFileName" )
    {
        recursionChecker checker( replace_target_file_name_guard_ );
        
        if ( checker.stop() )
            return QString();

        return replace( item->getCurrentConfig().outputFile, item );
    }
    else if ( key == "TargetExt" )
    {
        recursionChecker checker( replace_target_ext_guard_ );
        
        if ( checker.stop() )
            return QString();

        return replace( item->getCurrentConfig().outputFile, item ).section('.', -2, -1);
    }
    
    // If everything else fails, forward to parent
    return getReplacement(key, item->parent() );
}

QString MsvcVariableReplacer::visit( QString const & key, MsvcSolutionItem const * item )
{
    if ( key == "SolutionDir" )
    {
        return visit( "InputDir", static_cast<KDevelop::ProjectBaseItem const *>(item) );
    }
    else if ( key == "SolutionPath" )
    {
        return visit( "InputPath", static_cast<KDevelop::ProjectBaseItem const *>(item) );
    }
    else if ( key == "SolutionName" )
    {
        return visit( "InputName", static_cast<KDevelop::ProjectBaseItem const *>(item) );
    }
    else if ( key == "ProjectFileName" )
    {
        return visit( "SolutionFileName", static_cast<KDevelop::ProjectBaseItem const *>(item) );
    }
    else if ( key == "SolutionExt" )
    {
        return visit( "InputExt", static_cast<KDevelop::ProjectBaseItem const *>(item) );
    }

    // If everything else fails, forward to parent
    return getReplacement(key, item->parent() );
}
