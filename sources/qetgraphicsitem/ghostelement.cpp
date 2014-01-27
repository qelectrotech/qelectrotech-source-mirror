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
#include "ghostelement.h"
#include "qet.h"
#include "terminal.h"
#include "elementtextitem.h"
#include "diagramcommands.h"

/**
	Constructeur
	@param location Emplacement de la definition d'element a utiliser
	@param qgi Le QGraphicsItem parent de cet element
	@param d Le schema affichant cet element
*/
GhostElement::GhostElement(
	const ElementsLocation &location,
	QGraphicsItem *qgi,
	Diagram *d
) :
	CustomElement(location, qgi, d)
{
	QString tooltip_string = QString(
		tr("<u>\311l\351ment manquant\240:</u> %1")
	).arg(location_.toString());
	setToolTip(tooltip_string);
}

/**
	Destructeur
*/
GhostElement::~GhostElement() {
}


/**
	@param e L'element XML a analyser.
	@param table_id_adr Reference vers la table de correspondance entre les IDs
	du fichier XML et les adresses en memoire. Si l'import reussit, il faut y
	ajouter les bons couples (id, adresse).
	@return true si l'import a reussi, false sinon
*/
bool GhostElement::fromXml(QDomElement &e, QHash<int, Terminal *> &table_id_adr, bool handle_inputs_rotation) {
	// instancie les bornes decrites dans l'element XML
	terminalsFromXml(e, table_id_adr);
	
	// instancie les champs de texte decrits dans l'element XML
	foreach(QDomElement qde, QET::findInDomElement(e, "inputs", "input")) {
		qde.setAttribute("size", 9); // arbitraire
		if (ElementTextItem *new_input = CustomElement::parseInput(qde)) {
			new_input -> fromXml(qde);
		}
		qde.removeAttribute("size");
	}
	
	/*
		maintenant que l'element fantome connait toutes les bornes et tous les
		champs de texte, on peut determiner une taille appropriee
	*/
	QRect final_bounding_rect = minimalBoundingRect().united(childrenBoundingRect()).toAlignedRect();
	setSize(final_bounding_rect.width(), final_bounding_rect.height());
	setHotspot(QPoint() - final_bounding_rect.topLeft());
	setInternalConnections(true);
	
	// on peut desormais confectionner le rendu de l'element
	generateDrawings();
	
	// position, selection
	setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	
	// orientation
	bool conv_ok;
	int read_ori = e.attribute("orientation").toInt(&conv_ok);
	if (!conv_ok || read_ori < 0 || read_ori > 3) read_ori = 0;
	if (handle_inputs_rotation) {
		rotateBy(90*read_ori);
	} else {
		applyRotation(90*read_ori);
	}
	return(true);
}

/**
	@return le bounding rect minimum, utilise si l'element fantome n'a ni champ
	de texte ni borne.
*/
QRectF GhostElement::minimalBoundingRect() const {
	return(
		QRectF(
			QPointF(-10.0, -10.0),
			QSizeF(20.0, 20.0)
		)
	);
}

/**
	Gere l'import des bornes
	@param e L'element XML a analyser.
	@param table_id_adr Reference vers la table de correspondance entre les IDs
	du fichier XML et les adresses en memoire. Si l'import reussit, il faut y
	ajouter les bons couples (id, adresse).
	@return true si l'import a reussi, false sinon
*/
bool GhostElement::terminalsFromXml(QDomElement &e, QHash<int, Terminal *> &table_id_adr) {
	// instancie les bornes decrites dans l'element XML
	foreach(QDomElement qde, QET::findInDomElement(e, "terminals", "terminal")) {
		if (!Terminal::valideXml(qde)) continue;
		
		// modifie certains attributs pour que l'analyse par la classe CustomElement reussisse
		int previous_x_value   = qde.attribute("x").toInt();
		int previous_y_value   = qde.attribute("y").toInt();
		int previous_ori_value = qde.attribute("orientation").toInt();
		
		qreal x_add = 0.0, y_add = 0.0;
		if (previous_ori_value == QET::North)      y_add = -Terminal::terminalSize;
		else if (previous_ori_value == QET::East)  x_add = Terminal::terminalSize;
		else if (previous_ori_value == QET::South) y_add = Terminal::terminalSize;
		else if (previous_ori_value == QET::West)  x_add = -Terminal::terminalSize;
		qde.setAttribute("x",           previous_x_value + x_add);
		qde.setAttribute("y",           previous_y_value + y_add);
		qde.setAttribute("orientation", QET::orientationToString(static_cast<QET::Orientation>(previous_ori_value)));
		
		if (Terminal *new_terminal = CustomElement::parseTerminal(qde)) {
			table_id_adr.insert(qde.attribute("id").toInt(), new_terminal);
		}
		
		// restaure les attributs modifies
		qde.setAttribute("x",           previous_x_value);
		qde.setAttribute("y",           previous_y_value);
		qde.setAttribute("orientation", previous_ori_value);
	}
	return(true);
}

/**
	Genere les rendus de l'element fantome : il s'agit d'un rectangle
	representant grosso modo l'espace que devait prendre l'element initial.
	En son centre est dessine un point d'interrogation. Une petite croix indique
	le point de saisie de l'element.
*/
void GhostElement::generateDrawings() {
	// style de dessin
	QPen t(QBrush(Qt::black), 1.0);
	
	// rendu normal
	QPainter qp;
	qp.begin(&drawing);
	qp.setPen(t);
	qp.setRenderHint(QPainter::Antialiasing, false);
	generateDrawing(&qp);
	qp.end();
	
	// rendu low_zoom
	QPainter low_zoom_qp;
	low_zoom_qp.begin(&low_zoom_drawing);
	t.setCosmetic(true);
	low_zoom_qp.setRenderHint(QPainter::Antialiasing, false);
	low_zoom_qp.setPen(t);
	generateDrawing(&low_zoom_qp);
	low_zoom_qp.end();
}

/**
	Genere un rendu de l'element fantome
	@see generateDrawings
*/
void GhostElement::generateDrawing(QPainter *painter) {
	// une petite croix indique le point de saisie de l'element
	painter -> drawLine(QLineF(-1.0, 0.0, 1.0, 0.0));
	painter -> drawLine(QLineF(0.0, -1.0, 0.0, 1.0));
	
	// rectangle avec un point d'interrogation au centre
	QRectF drawn_rect = boundingRect().adjusted(4.0, 4.0, -4.0, -4.0);
	painter -> drawRect(drawn_rect);
	painter -> drawText(drawn_rect, Qt::AlignHCenter | Qt::AlignVCenter, "?");
}
