/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef AUTONUMSELECTORWIDGET_H
#define AUTONUMSELECTORWIDGET_H

#include <QWidget>

namespace Ui {
	class AutonumSelectorWidget;
}

/**
 * @brief The AutonumSelectorWidget class
 * This widget show a combobox to select an autonum and a button to edit the autonum
 * The combo box is empty and the button isn't linked with anything, it's the role of caller
 * of this class to fill the combo box and edit the connection with the button.
 */
class AutonumSelectorWidget : public QWidget
{
		Q_OBJECT

	public:
		explicit AutonumSelectorWidget(QWidget *parent = 0);
		explicit AutonumSelectorWidget(const QList <QString> &text, QWidget *parent = 0);
		~AutonumSelectorWidget();

		void setCurrentItem (const QString &text);
		QString text() const;

	private:
		Ui::AutonumSelectorWidget *ui;
};

#endif // AUTONUMSELECTORWIDGET_H
