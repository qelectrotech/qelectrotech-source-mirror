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
#include "elementtextpattern.h"

#include "diagram.h"
#include "qet.h"
#include "qetapp.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/elementtextitemgroup.h"
#include "ui/importelementtextpatterndialog.h"
#include "undocommand/addelementtextcommand.h"
#include "undocommand/deleteqgraphicsitemcommand.h"
#include "shownkinds.h"

#include <QDir>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QInputDialog>
#include <QMessageBox>
#include <QObject>
#include <QStringList>

/**
	@brief ExportElementTextPattern::ExportElementTextPattern
	Constructor
	@param elmt
*/
ExportElementTextPattern::ExportElementTextPattern(Element *elmt) :
	m_element(elmt)
{
		//Create the conf dir if not yet.
	QDir dir(QETApp::configDir()+ "/element_texts_pattern");
	if(!dir.exists())
	{
		dir.cdUp();
		dir.mkdir("element_texts_pattern");
		dir.cd("element_texts_pattern");
	}
	
		//Get the name of conf
	bool ok;
	m_name = getName(ok);
	if(ok == false)
		return;
	
		//Check if a conf with the same name already exist
	if (QFileInfo::exists(dir.absoluteFilePath(m_name % ".xml")))
	{
		bool r = QMessageBox::question(parentWidget(),
									   QObject::tr("Text configuration"),
									   QObject::tr("A text configuration named <<%1 >> already exists.\n"
												   "Do you want to replace it ?").arg(m_name));
		
		if(r == false)
			return;
	}
	
	QDomDocument doc = xmlConf();
	QET::writeXmlFile(doc, dir.absoluteFilePath(m_name % ".xml"));
}

/**
	@brief ExportElementTextConf::getName
	Open a dialog to let user set the name of the conf and return it
	@return 
*/
QString ExportElementTextPattern::getName(bool &ok) const
{	
	QString text = QInputDialog::getText(parentWidget(),
										 QObject::tr("Configuration name"),
										 QObject::tr("Enter the name of the configuration to create"),
										 QLineEdit::Normal,
										 QString(),
										 &ok);
	
	text.replace(" ", "_");
	text.replace(".","");
	return text;
}

QWidget *ExportElementTextPattern::parentWidget() const
{
	QWidget *parent = nullptr;
	if(m_element->scene() && !m_element->scene()->views().isEmpty())
		parent = m_element->scene()->views().first();
	
	return parent;
}

QDomDocument ExportElementTextPattern::xmlConf() const
{
	QDomDocument doc;
	QDomElement root = doc.createElement("Element_texts_pattern");
	root.setAttribute("name", m_name);
	doc.appendChild(root);
	
	QHash<Terminal *, int> H;
	QDomElement elmt = m_element->toXml(doc, H);
	QDomElement texts = elmt.firstChildElement("dynamic_texts");
	QDomElement groups = elmt.firstChildElement("texts_groups");
	
	if(texts.tagName() == "dynamic_texts")
		root.appendChild(texts);
	if(groups.tagName() == "texts_groups")
		root.appendChild(groups);
	
	return doc;
}

//*******************//
//******IMPORT*******//
//*******************//
ImportElementTextPattern::ImportElementTextPattern(Element *elmt):
	m_element(elmt)
{
	bool exist = true;
	QDir dir(QETApp::configDir()+ "/element_texts_pattern");
	
	if(!dir.exists())
		exist = false;

	QStringList entry = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
	QStringList result = entry.filter(".xml");
	if(result.isEmpty())
		exist = false;
	
	if(!exist)
	{
		QMessageBox::information(
					parentWidget(),
					QObject::tr("Text configuration"),
					QObject::tr("No existing text configuration."));
		return;
	}
	
	bool ok=false;
	bool erase = false;
		//Remove the .xml extension of the files
	result.replaceInStrings(".xml", "");
	QString name = getName(result, &ok, &erase);
	
	if(!ok || name.isEmpty())
		return;
	else
		apply(name, erase);
}

