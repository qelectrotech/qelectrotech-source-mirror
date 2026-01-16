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
#ifndef ELEMENTPROPERTIESEDITORWIDGET_H
#define ELEMENTPROPERTIESEDITORWIDGET_H

#include "../../diagramcontext.h"
#include "../../properties/elementdata.h"

#include <QAbstractButton>
#include <QDialog>

namespace Ui {
	class ElementPropertiesEditorWidget;
}

/**
	@brief The ElementPropertiesEditorWidget class
	This class provide a dialog for edit various property of element, like
	the type (master, slave, report etc....) and kind info.
*/
class ElementPropertiesEditorWidget : public QDialog
{
	Q_OBJECT

	//METHODS
	public:
		explicit ElementPropertiesEditorWidget(ElementData data, QWidget *parent = nullptr);
		~ElementPropertiesEditorWidget() override;

		ElementData editedData() {return m_data;}

	private:
		void upDateInterface();
		void setUpInterface();
		void updateTree();
		void populateTree();

		//SLOTS
	private slots:
		void on_m_buttonBox_accepted();
		void on_m_base_type_cb_currentIndexChanged(int index);

		//ATTRIBUTES
	private:
		Ui::ElementPropertiesEditorWidget *ui;
		ElementData m_data;
};

#endif // ELEMENTPROPERTIESEDITORWIDGET_H
