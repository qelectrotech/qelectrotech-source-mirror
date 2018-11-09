/*
	Copyright 2006-2018 The QElectroTech Team
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
#ifndef REPLACEELEMENTDIALOG_H
#define REPLACEELEMENTDIALOG_H

#include <QDialog>
#include "diagramcontext.h"

class ElementInfoPartWidget;

namespace Ui {
	class ReplaceElementDialog;
}

class ReplaceElementDialog : public QDialog
{
	Q_OBJECT
	
	public:
		explicit ReplaceElementDialog(DiagramContext context, QWidget *parent = nullptr);
		~ReplaceElementDialog();
	
		void setContext(DiagramContext context);
		DiagramContext context() const;
	
	private:
		void buildWidget();
		
		
		Ui::ReplaceElementDialog *ui;
		QList <ElementInfoPartWidget *>  m_eipw_list;
		DiagramContext m_context;
};

#endif // REPLACEELEMENTDIALOG_H
