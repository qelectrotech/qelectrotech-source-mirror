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
#ifndef SHORTCUTSCONFIGPAGE_H
#define SHORTCUTSCONFIGPAGE_H

#include "configpage.h"

#include <QKeySequence>

class QComboBox;
class QKeySequenceEdit;
class QLabel;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

/**
	@brief The ShortcutsConfigPage class
	Configuration page listing every shortcut registered with ShortcutManager,
	letting the user rebind, reset and search them. Bindings are only
	persisted (via ShortcutManager::setSequence()) when applyConf() runs, i.e.
	when the user validates the surrounding ConfigDialog.
*/
class ShortcutsConfigPage : public ConfigPage
{
		Q_OBJECT

	public:
		explicit ShortcutsConfigPage(QWidget *parent = nullptr);
		~ShortcutsConfigPage() override;

		void applyConf() override;
		QString title() const override;
		QIcon icon() const override;

	private slots:
		void filterRows(const QString &filter_text);
		void quickFilterChanged(int index);
		void checkConflicts();
		void resetAllRows();

	private:
		enum QuickFilter {
			ShowAll = 0,
			BoundOnly,
			UnboundOnly,
			ConflictsOnly
		};

		struct Row {
			QString id;
			QString category;
			QString description;
			QKeySequence default_sequence;
			QKeySequenceEdit *edit;
			QTreeWidgetItem *item;
			bool conflicted;
		};

		void populateTable();
		void applyFilter();
		void resetRow(int row_index);

		QLineEdit *m_filter_edit;
		QComboBox *m_quick_filter;
		QLabel *m_count_label;
		QTreeWidget *m_tree;
		QList<Row> m_rows;
};

#endif // SHORTCUTSCONFIGPAGE_H
