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
#ifndef TOOLBARCOMMANDSCONFIGPAGE_H
#define TOOLBARCOMMANDSCONFIGPAGE_H

#include "configpage.h"
#include "../../diagramtoolbarsettings.h"

#include <QHash>

class QComboBox;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;

/**
	@brief The ToolbarCommandsConfigPage class
	Choose what each of the diagram editor's toolbars holds, and add,
	rename or delete toolbars of one's own. Commands are dragged or added
	from the list on the left. Changes are kept per toolbar while the
	dialog is open, then saved and applied to the open windows by
	applyConf().
*/
class ToolbarCommandsConfigPage : public ConfigPage
{
		Q_OBJECT

	public:
		explicit ToolbarCommandsConfigPage(QWidget *parent = nullptr);

		void applyConf() override;
		QString title() const override;
		QIcon icon() const override;

	private:
		void fillToolbarCombo(const QString &current);
		void showToolbar();
		void storeToolbar();
		void fillAvailable();
		void addSelected();
		void addSeparator();
		void removeSelected();
		void moveSelected(int step);
		void resetToolbar();
		void newToolbar();
		void renameToolbar();
		void deleteToolbar();
		QListWidgetItem *makeItem(const QString &id) const;

		QComboBox *m_toolbar;
		QLineEdit *m_filter;
		QListWidget *m_available;
		QListWidget *m_chosen;
		QPushButton *m_rename;
		QPushButton *m_delete;
		QString m_shown;
		QList<DiagramToolbarSettings::Toolbar> m_custom;
		QHash<QString, QStringList> m_pending;
		QHash<QString, QString> m_descriptions;
};

#endif // TOOLBARCOMMANDSCONFIGPAGE_H
