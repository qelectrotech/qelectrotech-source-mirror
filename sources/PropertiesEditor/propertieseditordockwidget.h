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
#ifndef SELECTIONPROPERTIESDOCKWIDGET_H
#define SELECTIONPROPERTIESDOCKWIDGET_H

#include <QDockWidget>

class PropertiesEditorWidget;

namespace Ui {
	class PropertiesEditorDockWidget;
}

class PropertiesEditorDockWidget : public QDockWidget
{
		Q_OBJECT

	public:
		explicit PropertiesEditorDockWidget(QWidget *parent = 0);
		~PropertiesEditorDockWidget();

		virtual void clear();
		virtual void apply();
		virtual void reset();
		bool addEditor (PropertiesEditorWidget *editor, int index = 0);
		QList<PropertiesEditorWidget *> editors() const;
		bool removeEditor (PropertiesEditorWidget *editor);

	protected:
		QList <PropertiesEditorWidget *> m_editor_list;

	private:
		Ui::PropertiesEditorDockWidget *ui;
};

#endif // SELECTIONPROPERTIESDOCKWIDGET_H
