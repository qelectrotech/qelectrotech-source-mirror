#include <math.h>
#include "conducteur.h"
#include "contacteur.h"
#include "elementperso.h"
#include "schema.h"

/**
	Constructeur
	@param parent Le QObject parent du schema
*/
Schema::Schema(QObject *parent) : QGraphicsScene(parent) {
	setBackgroundBrush(Qt::white);
	poseur_de_conducteur = new QGraphicsLineItem(0, 0);
	poseur_de_conducteur -> setZValue(1000000);
	QPen t;
	t.setColor(Qt::black);
	t.setWidthF(1.5);
	t.setStyle(Qt::DashLine);
	poseur_de_conducteur -> setPen(t);
	poseur_de_conducteur -> setLine(QLineF(QPointF(0.0, 0.0), QPointF(0.0, 0.0)));
	doit_dessiner_grille = true;
	connect(this, SIGNAL(changed(const QList<QRectF> &)), this, SLOT(slot_checkSelectionChange()));
}

/**
	Dessine l'arriere-plan du schema, cad la grille.
	@param p Le QPainter a utiliser pour dessiner
	@param r Le rectangle de la zone a dessiner
*/
void Schema::drawBackground(QPainter *p, const QRectF &r) {
	p -> save();
	
	// desactive tout antialiasing
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, false);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	// dessine un fond blanc
	p -> setPen(Qt::NoPen);
	p -> setBrush(Qt::white);
	p -> drawRect(r);
	
	if (doit_dessiner_grille) {
		// dessine les points de la grille
		p -> setPen(Qt::black);
		p -> setBrush(Qt::NoBrush);
		qreal limite_x = r.x() + r.width();
		qreal limite_y = r.y() + r.height();
		
		int g_x = (int)ceil(r.x());
		while (g_x % GRILLE_X) ++ g_x;
		int g_y = (int)ceil(r.y());
		while (g_y % GRILLE_Y) ++ g_y;
		
		for (int gx = g_x ; gx < limite_x ; gx += GRILLE_X) {
			for (int gy = g_y ; gy < limite_y ; gy += GRILLE_Y) {
				p -> drawPoint(gx, gy);
			}
		}
		
		p -> drawLine(0, 0, 0, 10);
		p -> drawLine(0, 0, 10, 0);
	}
	p -> restore();
}

QImage Schema::toImage() {
	
	QRectF vue = itemsBoundingRect();
	// la marge  = 5 % de la longueur necessaire
	qreal marge = 0.05 * vue.width();
	vue.translate(-marge, -marge);
	vue.setWidth(vue.width() + 2.0 * marge);
	vue.setHeight(vue.height() + 2.0 * marge);
	QSize dimensions_image = vue.size().toSize();
	
	QImage pix = QImage(dimensions_image, QImage::Format_RGB32);
	QPainter p;
	bool painter_ok = p.begin(&pix);
	if (!painter_ok) return(QImage());
	
	// rendu antialiase
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::TextAntialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);
	
	render(&p, pix.rect(), vue, Qt::KeepAspectRatio);
	p.end();
	return(pix);
}

