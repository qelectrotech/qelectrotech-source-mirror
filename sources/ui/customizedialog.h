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
#ifndef CUSTOMIZEDIALOG_H
#define CUSTOMIZEDIALOG_H

#include <QDialog>
#include <QList>

class ConfigPage;
class QTabWidget;

/**
	@brief The CustomizeDialog class
	Everything about how the user works in one window, one tab per
	configuration page: toolbars, what they hold, the shortcut bar, the
	keyboard and the mouse gestures, like SolidWorks' Tools > Customize.
	The pages are the ones of the configuration dialog, which still shows
	them too. OK applies every page, Cancel none.
*/
class CustomizeDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit CustomizeDialog(QWidget *parent = nullptr);

		void addPage(ConfigPage *page);
		QList<ConfigPage *> pages() const;
		void applyConf();

	private:
		QTabWidget *m_tabs;
		QList<ConfigPage *> m_pages;
};

#endif // CUSTOMIZEDIALOG_H
