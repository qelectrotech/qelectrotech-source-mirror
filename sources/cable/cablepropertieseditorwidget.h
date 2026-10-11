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
#ifndef CABLEPROPERTIESEDITORWIDGET_H
#define CABLEPROPERTIESEDITORWIDGET_H

#include "cable.h"

#include "../PropertiesEditor/propertieseditorwidget.h"

#include <QList>
#include <QMetaObject>
#include <QPointer>

class CablePropertiesWidget;
class CablePart;
class QLabel;
class QListWidget;
class QListWidgetItem;

/**
	@brief The cable shown in the "Propriétés de la sélection" panel.

	Selecting a line of a cable used to bring up nothing at all there:
	the panel only knew the item types it already had an editor for.
	Now it holds the very fields the cable is filled in with -- each of
	them with its own tick saying whether the drawing shows it -- and
	what stands there is written back live, one undo step per edit.
*/
class CablePropertiesEditorWidget : public PropertiesEditorWidget
{
		Q_OBJECT

	public:
		explicit CablePropertiesEditorWidget(Cable *cable = nullptr,
											 CablePart *part = nullptr,
											 QWidget *parent = nullptr);

		void setCable(Cable *cable, CablePart *part = nullptr);

		void apply() override;
		void reset() override;
		void updateUi() override;
		QUndoCommand *associatedUndo() const override;
		QString title() const override;
		bool setLiveEdit(bool live_edit) override;

	private:
			///Write what is being typed onto the cable right away
		void applyLive();
			///He asked for the window choosing another type for this cable
		void changeTypeRequested();
			///Say how many cores of this cable are wired
		void updateInfo();
			/**
				List the cores which stand nowhere on the sheet yet, under
				the count of wired ones: those are the ones he can still
				put down, and double-clicking one puts it down straight
				from here, on the line this panel was opened for.
			*/
		void updateFreeCores();
			///He double-clicked one of those cores: give it this line
		void placeRequested(QListWidgetItem *item);
			/**
				He right-clicked one of them: a small menu with the one
				thing which can be done to a core which stands nowhere
				-- putting it down -- for whoever does not know that a
				double click does the same.
				@param pos where he clicked, in list coordinates
			*/
		void freeCoreMenu(const QPoint &pos);

	private:
		CablePropertiesWidget *m_fields = nullptr;
		QLabel *m_info = nullptr;
			///The cores which stand nowhere yet, listed under the count
		QLabel *m_free_label = nullptr;
		QListWidget *m_free_cores = nullptr;
		QPointer<Cable> m_cable;
			/**
				The line this panel was opened by, which is the one a core
				picked here is put down on: taking a core always belongs to
				the line whose dialog was asked for, never to the first
				line of the cable. Null while no single line is selected,
				and then there is nothing to put a core down on and the
				list takes no room.
			*/
		QPointer<CablePart> m_part;
			///What the panel showed before the edit: the fields which
			///differ from it are the ones the user changed
		CableProperties m_shown;
			///The cable the text is being typed straight into, so that
			///reading the cable back only skips the very field which is
			///under the fingers of the user -- and never for another
			///cable which has just become the selected one
		QPointer<Cable> m_live_cable;
		QMetaObject::Connection m_live_connection;
		bool m_updating = false;
};

#endif // CABLEPROPERTIESEDITORWIDGET_H
