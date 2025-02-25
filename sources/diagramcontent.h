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
#ifndef DIAGRAM_CONTENT_H
#define DIAGRAM_CONTENT_H

#include <QSet>
#include <QVector>

class QGraphicsItem;
class Conductor;
class Element;
class IndependentTextItem;
class DiagramImageItem;
class QetShapeItem;
class DynamicElementTextItem;
class ElementTextItemGroup;
class Diagram;
class DiagramTextItem;
class QetGraphicsTableItem;
class TerminalStripItem;

/**
	This class provides a container that makes the transmission of diagram content
	to other functions/methods easier. The different kind of items are made
	available through a handful of filter-aware methods. Considering selected
	elements are to be moved, the filter notably distinguishes conductors to be
	moved from those to be updated.
	Please note this container does not systematically contains a whole
	diagram: it may describe only a part of it, e.g. selected items.
*/
class DiagramContent
{
	public:
		DiagramContent();
		DiagramContent(Diagram *diagram, bool selected = true);
		~DiagramContent();
		
		/// Used to filter the different items carried by this container.
		enum Filter {
			Elements = 1,
			TextFields = 2,
			ElementTextFields = 4,
			Images = 8,
			ConductorsToMove = 16,
			ConductorsToUpdate = 32,
			OtherConductors = 64,
			AnyConductor = 112,
			Shapes = 128,
			TextGroup = 256,
			Tables = 512,
			TerminalStrip = 1024,
			All = 2047,
			SelectedOnly = 2048
		};
		
		QList<Element *> m_elements;
		QSet<IndependentTextItem *> m_text_fields;
		QSet<DiagramImageItem *> m_images;
		QSet<QetShapeItem *> m_shapes;
		QList<Conductor *> m_conductors_to_update;
		QList<Conductor *> m_conductors_to_move;
		QList<Conductor *> m_other_conductors;
		QList<Conductor *> m_potential_conductors;
		QSet<DynamicElementTextItem *> m_element_texts;
		QSet<ElementTextItemGroup *> m_texts_groups;
		QList<QGraphicsItem *> m_selected_items;
		QVector<QetGraphicsTableItem *> m_tables;
		QVector<TerminalStripItem *> m_terminal_strip;

		
		QList<DiagramTextItem *> selectedTexts() const;
		QList<ElementTextItemGroup *> selectedTextsGroup() const;
		QList<Conductor *> conductors(int = AnyConductor) const;
		bool hasDeletableItems() const;
		bool hasCopiableItems() const;
		QList<QGraphicsItem *> items(int = All) const;
		QString sentence(int = All) const;
		int count(int = All) const;
		void clear();
		int removeNonMovableItems();
		
		DiagramContent& operator+=(const DiagramContent& other);
		bool potentialIsManaged(QList<Conductor *>conductors);
		bool hasTextEditing();
};
QDebug &operator<<(QDebug, DiagramContent &);
#endif
