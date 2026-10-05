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
#ifndef TOOLBARSCONFIGPAGE_H
#define TOOLBARSCONFIGPAGE_H

#include "configpage.h"

class QCheckBox;
class QComboBox;

/**
	@brief The ToolbarsConfigPage class
	Toolbar icon size, text with the icons, and locking the toolbars in
	place, for every window. Saved and applied to the open windows by
	applyConf().
*/
class ToolbarsConfigPage : public ConfigPage
{
		Q_OBJECT

	public:
		explicit ToolbarsConfigPage(QWidget *parent = nullptr);

		void applyConf() override;
		QString title() const override;
		QIcon icon() const override;

	private:
		QComboBox *m_icon_size;
		QComboBox *m_button_style;
		QCheckBox *m_locked;
};

#endif // TOOLBARSCONFIGPAGE_H
