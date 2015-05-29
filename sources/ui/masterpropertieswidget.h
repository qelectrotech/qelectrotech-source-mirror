/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef MASTERPROPERTIESWIDGET_H
#define MASTERPROPERTIESWIDGET_H

#include <QWidget>
#include <QHash>
#include "abstractelementpropertieseditorwidget.h"

class QListWidgetItem;
class Element;
class QUndoCommand;
class QETProject;
class Diagram;

namespace Ui {
	class MasterPropertiesWidget;
}

/**
 * @brief The MasterPropertiesWidget class
 * This class is a widget for make link between a master element with several slave element.
 * This class embenddedthe undo/redo command when apply new connection.
 */

class MasterPropertiesWidget : public AbstractElementPropertiesEditorWidget
{
	Q_OBJECT

	public:
		explicit MasterPropertiesWidget(Element *elmt, QWidget *parent = 0);
		~MasterPropertiesWidget();

		void setElement (Element *element);
		void apply();
		void reset();
		QUndoCommand *associatedUndo () const;
		QString title() const {return tr("Référence croisée (maitre)");}
		bool setLiveEdit(bool live_edit);

	public slots:
		void updateUi();

	private slots:
		void on_link_button_clicked();
		void on_unlink_button_clicked();
		void showElementFromLWI(QListWidgetItem *lwi);
		void showedElementWasDeleted ();
		void diagramWasdeletedFromProject();

	private:
	Ui::MasterPropertiesWidget *ui;
	QHash <QListWidgetItem *, Element *> lwi_hash;
	Element *m_showed_element;
	QETProject *m_project;
};

#endif // MASTERPROPERTIESWIDGET_H
