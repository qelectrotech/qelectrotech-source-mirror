/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "configdialog.h"
#include "configpages.h"
#include "qetapp.h"

/**
	Constructeur
	@param parent QWidget parent
*/
ConfigDialog::ConfigDialog(QWidget *parent) : QDialog(parent) {
	// liste des pages
	pages_list = new QListWidget();
	pages_list -> setViewMode(QListView::IconMode);
	pages_list -> setIconSize(QSize(110, 110));
	pages_list -> setMovement(QListView::Static);
	pages_list -> setMinimumWidth(150);
	pages_list -> setMaximumWidth(150);
	pages_list -> setSpacing(4);
	
	// pages
	pages_widget = new QStackedWidget();
	
	// boutons
	buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	
	// layouts
	QHBoxLayout *hlayout1 = new QHBoxLayout();
	hlayout1 -> addWidget(pages_list);
	hlayout1 -> addWidget(pages_widget);
	
	QVBoxLayout *vlayout1 = new QVBoxLayout();
	vlayout1 -> addLayout(hlayout1);
	vlayout1 -> addWidget(buttons);
	setLayout(vlayout1);
	
	// connexion signaux / slots
	connect(buttons, SIGNAL(accepted()), this, SLOT(applyConf()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
	connect(pages_list, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
	
#ifdef Q_WS_MAC
	if (parent) {
		setWindowFlags(Qt::Sheet);
	}
#endif
}

/// Destructeur
ConfigDialog::~ConfigDialog() {
}

/**
	Gere les changements de page dans le dialogue de configuration
*/
void ConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous) {
	if (!current) current = previous;
	pages_widget -> setCurrentIndex(pages_list -> row(current));
}

/**
	Construit la liste des pages sur la gauche
*/
void ConfigDialog::buildPagesList() {
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
void ConfigDialog::applyConf() {
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
