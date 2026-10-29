/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QScrollArea>
#include "configdialog.h"
#include "ui/configpage/configpages.h"
#include "qetapp.h"
#include "qet.h"

#include "machine_info.h"
#include "qeticons.h"
#include "ui/configpage/generalconfigurationpage.h"
#include "ui/configpage/shortcutsconfigpage.h"
#include "ui/configpage/shortcutbarconfigpage.h"
#include "ui/configpage/gesturesconfigpage.h"
#include "ui/configpage/toolbarsconfigpage.h"
#include "ui/configpage/toolbarcommandsconfigpage.h"
#ifdef QET_SPACEMOUSE_SUPPORT
#include "ui/configpage/spacemouseconfigpage.h"
#endif
#include <QShowEvent>

/**
	Constructeur
	@param parent QWidget parent
*/
ConfigDialog::ConfigDialog(QWidget *parent) : QDialog(parent) {
	//ScrollArea for low screens
	QScrollArea *scroll = new QScrollArea(this);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	// liste des pages
	pages_list = new QListWidget();
	pages_list -> setViewMode(QListView::IconMode);
	if(MachineInfo::instance()->i_max_screen_height() <= 1080){
		pages_list -> setIconSize(QSize(64, 64));
	} else {
		pages_list -> setIconSize(QSize(128, 128));
	}
	pages_list -> setMovement(QListView::Static);
	pages_list -> setMinimumWidth(168);
	pages_list -> setMaximumWidth(168);
	pages_list -> setSpacing(16);
	pages_list -> setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// pages
	pages_widget = new QStackedWidget();
	// boutons
	buttons = new QDialogButtonBox(
				QDialogButtonBox::Ok
				|QDialogButtonBox::Cancel);


	QWidget *viewport = new QWidget(this);
	scroll->setWidget(viewport);
	scroll->setWidgetResizable(true);

	// layouts
	QHBoxLayout *hlayout1 = new QHBoxLayout(viewport);
	// add needed widgets to layout "hlayout1"
	hlayout1 -> addWidget(pages_list);
	hlayout1 -> addWidget(pages_widget);

	//add hlayout1 to widget
	viewport->setLayout(hlayout1);

	// Add a layout for QDialog
	QVBoxLayout *dialog_layout = new QVBoxLayout(this);
	dialog_layout -> addWidget(scroll); // add scroll to the QDialog's layout
	dialog_layout -> addWidget(buttons);
	setLayout(dialog_layout);

	// connexion signaux / slots
	connect(buttons, &QDialogButtonBox::accepted, this, &ConfigDialog::applyConf);
	connect(buttons, &QDialogButtonBox::rejected, this, &ConfigDialog::reject);
	connect(pages_list, &QListWidget::currentRowChanged, this, &ConfigDialog::showPage23);

	// set maximum a bit smaller than available size = (screen-size - Task-Bar):
	setMaximumSize((int)(0.94 * MachineInfo::instance()->i_max_available_width()),
				   (int)(0.94 * MachineInfo::instance()->i_max_available_height()));
	resize(std::min(1400,maximumWidth()),
		   std::min(1000,maximumHeight()));

#ifdef Q_OS_MACOS
	if (parent) {
		setWindowFlags(Qt::Sheet);
	}
#endif

	QET::trackDialogGeometry(this);
}

/// Destructeur
ConfigDialog::~ConfigDialog()
{
}

/**
	Construit la liste des pages sur la gauche
*/
void ConfigDialog::showEvent(QShowEvent *event)
{
    if (pages_list->currentRow() < 0 && !pages.isEmpty()) setCurrentPage(0);
    QDialog::showEvent(event);
}

void ConfigDialog::showPage23(int index)
{
    if (index < 0 || index >= pages.size()) return;
    if (!pages[index] && m_factories23[index]) {
        // Keep the placeholder in the stack: page indices and the selection
        // never change while constructing a page. Unopened pages write nothing.
        auto *holder = pages_widget->widget(index);
        auto *page = m_factories23[index]();
        pages[index] = page;
        auto *layout = new QVBoxLayout(holder);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(page);
        m_factories23[index] = {};
    }
    pages_widget->setCurrentIndex(index);
}

