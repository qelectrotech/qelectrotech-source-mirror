/*
	Copyright 2026 Trovo Tech Solutions
	This file is part of a custom feature set built on QElectroTech.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef WIRESPECDIALOG_H
#define WIRESPECDIALOG_H

#include "wirespec.h"

#include <QDialog>

class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QComboBox;
class QPlainTextEdit;
class CoreColorEditor;
class WireColorComboBox;

/**
	@brief The WireSpecDialog class
	Add / edit form for a single WireSpec. Core colours are entered as a
	comma-separated list for now; the dedicated multi-core builder + colour
	picker is Phase 3. When editing, the wire_id field is read-only (it is the
	primary key).
*/
class WireSpecDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit WireSpecDialog(QWidget *parent = nullptr);

		void setWireSpec(const WireSpec &spec, bool editing);
		WireSpec wireSpec() const;

	private:
		void buildUi();

	private:
		QLineEdit      *m_wire_id          = nullptr;
		QLineEdit      *m_manufacturer     = nullptr;
		QLineEdit      *m_mfr_part_no      = nullptr;
		QLineEdit      *m_supplier         = nullptr;
		QLineEdit      *m_supplier_part_no = nullptr;
		QDoubleSpinBox *m_cross_section    = nullptr;
		QDoubleSpinBox *m_outer_dia        = nullptr;
		QDoubleSpinBox *m_insulation_dia   = nullptr;
		QSpinBox        *m_num_cores       = nullptr;
		CoreColorEditor *m_core_colors     = nullptr;
		QCheckBox       *m_shield          = nullptr;
		QComboBox       *m_shield_type     = nullptr;
		QSpinBox        *m_voltage         = nullptr;
		QSpinBox        *m_temp            = nullptr;
		QCheckBox       *m_flexible        = nullptr;
		WireColorComboBox *m_color_primary = nullptr;
		QPlainTextEdit  *m_notes           = nullptr;
};

#endif // WIRESPECDIALOG_H
