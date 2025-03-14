/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef ELEMENTPROPERTIESWIDGET_H
#define ELEMENTPROPERTIESWIDGET_H

#include "abstractelementpropertieseditorwidget.h"

class Element;
class Diagram;
class QTabWidget;
class ElementsLocation;
class DynamicElementTextItem;
class ElementTextItemGroup;


class ElementPropertiesWidget : public AbstractElementPropertiesEditorWidget
{
		Q_OBJECT

	public:
		explicit ElementPropertiesWidget(Element *elmt, QWidget *parent = nullptr);
		explicit ElementPropertiesWidget(DynamicElementTextItem *text, QWidget *parent = nullptr);
		explicit ElementPropertiesWidget(ElementTextItemGroup *group, QWidget *parent = nullptr);
		void setElement(Element *element) override;
		void setDynamicText(DynamicElementTextItem *text);
		void setTextsGroup(ElementTextItemGroup *group);
		void apply() override;
		void reset() override;
		bool setLiveEdit(bool live_edit) override;

	public slots:
		void findInPanel ();
		void editElement ();

	private:
		void buildGui();
		void updateUi() override;
		void addGeneralWidget();
		QWidget *generalWidget();

	signals:
		void findEditClicked();

	private:
		Diagram *m_diagram;
		QTabWidget *m_tab;
		QList <AbstractElementPropertiesEditorWidget *> m_list_editor;
		QWidget *m_general_widget;
};

#endif // ELEMENTPROPERTIESWIDGET_H
