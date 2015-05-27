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
#ifndef ELEMENTSELECTORWIDGET_H
#define ELEMENTSELECTORWIDGET_H

#include <QWidget>
class Element;
class QSignalMapper;
class QButtonGroup;

namespace Ui {
	class ElementSelectorWidget;
}

/**
 * @brief The ElementSelectorWidget class
 * This class provide a widget with a list of element.
 * User can select an element in the list and higligth it.
 * For know what element is selected, call selectedElement.
 */
class ElementSelectorWidget : public QWidget
{
	Q_OBJECT

	///Methods
	public:
	explicit ElementSelectorWidget(QList <Element *> elmt_list, QWidget *parent = 0);
	~ElementSelectorWidget();
	Element * selectedElement () const{return selected_element;}
	void showElement(Element *elmt);
	void clear();
	void setList(QList <Element *> elmt_list);

	QStringList filter () const;

	signals:
		void elementSelected (Element *element);

	public slots:
		void filtered(const QString &str);

	private:
		void buildInterface();

	private slots:
		void setSelectedElement  (const int i);
		void showElementFromList (const int i);
		void showedElementWasDeleted ();


	///Attributes
	private:
	Ui::ElementSelectorWidget *ui;
	QList <Element *>          elements_list;
	QSignalMapper             *sm_, *sm_show_;
	Element                   *selected_element, *showed_element;
	QList <QWidget *>          content_list;
	QStringList                in_filter,  //In filter is used inside this class to filter the content of this widget
							   out_filter; //Out filter is used to return (with the method filter) a list of
										   //available string to filter the content of this widget
	QButtonGroup               *m_button_group;
};

#endif // ELEMENTSELECTORWIDGET_H
