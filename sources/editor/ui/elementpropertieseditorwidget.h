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

class QTableWidget;
class QSpinBox;
class QCheckBox;
class QGroupBox;
class QPushButton;
class QLineEdit;

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
		void populateSlaveGroupsTable();
		void readSlaveGroupsFromTable();
		void createPlcConfigWidgets();
		void populatePlcTable();
		void readPlcTable();

		//SLOTS
	private slots:
		void on_m_buttonBox_accepted();
		void on_m_base_type_cb_currentIndexChanged(int index);
		void on_m_slave_groups_checkbox_toggled(bool checked);
		void on_max_slaves_checkbox_toggled(bool checked);
		void plcAddRow();
		void plcRemoveRow();
		void plcPasteFromClipboard();
		void plcTerminalCountChanged(int row, int count);
		void plcSelectHeaderFont();
		void plcSelectCellFont();

		//ATTRIBUTES
	private:
		Ui::ElementPropertiesEditorWidget *ui;
		ElementData m_data;

		// PLC configuration widgets (created programmatically)
		QGroupBox *m_plc_gb = nullptr;
		QTableWidget *m_plc_table = nullptr;
		QTableWidget *m_plc_terminal_table = nullptr;
		QCheckBox *m_plc_break_checkboxes[4] = {nullptr, nullptr, nullptr, nullptr};
		QSpinBox *m_plc_break_spinboxes[4] = {nullptr, nullptr, nullptr, nullptr};
		QSpinBox *m_plc_row_height_spinbox = nullptr;
		QPushButton *m_plc_header_font_btn = nullptr;
		QPushButton *m_plc_cell_font_btn = nullptr;
		QCheckBox   *m_plc_show_headers_cb = nullptr;
		QFont m_plc_header_font;
		QFont m_plc_cell_font;
		QList<QCheckBox *> m_plc_col_visibility_checkboxes;
		QList<QSpinBox *>  m_plc_col_width_spinboxes;
		QList<QLineEdit *> m_plc_col_name_edits;
};

#endif // ELEMENTPROPERTIESEDITORWIDGET_H
