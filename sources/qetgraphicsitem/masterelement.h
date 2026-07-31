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
#ifndef MASTERELEMENT_H
#define MASTERELEMENT_H

#include "element.h"
#include <QHash>
#include <QMetaObject>

class CrossRefItem;

/**
	@brief The MasterElement class
	This class is a custom element, with extended behavior
	to be a master element. Master element can be linked with slave element
	and display a cross ref item for know with what other element he is linked
*/
class MasterElement : public Element
{
	Q_OBJECT

	public:
		explicit MasterElement(
			const ElementsLocation &,
			QGraphicsItem * = nullptr,
			int * = nullptr);
		~MasterElement() override;

		void linkToElement     (Element *elmt) override;
		void unlinkAllElements () override;
		void unlinkElement     (Element *elmt) override;
		void initLink          (QETProject *project) override;
		QRectF XrefBoundingRect() const;

		bool isFull() const; // Check Slave-Limit
		
	protected:
		QVariant itemChange(
				GraphicsItemChange change,
				const QVariant &value) override;

	private:
		void xrefPropertiesChanged();
		void aboutDeleteXref ();
		void connectSlavePositionUpdates(Element *slave);
		void disconnectSlavePositionUpdates(Element *slave);

	private:
		CrossRefItem *m_Xref_item = nullptr;
		bool m_first_scene_change = true;
		QHash<Element*, QMetaObject::Connection> m_slave_x_conn;
		QHash<Element*, QMetaObject::Connection> m_slave_y_conn;
};

#endif // MASTERELEMENT_H
