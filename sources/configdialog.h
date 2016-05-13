/*
	Copyright 2006-2015 The QElectroTech Team
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
	This class represents the configuration dialog for QElectroTech.
	It displays "configuration pages", each page having to provide an icon and
	a title.
*/
class ConfigDialog : public QDialog {
	Q_OBJECT
	// constructors, destructor
	public:
	ConfigDialog(QWidget * = 0);
	virtual ~ConfigDialog();
	private:
	ConfigDialog(const ConfigDialog &);

    public:
	QList<ConfigPage *> pages;

	
	// methods
	public slots:
	void applyConf();
	void addPage(ConfigPage *);
	void setCurrentPage(const int index);
	
	private:
	void buildPagesList();
	void addPageToList(ConfigPage *);
	
	// attributes
	private:
	QListWidget *pages_list;
	QStackedWidget *pages_widget;
	QDialogButtonBox *buttons;



};
#endif