/**
	Exporte tout ou partie du schema 
	@param schema Booleen (a vrai par defaut) indiquant si le XML genere doit representer tout le schema ou seulement les elements selectionnes
	@return Un Document XML (QDomDocument)
*/
QDomDocument Schema::toXml(bool schema) {
	// document
	QDomDocument document;
	
	// racine de l'arbre XML
	QDomElement racine = document.createElement("schema");
	
	// proprietes du schema
	if (schema) {
		if (!auteur.isNull()) racine.setAttribute("auteur", auteur);
		if (!date.isNull())   racine.setAttribute("date", date.toString("yyyyMMdd"));
		if (!titre.isNull())  racine.setAttribute("titre", titre);
	}
	document.appendChild(racine);
	
	// si le schema ne contient pas d'element (et donc pas de conducteurs), on retourne de suite le document XML
	if (items().isEmpty()) return(document);
	
	// creation de deux listes : une qui contient les elements, une qui contient les conducteurs
	QList<Element *> liste_elements;
	QList<Conducteur *> liste_conducteurs;
	
	
	// Determine les elements a « XMLiser »
	foreach(QGraphicsItem *qgi, items()) {
		if (Element *elmt = qgraphicsitem_cast<Element *>(qgi)) {
			if (schema) liste_elements << elmt;
			else if (elmt -> isSelected()) liste_elements << elmt;
		} else if (Conducteur *f = qgraphicsitem_cast<Conducteur *>(qgi)) {
			if (schema) liste_conducteurs << f;
			// lorsqu'on n'exporte pas tout le schema, il faut retirer les conducteurs non selectionnes
			// et pour l'instant, les conducteurs non selectionnes sont les conducteurs dont un des elements n'est pas relie
			else if (f -> borne1 -> parentItem() -> isSelected() && f -> borne2 -> parentItem() -> isSelected()) liste_conducteurs << f;
		}
	}
	
	// enregistrement des elements
	if (liste_elements.isEmpty()) return(document);
	int id_borne = 0;
	// table de correspondance entre les adresses des bornes et leurs ids
	QHash<Borne *, int> table_adr_id;
	QDomElement elements = document.createElement("elements");
	foreach(Element *elmt, liste_elements) {
		QDomElement element = document.createElement("element");
		
		// type, position, selection et orientation
		element.setAttribute("type", QFileInfo(elmt -> typeId()).fileName());
		element.setAttribute("x", elmt -> pos().x());
		element.setAttribute("y", elmt -> pos().y());
		if (elmt -> isSelected()) element.setAttribute("selected", "selected");
		element.setAttribute("sens", elmt -> orientation() ? "true" : "false");
		
		// enregistrements des bornes de chaque appareil
		QDomElement bornes = document.createElement("bornes");
		// pour chaque enfant de l'element
		foreach(QGraphicsItem *child, elmt -> children()) {
			// si cet enfant est une borne
			if (Borne *p = qgraphicsitem_cast<Borne *>(child)) {
				// alors on enregistre la borne
				QDomElement borne = p -> toXml(document);
				borne.setAttribute("id", id_borne);
				table_adr_id.insert(p, id_borne ++);
				bornes.appendChild(borne);
			}
		}
		element.appendChild(bornes);
		
		/**
			@todo appeler une methode virtuelle de la classe Element qui permettra
			aux developpeurs d'elements de personnaliser l'enregistrement des elements
		*/
		elements.appendChild(element);
	}
	racine.appendChild(elements);
	
	// enregistrement des conducteurs
	if (liste_conducteurs.isEmpty()) return(document);
	QDomElement conducteurs = document.createElement("conducteurs");
	foreach(Conducteur *f, liste_conducteurs) {
		QDomElement conducteur = document.createElement("conducteur");
		conducteur.setAttribute("borne1", table_adr_id.value(f -> borne1));
		conducteur.setAttribute("borne2", table_adr_id.value(f -> borne2));
		conducteurs.appendChild(conducteur);
	}
	racine.appendChild(conducteurs);
	
	// on retourne le document XML ainsi genere
	return(document);
}

void Schema::reset() {
	/// @todo implementer cette fonction
}

