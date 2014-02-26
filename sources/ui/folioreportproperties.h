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
#ifndef FOLIOREPORTPROPERTIES_H
#define FOLIOREPORTPROPERTIES_H

#include <QWidget>
#include <qetgraphicsitem/element.h>

namespace Ui {
	class FolioReportProperties;
}

/**
 * @brief The FolioReportProperties class
 * This class is a widget for make link between two reports element.
 * This class embendded the undo/redo command when apply new connection.
 */
class FolioReportProperties : public QWidget
{
	Q_OBJECT

	public:
	explicit FolioReportProperties(Element *elmt, QWidget *parent = 0);
	~FolioReportProperties();
	void Apply();

	private slots:
	void linkToElement(const int i) {element_to_link = element_list.at(i);}
	void unlinkClicked();
	void showElement(Element *elmt);
	void showElementFromList (const int i);
	void on_button_this_clicked();
	void on_button_linked_clicked();

	private:
	void buildRadioList();
	void buildUnlinkButton();

	Element *element_, *element_to_link;
	QList <Element *> element_list;
	Ui::FolioReportProperties *ui;
	QSignalMapper *sm_, *sm_show_;
	QWidget *unlink_widget;
	bool unlink;
};

#endif // FOLIOREPORTPROPERTIES_H
