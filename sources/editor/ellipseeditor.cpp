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
#include "ellipseeditor.h"
#include "styleeditor.h"
#include "partellipse.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementscene.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param ellipse L'ellipse a editer
	@param parent le Widget parent
*/
EllipseEditor::EllipseEditor(QETElementEditor *editor, PartEllipse *ellipse, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(ellipse),
	m_locked(false)
{
	style_ = new StyleEditor(editor);
	
	x = new QDoubleSpinBox();
	y = new QDoubleSpinBox();
	h = new QDoubleSpinBox();
	v = new QDoubleSpinBox();
	
	x->setRange(-5000, 5000);
	y->setRange(-5000, 5000);
	h->setRange(-5000, 5000);
	v->setRange(-5000, 5000);
	
	QVBoxLayout *v_layout = new QVBoxLayout(this);
	
	QGridLayout *grid = new QGridLayout();
	grid -> addWidget(new QLabel(tr("Centre : ")),       0, 0);
	grid -> addWidget(new QLabel("x"),                   1, 0, Qt::AlignRight);
	grid -> addWidget(x,                                 1, 1);
	grid -> addWidget(new QLabel("y"),                   1, 2);
	grid -> addWidget(y,                                 1, 3);
	grid -> addWidget(new QLabel(tr("Diamètres : ")), 2, 0);
	grid -> addWidget(new QLabel(tr("horizontal :")),    3, 0);
	grid -> addWidget(h,                                 3, 1);
	grid -> addWidget(new QLabel(tr("vertical :")),      4, 0);
	grid -> addWidget(v,                                 4, 1);
	
	v_layout -> addWidget(style_);
	v_layout -> addLayout(grid);
	
	activeConnections(true);
	updateForm();
}

/// Destructeur
EllipseEditor::~EllipseEditor() {
}

/**
	Permet de specifier a cet editeur quelle primitive il doit editer. A noter
	qu'un editeur peut accepter ou refuser d'editer une primitive.
	L'editeur d'ellipse acceptera d'editer la primitive new_part s'il s'agit
	d'un objet de la classe PartEllipse.
	@param new_part Nouvelle primitive a editer
	@return true si l'editeur a accepter d'editer la primitive, false sinon
*/
bool EllipseEditor::setPart(CustomElementPart *new_part)
{
	if (!new_part)
	{
		if (part)
			disconnect(part, &PartEllipse::rectChanged, this, &EllipseEditor::updateForm);
		part = 0;
		style_ -> setPart(0);
		return(true);
	}
	if (PartEllipse *part_ellipse = dynamic_cast<PartEllipse *>(new_part))
	{
		if (part == part_ellipse) return true;
		if (part)
			disconnect(part, &PartEllipse::rectChanged, this, &EllipseEditor::updateForm);
		part = part_ellipse;
		style_ -> setPart(part);
		updateForm();
		connect(part, &PartEllipse::rectChanged, this, &EllipseEditor::updateForm);
		return(true);
	}
	return(false);
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *EllipseEditor::currentPart() const {
	return(part);
}

void EllipseEditor::editingFinished()
{
	if (m_locked) return;
	m_locked = true;
	QPointF point = part->mapFromScene(x->value() - h->value()/2, y->value() - v->value()/2);
	QRectF rect(point, QSizeF(h->value(), v->value()));

	if (rect != part->property("rect"))
	{
		QPropertyUndoCommand *undo= new QPropertyUndoCommand(part, "rect", part->property("rect"), rect);
		undo->setText("Modifier une ellipse");
		undo->enableAnimation();
		elementScene()->undoStack().push(undo);
	}

	m_locked = false;
}

/**
	Met a jour le formulaire d'edition
*/
void EllipseEditor::updateForm()
{
	if (!part) return;
	activeConnections(false);
	QRectF rect = part->property("rect").toRectF();
	x->setValue(part->mapToScene(rect.topLeft()).x() + (rect.width()/2));
	y->setValue(part->mapToScene(rect.topLeft()).y() + (rect.height()/2));
	h->setValue(rect.width());
	v->setValue(rect.height());
	activeConnections(true);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void EllipseEditor::activeConnections(bool active)
{
	if (active)
	{
		connect(x, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
		connect(y, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
		connect(h, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
		connect(v, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
	}
	else
	{
		disconnect(x, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
		disconnect(y, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
		disconnect(h, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
		disconnect(v, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
	}
}
