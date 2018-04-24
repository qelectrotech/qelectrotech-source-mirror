/*
	Copyright 2006-2018 The QElectroTech team
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
#ifndef MULTIPASTEDIALOG_H
#define MULTIPASTEDIALOG_H

#include <QDialog>
#include "diagramcontent.h"
#include "QDomDocument"

class Diagram;

namespace Ui {
	class MultiPasteDialog;
}

class MultiPasteDialog : public QDialog
{
	Q_OBJECT
	
	public:
		explicit MultiPasteDialog(Diagram *diagram, QWidget *parent = nullptr);
		~MultiPasteDialog();
		void updatePreview();
	
	private slots:
		void on_m_button_box_accepted();
		
	private:
		Ui::MultiPasteDialog *ui;
		Diagram *m_diagram = nullptr;
		DiagramContent m_pasted_content;
		QPointF m_origin;
		QDomDocument m_document;
		bool m_accept = false;
		QList<DiagramContent> m_pasted_content_list;
};

#endif // MULTIPASTEDIALOG_H
