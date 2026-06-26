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
class QLabel;
class CoreColorEditor;
class WireColorComboBox;

/**
	@brief The WireSpecDialog class
	Tabbed add / edit form for a wire / cable, modelled on the SolidWorks
	Electrical "New cable reference" dialog:
	  - General tab: identity, supplier and characteristics (grouped).
	  - Cable cores tab: the per-core editor (each core = base + up to 2 tracer
	    colours), with Add / Remove core buttons.
	When editing, the wire_id field is read-only (it is the primary key).
*/
class WireSpecDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit WireSpecDialog(QWidget *parent = nullptr);

		void setWireSpec(const WireSpec &spec, bool editing);
		WireSpec wireSpec() const;

	private:
		QWidget *buildGeneralTab();
		QWidget *buildCoresTab();
		void updateCoreCountLabel();

	private:
		QLineEdit      *m_wire_id          = nullptr;
		QLineEdit      *m_family           = nullptr;
		QLineEdit      *m_manufacturer     = nullptr;
		QLineEdit      *m_mfr_part_no      = nullptr;
		QLineEdit      *m_supplier         = nullptr;
		QLineEdit      *m_supplier_part_no = nullptr;
		QDoubleSpinBox *m_cross_section    = nullptr;
		QDoubleSpinBox *m_outer_dia        = nullptr;
		QDoubleSpinBox *m_insulation_dia   = nullptr;
		QCheckBox      *m_shield           = nullptr;
		QComboBox      *m_shield_type      = nullptr;
		QSpinBox       *m_voltage          = nullptr;
		QSpinBox       *m_temp             = nullptr;
		QCheckBox      *m_flexible         = nullptr;
		WireColorComboBox *m_color_primary = nullptr;
		QPlainTextEdit *m_notes            = nullptr;

		CoreColorEditor *m_cores           = nullptr;
		QLabel          *m_core_count_lbl  = nullptr;
};

#endif // WIRESPECDIALOG_H
