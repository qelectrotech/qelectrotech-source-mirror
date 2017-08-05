/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef ELEMENTINFOWIDGET_H
#define ELEMENTINFOWIDGET_H

#include <QWidget>
#include "abstractelementpropertieseditorwidget.h"
#include "diagramcontext.h"

class Element;
class QUndoCommand;
class ElementInfoPartWidget;
class ChangeElementInformationCommand;

namespace Ui {
	class ElementInfoWidget;
}

/**
 * @brief The ElementInfoWidget class
 * this class is a widget to edit an element informations.
 */
class ElementInfoWidget : public AbstractElementPropertiesEditorWidget
{
	Q_OBJECT

		//METHODS
	public:
		explicit ElementInfoWidget(Element *elmt, QWidget *parent = nullptr);
		~ElementInfoWidget() override;

		void setElement(Element *element) override;
		void apply() override;
		QUndoCommand *associatedUndo () const override;
		QString title() const override {return tr("Informations");}
		bool setLiveEdit(bool live_edit) override;
		void updateUi() override;
		DiagramContext currentInfo() const;


	protected:
		bool event(QEvent *event) override;
		void enableLiveEdit() override;
		void disableLiveEdit() override;

	private:
		void buildInterface();
		ElementInfoPartWidget *infoPartWidgetForKey(const QString &key) const;

	private slots:
		void firstActivated();
		void elementInfoChange();

		//ATTRIBUTES
	private:
		Ui::ElementInfoWidget           *ui;
		QList <ElementInfoPartWidget *>  m_eipw_list;
		bool m_first_activation;
};

#endif // ELEMENTINFOWIDGET_H
