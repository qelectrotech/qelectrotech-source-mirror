/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "terminaleditor.h"
#include "partterminal.h"
#include "qeticons.h"

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param term La borne a editer
	@param parent QWidget parent de ce widget
*/
TerminalEditor::TerminalEditor(QETElementEditor *editor, PartTerminal *term, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(term)
{
	qle_x = new QLineEdit();
	qle_y = new QLineEdit();
	
	qle_x -> setValidator(new QDoubleValidator(qle_x));
	qle_y -> setValidator(new QDoubleValidator(qle_y));
	
	orientation = new QComboBox();
	orientation -> addItem(QET::Icons::North, tr("Nord"),  QET::North);
	orientation -> addItem(QET::Icons::East,  tr("Est"),   QET::East);
	orientation -> addItem(QET::Icons::South, tr("Sud"),   QET::South);
	orientation -> addItem(QET::Icons::West,  tr("Ouest"), QET::West);
	
	qle_number = new QLineEdit();
	qle_name   = new QLineEdit();
	qcheck_name_visible = new QCheckBox(tr("Visible"));
	
	QVBoxLayout *main_layout = new QVBoxLayout();
	main_layout -> addWidget(new QLabel(tr("Position : ")));
	
	QHBoxLayout *position = new QHBoxLayout();
	position -> addWidget(new QLabel(tr("x : ")));
	position -> addWidget(qle_x                 );
	position -> addWidget(new QLabel(tr("y : ")));
	position -> addWidget(qle_y                 );
	main_layout -> addLayout(position);
	
	QHBoxLayout *ori = new QHBoxLayout();
	ori -> addWidget(new QLabel(tr("Orientation : ")));
	ori -> addWidget(orientation                     );
	main_layout -> addLayout(ori);
	
	QHBoxLayout *name = new QHBoxLayout();
	name -> addWidget(new QLabel(tr("Nom : ")));
	name -> addWidget(qle_name                     );
	name -> addWidget(qcheck_name_visible                     );
	main_layout -> addLayout(name);
	
	QHBoxLayout *num = new QHBoxLayout();
	num -> addWidget(new QLabel(tr("Num\351ro : ")));
	num -> addWidget(qle_number                     );
	main_layout -> addLayout(num);
	
	main_layout -> addStretch();
	setLayout(main_layout);
	
	activeConnections(true);
	updateForm();
}

/// Destructeur
TerminalEditor::~TerminalEditor() {
};

/**
	Permet de specifier a cet editeur quelle primitive il doit editer. A noter
	qu'un editeur peut accepter ou refuser d'editer une primitive.
	L'editeur de borne acceptera d'editer la primitive new_part s'il s'agit d'un
	objet de la classe PartTerminal.
	@param new_part Nouvelle primitive a editer
	@return true si l'editeur a accepter d'editer la primitive, false sinon
*/
bool TerminalEditor::setPart(CustomElementPart *new_part) {
	if (!new_part) {
		part = 0;
		return(true);
	}
	if (PartTerminal *part_terminal = dynamic_cast<PartTerminal *>(new_part)) {
		part = part_terminal;
		updateForm();
		return(true);
	} else {
		return(false);
	}
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *TerminalEditor::currentPart() const {
	return(part);
}

/**
	Met a jour la borne a partir des donnees du formulaire
*/
void TerminalEditor::updateTerminal() {
	if (!part) return;
	part -> setPos(qle_x -> text().toDouble(), qle_y -> text().toDouble());
	part -> setOrientation(
		static_cast<QET::Orientation>(
			orientation -> itemData(
				orientation -> currentIndex()
			).toInt()
		)
	);
	part -> setNumber( qle_number->text() );
	part -> setName  ( qle_name->text() );
	part -> setNameHidden( !qcheck_name_visible ->isChecked() );
}

/// WARNING!!!! on addChangePartCommand the prop accept only the simple string! (NOT /:;,?...)
/// Met a jour l'abscisse de la position de la borne et cree un objet d'annulation
void TerminalEditor::updateTerminalX() { addChangePartCommand(tr("abscisse"),    part, "x",           qle_x -> text().toDouble()); updateForm(); }
/// Met a jour l'ordonnee de la position de la borne et cree un objet d'annulation
void TerminalEditor::updateTerminalY() { addChangePartCommand(tr("ordonn\351e"), part, "y",           qle_y -> text().toDouble()); updateForm(); }
/// Met a jour l'orientation de la borne et cree un objet d'annulation
void TerminalEditor::updateTerminalO() { addChangePartCommand(tr("orientation"), part, "orientation", orientation -> itemData(orientation -> currentIndex()).toInt()); }
/// update Number and name, create cancel object
void TerminalEditor::updateTerminalNum() {
	addChangePartCommand(tr("num\351ro: ")+qle_number -> text(), part, "number", qle_number -> text());
	updateForm();
}
void TerminalEditor::updateTerminalName() {
	addChangePartCommand(tr("nom: ")+qle_name -> text(), part, "name", qle_name -> text());
	updateForm();
}
void TerminalEditor::updateTerminalNameVisible() {
	addChangePartCommand(tr("nom visible: ")+QString::number( qcheck_name_visible->isChecked()), part, "nameHidden", !qcheck_name_visible -> isChecked());
	updateForm();
}
/**
	Met a jour le formulaire d'edition
*/
void TerminalEditor::updateForm() {
	if (!part) return;
	activeConnections(false);
	qle_x -> setText(part -> property("x").toString());
	qle_y -> setText(part -> property("y").toString());
	orientation -> setCurrentIndex(static_cast<int>(part -> orientation()));
	qle_number -> setText(part -> number() );
	qle_name -> setText(part -> nameOfTerminal() );
	qcheck_name_visible ->setChecked( !part -> nameIsHidden() );
	activeConnections(true);
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void TerminalEditor::activeConnections(bool active) {
	if (active) {
		connect(qle_x,       SIGNAL(editingFinished()), this, SLOT(updateTerminalX()));
		connect(qle_y,       SIGNAL(editingFinished()), this, SLOT(updateTerminalY()));
		connect(orientation, SIGNAL(activated(int)),    this, SLOT(updateTerminalO()));
		connect(qle_number,  SIGNAL(editingFinished()), this, SLOT(updateTerminalNum()));
		connect(qle_name,    SIGNAL(editingFinished()), this, SLOT(updateTerminalName()));
		connect(qcheck_name_visible,    SIGNAL(stateChanged ( int)), this, SLOT(updateTerminalNameVisible()));
	} else {
		disconnect(qle_x,       SIGNAL(editingFinished()), this, SLOT(updateTerminalX()));
		disconnect(qle_y,       SIGNAL(editingFinished()), this, SLOT(updateTerminalY()));
		disconnect(orientation, SIGNAL(activated(int)),    this, SLOT(updateTerminalO()));
		disconnect(qle_number,  SIGNAL(editingFinished()), this, SLOT(updateTerminalNum()));
		disconnect(qle_name,    SIGNAL(editingFinished()), this, SLOT(updateTerminalName()));
		disconnect(qcheck_name_visible,    SIGNAL(stateChanged ( int)), this, SLOT(updateTerminalNameVisible()));
	}
}
