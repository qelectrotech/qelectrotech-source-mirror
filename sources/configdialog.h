/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef CONFIG_DIALOG_H
#define CONFIG_DIALOG_H
#include <QDialog>
class ConfigPage;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QDialogButtonBox;
/**
	Cette classe represente le dialogue de configuration de QElectroTech.
	Il s'agit d'un dialogue affichant des "pages de configuration".
	Chaque page de configuration doit fournir une icone et un titre.
*/
class ConfigDialog : public QDialog {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ConfigDialog(QWidget * = 0);
	virtual ~ConfigDialog();
	private:
	ConfigDialog(const ConfigDialog &);
	
	// methodes
	public slots:
	void changePage(QListWidgetItem *, QListWidgetItem *);
	void applyConf();
	void addPage(ConfigPage *);
	
	private:
	void buildPagesList();
	void addPageToList(ConfigPage *);
	
	// attributs
	private:
	QListWidget *pages_list;
	QStackedWidget *pages_widget;
	QDialogButtonBox *buttons;
	QList<ConfigPage *> pages;
};
#endif
