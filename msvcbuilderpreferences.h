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

#ifndef MSVCBUILDERPREFERENCES_H
#define MSVCBUILDERPREFERENCES_H

#include <project/projectconfigpage.h>

namespace Ui {
    class MsvcConfig;
}

class MsvcBuilderPreferences : public KDevelop::ConfigPage
{
    Q_OBJECT

public:
    explicit MsvcBuilderPreferences(KDevelop::IPlugin* plugin,
                                    const KDevelop::ProjectConfigOptions& options,
                                    QWidget* parent = nullptr);
    ~MsvcBuilderPreferences() override;

public slots:
    void apply() override;
    void reset() override;
    QString name() const override;

private:
    KDevelop::IProject* m_project;

    Ui::MsvcConfig* m_configUi;
};


#endif // MSVCBUILDERPREFERENCES_H