void ConfigDialog::addLazyPage23(const QString &title, const QIcon &icon,
                               std::function<ConfigPage *()> factory)
{
    if (!factory) return;
    pages.append(nullptr);
    m_factories23.append(std::move(factory));
    pages_widget->addWidget(new QWidget);
    auto *button = new QListWidgetItem(icon, title, pages_list);
    button->setTextAlignment(Qt::AlignHCenter);
    button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}


/**
	Add the \a page ConfigPage to this configuration dialog.
*/
void ConfigDialog::addPageToList(ConfigPage *page) {
	QListWidgetItem *new_button = new QListWidgetItem(pages_list);
	new_button -> setIcon(page -> icon());
	new_button -> setText(page -> title());
	new_button -> setTextAlignment(Qt::AlignHCenter);
	new_button -> setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

/**
	Applique la configuration de toutes les pages
*/
void ConfigDialog::applyConf()
{
	foreach(ConfigPage *page, pages) {
		if (page) page -> applyConf();
	}
	accept();
}

/**
	Ajoute une page au dialogue de configuration
*/
void ConfigDialog::addPage(ConfigPage *page) {
	if (!page || pages.contains(page)) return;
	pages << page;
    m_factories23.append(std::function<ConfigPage *()>{});
	pages_widget -> addWidget(page);
	addPageToList(page);
}

/**
	@brief ConfigDialog::setCurrentPage
	Set the current index to index
	@param index
*/
void ConfigDialog::setCurrentPage(const int index) {
	pages_list->setCurrentRow(index);
    showPage23(index);
}

void ConfigDialog::addApplicationPages23()
{
    addLazyPage23(QCoreApplication::translate("GeneralConfigurationPage", "General", "configuration page title"), QET::Icons::Settings, []() { return new GeneralConfigurationPage; });
    addLazyPage23(QCoreApplication::translate("NewDiagramPage", "New project", "configuration page title"), QET::Icons::Projects, []() { return new NewDiagramPage; });
    addLazyPage23(QCoreApplication::translate("ExportConfigPage", "Export", "configuration page title"), QET::Icons::DocumentExport, []() { return new ExportConfigPage; });
    addLazyPage23(QCoreApplication::translate("PrintConfigPage", "Printing", "configuration page title"), QET::Icons::Printer, []() { return new PrintConfigPage; });
    addLazyPage23(QCoreApplication::translate("ShortcutsConfigPage", "Shortcuts", "configuration page title"), QET::Icons::ConfigureShortcuts, []() { return new ShortcutsConfigPage; });
    addLazyPage23(QCoreApplication::translate("ShortcutBarConfigPage", "Shortcut bar", "configuration page title"), QET::Icons::ConfigureShortcuts, []() { return new ShortcutBarConfigPage; });
    addLazyPage23(QCoreApplication::translate("GesturesConfigPage", "Mouse gestures", "configuration page title"), QET::Icons::ConfigureShortcuts, []() { return new GesturesConfigPage; });
    addLazyPage23(QCoreApplication::translate("ToolbarsConfigPage", "Toolbars", "configuration page title"), QET::Icons::ConfigureToolbars, []() { return new ToolbarsConfigPage; });
    addLazyPage23(QCoreApplication::translate("ToolbarCommandsConfigPage", "Toolbar contents", "configuration page title: what each toolbar holds"), QET::Icons::ConfigureToolbars, []() { return new ToolbarCommandsConfigPage; });
#ifdef QET_SPACEMOUSE_SUPPORT
    addLazyPage23(QCoreApplication::translate("SpaceMouseConfigPage", "3D mouse", "configuration page title"), QET::Icons::ConfigureToolbars, []() { return new SpaceMouseConfigPage; });
#endif
}
