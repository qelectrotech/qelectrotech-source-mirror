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
#ifndef ELEMENTTEXTPATTERN_H
#define ELEMENTTEXTPATTERN_H

#include <QString>
#include <QDomDocument>
#include <QFont>
#include <QColor>
#include <QPointF>
#include <QList>
#include <QUndoCommand>
#include <QPointer>
#include "qetgraphicsitem/dynamicelementtextitem.h"

class Element;
class QWidget;
class ElementTextItemGroup;

class ExportElementTextPattern
{
	public:
		ExportElementTextPattern(Element *elmt);
		
	private:
		QString getName(bool &ok) const;
		QWidget *parentWidget() const;
		QDomDocument xmlConf() const;
		
	private:
		Element *m_element = nullptr;
		QString m_name;
};

class ImportElementTextPattern
{
	public:
		ImportElementTextPattern(Element *elmt);
		
	private:
		QString getName(const QStringList& list,
				bool *ok,
				bool *erase) const;
		QWidget *parentWidget() const;
		void apply(QString name, bool erase = false) const;
		
	private:
		Element *m_element = nullptr;
};

/**
 * @brief Description of formatting and layout for an element text item.
 */
struct DynamicTextFormatItem {
	DynamicElementTextItem::TextFrom textFrom = DynamicElementTextItem::UserText;
	QString infoName;
	QString compositeText;
	QFont font;
	QColor color = Qt::black;
	Qt::Alignment alignment = (Qt::AlignLeft | Qt::AlignTop);
	QPointF pos;
	qreal rotation = 0;
	bool keepVisualRotation = false;
	bool rotationPointCenter = false;
	bool frame = false;
	qreal textWidth = -1;
	bool visible = true;
	QString groupName;
	int index = 0;
	bool isTerminalText = false;
};

/**
 * @brief Description of formatting and layout for an element text group.
 */
struct TextGroupFormatItem {
	QString name;
	QPointF pos;
	qreal rotation = 0;
	Qt::Alignment alignment = Qt::AlignLeft;
	int verticalAdjustment = 0;
	bool frame = false;
	bool holdToBottomPage = false;
};

/**
 * @brief Stores the formatting pattern (font, size, style, colors, alignment, positions, offsets, frame)
 * captured from a source Element. Acts as the Format Painter template clipboard.
 */
class ElementTextFormatPattern
{
	public:
		ElementTextFormatPattern() = default;

		static ElementTextFormatPattern fromElement(Element *element);
		static bool hasPattern();
		static const ElementTextFormatPattern &currentPattern();
		static void setCurrentPattern(const ElementTextFormatPattern &pattern);
		static bool isTerminalTextItem(const DynamicElementTextItem *deti, const Element *element);

		bool isValid() const { return !m_text_items.isEmpty(); }
		QString sourceElementName() const { return m_source_element_name; }

		const QList<DynamicTextFormatItem> &textItems() const { return m_text_items; }
		const QList<TextGroupFormatItem> &groups() const { return m_groups; }

		bool hasTerminalTextFormat() const;
		DynamicTextFormatItem terminalTextFormat() const;

	private:
		QString m_source_element_name;
		QList<DynamicTextFormatItem> m_text_items;
		QList<TextGroupFormatItem> m_groups;

		static ElementTextFormatPattern s_current_pattern;
		static bool s_has_pattern;
};

/**
 * @brief Undo command to apply a text formatting pattern to one or multiple elements.
 * Field content (labels, designations, comments, article numbers) is preserved unchanged.
 */
class ApplyElementTextFormatCommand : public QUndoCommand
{
	public:
		ApplyElementTextFormatCommand(const QList<Element *> &elements,
		                              const ElementTextFormatPattern &pattern,
		                              QUndoCommand *parent = nullptr);
		ApplyElementTextFormatCommand(Element *element,
		                              const ElementTextFormatPattern &pattern,
		                              QUndoCommand *parent = nullptr);
		~ApplyElementTextFormatCommand() override;

		void undo() override;
		void redo() override;

	private:
		struct ElementFormatChange {
			QPointer<Element> element;

			struct TextChange {
				QPointer<DynamicElementTextItem> deti;
				bool wasCreated = false;

				// Old state
				QFont oldFont;
				QColor oldColor;
				Qt::Alignment oldAlignment;
				QPointF oldPos;
				qreal oldRotation = 0;
				bool oldKeepVisualRotation = false;
				bool oldRotationPointCenter = false;
				bool oldFrame = false;
				qreal oldTextWidth = -1;
				bool oldVisible = true;
				QString oldGroupName;

				// New state
				QFont newFont;
				QColor newColor;
				Qt::Alignment newAlignment;
				QPointF newPos;
				qreal newRotation = 0;
				bool newKeepVisualRotation = false;
				bool newRotationPointCenter = false;
				bool newFrame = false;
				qreal newTextWidth = -1;
				bool newVisible = true;
				QString newGroupName;
			};
			QList<TextChange> textChanges;

			struct GroupChange {
				QString name;
				QPointer<ElementTextItemGroup> group;
				bool wasCreated = false;

				// Old state
				QPointF oldPos;
				qreal oldRotation = 0;
				Qt::Alignment oldAlignment = Qt::AlignLeft;
				int oldVAdjust = 0;
				bool oldFrame = false;
				bool oldHoldBottom = false;

				// New state
				QPointF newPos;
				qreal newRotation = 0;
				Qt::Alignment newAlignment = Qt::AlignLeft;
				int newVAdjust = 0;
				bool newFrame = false;
				bool newHoldBottom = false;
			};
			QList<GroupChange> groupChanges;
		};

		QList<ElementFormatChange> m_changes;
};

#endif // ELEMENTTEXTCONFIGURATION_H
