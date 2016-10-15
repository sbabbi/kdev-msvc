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


#include "msvcbuilderpreferences.h"

#include "msvcconfig.h"
#include "debug.h"
#include "ui_msvcconfig.h"

#include <KLocalizedString>

#include <interfaces/iproject.h>

MsvcBuilderPreferences::MsvcBuilderPreferences(KDevelop::IPlugin* plugin,
                                               const KDevelop::ProjectConfigOptions& options, 
                                               QWidget* parent) :
    KDevelop::ConfigPage(plugin, nullptr, parent),
    m_project(options.project),
    m_configUi(new Ui::MsvcConfig)
{
    m_configUi->setupUi(this);

    auto compilers = MsvcConfig::findMSVC();

    KComboBox * compVersionComboBox = m_configUi->version_combo;
    for (auto const & x : compilers )
    {
        const int index = compVersionComboBox->count() - 1;
        compVersionComboBox->insertItem( index, x.fullName, QVariant::fromValue(x.path) );
    }

    const int versionComboCount = compVersionComboBox->count();
    compVersionComboBox->setCurrentIndex(0);

    auto showHideCustomCompPath = [this, versionComboCount](int currentIndex)
    {
        // Show only if the current selected index is the last one.
        const bool hidden = currentIndex < versionComboCount - 1;

        m_configUi->devenv_exe_label->setHidden(hidden);
        m_configUi->builder_path->setHidden(hidden);
    };

    showHideCustomCompPath( versionComboCount > 1);

    connect(compVersionComboBox, static_cast<void (QComboBox::*)(int)>( &KComboBox::currentIndexChanged ),
            this, showHideCustomCompPath);
}

MsvcBuilderPreferences::~MsvcBuilderPreferences()
{
    delete m_configUi;
}

void MsvcBuilderPreferences::apply()
{
    qCDebug(KDEV_MSVC) << "Saving data";
    KConfigGroup cg(m_project->projectConfiguration(), MsvcConfig::CONFIG_GROUP);

    KComboBox * compVersionComboBox = m_configUi->version_combo;

    const bool customCompilerPath = compVersionComboBox->currentIndex() >= compVersionComboBox->count() - 1;

    KDevelop::Path compilerPath = customCompilerPath  ?
        KDevelop::Path( m_configUi->builder_path->url() ) :
        compVersionComboBox->itemData(compVersionComboBox->currentIndex()).value<KDevelop::Path>();

    cg.writeEntry( MsvcConfig::DEVENV_BINARY, compilerPath.toLocalFile() );
    cg.writeEntry( MsvcConfig::MSVC_INCLUDE, m_configUi->msvc_include->url().toLocalFile() );
    
    //TODO saving currentText is not very pretty...
    cg.writeEntry( MsvcConfig::ACTIVE_CONFIGURATION, m_configUi->config_combo->currentText() );
    cg.writeEntry( MsvcConfig::ACTIVE_ARCHITECTURE, m_configUi->arch_combo->currentText() );
}

void MsvcBuilderPreferences::reset()
{
    qCDebug(KDEV_MSVC) << "loading data";
    // refresh combobox
    KConfigGroup cg(m_project->projectConfiguration(), MsvcConfig::CONFIG_GROUP);
    
    KDevelop::Path compilerPath (cg.readEntry( MsvcConfig::DEVENV_BINARY, QString() ) );

    KComboBox * compVersionComboBox = m_configUi->version_combo;

    int selectedIndex = compVersionComboBox->count() - 1;
    for ( int i = 0; i < compVersionComboBox->count() - 1; ++i )
    {
        if ( compVersionComboBox->itemData(i).value<KDevelop::Path>() == compilerPath )
        {
            selectedIndex = i;
            break;
        }
    }

    compVersionComboBox->setCurrentIndex( selectedIndex );
    m_configUi->builder_path->setUrl( compilerPath.toUrl() );
    m_configUi->msvc_include->setUrl( cg.readEntry( MsvcConfig::MSVC_INCLUDE, QString() ) );
    m_configUi->config_combo->setCurrentItem( cg.readEntry( MsvcConfig::ACTIVE_CONFIGURATION, QString() ) );
    m_configUi->arch_combo->setCurrentItem( cg.readEntry( MsvcConfig::ACTIVE_ARCHITECTURE, QString() ) );
}

QString MsvcBuilderPreferences::name() const
{
    return i18n("MSVC");
}
