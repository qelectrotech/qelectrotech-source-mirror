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
#ifndef PROPERTIESEDITORWIDGET_H
#define PROPERTIESEDITORWIDGET_H

#include <QWidget>

class QUndoCommand;

/**
 * @brief The PropertiesEditorWidget class
 * This class extend QWidget method for have common way
 * to edit propertie.
 */
class PropertiesEditorWidget : public QWidget
{
		Q_OBJECT
	public:
		explicit PropertiesEditorWidget(QWidget *parent = 0);

		virtual void apply() {}
		virtual void reset() {}
		virtual QUndoCommand *associatedUndo () const;
		virtual QString title() const;
		virtual void updateUi() {}

		virtual bool setLiveEdit (bool live_edit);
		bool isLiveEdit() const;

	protected:
		virtual void enableLiveEdit() {}
		virtual void disableLiveEdit() {}

	protected:
		bool m_live_edit;
};

#endif // PROPERTIESEDITORWIDGET_H
