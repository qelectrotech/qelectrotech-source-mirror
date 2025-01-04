/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef DIAGRAMEDITORHANDLERSIZEWIDGET_H
#define DIAGRAMEDITORHANDLERSIZEWIDGET_H

#include <QWidget>

namespace Ui {
	class DiagramEditorHandlerSizeWidget;
}

/**
 * @brief The DiagramEditorHandlerSizeWidget class
 * A little widget with a combo box to edit the size of
 * the graphics handler item in a QetDiagramEditor.
 * Just create a new instance of this widget, everything
 * is done by the widget itself.
 */
class DiagramEditorHandlerSizeWidget : public QWidget
{
		Q_OBJECT

	public:
		explicit DiagramEditorHandlerSizeWidget(QWidget *parent = nullptr);
		~DiagramEditorHandlerSizeWidget();

	private slots:
		void on_comboBox_currentIndexChanged(int index);

	private:
		Ui::DiagramEditorHandlerSizeWidget *ui;
};

#endif // DIAGRAMEDITORHANDLERSIZEWIDGET_H
