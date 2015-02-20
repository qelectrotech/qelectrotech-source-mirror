/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef XREFPROPERTIESWIDGET_H
#define XREFPROPERTIESWIDGET_H

#include <QWidget>
#include "properties/xrefproperties.h"

namespace Ui {
	class XRefPropertiesWidget;
}

/**
 * @brief The XRefPropertiesWidget class
 * This class provide a widget to edit the XRefProperties
 */
class XRefPropertiesWidget : public QWidget
{
	Q_OBJECT

	public:
	XRefPropertiesWidget(QHash <QString, XRefProperties> properties = QHash <QString, XRefProperties>(), QWidget *parent = 0);
	~XRefPropertiesWidget();

	void setProperties (const QHash <QString, XRefProperties> &properties);
	QHash <QString, XRefProperties> properties();

	void setReadOnly (bool = true);

	private:
	void buildUi();
	void saveProperties(int index);

	private slots:
	void updateDisplay();
	void typeChanged();


	private:
	Ui::XRefPropertiesWidget *ui;
	QHash <QString, XRefProperties> m_properties;
	int m_previous_type_index;
};

#endif // XREFPROPERTIESWIDGET_H
