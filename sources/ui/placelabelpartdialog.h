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
#ifndef PLACELABELPARTDIALOG_H
#define PLACELABELPARTDIALOG_H

#include <materiallist/materiallist.h>

#include <QDialog>
#include <QPointer>

class Element;
class QCompleter;
class QLabel;
class QModelIndex;
class QStandardItemModel;
class QLineEdit;
class QUndoCommand;
class DeviceType;

/**
	@brief The PlaceLabelPartDialog class
	One small dialog per symbol just placed: its device label and its part
	number, the part bringing the device type. Asked when the setting
	QetSettings::askLabelAndPartPlacingSymbol() is on, for a master, a
	simple symbol or a terminal.

	The label proposed is the next free one of the symbol's class letter;
	for a terminal, the next free terminal of the strip of the last one
	placed in the project (X1:3, then X1:4), and a strip name alone (X2)
	means its next free terminal. The part number is completed from the
	material list, which can also be opened to choose a part.
*/
class PlaceLabelPartDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit PlaceLabelPartDialog(Element *symbol, QWidget *parent = nullptr);

		QString label() const;
		QString part() const;
		MaterialRecord record() const;
		void setLabel(const QString &label);
		void setPart(const QString &part);
		void accept() override;

			/// Whether placing @p symbol asks its label and part: the
			/// setting is on, and it is a master, a simple symbol or a terminal
		static bool asksFor(Element *symbol);
		static QString proposedLabel(Element *symbol);
			/// The part numbers already used in @p symbol's project, other
			/// than by @p symbol itself
		static QStringList usedParts(Element *symbol);
			/// What @p text typed for @p symbol's label means: a strip
			/// name alone, for a terminal, is the strip's next free terminal
		static QString resolvedLabel(Element *symbol, const QString &text);
		static QString nextFreeTerminal(Element *symbol, const QString &strip);
		static bool terminalExists(Element *symbol, const QString &label);
			/// The undo step giving @p symbol @p label, the values of @p part
			/// and its device type; nullptr if nothing changes
		static QUndoCommand *apply(Element *symbol, const QString &label,
								   const MaterialRecord &part);
			/// The place (=plant+location) @p symbol takes by joining the
			/// device its label names, when it is not the one it has:
			/// empty if the label names no device or the place is the same
		static QString placeToJoin(Element *symbol, const QString &label);
			/// What giving @p symbol the label @p label and the part
			/// @p part does to its terminals: the device type it takes
			/// (the part's, else that of the device the label names), and
			/// the first part of that type no other symbol of the device
			/// draws which this symbol's drawing can take the pins of
		struct Pins
		{
			QString type;           ///< the device type, empty when none
			int part = -1;          ///< the part, -1 when none
			int way = -1;
			QStringList names;      ///< its pins, as the terminals are named
			bool already = false;   ///< the terminals carry these names already
			bool none_free = false; ///< a type, but no part free for this symbol
		};
		static Pins pinsFor(Element *symbol, const QString &label, const MaterialRecord &part);
			/// apply() pushed on the symbol's folio, and the symbol takes
			/// the terminal names of its part (DeviceTypeAssign), all in one
			/// undo step. The symbol is replaced then: the one now in the
			/// folio is returned (@p symbol when it is not). @p none_free
			/// tells that a type was given but no part could be
		static Element *applyAndPush(Element *symbol, const QString &label,
									 const MaterialRecord &part, bool *none_free = nullptr);
			/// Whether the device type question (DeviceTypeChooser) is still
			/// to be asked for @p symbol, which a part may have answered
		static bool asksDeviceType(Element *symbol);
			/// Ask, apply, then the device type question when still needed
		static void askFor(Element *symbol, QWidget *parent = nullptr);

	private:
		void partEdited(const QString &text);
		void suggest(const QString &text);
		void suggestionChosen(const QModelIndex &index);
		void choosePart();
		void showPart();
		static QString partNumberOf(const MaterialRecord &record);

		QPointer<Element> m_symbol;
		QLineEdit *m_label = nullptr;
		QLineEdit *m_part = nullptr;
		QLabel *m_info = nullptr;
		QCompleter *m_completer = nullptr;
		QStandardItemModel *m_suggestions = nullptr;
		MaterialListData m_data;
		QStringList m_used;
		MaterialRecord m_record;
};

#endif // PLACELABELPARTDIALOG_H
