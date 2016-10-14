
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

#ifndef MSVCIMPORTJOB_H
#define MSVCIMPORTJOB_H

#include <KJob>
#include <KCompositeJob>

#include <kdevplatform/util/path.h>

template<class> class QFutureWatcher;
class QXmlStreamReader;

class MsvcProjectItem;
class MsvcSolutionItem;

namespace KDevelop
{
class ProjectBaseItem;
class ProjectFolderItem;
}

class MsvcImportJob : public KJob
{
    Q_OBJECT
public:
    explicit MsvcImportJob( MsvcProjectItem * dom);
    
    void start() override;

protected:
    bool doKill() override;

private slots:
    void aboutToShutdown();

private:
    void run();
    void parseFileList(KDevelop::ProjectBaseItem * parent,
                       QXmlStreamReader & reader) const;

    void parseVcProj(QXmlStreamReader & reader) const;

    MsvcProjectItem * m_dom;
    KDevelop::Path m_file;
    QFutureWatcher<void> * m_futureWatcher;
    bool m_canceled;
};

class MsvcImportSolutionJob : public KCompositeJob
{
    Q_OBJECT
public:
    explicit MsvcImportSolutionJob(MsvcSolutionItem* dom);
    
    void start() override;

protected:
    void slotResult(KJob*) override;
    bool doKill() override;

private slots:
    void addProject( const QString & relativePath );

private:
    //BUG likely data races everywhere
    //TODO make it cancellable
    void run();
    void reconsider();

    MsvcSolutionItem * m_dom;
    KDevelop::Path m_solutionPath;
    QFutureWatcher<void> * m_futureWatcher;
    bool m_finished;
};


#endif //MSVCIMPORTJOB_H
