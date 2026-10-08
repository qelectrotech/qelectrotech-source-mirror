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
#ifndef CABLEPROPERTIESDIALOG_H
#define CABLEPROPERTIESDIALOG_H

#include "cable.h"

#include <QDialog>
#include <QPointer>

class CablePropertiesWidget;
class CablePart;
class Diagram;
class QLabel;
class QListWidget;
class QListWidgetItem;

/**
	@brief The dialog which edits a cable once it is drawn.

	Everything it holds belongs to the cable itself rather than to the
	line which was drawn: whatever is typed here applies to every section
	of that cable, on every folio, and the conductors it feeds get their
	cable field worked out again as soon as it says it changed.

	Under the count of cores it lists the cores of the type which stand
	nowhere on the sheet yet: double-clicking one sends it to the very
	line this dialog was asked from, which takes the mouse until he
	clicks and leaves the core standing there.
*/
class CablePropertiesDialog : public QDialog
{
		Q_OBJECT

	public:
		/**
			@param cable the cable being edited
			@param diagram the folio whose undo stack takes the edit
			@param part the very line which was double-clicked, where a
			core chosen in this dialog is to be put down
			@param parent
		*/
		explicit CablePropertiesDialog(Cable *cable,
									   Diagram *diagram = nullptr,
									   CablePart *part = nullptr,
									   QWidget *parent = nullptr);

		void accept() override;

	private:
		///Everything the form holds goes into the cable, as one step
		void apply();
		///What the count of cores under the form says right now
		void updateCoreCount();
		///He asked for the window choosing another type for this cable
		void changeTypeRequested();
		///Lists the cores of the type which stand nowhere yet
		void fillFreeCores();
		///He double-clicked one of them: put it down on the line
		void placeRequested(QListWidgetItem *item);
			/**
				The small menu a right click on a core which stands
				nowhere opens: one entry doing what a double click does
				-- the dialog closes, the line takes the mouse, the mark
				shows on it at once and the mouse is put down there.
				@param pos where he right-clicked, in list coordinates
			*/
		void freeCoreMenu(const QPoint &pos);

	private:
		QPointer<Cable> m_cable;
		Diagram *m_diagram = nullptr;
			///The line whose dialog this is, and the one a core chosen
			///here is put down on
		QPointer<CablePart> m_part;
		CablePropertiesWidget *m_fields = nullptr;
		QLabel *m_cores = nullptr;
		QLabel *m_free_label = nullptr;
		QListWidget *m_free_cores = nullptr;
			///True from the moment he picks a core to put down
		bool m_placing = false;
};

#endif // CABLEPROPERTIESDIALOG_H
