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
#ifndef AUTONUMBERINGDOCKWIDGET_H
#define AUTONUMBERINGDOCKWIDGET_H

#include "../../projectview.h"
#include "../../qetproject.h"

#include <QDockWidget>

class QComboBox;
class QLineEdit;
class QSpinBox;

namespace Ui {
	class AutoNumberingDockWidget;
}

class AutoNumberingDockWidget : public QDockWidget
{
		Q_OBJECT

	public:
		explicit AutoNumberingDockWidget(QWidget *parent = nullptr);
		~AutoNumberingDockWidget() override;

		void setContext();
		void setProject(QETProject*, ProjectView*);

	public slots:
		void setActive();
		void setConductorActive(DiagramView*);

	private slots:
		void on_m_conductor_cb_activated(int);
		void on_m_element_cb_activated(int);
		void on_m_folio_cb_activated(int);
		void conductorAutoNumChanged();
		void elementAutoNumChanged();
		void folioAutoNumChanged();
		void clear();
		void projectClosed();
		void refreshValueFields();

		void on_m_configure_pb_clicked();

		void on_m_conductor_reset_start_pb_clicked();
		void on_m_element_reset_start_pb_clicked();
		void on_m_folio_reset_start_pb_clicked();

		void on_m_conductor_value_le_editingFinished();
		void on_m_element_value_le_editingFinished();
		void on_m_folio_value_le_editingFinished();

		void on_m_conductor_increase_sb_valueChanged(int);
		void on_m_element_increase_sb_valueChanged(int);
		void on_m_folio_increase_sb_valueChanged(int);

	signals:
		void folioAutoNumChanged(QString);

	private:
		enum class AutoNumCategory { Conductor, Element, Folio };

			/// The four widgets that make up one category's row, bundled so
			/// refreshRow() can be called with just a category instead of
			/// four pointers that must always be passed in matching sets.
		struct Row
		{
			QComboBox *combo;
			QLineEdit *value;
			QSpinBox  *increase;
			QLineEdit *next;
		};
		Row rowFor(AutoNumCategory category) const;

		/**
			@brief resetAutoNum
			Reset the numerotation context currently selected in combo_box
			(for the given category) to a per-type starting value. Does
			nothing if no context is selected.
		*/
		void resetAutoNum(QComboBox *combo_box, AutoNumCategory category);

			/// Read/write the numerotation context named in combo_box.
		NumerotationContext contextFor(QComboBox *combo_box, AutoNumCategory category) const;
		void storeContext(QComboBox *combo_box, AutoNumCategory category, const NumerotationContext &context);

			/// Index of the counter the value field shows and edits: the last
			/// part that actually progresses. Returns -1 when the context has
			/// no such part (e.g. it is only fixed text).
		static int counterIndex(const NumerotationContext &context);

			/// Refresh a value field from its context, and apply a typed value.
		void refreshValueField(QComboBox *combo_box, QLineEdit *line_edit, AutoNumCategory category);
		void applyValueField(QComboBox *combo_box, QLineEdit *line_edit, AutoNumCategory category);

			/// Refresh/apply the increment spin box the same way.
		void refreshIncreaseField(QComboBox *combo_box, QSpinBox *increase_sb, AutoNumCategory category);
		void applyIncreaseField(QComboBox *combo_box, QSpinBox *increase_sb, AutoNumCategory category);

			/// Show what the counter will read after one more step, using
			/// the same NumerotationContextCommands engine the Suivant
			/// button in the full configuration dialog already advances
			/// the whole context with -- so the preview can never disagree
			/// with what actually happens when the number is next consumed.
		void refreshNextField(QComboBox *combo_box, QLineEdit *next_edit, AutoNumCategory category);

			/// Refresh a whole row -- value, increment and next-value
			/// preview -- in one call. Every refresh call site needs the
			/// increment and preview kept in step with the value now, so
			/// this replaces refreshValueField() at each of them.
		void refreshRow(AutoNumCategory category);

		Ui::AutoNumberingDockWidget *ui;
		QETProject* m_project = nullptr;
		ProjectView* m_project_view = nullptr;

};

#endif // AUTONUMBERINGDOCKWIDGET_H
