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

		void on_m_configure_pb_clicked();

		void on_m_conductor_reset_start_pb_clicked();
		void on_m_conductor_reset_placeholder_pb_clicked();
		void on_m_element_reset_start_pb_clicked();
		void on_m_element_reset_placeholder_pb_clicked();
		void on_m_folio_reset_start_pb_clicked();
		void on_m_folio_reset_placeholder_pb_clicked();

	signals:
		void folioAutoNumChanged(QString);

	private:
		enum class AutoNumCategory { Conductor, Element, Folio };

		/**
			@brief resetAutoNum
			Reset the numerotation context currently selected in combo_box
			(for the given category) either to a per-type starting value
			(to_placeholder = false) or to the literal placeholder "?" on
			every part (to_placeholder = true). Does nothing if no context
			is selected.
		*/
		void resetAutoNum(QComboBox *combo_box, AutoNumCategory category, bool to_placeholder);

		Ui::AutoNumberingDockWidget *ui;
		QETProject* m_project = nullptr;
		ProjectView* m_project_view = nullptr;

};

#endif // AUTONUMBERINGDOCKWIDGET_H