/**
	@brief ImportElementTextPattern::getName
	Open a dialog to let user select a conf
	@param list
	@param ok
	@param erase
	@return
*/
QString ImportElementTextPattern::getName(const QStringList& list,
					  bool *ok,
					  bool *erase) const
{
	return ImportElementTextPatternDialog::getItem(
				parentWidget(),
				QObject::tr("Select a text configuration"),
				QObject::tr("Select the text configuration to add to the element"),
				list,
				ok,
				erase);
}

QWidget *ImportElementTextPattern::parentWidget() const
{
	QWidget *parent = nullptr;
	if(m_element->scene() && !m_element->scene()->views().isEmpty())
		parent = m_element->scene()->views().first();
	
	return parent;
}


/**
	@brief ImportElementTextPattern::apply
	Apply the user choice
	@param name : the name of the selected pattern
	@param erase : erase the existing texts and groups of element.
*/
void ImportElementTextPattern::apply(QString name, bool erase) const
{
	if(!name.endsWith(".xml"))
		name.append(".xml");
	
	QFile conf_file(QETApp::configDir() % "/element_texts_pattern/" % name);
	if(!conf_file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;
	
		//Load the content of the xml file
	QDomDocument xml_conf;
	if(!xml_conf.setContent(&conf_file))
		return;
	
	QDomElement root = xml_conf.firstChildElement("Element_texts_pattern");
		
		//Get the text
	QList <QDomElement> texts = QET::findInDomElement(root, "dynamic_texts", "dynamic_elmt_text");
	if (texts.isEmpty())
		return;
	
		//Replace the original uuid of texts in the xml description, by a new one for every texts
	QHash<QUuid, QUuid> uuid_hash;
	for(QDomElement text : texts)
	{
		QUuid original_uuid(text.attribute("uuid"));
		QUuid new_uuid = QUuid::createUuid();
		text.setAttribute("uuid", new_uuid.toString());
		
		uuid_hash.insert(original_uuid, new_uuid);
	}
	
		//In each group of the xml description, replace the original uuids, by the news created upper. 
	QList <QDomElement> groups = QET::findInDomElement(root, "texts_groups", "texts_group");
	for(const QDomElement& group : groups)
	{
		for(QDomElement text : QET::findInDomElement(group, "texts", "text"))
		{
			QUuid original_uuid(text.attribute("uuid"));
			QUuid new_uuid = uuid_hash.value(original_uuid);
			if(!new_uuid.isNull())
				text.setAttribute("uuid", new_uuid.toString());
		}
	}
	
	QUndoStack &undo_stack = m_element->diagram()->undoStack();
	undo_stack.beginMacro(QObject::tr("Import the text configuration : %1").arg(name.remove(".xml")));
	
		//erase existing texts and groups
	if (erase)
	{
		for (ElementTextItemGroup *group : m_element->textGroups()) {
			undo_stack.push(new RemoveTextsGroupCommand(m_element, group));
		}
		for (DynamicElementTextItem *deti : m_element->dynamicTextItems())
		{
			DiagramContent dc;
			dc.m_element_texts << deti;
			undo_stack.push(new DeleteQGraphicsItemCommand(m_element->diagram(), dc));
		}
	}
	
		//Add the texts to element
	for(const QDomElement& text : texts)
	{
		DynamicElementTextItem *deti = new DynamicElementTextItem(m_element);
		undo_stack.push(new AddElementTextCommand(m_element, deti));
		deti->fromXml(text);
	}
		//Add the groups to element
	for(const QDomElement& xml_group : groups)
		undo_stack.push(new AddTextsGroupCommand(m_element, xml_group));
	
	undo_stack.endMacro();
}

//=============================================================================
// ElementTextFormatPattern & ApplyElementTextFormatCommand (Format Painter)
//=============================================================================

#include "terminal.h"

ElementTextFormatPattern ElementTextFormatPattern::s_current_pattern;
bool ElementTextFormatPattern::s_has_pattern = false;

bool ElementTextFormatPattern::hasPattern()
{
	return s_has_pattern && s_current_pattern.isValid();
}

const ElementTextFormatPattern &ElementTextFormatPattern::currentPattern()
{
	return s_current_pattern;
}

void ElementTextFormatPattern::setCurrentPattern(const ElementTextFormatPattern &pattern)
{
	s_current_pattern = pattern;
	s_has_pattern = pattern.isValid();
}

ElementTextFormatPattern ElementTextFormatPattern::fromElement(Element *element)
{
	ElementTextFormatPattern pattern;
	if (!element) return pattern;

	pattern.m_source_element_name = element->name();

	// 1. Capture groups
	for (ElementTextItemGroup *group : element->textGroups()) {
		TextGroupFormatItem gItem;
		gItem.name = group->name();
		gItem.pos = group->pos();
		gItem.rotation = group->rotation();
		gItem.alignment = group->alignment();
		gItem.verticalAdjustment = group->verticalAdjustment();
		gItem.frame = group->frame();
		gItem.holdToBottomPage = group->holdToBottomPage();
		pattern.m_groups.append(gItem);
	}

	// 2. Capture all dynamic texts
	int idx = 0;
	// Ungrouped texts
	for (DynamicElementTextItem *deti : element->dynamicTextItems()) {
		DynamicTextFormatItem tItem;
		tItem.textFrom = deti->textFrom();
		tItem.infoName = deti->infoName();
		tItem.compositeText = deti->toPlainText();
		tItem.font = deti->font();
		tItem.color = deti->color();
		tItem.alignment = deti->alignment();
		tItem.pos = deti->pos();
		tItem.rotation = deti->rotation();
		tItem.keepVisualRotation = deti->keepVisualRotation();
		tItem.rotationPointCenter = deti->rotationPointCenter();
		tItem.frame = deti->frame();
		tItem.textWidth = deti->textWidth();
		tItem.visible = ShownKinds::wantsVisible(deti);
		tItem.groupName = QString();
		tItem.index = idx++;
		tItem.isTerminalText = isTerminalTextItem(deti, element);
		pattern.m_text_items.append(tItem);
	}

	// Grouped texts
	for (ElementTextItemGroup *group : element->textGroups()) {
		for (DynamicElementTextItem *deti : group->texts()) {
			DynamicTextFormatItem tItem;
			tItem.textFrom = deti->textFrom();
			tItem.infoName = deti->infoName();
			tItem.compositeText = deti->toPlainText();
			tItem.font = deti->font();
			tItem.color = deti->color();
			tItem.alignment = deti->alignment();
			tItem.pos = deti->pos();
			tItem.rotation = deti->rotation();
			tItem.keepVisualRotation = deti->keepVisualRotation();
			tItem.rotationPointCenter = deti->rotationPointCenter();
			tItem.frame = deti->frame();
			tItem.textWidth = deti->textWidth();
			tItem.visible = ShownKinds::wantsVisible(deti);
			tItem.groupName = group->name();
			tItem.index = idx++;
			tItem.isTerminalText = isTerminalTextItem(deti, element);
			pattern.m_text_items.append(tItem);
		}
	}

	return pattern;
}

bool ElementTextFormatPattern::hasTerminalTextFormat() const
{
	for (const DynamicTextFormatItem &item : m_text_items) {
		if (item.isTerminalText) return true;
	}
	return false;
}

DynamicTextFormatItem ElementTextFormatPattern::terminalTextFormat() const
{
	for (const DynamicTextFormatItem &item : m_text_items) {
		if (item.isTerminalText) return item;
	}
	return DynamicTextFormatItem();
}

bool ElementTextFormatPattern::isTerminalTextItem(const DynamicElementTextItem *deti, const Element *element)
{
	if (!deti || !element) return false;
	if (deti->textFrom() != DynamicElementTextItem::UserText) return false;

	const QString text = deti->toPlainText().trimmed();
	if (text.isEmpty()) return false;

	const QList<Terminal *> terms = element->terminals();
	for (const Terminal *t : terms) {
		if (!t) continue;
		if (t->name().trimmed() == text) {
			return true;
		}
	}

	// Secondary check: text is short (<= 6 chars, typical pin designation)
	// and located close to a terminal (within 40 pixels)
	if (text.length() <= 6) {
		for (const Terminal *t : terms) {
			if (!t) continue;
			QPointF diff = deti->pos() - t->pos();
			if ((diff.x() * diff.x() + diff.y() * diff.y()) <= (40.0 * 40.0)) {
				return true;
			}
		}
	}

	return false;
}

ApplyElementTextFormatCommand::ApplyElementTextFormatCommand(
	Element *element,
	const ElementTextFormatPattern &pattern,
	QUndoCommand *parent)
	: ApplyElementTextFormatCommand(QList<Element *>{element}, pattern, parent)
{
}

ApplyElementTextFormatCommand::ApplyElementTextFormatCommand(
	const QList<Element *> &elements,
	const ElementTextFormatPattern &pattern,
	QUndoCommand *parent)
	: QUndoCommand(parent)
{
	setText(QObject::tr("Apply text format"));

	for (Element *element : elements) {
		if (!element) continue;

		ElementFormatChange elChange;
		elChange.element = element;

		// Collect all existing texts on target element
		QList<DynamicElementTextItem *> targetTexts;
		targetTexts << element->dynamicTextItems();
		for (ElementTextItemGroup *g : element->textGroups()) {
			targetTexts << g->texts();
		}

		// Setup group changes
		for (const TextGroupFormatItem &gItem : pattern.groups()) {
			ElementFormatChange::GroupChange gc;
			gc.name = gItem.name;
			ElementTextItemGroup *existingGroup = element->textGroup(gItem.name);
			if (existingGroup) {
				gc.group = existingGroup;
				gc.wasCreated = false;
				gc.oldPos = existingGroup->pos();
				gc.oldRotation = existingGroup->rotation();
				gc.oldAlignment = existingGroup->alignment();
				gc.oldVAdjust = existingGroup->verticalAdjustment();
				gc.oldFrame = existingGroup->frame();
				gc.oldHoldBottom = existingGroup->holdToBottomPage();
			} else {
				gc.group = nullptr;
				gc.wasCreated = true;
			}
			gc.newPos = gItem.pos;
			gc.newRotation = gItem.rotation;
			gc.newAlignment = gItem.alignment;
			gc.newVAdjust = gItem.verticalAdjustment;
			gc.newFrame = gItem.frame;
			gc.newHoldBottom = gItem.holdToBottomPage;
			elChange.groupChanges.append(gc);
		}

		// Text matching
		QSet<DynamicElementTextItem *> matchedTarget;
		QSet<int> matchedPatternIndices;

		// Identify terminal / pole texts on the target element
		QSet<DynamicElementTextItem *> targetTerminalTexts;
		for (DynamicElementTextItem *tDeti : targetTexts) {
			if (ElementTextFormatPattern::isTerminalTextItem(tDeti, element)) {
				targetTerminalTexts.insert(tDeti);
			}
		}

		// Handle Terminal / Pole texts:
		// They belong to physical terminals/poles of the element (e.g. 1..6, A1/A2, 13/14).
		// Their positions, rotations, frames and visibilities are NEVER modified!
		// If the pattern provides a terminal text style (font, color), apply it uniformly to all terminal texts.
		const bool hasTermFmt = pattern.hasTerminalTextFormat();
		const DynamicTextFormatItem termFmt = hasTermFmt ? pattern.terminalTextFormat() : DynamicTextFormatItem();

		for (DynamicElementTextItem *tDeti : targetTerminalTexts) {
			matchedTarget.insert(tDeti); // Exclude from general user text matching

			// Only record a change if font or color actually changes
			if (hasTermFmt && (tDeti->font() != termFmt.font || tDeti->color() != termFmt.color)) {
				ElementFormatChange::TextChange tc;
				tc.deti = tDeti;
				tc.wasCreated = false;
				tc.oldFont = tDeti->font();
				tc.oldColor = tDeti->color();
				tc.oldAlignment = tDeti->alignment();
				tc.oldPos = tDeti->pos();
				tc.oldRotation = tDeti->rotation();
				tc.oldKeepVisualRotation = tDeti->keepVisualRotation();
				tc.oldRotationPointCenter = tDeti->rotationPointCenter();
				tc.oldFrame = tDeti->frame();
				tc.oldTextWidth = tDeti->textWidth();
				tc.oldVisible = ShownKinds::wantsVisible(tDeti);
				tc.oldGroupName = tDeti->parentGroup() ? tDeti->parentGroup()->name() : QString();

				tc.newFont = termFmt.font;
				tc.newColor = termFmt.color;
				// Positions, alignments, rotations, frames and visibilities are strictly preserved on terminals!
				tc.newAlignment = tc.oldAlignment;
				tc.newPos = tc.oldPos;
				tc.newRotation = tc.oldRotation;
				tc.newKeepVisualRotation = tc.oldKeepVisualRotation;
				tc.newRotationPointCenter = tc.oldRotationPointCenter;
				tc.newFrame = tc.oldFrame;
				tc.newTextWidth = tc.oldTextWidth;
				tc.newVisible = tc.oldVisible; // Always retain visibility!
				tc.newGroupName = tc.oldGroupName;

				elChange.textChanges.append(tc);
			}
		}

		// Pass 1: ElementInfo with same infoName
		for (int i = 0; i < pattern.textItems().size(); ++i) {
			const DynamicTextFormatItem &pItem = pattern.textItems().at(i);
			if (pItem.textFrom == DynamicElementTextItem::ElementInfo && !pItem.infoName.isEmpty()) {
				for (DynamicElementTextItem *tDeti : targetTexts) {
					if (!matchedTarget.contains(tDeti) &&
					    tDeti->textFrom() == DynamicElementTextItem::ElementInfo &&
					    tDeti->infoName() == pItem.infoName) {

						matchedTarget.insert(tDeti);
						matchedPatternIndices.insert(i);

						ElementFormatChange::TextChange tc;
						tc.deti = tDeti;
						tc.wasCreated = false;
						tc.oldFont = tDeti->font();
						tc.oldColor = tDeti->color();
						tc.oldAlignment = tDeti->alignment();
						tc.oldPos = tDeti->pos();
						tc.oldRotation = tDeti->rotation();
						tc.oldKeepVisualRotation = tDeti->keepVisualRotation();
						tc.oldRotationPointCenter = tDeti->rotationPointCenter();
						tc.oldFrame = tDeti->frame();
						tc.oldTextWidth = tDeti->textWidth();
						tc.oldVisible = ShownKinds::wantsVisible(tDeti);
						tc.oldGroupName = tDeti->parentGroup() ? tDeti->parentGroup()->name() : QString();

						tc.newFont = pItem.font;
						tc.newColor = pItem.color;
						tc.newAlignment = pItem.alignment;
						tc.newPos = pItem.pos;
						tc.newRotation = pItem.rotation;
						tc.newKeepVisualRotation = pItem.keepVisualRotation;
						tc.newRotationPointCenter = pItem.rotationPointCenter;
						tc.newFrame = pItem.frame;
						tc.newTextWidth = pItem.textWidth;
						tc.newVisible = pItem.visible;
						tc.newGroupName = pItem.groupName;

						elChange.textChanges.append(tc);
						break;
					}
				}
			}
		}

		// Pass 2: UserText (non-terminal texts only)
		for (int i = 0; i < pattern.textItems().size(); ++i) {
			if (matchedPatternIndices.contains(i)) continue;
			const DynamicTextFormatItem &pItem = pattern.textItems().at(i);
			if (pItem.isTerminalText) continue; // Skip terminal texts in generic user text matching
			if (pItem.textFrom == DynamicElementTextItem::UserText) {
				DynamicElementTextItem *foundDeti = nullptr;
				for (DynamicElementTextItem *tDeti : targetTexts) {
					if (!matchedTarget.contains(tDeti) &&
					    tDeti->textFrom() == DynamicElementTextItem::UserText &&
					    !pItem.compositeText.isEmpty() &&
					    tDeti->toPlainText() == pItem.compositeText) {
						foundDeti = tDeti;
						break;
					}
				}
				if (!foundDeti) {
					for (DynamicElementTextItem *tDeti : targetTexts) {
						if (!matchedTarget.contains(tDeti) &&
						    tDeti->textFrom() == DynamicElementTextItem::UserText) {
							foundDeti = tDeti;
							break;
						}
					}
				}
				if (foundDeti) {
					matchedTarget.insert(foundDeti);
					matchedPatternIndices.insert(i);

					ElementFormatChange::TextChange tc;
					tc.deti = foundDeti;
					tc.wasCreated = false;
					tc.oldFont = foundDeti->font();
					tc.oldColor = foundDeti->color();
					tc.oldAlignment = foundDeti->alignment();
					tc.oldPos = foundDeti->pos();
					tc.oldRotation = foundDeti->rotation();
					tc.oldKeepVisualRotation = foundDeti->keepVisualRotation();
					tc.oldRotationPointCenter = foundDeti->rotationPointCenter();
					tc.oldFrame = foundDeti->frame();
					tc.oldTextWidth = foundDeti->textWidth();
					tc.oldVisible = ShownKinds::wantsVisible(foundDeti);
					tc.oldGroupName = foundDeti->parentGroup() ? foundDeti->parentGroup()->name() : QString();

					tc.newFont = pItem.font;
					tc.newColor = pItem.color;
					tc.newAlignment = pItem.alignment;
					tc.newPos = pItem.pos;
					tc.newRotation = pItem.rotation;
					tc.newKeepVisualRotation = pItem.keepVisualRotation;
					tc.newRotationPointCenter = pItem.rotationPointCenter;
					tc.newFrame = pItem.frame;
					tc.newTextWidth = pItem.textWidth;
					tc.newVisible = pItem.visible;
					tc.newGroupName = pItem.groupName;

					elChange.textChanges.append(tc);
				}
			}
		}

		// Pass 3: CompositeText or other unmatched (non-terminal)
		for (int i = 0; i < pattern.textItems().size(); ++i) {
			if (matchedPatternIndices.contains(i)) continue;
			const DynamicTextFormatItem &pItem = pattern.textItems().at(i);
			if (pItem.isTerminalText) continue;
			DynamicElementTextItem *foundDeti = nullptr;
			for (DynamicElementTextItem *tDeti : targetTexts) {
				if (!matchedTarget.contains(tDeti) && tDeti->textFrom() == pItem.textFrom) {
					foundDeti = tDeti;
					break;
				}
			}
			if (foundDeti) {
				matchedTarget.insert(foundDeti);
				matchedPatternIndices.insert(i);

				ElementFormatChange::TextChange tc;
				tc.deti = foundDeti;
				tc.wasCreated = false;
				tc.oldFont = foundDeti->font();
				tc.oldColor = foundDeti->color();
				tc.oldAlignment = foundDeti->alignment();
				tc.oldPos = foundDeti->pos();
				tc.oldRotation = foundDeti->rotation();
				tc.oldKeepVisualRotation = foundDeti->keepVisualRotation();
				tc.oldRotationPointCenter = foundDeti->rotationPointCenter();
				tc.oldFrame = foundDeti->frame();
				tc.oldTextWidth = foundDeti->textWidth();
				tc.oldVisible = ShownKinds::wantsVisible(foundDeti);
				tc.oldGroupName = foundDeti->parentGroup() ? foundDeti->parentGroup()->name() : QString();

				tc.newFont = pItem.font;
				tc.newColor = pItem.color;
				tc.newAlignment = pItem.alignment;
				tc.newPos = pItem.pos;
				tc.newRotation = pItem.rotation;
				tc.newKeepVisualRotation = pItem.keepVisualRotation;
				tc.newRotationPointCenter = pItem.rotationPointCenter;
				tc.newFrame = pItem.frame;
				tc.newTextWidth = pItem.textWidth;
				tc.newVisible = pItem.visible;
				tc.newGroupName = pItem.groupName;

				elChange.textChanges.append(tc);
			}
		}

		// Pass 4: Pattern ElementInfo items that don't exist in target element
		for (int i = 0; i < pattern.textItems().size(); ++i) {
			if (matchedPatternIndices.contains(i)) continue;
			const DynamicTextFormatItem &pItem = pattern.textItems().at(i);
			if (pItem.textFrom == DynamicElementTextItem::ElementInfo && !pItem.infoName.isEmpty()) {
				ElementFormatChange::TextChange tc;
				tc.deti = nullptr;
				tc.wasCreated = true;
				tc.newFont = pItem.font;
				tc.newColor = pItem.color;
				tc.newAlignment = pItem.alignment;
				tc.newPos = pItem.pos;
				tc.newRotation = pItem.rotation;
				tc.newKeepVisualRotation = pItem.keepVisualRotation;
				tc.newRotationPointCenter = pItem.rotationPointCenter;
				tc.newFrame = pItem.frame;
				tc.newTextWidth = pItem.textWidth;
				tc.newVisible = pItem.visible;
				tc.newGroupName = pItem.groupName;
				tc.oldGroupName = pItem.infoName; // infoName stored temporarily in oldGroupName
				elChange.textChanges.append(tc);
			}
		}

		// Pass 5 (Destructive hiding of unmatched texts) is completely removed:
		// Non-matching target texts (such as additional poles on 3-pole/4-pole elements)
		// are never hidden and retain their exact positions and visibilities!

		m_changes.append(elChange);
	}
}

ApplyElementTextFormatCommand::~ApplyElementTextFormatCommand()
{
	for (ElementFormatChange &elChange : m_changes) {
		Element *element = elChange.element.data();
		for (ElementFormatChange::TextChange &tc : elChange.textChanges) {
			if (tc.wasCreated && tc.deti) {
				DynamicElementTextItem *deti = tc.deti.data();
				if (!deti) continue;
				if (deti->parentGroup()) continue;
				if (!element || !element->dynamicTextItems().contains(deti)) {
					delete deti;
				}
			}
		}
		for (ElementFormatChange::GroupChange &gc : elChange.groupChanges) {
			if (gc.wasCreated && gc.group) {
				ElementTextItemGroup *group = gc.group.data();
				if (!group) continue;
				if (!element || !element->textGroups().contains(group)) {
					delete group;
				}
			}
		}
	}
}

void ApplyElementTextFormatCommand::redo()
{
	for (ElementFormatChange &elChange : m_changes) {
		Element *element = elChange.element.data();
		if (!element) continue;

		// 1. Create and update groups
		for (ElementFormatChange::GroupChange &gc : elChange.groupChanges) {
			ElementTextItemGroup *group = gc.group.data();
			if (!group) {
				group = element->textGroup(gc.name);
				if (!group) {
					group = element->addTextGroup(gc.name);
				}
				gc.group = group;
			} else {
				if (!element->textGroups().contains(group)) {
					element->addTextGroup(group);
				}
			}
			if (group) {
				group->setPos(gc.newPos);
				group->setRotation(gc.newRotation);
				group->setAlignment(gc.newAlignment);
				group->setVerticalAdjustment(gc.newVAdjust);
				group->setFrame(gc.newFrame);
				group->setHoldToBottomPage(gc.newHoldBottom);
			}
		}

		// 2. Apply texts
		for (ElementFormatChange::TextChange &tc : elChange.textChanges) {
			DynamicElementTextItem *deti = tc.deti.data();
			if (tc.wasCreated) {
				if (!deti) {
					deti = new DynamicElementTextItem(element);
					deti->setTextFrom(DynamicElementTextItem::ElementInfo);
					deti->setInfoName(tc.oldGroupName);
					element->addDynamicTextItem(deti);
					tc.deti = deti;
				} else {
					deti->setParentItem(element);
					if (!element->dynamicTextItems().contains(deti)) {
						element->addDynamicTextItem(deti);
					}
				}
			}
			if (!deti) continue;

			// Group membership
			if (!tc.newGroupName.isEmpty()) {
				ElementTextItemGroup *targetGroup = element->textGroup(tc.newGroupName);
				if (targetGroup) {
					if (deti->parentGroup() != targetGroup) {
						if (deti->parentGroup()) {
							element->removeTextFromGroup(deti, deti->parentGroup());
						}
						element->addTextToGroup(deti, targetGroup);
					}
				}
			} else {
				if (deti->parentGroup()) {
					element->removeTextFromGroup(deti, deti->parentGroup());
				}
			}

			// Apply properties
			deti->setFont(tc.newFont);
			deti->setColor(tc.newColor);
			deti->setAlignment(tc.newAlignment);
			deti->setPos(tc.newPos);
			deti->setRotation(tc.newRotation);
			deti->setKeepVisualRotation(tc.newKeepVisualRotation);
			deti->setRotationPointCenter(tc.newRotationPointCenter);
			deti->setFrame(tc.newFrame);
			deti->setTextWidth(tc.newTextWidth);
			ShownKinds::setVisible(deti, tc.newVisible);
			deti->update();
		}

		element->update();
		if (element->diagram()) {
			element->diagram()->update();
		}
	}
}

void ApplyElementTextFormatCommand::undo()
{
	for (ElementFormatChange &elChange : m_changes) {
		Element *element = elChange.element.data();
		if (!element) continue;

		// 1. Restore texts
		for (ElementFormatChange::TextChange &tc : elChange.textChanges) {
			DynamicElementTextItem *deti = tc.deti.data();
			if (!deti) continue;

			if (tc.wasCreated) {
				if (deti->parentGroup()) {
					element->removeTextFromGroup(deti, deti->parentGroup());
				}
				element->removeDynamicTextItem(deti);
				if (deti->scene()) {
					deti->scene()->removeItem(deti);
				}
				continue;
			}

			// Restore group membership
			if (!tc.oldGroupName.isEmpty()) {
				ElementTextItemGroup *origGroup = element->textGroup(tc.oldGroupName);
				if (origGroup) {
					if (deti->parentGroup() != origGroup) {
						if (deti->parentGroup()) {
							element->removeTextFromGroup(deti, deti->parentGroup());
						}
						element->addTextToGroup(deti, origGroup);
					}
				}
			} else {
				if (deti->parentGroup()) {
					element->removeTextFromGroup(deti, deti->parentGroup());
				}
			}

			// Restore properties
			deti->setFont(tc.oldFont);
			deti->setColor(tc.oldColor);
			deti->setAlignment(tc.oldAlignment);
			deti->setPos(tc.oldPos);
			deti->setRotation(tc.oldRotation);
			deti->setKeepVisualRotation(tc.oldKeepVisualRotation);
			deti->setRotationPointCenter(tc.oldRotationPointCenter);
			deti->setFrame(tc.oldFrame);
			deti->setTextWidth(tc.oldTextWidth);
			ShownKinds::setVisible(deti, tc.oldVisible);
			deti->update();
		}

		// 2. Restore groups
		for (ElementFormatChange::GroupChange &gc : elChange.groupChanges) {
			ElementTextItemGroup *group = gc.group.data();
			if (!group) continue;

			if (gc.wasCreated) {
				element->removeTextGroup(group);
				continue;
			}

			group->setPos(gc.oldPos);
			group->setRotation(gc.oldRotation);
			group->setAlignment(gc.oldAlignment);
			group->setVerticalAdjustment(gc.oldVAdjust);
			group->setFrame(gc.oldFrame);
			group->setHoldToBottomPage(gc.oldHoldBottom);
		}

		element->update();
		if (element->diagram()) {
			element->diagram()->update();
		}
	}
}
