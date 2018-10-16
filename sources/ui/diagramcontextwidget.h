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
#ifndef DIAGRAMCONTEXTWIDGET_H
#define DIAGRAMCONTEXTWIDGET_H

#include <QWidget>
#include "diagramcontext.h"

namespace Ui {
	class DiagramContextWidget;
}

class DiagramContextWidget : public QWidget
{
	Q_OBJECT
	
	public:
		explicit DiagramContextWidget(QWidget *parent = nullptr);
		~DiagramContextWidget();
	
		DiagramContext context() const;
		void setContext (const DiagramContext &context);
		int nameLessRowsCount() const;
		
	public slots:
		void clear();
		int highlightNonAcceptableKeys();
		void refreshFormatLabel();
		
	private slots:
		void checkTableRows();
		
	private:
		Ui::DiagramContextWidget *ui;
};

#endif // DIAGRAMCONTEXTWIDGET_H
