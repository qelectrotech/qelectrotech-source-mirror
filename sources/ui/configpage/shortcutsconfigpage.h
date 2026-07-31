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

class QKeySequenceEdit;
class QLineEdit;
class QTableWidget;

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
		void checkConflicts();
		void resetAllRows();

	private:
		struct Row {
			QString id;
			QKeySequence default_sequence;
			QKeySequenceEdit *edit;
		};

		void populateTable();
		void resetRow(int row_index);

		QLineEdit *m_filter_edit;
		QTableWidget *m_table;
		QList<Row> m_rows;
};

#endif // SHORTCUTSCONFIGPAGE_H
