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

#ifndef MSVCMODELITEMS_H
#define MSVCMODELITEMS_H

#include <QUuid>

#include <project/projectmodel.h>
#include <kdevplatform/util/path.h>

#include "msvcprojectconfig.h"

class MsvcFilterItem : public KDevelop::ProjectBaseItem
{
public:
    using KDevelop::ProjectBaseItem::ProjectBaseItem;

    int type() const override { return Folder; }
    
    bool lessThan( const KDevelop::ProjectBaseItem* item ) const override;
    
    QString iconName() const override;
    
    RenameStatus rename(const QString &) override
    {
        return ProjectManagerRenameFailed;
    }
};

class MsvcExecutableTargetItem : public KDevelop::ProjectExecutableTargetItem
{
public:
    using KDevelop::ProjectExecutableTargetItem::ProjectExecutableTargetItem;

    QUrl builtUrl() const override;
    QUrl installedUrl() const override { return QUrl(); }

private:    
};

class MsvcProjectItem : public KDevelop::ProjectFolderItem
{
public:
    MsvcProjectItem( KDevelop::IProject* , 
                     const KDevelop::Path& path,
                     ProjectBaseItem* parent = nullptr );
    
    bool lessThan( const KDevelop::ProjectBaseItem* item ) const override;
    
    RenameStatus rename(const QString & newName) override
    {
        return KDevelop::ProjectBaseItem::rename(newName);
    }
    
    void addConfiguration( MsvcProjectConfig const & );
    bool setCurrentConfiguration( QString const & fullname);
    
    MsvcProjectConfig getCurrentConfig() const;
    
    void setUuid(QUuid uuid)
    {
        uuid_ = uuid;
    }
    
    QUuid uuid() const { return uuid_; }

private:
    QString current_config_;
    QHash< QString, MsvcProjectConfig > configurations_;
    QUuid uuid_;
};

class MsvcSolutionItem : public KDevelop::ProjectBuildFolderItem
{
public:
    MsvcSolutionItem( KDevelop::IProject* , 
                      const KDevelop::Path& path,
                      ProjectBaseItem* parent = nullptr );
    
    bool lessThan( const KDevelop::ProjectBaseItem* item ) const override;
    
    RenameStatus rename(const QString & newName) override
    {
        return KDevelop::ProjectBaseItem::rename(newName);
    }
    
    void setCurrentConfig(QString const & name);
    
    void addProjectConfig(QString const & mainCfg,
                          QUuid const & project,
                          QString const & projectCfg)
    {
        config_map_[ mainCfg ][project] = projectCfg;
    }

private:
    MsvcProjectItem* findProjectByUuid(const QUuid &) const;
    
    QHash< QString, QHash<QUuid, QString> > config_map_;
};

class MsvcVariableReplacer
{
public:
    QString replace( QString s, KDevelop::ProjectBaseItem const * item );
    QStringList replace( QStringList s, KDevelop::ProjectBaseItem const * item )
    {
        for (QString & x : s) 
            x = replace(x, item);
        return s;
    }
    
    QString getReplacement( QString const & key, KDevelop::ProjectBaseItem const * item );
    
private:
    QString visit( QString const & key, KDevelop::ProjectBaseItem const * item );
    QString visit( QString const & key, MsvcProjectItem const * item );
    QString visit( QString const & key, MsvcSolutionItem const * item );
    
    struct recursionChecker
    {
        explicit recursionChecker(bool & guard) :
            guard_(guard),
            stop_(guard)
        {
            guard_ = true;
        }
        
        ~recursionChecker() { guard_ = false; }
        
        bool stop() const { return stop_; }

    private:
        bool & guard_;
        bool stop_;
    };
    
    bool replace_target_dir_guard_ = false,
         replace_target_path_guard_ = false,
         replace_target_name_guard_ = false,
         replace_target_file_name_guard_ = false,
         replace_target_ext_guard_ = false;
};

#endif //MSVCMODELITEMS_H
