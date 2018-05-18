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
#ifndef ADDELEMENTTEXTCOMMAND_H
#define ADDELEMENTTEXTCOMMAND_H

#include <QUndoCommand>
#include <QPointer>
#include <QDomElement>

class Element;
class DynamicElementTextItem;
class ElementTextItemGroup;

/**
 * @brief The AddElementTextCommand class
 * Manage the adding of element text
 */
class AddElementTextCommand : public QUndoCommand
{
	public:
		AddElementTextCommand(Element *element, DynamicElementTextItem *deti, QUndoCommand *parent = nullptr);
		~AddElementTextCommand() override;
		
		void undo() override;
		void redo() override;
		
	private:
		Element *m_element = nullptr;
		DynamicElementTextItem *m_text = nullptr;
};

/**
 * @brief The AddTextsGroupCommand class
 * Manage the adding of a texts group
 */
class AddTextsGroupCommand : public QUndoCommand
{
	public:
		AddTextsGroupCommand(Element *element, QString groupe_name, QUndoCommand *parent = nullptr);
		AddTextsGroupCommand(Element *element, QDomElement dom_element, QUndoCommand *parent = nullptr);
		AddTextsGroupCommand(Element *element, QString groupe_name, QList<DynamicElementTextItem *> texts_list, QUndoCommand *parent = nullptr);
		~AddTextsGroupCommand() override;
		
		void undo() override;
		void redo() override;
		
	private:
		QPointer<Element> m_element;
		QPointer<ElementTextItemGroup> m_group;
		QList <DynamicElementTextItem *> m_deti_list;
		QDomElement m_dom_element;
		QString m_name;
		bool m_first_undo = true;
};

/**
 * @brief The RemoveTextsGroupCommand class
 * Manage the removinf of a texts group
 */
class RemoveTextsGroupCommand : public QUndoCommand
{
	public:
		RemoveTextsGroupCommand(Element *element, ElementTextItemGroup *group, QUndoCommand *parent = nullptr);
		~RemoveTextsGroupCommand() override;
		
		void undo() override;
		void redo() override;
		
	private:
		QPointer<Element> m_element;
		QPointer<ElementTextItemGroup> m_group;
		QList<QPointer<DynamicElementTextItem>> m_text_list;
};

class AddTextToGroupCommand : public QUndoCommand
{
	public:
		AddTextToGroupCommand(DynamicElementTextItem *text, ElementTextItemGroup *group, QUndoCommand *parent = nullptr);
		~AddTextToGroupCommand() override;
		
		void undo() override;
		void redo() override;
		
	private:
		QPointer<DynamicElementTextItem> m_text;
		QPointer<ElementTextItemGroup> m_group;
		QPointer<Element> m_element;
};

class RemoveTextFromGroupCommand : public QUndoCommand
{
	public:
		RemoveTextFromGroupCommand(DynamicElementTextItem *text, ElementTextItemGroup *group, QUndoCommand *parent = nullptr);
		~RemoveTextFromGroupCommand() override;
		
		void undo() override;
		void redo() override;
		
	private:
		QPointer<DynamicElementTextItem> m_text;
		QPointer<ElementTextItemGroup> m_group;
		QPointer<Element> m_element;
};

class AlignmentTextsGroupCommand : public QUndoCommand
{
	public:
		AlignmentTextsGroupCommand(ElementTextItemGroup *group, Qt::Alignment new_alignment, QUndoCommand *parent = nullptr);
		~AlignmentTextsGroupCommand() override;
		
		int id() const override{return 6;}
		bool mergeWith(const QUndoCommand *other) override;
		void undo() override;
		void redo() override;
		
	private:
		QPointer<ElementTextItemGroup> m_group;
		Qt::Alignment m_previous_alignment,
					  m_new_alignment;
		QHash<DynamicElementTextItem *, QPointF> m_texts_pos;
};

#endif // ADDELEMENTTEXTCOMMAND_H
