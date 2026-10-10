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
#ifndef CABLECREATEDIALOG_H
#define CABLECREATEDIALOG_H

#include "../cablelist/cabletypelist.h"
#include "cable.h"
#include "cablemanager.h"

#include <QDialog>
#include <QList>
#include <QPointer>
#include <QSortFilterProxyModel>

class Diagram;
class QCheckBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QStandardItem;
class QStandardItemModel;
class QTableView;
class QTabWidget;
class QWidget;
class QPushButton;

/**
	@brief Filter keeping the rows the user is looking for.

	Every word he types has to be found somewhere in the row, whatever
	the column it sits in, the same way the material file is searched.
	On top of that it can hide the rows which are of no use to the cable
	being drawn -- the checkbox each of the two tabs carries.
*/
class CableFilterProxy : public QSortFilterProxyModel
{
		Q_OBJECT

	public:
		static constexpr int UnsuitableRole = Qt::UserRole + 1;

		explicit CableFilterProxy(QObject *parent = nullptr);

		void setTokens(const QStringList &tokens);
		void setHideUnsuitable(bool hide);

	protected:
		bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

	private:
		QStringList m_tokens;
		bool m_hide_unsuitable = true;
};

/**
	@brief The small window filling in one line of the cable type file.

	It is the cable counterpart of the window the material list opens to
	add one of its own articles: one field per column of the file, in
	file order, and a column the program doesn't know still gets its
	field since it belongs to the user rather than to QElectroTech. The
	two columns a cable type is read by get the shape they need -- a
	number which counts, and colours written the way the file separates
	them.
*/
class CableTypeEntryDialog : public QDialog
{
	Q_OBJECT

	public:
		/**
			@param columns the columns of the cable type file, in file
			order
			@param parent
		*/
		explicit CableTypeEntryDialog(const QStringList &columns, QWidget *parent = nullptr);

			///The line the form describes, empty fields left out
		CableTypeRecord record() const;
			/**
				Fill the form with a line which is already in the file,
				for the window where that line is edited rather than
				added: the window then says so, and every field carries
				what the file holds about that column.
				@param values the line as it stands in the file
			*/
		void setValues(const CableTypeRecord &values);

		void accept() override;

	private:
		QStringList m_columns;
			///One field per column, in file order, for the tab key
		QList<QWidget *> m_fields;
			///What the window says it is about, changed when editing
		QLabel *m_intro = nullptr;
};

/**
	@brief The dialog which opens as soon as a cable line has been drawn.

	It says how many cores were recognised, then lets the user either
	pick a type from the cable type file -- a brand new cable -- or pick
	one of the cables the project already holds and give it more cores.
	Both tabs carry a search field and the checkbox hiding what cannot be
	used, exactly like the material selection does.

	The very same window opens to change the type of a cable which
	already exists: it then shows the type tab alone, filled in with what
	that cable holds, and takes the chosen type onto the cable -- cores
	reworked, marks redrawn -- as one single undo step. It carries the
	button filling in a new line of the cable type file, the way the
	material window carries its own.
*/
class CableCreateDialog : public QDialog
{
		Q_OBJECT

	public:
		CableCreateDialog(Diagram *diagram,
						  const QList<CableCrossing> &crossings,
						  const CablePartData &part,
						  QWidget *parent = nullptr);

		/**
			The same window, opened to change the type of a cable which
			already exists rather than to settle a line just drawn.
			@param diagram the folio whose undo stack takes the change
			@param cable the cable whose type is being changed
			@param parent
		*/
		CableCreateDialog(Diagram *diagram,
						  Cable *cable,
						  QWidget *parent = nullptr);

			///The cable the user settled on, null when he cancelled
		Cable *cable() const {return m_cable;}
			///The cores this cable took away from cables the project
			///already held, so that undo gives them back
		QList<CableCoreTaken> takenCores() const {return m_taken;}

		void accept() override;

	private:
		void build();
		void buildNewTab();
		void buildExistingTab();
		void buildPropertyRow();
		void loadTypeFile();
		void refreshExisting();
		void selectionChanged();
			///Fill the name field in and remember what this window put there
		void fillDesignation(const QString &text, bool by_hand);
		void assignCrossings(const QList<int> &core_slots);
		QList<CableCoreTaken> coresTakenFromOthers(const QUuid &target) const;
		QList<int> nextFreeSlots(int count) const;
		QModelIndex currentSourceIndex() const;
		bool currentSelectionIsUnsuitable() const;
		QString unsuitableNote() const;
		QStandardItem *cell(const QString &value, bool unsuitable) const;
			///True while this window changes the type of an existing cable
		bool changeMode() const {return m_change != nullptr;}
			///How many cores whatever he picks has to hold at least
		int neededCores() const;
			///Where the cable type file stands
		QString typeFilePath() const;
			///The button filling in a new line of the cable type file
		void addTypeRecord();
			///The button rewriting the line of the cable type file he picked
		void editTypeRecord();
			///Take the chosen type onto the cable being changed
		void acceptTypeChange();

		//ATTRIBUTES
	private:
		QPointer<Diagram> m_diagram;
		QList<CableCrossing> m_crossings;
		CablePartData m_part;
		Cable *m_cable = nullptr;
			///The cable whose type is changed, null while a line is drawn
		QPointer<Cable> m_change;
			///The cores other cables lost to this one, given back by undo
		QList<CableCoreTaken> m_taken;

		QLabel *m_header = nullptr;
		QTabWidget *m_tabs = nullptr;
		QDialogButtonBox *m_buttons = nullptr;

		QLineEdit *m_new_search = nullptr;
		QCheckBox *m_new_hide = nullptr;
		QTableView *m_new_table = nullptr;
		QStandardItemModel *m_new_model = nullptr;
		CableFilterProxy *m_new_proxy = nullptr;
		QLabel *m_new_note = nullptr;
			///The button rewriting the chosen line of the file, which
			///can only work on a line which was picked
		QPushButton *m_edit_button = nullptr;
		CableTypeListData m_types;

		QLineEdit *m_existing_search = nullptr;
		QCheckBox *m_existing_hide = nullptr;
		QTableView *m_existing_table = nullptr;
		QStandardItemModel *m_existing_model = nullptr;
		CableFilterProxy *m_existing_proxy = nullptr;
		QLabel *m_existing_note = nullptr;
		QList<Cable *> m_existing;

		QWidget *m_property_row = nullptr;
		QLineEdit *m_designation = nullptr;
			/**
				The name exactly as this window filled it in, and where
				that name came from: whatever he types over it from then
				on is a name of his own (see accept())
			*/
		QString m_designation_prefill;
		bool m_designation_prefill_by_hand = false;
		QLineEdit *m_installation = nullptr;
		QLineEdit *m_location = nullptr;
};

#endif // CABLECREATEDIALOG_H
