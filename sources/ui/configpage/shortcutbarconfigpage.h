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
#ifndef SHORTCUTBARCONFIGPAGE_H
#define SHORTCUTBARCONFIGPAGE_H

#include "configpage.h"
#include "../../shortcutbarsettings.h"

#include <QHash>

class QComboBox;
class QListWidget;

/**
	@brief The ShortcutBarConfigPage class
	Choose which commands the diagram editor's shortcut bar shows, and in
	what order, for each selection context. Any command registered with
	ShortcutManager by the diagram editor can be added. Changes are kept
	per context while the dialog is open and saved by applyConf().
*/
class ShortcutBarConfigPage : public ConfigPage
{
		Q_OBJECT

	public:
		explicit ShortcutBarConfigPage(QWidget *parent = nullptr);

		void applyConf() override;
		QString title() const override;
		QIcon icon() const override;

	private:
		void showContext();
		void storeContext();
		void addSelected();
		void removeSelected();
		void moveSelected(int step);
		void resetContext();
		void appendItem(QListWidget *list, const QString &id);

		QComboBox *m_context;
		QListWidget *m_available;
		QListWidget *m_chosen;
		ShortcutBarSettings::Context m_shown = ShortcutBarSettings::Canvas;
		QHash<int, QStringList> m_pending;
		QHash<QString, QString> m_descriptions;
};

#endif // SHORTCUTBARCONFIGPAGE_H
