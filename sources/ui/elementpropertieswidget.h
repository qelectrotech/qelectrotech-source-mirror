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
#ifndef ELEMENTPROPERTIESWIDGET_H
#define ELEMENTPROPERTIESWIDGET_H

#include <QtGui>
#include <qetgraphicsitem/element.h>
#include <diagram.h>
#include <folioreportproperties.h>
#include <elementinfowidget.h>
#include <masterpropertieswidget.h>

class elementpropertieswidget : public QDialog
{
	Q_OBJECT
	public:
	explicit elementpropertieswidget(Element *elmt, QWidget *parent = 0);

	private:
	QWidget* generalWidget();
	void buildInterface();

	signals:
	/// Signal emitted when users wish to locate an element from the diagram within elements collection
	void findElementRequired(const ElementsLocation &);
	/// Signal emitted when users wish to edit an element from the diagram
	void editElementRequired(const ElementsLocation &);

	public slots:
	void standardButtonClicked (QAbstractButton *);
	void findInPanel ();
	void editElement ();

	private:
	FolioReportProperties *frp_;
	ElementInfoWidget *eiw_;
	MasterPropertiesWidget *mpw_;
	QDialogButtonBox *dbb;
	Element *element_;
	Diagram *diagram_;
	QTabWidget *tab_;
	QPushButton *find_in_panel, *edit_element;

};

#endif // ELEMENTPROPERTIESWIDGET_H
