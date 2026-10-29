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
#include "dimensionpropertieswidget.h"
#include "ui_dimensionpropertieswidget.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../qetgraphicsitem/dimensionitem.h"

DimensionPropertiesWidget::DimensionPropertiesWidget(DimensionItem *item, QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::DimensionPropertiesWidget),
	m_item(nullptr)
{
	ui->setupUi(this);

	ui->m_style_cb->addItem(tr("Arrows"), DimensionItem::Arrows);
	ui->m_style_cb->addItem(tr("Architectural Slashes"), DimensionItem::Slashes);

	if (item) {
		setItem(item);
	} else {
		setEnabled(false);
	}
}

DimensionPropertiesWidget::DimensionPropertiesWidget(const QList<DimensionItem *> &items, QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::DimensionPropertiesWidget),
	m_item(nullptr)
{
	ui->setupUi(this);

	ui->m_style_cb->addItem(tr("Arrows"), DimensionItem::Arrows);
	ui->m_style_cb->addItem(tr("Architectural Slashes"), DimensionItem::Slashes);

	setItems(items);
}

DimensionPropertiesWidget::~DimensionPropertiesWidget()
{
	clearConnections();
	delete ui;
}

void DimensionPropertiesWidget::setItem(DimensionItem *item)
{
	QList<DimensionItem *> list;
	if (item) {
		list.append(item);
	}
	setItems(list);
}

void DimensionPropertiesWidget::setItems(const QList<DimensionItem *> &items)
{
	clearConnections();
	m_items.clear();
	for (DimensionItem *it : items) {
		if (it) m_items.append(it);
	}

	m_item = m_items.isEmpty() ? nullptr : m_items.first();
	setEnabled(!m_items.isEmpty());

	setupConnections();
	updateUi();
}

void DimensionPropertiesWidget::setupConnections()
{
	for (const auto &it : m_items) {
		if (it) {
			m_connections << connect(it.data(), &DimensionItem::geometryChanged,
			                         this, &DimensionPropertiesWidget::updateUi);
			m_connections << connect(it.data(), &DimensionItem::propertiesChanged,
			                         this, &DimensionPropertiesWidget::updateUi);
		}
	}
}

void DimensionPropertiesWidget::clearConnections()
{
	for (const auto &c : m_connections) {
		disconnect(c);
	}
	m_connections.clear();
}

void DimensionPropertiesWidget::updateUi()
{
	if (!m_item) {
		setEnabled(false);
		return;
	}

	m_updating = true;

	qreal valMM = m_item->dimensionValueMM();
	ui->m_length_label->setText(QStringLiteral("%1 mm").arg(QString::number(valMM, 'f', 1)));

	ui->m_auto_cb->setChecked(m_item->isAutoValue());
	ui->m_custom_text_le->setEnabled(!m_item->isAutoValue());
	ui->m_custom_text_le->setText(m_item->customText());
	ui->m_offset_sb->setValue(m_item->offset());

	int styleIdx = ui->m_style_cb->findData(m_item->terminationStyle());
	if (styleIdx >= 0) {
		ui->m_style_cb->setCurrentIndex(styleIdx);
	}

	m_updating = false;
}

void DimensionPropertiesWidget::apply()
{
	// Live edit automatically applies changes
}

void DimensionPropertiesWidget::reset()
{
	updateUi();
}

void DimensionPropertiesWidget::on_m_auto_cb_toggled(bool checked)
{
	if (m_updating || !m_item) return;

	ui->m_custom_text_le->setEnabled(!checked);

	if (Diagram *d = m_item->diagram()) {
		d->undoStack().push(new QPropertyUndoCommand(m_item.data(), "isAutoValue", m_item->isAutoValue(), checked));
	} else {
		m_item->setAutoValue(checked);
	}
}

void DimensionPropertiesWidget::on_m_custom_text_le_editingFinished()
{
	if (m_updating || !m_item) return;

	QString newText = ui->m_custom_text_le->text();
	if (newText == m_item->customText()) return;

	if (Diagram *d = m_item->diagram()) {
		d->undoStack().push(new QPropertyUndoCommand(m_item.data(), "customText", m_item->customText(), newText));
	} else {
		m_item->setCustomText(newText);
	}
}

void DimensionPropertiesWidget::on_m_offset_sb_valueChanged(double val)
{
	if (m_updating || !m_item) return;
	if (qFuzzyCompare(val, m_item->offset())) return;

	if (Diagram *d = m_item->diagram()) {
		d->undoStack().push(new QPropertyUndoCommand(m_item.data(), "offset", m_item->offset(), val));
	} else {
		m_item->setOffset(val);
	}
}

void DimensionPropertiesWidget::on_m_style_cb_currentIndexChanged(int index)
{
	if (m_updating || !m_item || index < 0) return;

	DimensionItem::TerminationStyle newStyle = static_cast<DimensionItem::TerminationStyle>(
		ui->m_style_cb->currentData().toInt());

	if (newStyle == m_item->terminationStyle()) return;

	if (Diagram *d = m_item->diagram()) {
		d->undoStack().push(new QPropertyUndoCommand(m_item.data(), "terminationStyle",
		                                            static_cast<int>(m_item->terminationStyle()),
		                                            static_cast<int>(newStyle)));
	} else {
		m_item->setTerminationStyle(newStyle);
	}
}

