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

#ifndef MSVCPROJECTPARSER_H
#define MSVCPROJECTPARSER_H

#include <QFuture>
#include <QFutureInterface>
#include <QRunnable>

#include "msvcmodelitems.h"

class QXmlStreamReader;

/**
 * @brief Base class for VCproj / VCxProj parsers
 */
class MsvcProjectParser : public QRunnable
{
public:
    explicit MsvcProjectParser ( MsvcProjectItem * proj ) :
        m_proj(proj)
    {
    }

    virtual void run() override final;
    
    QFuture<void> getFuture() { return m_promise.future(); }

protected:
    virtual bool parse( QXmlStreamReader & ) = 0;

    bool isCanceled() const { return m_promise.isCanceled(); }
    
    MsvcProjectItem * projectItem() const { return m_proj; }
    KDevelop::Path projectPath() const { return m_proj->path(); }

private:
    QFutureInterface<void> m_promise;
    MsvcProjectItem * m_proj;
};

/**
 * @brief Parse a .vcproj file
 */
class MsvcVcProjParser : public MsvcProjectParser
{
public:
    explicit MsvcVcProjParser ( MsvcProjectItem * proj ) :
        MsvcProjectParser( proj )
    {
    }

private:
    virtual bool parse( QXmlStreamReader & ) override;

    /**
     * @brief parse a \<Files\> tag.
     */
    void parseFileList(KDevelop::ProjectBaseItem * parent,
                       QXmlStreamReader & reader) const;

    /**
     * @brief parse a \<VisualStudioProject\> tag
     */
    void parseVisualStudioProject(QXmlStreamReader& reader);
};

/**
 * @brief Parse a .vcxproj file
 */
class MsvcVcxProjParser : public MsvcProjectParser
{
public:
    explicit MsvcVcxProjParser ( MsvcProjectItem * proj ) :
        MsvcProjectParser( proj )
    {
    }

private:
    virtual bool parse( QXmlStreamReader & ) override;
    
    void parseFilterFile( QXmlStreamReader & );
    void parseItemGroup( QXmlStreamReader & );
    
};

#endif //MSVCPROJECTPARSER_H
