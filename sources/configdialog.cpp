/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "configpages.h"
#include "qetapp.h"

#include "machine_info.h"

/**
	Constructeur
	@param parent QWidget parent
*/
ConfigDialog::ConfigDialog(QWidget *parent) : QDialog(parent) {
	Machine_info *mymachineinfo= new Machine_info(this);
	//ScrollArea for low screens
	QScrollArea *scroll = new QScrollArea(this);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	// liste des pages
	pages_list = new QListWidget();
	pages_list -> setViewMode(QListView::IconMode);
	if(mymachineinfo->i_max_screen_height()<1000){
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
	dialog_layout->addWidget(scroll); // add scroll to the QDialog's layout
	dialog_layout -> addWidget(buttons);
	setLayout(dialog_layout);

	// connexion signaux / slots
	connect(buttons, SIGNAL(accepted()), this, SLOT(applyConf()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
	connect(pages_list, SIGNAL(currentRowChanged(int)),
		pages_widget, SLOT(setCurrentIndex(int)));

	setMaximumSize(mymachineinfo->i_max_screen_width(),
		       mymachineinfo->i_max_screen_height());
	resize(1400,1000);

#ifdef Q_OS_MACOS
	if (parent) {
		setWindowFlags(Qt::Sheet);
	}
#endif
}

/// Destructeur
ConfigDialog::~ConfigDialog()
{
}

/**
	Construit la liste des pages sur la gauche
*/
void ConfigDialog::buildPagesList()
{
	pages_list -> clear();
	foreach(ConfigPage *page, pages) {
		addPageToList(page);
	}
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
		page -> applyConf();
	}
	accept();
}

/**
	Ajoute une page au dialogue de configuration
*/
void ConfigDialog::addPage(ConfigPage *page) {
	if (!page || pages.contains(page)) return;
	pages << page;
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
}
