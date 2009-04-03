#include "ghostelement.h"
#include "qet.h"
#include "terminal.h"
#include "elementtextitem.h"

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
bool GhostElement::fromXml(QDomElement &e, QHash<int, Terminal *> &table_id_adr) {
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
	generateDrawing();
	
	// position, selection et orientation
	setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	bool conv_ok;
	int read_ori = e.attribute("orientation").toInt(&conv_ok);
	if (!conv_ok || read_ori < 0 || read_ori > 3) read_ori = ori.defaultOrientation();
	setOrientation((QET::Orientation)read_ori);
	
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
	Genere le rendu de l'element fantome : il s'agit d'un rectangle
	representant grosso modo l'espace que devait prendre l'element initial.
	En son centre est dessine un point d'interrogation. Une petite croix indique
	le point de saisie de l'element.
*/
void GhostElement::generateDrawing() {
	QPainter qp;
	qp.begin(&drawing);
	
	// style de dessin
	QPen t;
	t.setColor(Qt::black);
	t.setWidthF(1.0);
	t.setJoinStyle(Qt::BevelJoin);
	qp.setPen(t);
	
	// une petite croix indique le point de saisie de l'element
	qp.drawLine(QLineF(-1.0, 0.0, 1.0, 0.0));
	qp.drawLine(QLineF(0.0, -1.0, 0.0, 1.0));
	
	// rectangle avec un point d'interrogation au centre
	QRectF drawn_rect = boundingRect().adjusted(4.0, 4.0, -4.0, -4.0);
	qp.drawRect(drawn_rect);
	qp.drawText(drawn_rect, Qt::AlignHCenter | Qt::AlignVCenter, "?");
	
	qp.end();
}
