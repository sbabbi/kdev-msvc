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

#ifndef MSVCBUILDER_H
#define MSVCBUILDER_H

#include <project/interfaces/iprojectbuilder.h>

#include "devenvjob.h"

namespace KDevelop 
{
class ProjectBaseItem;
}

class MsvcBuilder : public KDevelop::IProjectBuilder
{
public:
    KJob* install(KDevelop::ProjectBaseItem* /*item*/, const QUrl &/*specificPrefix*/ = {}) override
    {
        return nullptr;
    }

    KJob* build(KDevelop::ProjectBaseItem * item ) override;
    KJob* clean(KDevelop::ProjectBaseItem * item) override;
    
private:
    DevEnvJob* runDevEnv(KDevelop::ProjectBaseItem  *, DevEnvJob::CommandType );

};

#endif //MSVCBUILDER_H

