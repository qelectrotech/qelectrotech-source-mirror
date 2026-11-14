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
#include "diagrameventaddstructurebox.h"

#include "../diagram.h"
#include "../qetgraphicsitem/structureboxitem.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGraphicsView>
#include <QLineEdit>

DiagramEventAddStructureBox::DiagramEventAddStructureBox(Diagram *diagram) :
	DiagramEventAddShape(diagram, QetShapeItem::Rectangle)
{
}

QetShapeItem *DiagramEventAddStructureBox::createShapeItem(QPointF p1, QPointF p2, QetShapeItem::ShapeType shape_type)
{
	Q_UNUSED(shape_type)
	auto box = new StructureBoxItem(p1, p2);
		//A new box is drawn in the colour chosen for structure boxes
	QPen pen = box->pen();
	pen.setColor(StructureBoxItem::newBoxColor());
	box->setPen(pen);
	return box;
}

/**
	@brief DiagramEventAddStructureBox::shapeDrawn
	A box without a place does nothing: if wanted, it is asked for at once.
*/
void DiagramEventAddStructureBox::shapeDrawn(QetShapeItem *shape)
{
	if (!StructureBoxItem::askPlaceWhenDrawn())
		return;
	auto box = static_cast<StructureBoxItem *>(shape);
	const auto views = m_diagram->views();
	askPlace(box, views.isEmpty() ? nullptr : views.first());
}

bool DiagramEventAddStructureBox::askPlace(StructureBoxItem *box, QWidget *parent)
{
	QDialog dialog(parent);
	dialog.setObjectName(QStringLiteral("askStructureBoxPlace"));
	dialog.setWindowTitle(tr("New structure box"));
	auto layout = new QFormLayout(&dialog);
	auto plant = new QLineEdit(box->plant(), &dialog);
	auto location = new QLineEdit(box->location(), &dialog);
	auto prefix = new QLineEdit(box->prefix(), &dialog);
	plant->setObjectName(QStringLiteral("plant"));
	location->setObjectName(QStringLiteral("location"));
	prefix->setObjectName(QStringLiteral("prefix"));
	layout->addRow(tr("Plant (=)"), plant);
	layout->addRow(tr("Location (+)"), location);
	layout->addRow(tr("Prefix (-)"), prefix);
	auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addRow(buttons);
	connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	location->setFocus();

	if (dialog.exec() != QDialog::Accepted)
		return false;
	box->setPlant(plant->text().trimmed());
	box->setLocation(location->text().trimmed());
	box->setPrefix(prefix->text().trimmed());
	return true;
}
