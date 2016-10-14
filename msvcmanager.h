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

#ifndef MSVCMANAGER_H
#define MSVCMANAGER_H

#include <project/abstractfilemanagerplugin.h>
#include <project/interfaces/ibuildsystemmanager.h>

class MsvcBuilder;

class MsvcProjectManager : public KDevelop::AbstractFileManagerPlugin, public KDevelop::IBuildSystemManager
{
    Q_OBJECT
    Q_INTERFACES( KDevelop::IBuildSystemManager )

public:
    
    explicit MsvcProjectManager(QObject * parent, const QVariantList &);

    //BEGIN AbstractFileManager
    KDevelop::ProjectFolderItem* import( KDevelop::IProject* project ) override;
    KJob* createImportJob( KDevelop::ProjectFolderItem* item) override;
    //END AbstractFileManager

    //BEGIN IBuildSystemManager
    KDevelop::IProjectBuilder*  builder() const override;

    KDevelop::Path::List includeDirectories(KDevelop::ProjectBaseItem*) const override;
    
    QHash<QString,QString> defines(KDevelop::ProjectBaseItem*) const override;
    
    bool hasIncludesOrDefines(KDevelop::ProjectBaseItem* item) const override;
    
    int perProjectConfigPages() const override;
    KDevelop::ConfigPage* perProjectConfigPage(int number, 
                                               const KDevelop::ProjectConfigOptions& options, 
                                               QWidget* parent) override;
    
    KDevelop::ProjectTargetItem* createTarget( const QString&, KDevelop::ProjectFolderItem* ) override
    {
        return nullptr;
    }

    bool removeTarget( KDevelop::ProjectTargetItem* ) override
    {
        return false;
    }

    QList<KDevelop::ProjectTargetItem*> targets(KDevelop::ProjectFolderItem*) const override
    {
        return {};
    }
    
    bool addFilesToTarget(const QList<KDevelop::ProjectFileItem*>&, KDevelop::ProjectTargetItem*) override
    {
        return false;
    }

    bool removeFilesFromTargets(const QList<KDevelop::ProjectFileItem*>&) override
    {
        return false;
    }

    KDevelop::Path buildDirectory(KDevelop::ProjectBaseItem*) const override
    {
        return KDevelop::Path{};
    }
    //END IBuildSystemManager

private:
    MsvcBuilder * m_builder = 0;
};

#endif //MSVCMANAGER_H
