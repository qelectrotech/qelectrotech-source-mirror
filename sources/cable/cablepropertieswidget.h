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
#ifndef CABLEPROPERTIESWIDGET_H
#define CABLEPROPERTIESWIDGET_H

#include "cable.h"

#include <QWidget>

class QCheckBox;
class QGridLayout;
class QLineEdit;
class QPushButton;
class QSpinBox;

/**
	@brief The fields a cable is filled in with, and the tick before each
	of them saying whether what stands there is written on the drawing.

	It holds no cable of its own: it is a plain form, filled in with
	setProperties() and read back with properties(). That way the dialog
	editing one cable and the selection panel of QElectroTech offer
	exactly the same fields in the same order, and neither of them can
	drift away from the other.

	Every field is followed by a checkbox: what the checkbox says is
	written next to the trunk line of the drawing, what it does not say
	stays in the cable alone and only shows up in lists.

	Behind every field stands a button opening the window where that one
	text is written: its font, its size, its style and -- for the texts
	drawn next to the line -- the way it lines up there. The colours of
	the cores get a button of their own at the foot of the form, all of
	them at once, because they are written in one row along the line and
	are never set apart.

	The type field carries one more button, on the right of that row. It
	does not say how the word is written: it asks for the window which
	opens when a cable line has just been drawn, where a type is picked
	out of the cable type file -- with its search, its filter and its own
	button for a type the file does not hold yet -- and taken onto the
	cable. Whoever holds this form answers typeChangeRequested().

	At the foot of the form stand the two distances he may nudge: one for
	the whole label, one for the row of colours. Both are on the drawing
	only -- the line, the slashes and the wiring stay where they are.
*/
class CablePropertiesWidget : public QWidget
{
		Q_OBJECT

	public:
		explicit CablePropertiesWidget(QWidget *parent = nullptr);

			///What the form holds right now
		CableProperties properties() const;
			///Fill the form in
		void setProperties(const CableProperties &properties);
			///True while one of the fields is being typed in
		bool editing() const;

	signals:
			///Emitted when a field or a tick was really changed by hand
		void propertiesEdited();
			///Emitted with every character typed, so the drawing can
			///follow the text as it is written rather than only once the
			///field is left
		void propertiesTyped();
			///Emitted when he asks for the window choosing the type of
			///this cable out of the cable type file
		void typeChangeRequested();

	private:
		void addField(QGridLayout *grid,
					  int row,
					  const QString &key,
					  const QString &name,
					  QLineEdit **edit,
					  QCheckBox **tick);
			///The button opening the window which sets one text
		QPushButton *addFormatButton(QGridLayout *grid, int row, const QString &key);
			/**
				Build one row of the two spin boxes nudging what is
				drawn: one for the label of the cable, one for the row
				of its colours.
				@param grid the form being built
				@param row where to put it
				@param name what the row moves
				@param tip what that distance does and does not touch
				@param dx receives the spin box for the distance along
				the line
				@param dy receives the spin box for the distance across
				it
			*/
		void addOffsetRow(QGridLayout *grid,
						  int row,
						  const QString &name,
						  const QString &tip,
						  QSpinBox **dx,
						  QSpinBox **dy);
			///Open the window setting one text, then say so if it changed
		void editFormat(const QString &key);
			///Let the button say which size that text is written with
		void refreshFormatButton(const QString &key);
			///The font this text is really written with right now
		QFont effectiveFont(const QString &key) const;
			///True when this key names the colours of the cores
		static bool coresKey(const QString &key) {return key == QLatin1String("cores");}

	private:
		QLineEdit *m_designation = nullptr;
			/**
				The name exactly as it was read from the cable, and
				whether that name was typed in: anything else he puts
				into the field from then on is his own work, and the
				cable is marked as carrying a name he chose (see
				properties())
			*/
		QString m_designation_text;
		bool m_designation_by_hand = false;
		QLineEdit *m_type = nullptr;
		QLineEdit *m_installation = nullptr;
		QLineEdit *m_location = nullptr;
		QLineEdit *m_length = nullptr;
		QCheckBox *m_show_designation = nullptr;
		QCheckBox *m_show_type = nullptr;
		QCheckBox *m_show_installation = nullptr;
		QCheckBox *m_show_location = nullptr;
		QCheckBox *m_show_length = nullptr;
			///What each text of this form is written with; a text which
			///is not in here follows what the preferences say
		QHash<QString, CableTextFormat> m_formats;
			///The button opening the window of each text, by name
		QHash<QString, QPushButton *> m_format_buttons;
			///How far the label of the cable is nudged on the drawing
		QSpinBox *m_text_offset_x = nullptr;
		QSpinBox *m_text_offset_y = nullptr;
			///How far the row of colours is nudged on the drawing
		QSpinBox *m_core_offset_x = nullptr;
		QSpinBox *m_core_offset_y = nullptr;
};

#endif // CABLEPROPERTIESWIDGET_H
