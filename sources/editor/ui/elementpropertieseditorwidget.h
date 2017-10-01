/*
	Copyright 2006-2017 The QElectroTech Team
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

#include <QDialog>
#include <QAbstractButton>
#include "diagramcontext.h"

namespace Ui {
	class ElementPropertiesEditorWidget;
}

/**
 * @brief The ElementPropertiesEditorWidget class
 * This class provide a dialog for edit various property of element, like
 * the type (master, slave, report etc....) and kind info.
 */
class ElementPropertiesEditorWidget : public QDialog
{
	Q_OBJECT

	//METHODS
	public:
		explicit ElementPropertiesEditorWidget(QString &basic_type, DiagramContext &kind_info, DiagramContext &elmt_info, QWidget *parent = nullptr);
		~ElementPropertiesEditorWidget() override;

		void upDateInterface();

	private:
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
		QString &m_basic_type;
		DiagramContext &m_kind_info,
					   &m_elmt_info;
};

#endif // ELEMENTPROPERTIESEDITORWIDGET_H
