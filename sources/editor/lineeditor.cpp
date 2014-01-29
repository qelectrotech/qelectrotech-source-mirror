/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "lineeditor.h"
#include "styleeditor.h"
#include "partline.h"
#include "qet.h"
#include "qeticons.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param line La ligne a editer
	@param parent le Widget parent
*/
LineEditor::LineEditor(QETElementEditor *editor, PartLine *line, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(line)
{
	style_ = new StyleEditor(editor);
	
	x1 = new QLineEdit();
	y1 = new QLineEdit();
	x2 = new QLineEdit();
	y2 = new QLineEdit();
	
	x1 -> setValidator(new QDoubleValidator(x1));
	y1 -> setValidator(new QDoubleValidator(y1));
	x2 -> setValidator(new QDoubleValidator(x2));
	y2 -> setValidator(new QDoubleValidator(y2));
	
	end1_type = new QComboBox();
	end1_type -> addItem(QET::Icons::EndLineNone,     tr("Normale",                "type of the 1st end of a line"), QET::None    );
	end1_type -> addItem(QET::Icons::EndLineSimple,   tr("Fl\350che simple",       "type of the 1st end of a line"), QET::Simple  );
	end1_type -> addItem(QET::Icons::EndLineTriangle, tr("Fl\350che triangulaire", "type of the 1st end of a line"), QET::Triangle);
	end1_type -> addItem(QET::Icons::EndLineCircle,   tr("Cercle",                 "type of the 1st end of a line"), QET::Circle  );
	end1_type -> addItem(QET::Icons::EndLineDiamond,  tr("Carr\351",               "type of the 1st end of a line"), QET::Diamond );
	end2_type = new QComboBox();
	end2_type -> addItem(QET::Icons::EndLineNone,     tr("Normale",                "type of the 2nd end of a line"), QET::None    );
	end2_type -> addItem(QET::Icons::EndLineSimple,   tr("Fl\350che simple",       "type of the 2nd end of a line"), QET::Simple  );
	end2_type -> addItem(QET::Icons::EndLineTriangle, tr("Fl\350che triangulaire", "type of the 2nd end of a line"), QET::Triangle);
	end2_type -> addItem(QET::Icons::EndLineCircle,   tr("Cercle",                 "type of the 2nd end of a line"), QET::Circle  );
	end2_type -> addItem(QET::Icons::EndLineDiamond,  tr("Carr\351",               "type of the 2nd end of a line"), QET::Diamond );
	
	end1_length = new QLineEdit();
	end2_length = new QLineEdit();
	
	end1_length -> setValidator(new QDoubleValidator(end1_length));
	end2_length -> setValidator(new QDoubleValidator(end2_length));
	
	QGridLayout *grid = new QGridLayout();
	grid -> addWidget(new QLabel("x1"),        0, 0);
	grid -> addWidget(x1,                      0, 1);
	grid -> addWidget(new QLabel("y1"),        0, 2);
	grid -> addWidget(y1,                      0, 3);
	grid -> addWidget(new QLabel("x2"),        1, 0);
	grid -> addWidget(x2,                      1, 1);
	grid -> addWidget(new QLabel("y2"),        1, 2);
	grid -> addWidget(y2,                      1, 3);
	
	QGridLayout *grid2 = new QGridLayout();
	grid2 -> addWidget(new QLabel(tr("Fin 1")), 0, 0);
	grid2 -> addWidget(end1_type,               0, 1);
	grid2 -> addWidget(end1_length,             0, 2);
	grid2 -> addWidget(new QLabel(tr("Fin 2")), 1, 0);
	grid2 -> addWidget(end2_type,               1, 1);
	grid2 -> addWidget(end2_length,             1, 2);
	
	QVBoxLayout *v_layout = new QVBoxLayout(this);
	v_layout -> addWidget(style_);
	v_layout -> addLayout(grid);
	v_layout -> addLayout(grid2);
	updateForm();
}

/// Destructeur
LineEditor::~LineEditor() {
}

/**
	Permet de specifier a cet editeur quelle primitive il doit editer. A noter
	qu'un editeur peut accepter ou refuser d'editer une primitive.
	L'editeur de ligne acceptera d'editer la primitive new_part s'il s'agit d'un
	objet de la classe PartLine.
	@param new_part Nouvelle primitive a editer
	@return true si l'editeur a accepter d'editer la primitive, false sinon
*/
bool LineEditor::setPart(CustomElementPart *new_part) {
	if (!new_part) {
		part = 0;
		style_ -> setPart(0);
		return(true);
	}
	if (PartLine *part_line = dynamic_cast<PartLine *>(new_part)) {
		part = part_line;
		style_ -> setPart(part);
		updateForm();
		return(true);
	} else {
		return(false);
	}
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *LineEditor::currentPart() const {
	return(part);
}

/**
	Met a jour la ligne a partir des donnees du formulaire
*/
void LineEditor::updateLine() {
	if (!part) return;
	part -> setFirstEndType(static_cast<QET::EndType>(end1_type -> currentIndex()));
	part -> setFirstEndLength(end1_length -> text().toDouble());
	part -> setSecondEndType(static_cast<QET::EndType>(end2_type -> currentIndex()));
	part -> setSecondEndLength(end2_length -> text().toDouble());
	part -> setLine(
		QLineF(
			part -> mapFromScene(
				x1 -> text().toDouble(),
				y1 -> text().toDouble()
			),
			part -> mapFromScene(
				x2 -> text().toDouble(),
				y2 -> text().toDouble()
			)
		)
	);
}

/// Met a jour l'abscisse du premier point de la ligne et cree un objet d'annulation
void LineEditor::updateLineX1() { addChangePartCommand(tr("abscisse point 1"),    part, "x1", x1 -> text().toDouble()); }
/// Met a jour l'ordonnee du premier point de la ligne et cree un objet d'annulation
void LineEditor::updateLineY1() { addChangePartCommand(tr("ordonn\351e point 1"), part, "y1", y1 -> text().toDouble()); }
/// Met a jour l'abscisse du second point de la ligne et cree un objet d'annulation
void LineEditor::updateLineX2() { addChangePartCommand(tr("abscisse point 2"),    part, "x2", x2 -> text().toDouble()); }
/// Met a jour l'ordonnee du second point de la ligne et cree un objet d'annulation
void LineEditor::updateLineY2() { addChangePartCommand(tr("ordonn\351e point 2"), part, "y2", y2 -> text().toDouble()); }
/// Met a jour le type de la premiere extremite
void LineEditor::updateLineEndType1() {   addChangePartCommand(tr("type fin 1"),     part, "end1",    end1_type -> currentIndex());   }
/// Met a jour la longueur de la premiere extremite
void LineEditor::updateLineEndLength1() { addChangePartCommand(tr("longueur fin 1"), part, "length1", end1_length -> text()); }
/// Met a jour le type de la seconde extremite
void LineEditor::updateLineEndType2() {   addChangePartCommand(tr("type fin 2"),     part, "end2",    end2_type -> currentIndex());   }
/// Met a jour la longueur de la seconde extremite
void LineEditor::updateLineEndLength2() { addChangePartCommand(tr("longueur fin 2"), part, "length2", end2_length -> text()); }

/**
	Met a jour le formulaire d'edition
*/
void LineEditor::updateForm() {
	if (!part) return;
	activeConnections(false);
	QPointF p1(part -> sceneP1());
	QPointF p2(part -> sceneP2());
	x1 -> setText(QString("%1").arg(p1.x()));
	y1 -> setText(QString("%1").arg(p1.y()));
	x2 -> setText(QString("%1").arg(p2.x()));
	y2 -> setText(QString("%1").arg(p2.y()));
	end1_type -> setCurrentIndex(part -> firstEndType());
	end1_length -> setText(QString("%1").arg(part -> firstEndLength()));
	end2_type -> setCurrentIndex(part -> secondEndType());
	end2_length -> setText(QString("%1").arg(part -> secondEndLength()));
	activeConnections(true);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void LineEditor::activeConnections(bool active) {
	if (active) {
		connect(x1, SIGNAL(editingFinished()), this, SLOT(updateLineX1()));
		connect(y1, SIGNAL(editingFinished()), this, SLOT(updateLineY1()));
		connect(x2, SIGNAL(editingFinished()), this, SLOT(updateLineX2()));
		connect(y2, SIGNAL(editingFinished()), this, SLOT(updateLineY2()));
		connect(end1_type,   SIGNAL(currentIndexChanged(int)), this, SLOT(updateLineEndType1()));
		connect(end1_length, SIGNAL(editingFinished()),        this, SLOT(updateLineEndLength1()));
		connect(end2_type,   SIGNAL(currentIndexChanged(int)), this, SLOT(updateLineEndType2()));
		connect(end2_length, SIGNAL(editingFinished()),        this, SLOT(updateLineEndLength2()));
	} else {
		disconnect(x1, SIGNAL(editingFinished()), this, SLOT(updateLineX1()));
		disconnect(y1, SIGNAL(editingFinished()), this, SLOT(updateLineY1()));
		disconnect(x2, SIGNAL(editingFinished()), this, SLOT(updateLineX2()));
		disconnect(y2, SIGNAL(editingFinished()), this, SLOT(updateLineY2()));
		disconnect(end1_type,   SIGNAL(currentIndexChanged(int)), this, SLOT(updateLineEndType1()));
		disconnect(end1_length, SIGNAL(editingFinished()),        this, SLOT(updateLineEndLength1()));
		disconnect(end2_type,   SIGNAL(currentIndexChanged(int)), this, SLOT(updateLineEndType2()));
		disconnect(end2_length, SIGNAL(editingFinished()),        this, SLOT(updateLineEndLength2()));
	}
}
