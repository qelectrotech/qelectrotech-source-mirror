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
#ifndef SELECTAUTONUMW_H
#define SELECTAUTONUMW_H

#include <QWidget>
#include "diagram.h"
#include "ui/numparteditorw.h"

namespace Ui {
	class SelectAutonumW;
}

class SelectAutonumW : public QWidget
{
	Q_OBJECT
	
	//METHODS
	public:
	explicit SelectAutonumW(const QList <Diagram *> &, Diagram * = 0, QWidget *parent = 0);
	~SelectAutonumW();
	
	private:
	void initDiagramChooser();
	void setCurrentContext ();
	NumerotationContext toNumContext() const;

	//SLOT
	private slots:
	void on_add_button_clicked();
	void on_remove_button_clicked();
	void on_diagram_chooser_activated();
	void on_buttonBox_clicked(QAbstractButton *);
	void applyEnable (bool = true);

	//ATTRIBUTS
	private:
	Ui::SelectAutonumW *ui;
	const QList <Diagram *> diagram_list;
	QList <NumPartEditorW *> num_part_list_;
};

#endif // SELECTAUTONUMW_H