/**
	Importe le schema decrit dans un document XML. Si une position est precisee, les elements importes sont positionnes de maniere a ce que le coin superieur gauche du plus petit rectangle pouvant les entourant tous (le bounding rect) soit a cette position.
	@param document Le document XML a analyser
	@param position La position du schema importe
	@return true si l'import a reussi, false sinon
*/
bool Schema::fromXml(QDomDocument &document, QPointF position) {
	QDomElement racine = document.documentElement();
	// le premier element doit etre un schema
	if (racine.tagName() != "schema") return(false);
	// lecture des attributs de ce schema
	auteur = racine.attribute("auteur");
	titre  = racine.attribute("titre");
	date   = QDate::fromString(racine.attribute("date"), "yyyyMMdd");
	
	// si la racine n'a pas d'enfant : le chargement est fini (schema vide)
	if (racine.firstChild().isNull()) return(true);
	
	// chargement de tous les Elements du fichier XML
	QList<Element *> elements_ajoutes;
	//uint nb_elements = 0;
	QHash< int, Borne *> table_adr_id;
	QHash< int, Borne *> &ref_table_adr_id = table_adr_id;
	for (QDomNode node = racine.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		// on s'interesse a l'element XML "elements" (= groupe d'elements)
		QDomElement elmts = node.toElement();
		if(elmts.isNull() || elmts.tagName() != "elements") continue;
		// parcours des enfants de l'element XML "elements"
		for (QDomNode n = elmts.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
			// on s'interesse a l'element XML "element" (elements eux-memes)
			QDomElement e = n.toElement();
			if (e.isNull() || !Element::valideXml(e)) continue;
			Element *element_ajoute;
			if ((element_ajoute = elementFromXml(e, ref_table_adr_id)) != NULL) elements_ajoutes << element_ajoute;
			else qDebug("Le chargement d'un element a echoue");
		}
	}
	
	// aucun Element n'a ete ajoute - inutile de chercher des conducteurs - le chargement est fini
	if (!elements_ajoutes.size()) return(true);
	
	// gere la translation des nouveaux elements si celle-ci est demandee
	if (position != QPointF()) {
		// determine quel est le coin superieur gauche du rectangle entourant les elements ajoutes
		qreal minimum_x = 0, minimum_y = 0;
		bool init = false;
		foreach (Element *elmt_ajoute, elements_ajoutes) {
			QPointF csg = elmt_ajoute -> mapToScene(elmt_ajoute -> boundingRect().topLeft());
			qreal px = csg.x();
			qreal py = csg.y();
			if (!init) {
				minimum_x = px;
				minimum_y = py;
				init = true;
			} else {
				if (px < minimum_x) minimum_x = px;
				if (py < minimum_y) minimum_y = py;
			}
		}
		qreal diff_x = position.x() - minimum_x;
		qreal diff_y = position.y() - minimum_y;
		foreach (Element *elmt_ajoute, elements_ajoutes) {
			elmt_ajoute -> setPos(elmt_ajoute -> pos().x() + diff_x, elmt_ajoute -> pos().y() + diff_y);
		}
	}
	
	// chargement de tous les Conducteurs du fichier XML
	for (QDomNode node = racine.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		// on s'interesse a l'element XML "conducteurs" (= groupe de conducteurs)
		QDomElement conducteurs = node.toElement();
		if(conducteurs.isNull() || conducteurs.tagName() != "conducteurs") continue;
		// parcours des enfants de l'element XML "conducteurs"
		for (QDomNode n = conducteurs.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
			// on s'interesse a l'element XML "element" (elements eux-memes)
			QDomElement f = n.toElement();
			if (f.isNull() || !Conducteur::valideXml(f)) continue;
			// verifie que les bornes que le conducteur relie sont connues
			int id_p1 = f.attribute("borne1").toInt();
			int id_p2 = f.attribute("borne2").toInt();
			if (table_adr_id.contains(id_p1) && table_adr_id.contains(id_p2)) {
				// pose le conducteur... si c'est possible
				Borne *p1 = table_adr_id.value(id_p1);
				Borne *p2 = table_adr_id.value(id_p2);
				if (p1 != p2) {
					bool peut_poser_conducteur = true;
					bool cia = ((Element *)p2 -> parentItem()) -> connexionsInternesAcceptees();
					if (!cia) foreach(QGraphicsItem *item, p2 -> parentItem() -> children()) if (item == p1) peut_poser_conducteur = false;
					if (peut_poser_conducteur) new Conducteur(table_adr_id.value(id_p1), table_adr_id.value(id_p2), 0, this);
				}
			} else qDebug() << "Le chargement du conducteur" << id_p1 << id_p2 << "a echoue";
		}
	}
	return(true);
}

/**
	Ajoute au schema l'Element correspondant au QDomElement passe en parametre
	@param e QDomElement a analyser
	@param table_id_adr Table de correspondance entre les entiers et les bornes
	@return true si l'ajout a parfaitement reussi, false sinon 
*/
Element *Schema::elementFromXml(QDomElement &e, QHash<int, Borne *> &table_id_adr) {
	// cree un element dont le type correspond à l'id type
	QString type = e.attribute("type");
	int etat;
	Element *nvel_elmt = new ElementPerso(type, 0, 0, &etat);
	/*switch(e.attribute("type").toInt()) {
		case 0: nvel_elmt = new Contacteur(); break;
		case 1: nvel_elmt = new DEL();        break;
		case 2: nvel_elmt = new Entree();     break;
	}*/
	if (etat != 0) return(false);
	bool retour = nvel_elmt -> fromXml(e, table_id_adr);
	if (!retour) {
		delete nvel_elmt;
	} else {
		// ajout de l'element au schema
		addItem(nvel_elmt);
		nvel_elmt -> setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
		nvel_elmt -> setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
		if (e.attribute("sens") == "false") nvel_elmt -> invertOrientation();
		nvel_elmt -> setSelected(e.attribute("selected") == "selected");
	}
	return(retour ? nvel_elmt : NULL);
}

void Schema::slot_checkSelectionChange() {
	static QList<QGraphicsItem *> cache_selecteditems = QList<QGraphicsItem *>();
	QList<QGraphicsItem *> selecteditems = selectedItems();
	if (cache_selecteditems != selecteditems) emit(selectionChanged());
	cache_selecteditems = selecteditems;
}
