/*
	Copyright 2006-2016 The QElectroTech Team
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
#define AUTONUMBERINGDOCKWIDGET_H_H

#include "qetproject.h"
#include "projectview.h"
#include <QDockWidget>

namespace Ui {
	class AutoNumberingDockWidget;
}

class AutoNumberingDockWidget : public QDockWidget
{
		Q_OBJECT

	public:
		explicit AutoNumberingDockWidget(QWidget *parent = 0, QETProject *project = nullptr);
		~AutoNumberingDockWidget();

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

	signals:
		void folioAutoNumChanged(QString);

	private:
		Ui::AutoNumberingDockWidget *ui;
		QETProject* project_;
		ProjectView* projectview_;

};

#endif // AUTONUMBERINGDOCKWIDGET_H
