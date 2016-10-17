
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
class MsvcProjectParser;

namespace KDevelop
{
class ProjectBaseItem;
class ProjectFolderItem;
class IProject;
}

class MsvcImportSolutionJob : public KJob
{
    Q_OBJECT
public:
    explicit MsvcImportSolutionJob(MsvcSolutionItem* dom);
    ~MsvcImportSolutionJob();
    
    void start() override;

protected:
    bool doKill() override;

private:
    void parseProject( const QString & relativePath );

private:
    void run();

    MsvcSolutionItem * m_dom;
    KDevelop::Path m_solutionPath;
    QFutureWatcher<void> * m_futureWatcher;
};


#endif //MSVCIMPORTJOB_H
