/*
	Copyright 2006-2009 Xavier Guerrin
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
#include <math.h>
#include "qetapp.h"
#include "conductor.h"
#include "customelement.h"
#include "diagram.h"
#include "elementtextitem.h"
#include "exportdialog.h"
#include "ghostelement.h"
#include "diagramcommands.h"
#include "diagramcontent.h"

const int   Diagram::xGrid  = 10;
const int   Diagram::yGrid  = 10;
const qreal Diagram::margin = 5.0;

/**
	Constructeur
	@param parent Le QObject parent du schema
*/
Diagram::Diagram(QObject *parent) :
	QGraphicsScene(parent),
	draw_grid(true),
	use_border(true),
	moved_elements_fetched(false),
	draw_terminals(true),
	project_(0),
	read_only_(false)
{
	undo_stack = new QUndoStack();
	qgi_manager = new QGIManager(this);
	setBackgroundBrush(Qt::white);
	conductor_setter = new QGraphicsLineItem(0, 0);
	conductor_setter -> setZValue(1000000);
	QPen t;
	t.setColor(Qt::black);
	t.setWidthF(1.5);
	t.setStyle(Qt::DashLine);
	conductor_setter -> setPen(t);
	conductor_setter -> setLine(QLineF(QPointF(0.0, 0.0), QPointF(0.0, 0.0)));
}

/**
	Destructeur
*/
Diagram::~Diagram() {
	// suppression de la liste des annulations - l'undo stack fait appel au qgimanager pour supprimer certains elements
	delete undo_stack;
	// suppression du QGIManager - tous les elements qu'il connait sont supprimes
	delete qgi_manager;
	
	// recense les items supprimables
	QList<QGraphicsItem *> deletable_items;
	foreach(QGraphicsItem *qgi, items()) {
		if (qgi -> parentItem()) continue;
		if (qgraphicsitem_cast<Conductor *>(qgi)) continue;
		deletable_items << qgi;
	}

	// suppression des items supprimables
	foreach(QGraphicsItem *qgi_d, deletable_items) {
		delete qgi_d;
	}
}

/**
	Dessine l'arriere-plan du schema, cad la grille.
	@param p Le QPainter a utiliser pour dessiner
	@param r Le rectangle de la zone a dessiner
*/
void Diagram::drawBackground(QPainter *p, const QRectF &r) {
	p -> save();
	
	// desactive tout antialiasing, sauf pour le texte
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	// dessine un fond blanc
	p -> setPen(Qt::NoPen);
	p -> setBrush(Qt::white);
	p -> drawRect(r);
	
	if (draw_grid) {
		// dessine les points de la grille
		p -> setPen(Qt::black);
		p -> setBrush(Qt::NoBrush);
		qreal limite_x = r.x() + r.width();
		qreal limite_y = r.y() + r.height();
		
		int g_x = (int)ceil(r.x());
		while (g_x % xGrid) ++ g_x;
		int g_y = (int)ceil(r.y());
		while (g_y % yGrid) ++ g_y;
		
		for (int gx = g_x ; gx < limite_x ; gx += xGrid) {
			for (int gy = g_y ; gy < limite_y ; gy += yGrid) {
				p -> drawPoint(gx, gy);
			}
		}
	}
	
	if (use_border) border_and_inset.draw(p, margin, margin);
	p -> restore();
}

/**
	Gere les enfoncements de touches du clavier
	@param e QKeyEvent decrivant l'evenement clavier
*/
void Diagram::keyPressEvent(QKeyEvent *e) {
	if (!isReadOnly()) {
		QPointF movement;
		switch(e -> key()) {
			case Qt::Key_Left:  movement = QPointF(-xGrid, 0.0); break;
			case Qt::Key_Right: movement = QPointF(+xGrid, 0.0); break;
			case Qt::Key_Up:    movement = QPointF(0.0, -yGrid); break;
			case Qt::Key_Down:  movement = QPointF(0.0, +yGrid); break;
		}
		if (!movement.isNull() && !focusItem()) {
			moveElements(movement);
		}
	}
	QGraphicsScene::keyPressEvent(e);
}

/**
	Gere les relachements de touches du clavier
	@param e QKeyEvent decrivant l'evenement clavier
*/
void Diagram::keyReleaseEvent(QKeyEvent *e) {
	if (!isReadOnly()) {
		// detecte le relachement d'une touche de direction ( = deplacement d'elements)
		if (
			(e -> key() == Qt::Key_Left || e -> key() == Qt::Key_Right  ||\
			 e -> key() == Qt::Key_Up    || e -> key() == Qt::Key_Down) &&\
			!current_movement.isNull()  && !e -> isAutoRepeat()
		) {
			// cree un objet d'annulation pour le mouvement qui vient de se finir
			undoStack().push(new MoveElementsCommand(this, selectedContent(), current_movement));
			invalidateMovedElements();
			current_movement = QPointF();
		}
	}
	QGraphicsScene::keyReleaseEvent(e);
}

/**
	Exporte le schema vers une image
	@return Une QImage representant le schema
*/
bool Diagram::toPaintDevice(QPaintDevice &pix, int width, int height, Qt::AspectRatioMode aspectRatioMode) {
	// determine la zone source =  contenu du schema + marges
	QRectF source_area;
	if (!use_border) {
		source_area = itemsBoundingRect();
		source_area.translate(-margin, -margin);
		source_area.setWidth (source_area.width () + 2.0 * margin);
		source_area.setHeight(source_area.height() + 2.0 * margin);
	} else {
		source_area = QRectF(
			0.0,
			0.0,
			border_and_inset.borderWidth () + 2.0 * margin,
			border_and_inset.borderHeight() + 2.0 * margin
		);
	}
	
	// si les dimensions ne sont pas precisees, l'image est exportee a l'echelle 1:1
	QSize image_size = (width == -1 && height == -1) ? source_area.size().toSize() : QSize(width, height);
	
	// prepare le rendu
	QPainter p;
	if (!p.begin(&pix)) return(false);
	
	// rendu antialiase
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::TextAntialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);
	
	// deselectionne tous les elements
	QList<QGraphicsItem *> selected_elmts = selectedItems();
	foreach (QGraphicsItem *qgi, selected_elmts) qgi -> setSelected(false);
	
	// effectue le rendu lui-meme
	render(&p, QRect(QPoint(0, 0), image_size), source_area, aspectRatioMode);
	p.end();
	
	// restaure les elements selectionnes
	foreach (QGraphicsItem *qgi, selected_elmts) qgi -> setSelected(true);
	
	return(true);
}

/**
	Permet de connaitre les dimensions qu'aura l'image generee par la methode toImage()
	@return La taille de l'image generee par toImage()
*/
QSize Diagram::imageSize() const {
	// determine la zone source =  contenu du schema + marges
	qreal image_width, image_height;
	if (!use_border) {
		QRectF items_rect = itemsBoundingRect();
		image_width  = items_rect.width();
		image_height = items_rect.height();
	} else {
		image_width  = border_and_inset.borderWidth();
		image_height = border_and_inset.borderHeight();
	}
	
	image_width  += 2.0 * margin;
	image_height += 2.0 * margin;
	
	// renvoie la taille de la zone source
	return(QSizeF(image_width, image_height).toSize());
}

/**
	@return true si le schema est considere comme vide, false sinon.
	Un schema vide ne contient ni element, ni conducteur, ni champ de texte
*/
bool Diagram::isEmpty() const {
	return(!items().count());
}

/**
	Exporte tout ou partie du schema 
	@param diagram Booleen (a vrai par defaut) indiquant si le XML genere doit
	representer tout le schema ou seulement les elements selectionnes
	@return Un Document XML (QDomDocument)
*/
QDomDocument Diagram::toXml(bool diagram) {
	// document
	QDomDocument document;
	
	// racine de l'arbre XML
	QDomElement racine = document.createElement("diagram");
	
	// proprietes du schema
	if (diagram) {
		if (!border_and_inset.author().isNull())    racine.setAttribute("author",   border_and_inset.author());
		if (!border_and_inset.date().isNull())      racine.setAttribute("date",     border_and_inset.date().toString("yyyyMMdd"));
		if (!border_and_inset.title().isNull())     racine.setAttribute("title",    border_and_inset.title());
		if (!border_and_inset.fileName().isNull())  racine.setAttribute("filename", border_and_inset.fileName());
		if (!border_and_inset.folio().isNull())     racine.setAttribute("folio",    border_and_inset.folio());
		racine.setAttribute("cols",    border_and_inset.nbColumns());
		racine.setAttribute("colsize", QString("%1").arg(border_and_inset.columnsWidth()));
		racine.setAttribute("rows",    border_and_inset.nbRows());
		racine.setAttribute("rowsize", QString("%1").arg(border_and_inset.rowsHeight()));
		// attribut datant de la version 0.1 - laisse pour retrocompatibilite
		racine.setAttribute("height",  QString("%1").arg(border_and_inset.diagramHeight()));
		racine.setAttribute("displaycols", border_and_inset.columnsAreDisplayed() ? "true" : "false");
		racine.setAttribute("displayrows", border_and_inset.rowsAreDisplayed()    ? "true" : "false");
		
		// type de conducteur par defaut
		QDomElement default_conductor = document.createElement("defaultconductor");
		defaultConductorProperties.toXml(default_conductor);
		racine.appendChild(default_conductor);
	}
	document.appendChild(racine);
	
	// si le schema ne contient pas d'element (et donc pas de conducteurs), on retourne de suite le document XML
	if (items().isEmpty()) return(document);
	
	// creation de trois listes : une qui contient les elements, une qui contient les conducteurs, une qui contient les champs de texte
	QList<Element *> list_elements;
	QList<Conductor *> list_conductors;
	QList<DiagramTextItem *> list_texts;
	
	// Determine les elements a "XMLiser"
	foreach(QGraphicsItem *qgi, items()) {
		if (Element *elmt = qgraphicsitem_cast<Element *>(qgi)) {
			if (diagram) list_elements << elmt;
			else if (elmt -> isSelected()) list_elements << elmt;
		} else if (Conductor *f = qgraphicsitem_cast<Conductor *>(qgi)) {
			if (diagram) list_conductors << f;
			// lorsqu'on n'exporte pas tout le diagram, il faut retirer les conducteurs non selectionnes
			// et pour l'instant, les conducteurs non selectionnes sont les conducteurs dont un des elements n'est pas relie
			else if (f -> terminal1 -> parentItem() -> isSelected() && f -> terminal2 -> parentItem() -> isSelected()) list_conductors << f;
		} else if (DiagramTextItem *dti = qgraphicsitem_cast<DiagramTextItem *>(qgi)) {
			if (!dti -> parentItem()) {
				if (diagram) list_texts << dti;
				else if (dti -> isSelected()) list_texts << dti;
			}
		}
	}
	
	// table de correspondance entre les adresses des bornes et leurs ids
	QHash<Terminal *, int> table_adr_id;
	
	// enregistrement des elements
	if (!list_elements.isEmpty()) {
		QDomElement elements = document.createElement("elements");
		foreach(Element *elmt, list_elements) {
			elements.appendChild(elmt -> toXml(document, table_adr_id));
		}
		racine.appendChild(elements);
	}
	
	// enregistrement des conducteurs
	if (!list_conductors.isEmpty()) {
		QDomElement conductors = document.createElement("conductors");
		foreach(Conductor *cond, list_conductors) {
			conductors.appendChild(cond -> toXml(document, table_adr_id));
		}
		racine.appendChild(conductors);
	}
	
	// enregistrement des champs de texte
	if (!list_texts.isEmpty()) {
		QDomElement inputs = document.createElement("inputs");
		foreach(DiagramTextItem *dti, list_texts) {
			inputs.appendChild(dti -> toXml(document));
		}
		racine.appendChild(inputs);
	}
	
	// on retourne le document XML ainsi genere
	return(document);
}

/**
	Importe le schema decrit dans un document XML. Si une position est
	precisee, les elements importes sont positionnes de maniere a ce que le
	coin superieur gauche du plus petit rectangle pouvant les entourant tous
	(le bounding rect) soit a cette position.
	@param document Le document XML a analyser
	@param position La position du schema importe
	@param consider_informations Si vrai, les informations complementaires
	(auteur, titre, ...) seront prises en compte
	@param content_ptr si ce pointeur vers un DiagramContent n'est pas NULL, il
	sera rempli avec le contenu ajoute au schema par le fromXml
	@return true si l'import a reussi, false sinon
*/
bool Diagram::fromXml(QDomDocument &document, QPointF position, bool consider_informations, DiagramContent *content_ptr) {
	QDomElement root = document.documentElement();
	return(fromXml(root, position, consider_informations, content_ptr));
}

/**
	Importe le schema decrit dans un element XML. Si une position est
	precisee, les elements importes sont positionnes de maniere a ce que le
	coin superieur gauche du plus petit rectangle pouvant les entourant tous
	(le bounding rect) soit a cette position.
	@param document Le document XML a analyser
	@param position La position du schema importe
	@param consider_informations Si vrai, les informations complementaires
	(auteur, titre, ...) seront prises en compte
	@param content_ptr si ce pointeur vers un DiagramContent n'est pas NULL, il
	sera rempli avec le contenu ajoute au schema par le fromXml
	@return true si l'import a reussi, false sinon
*/
bool Diagram::fromXml(QDomElement &document, QPointF position, bool consider_informations, DiagramContent *content_ptr) {
	QDomElement root = document;
	// le premier element doit etre un schema
	if (root.tagName() != "diagram") return(false);
	
	// lecture des attributs de ce schema
	if (consider_informations) {
		border_and_inset.setAuthor(root.attribute("author"));
		border_and_inset.setTitle(root.attribute("title"));
		border_and_inset.setDate(QDate::fromString(root.attribute("date"), "yyyyMMdd"));
		border_and_inset.setFileName(root.attribute("filename"));
		border_and_inset.setFolio(root.attribute("folio"));
		
		bool ok;
		// nombre de colonnes
		int nb_cols = root.attribute("cols").toInt(&ok);
		if (ok) border_and_inset.setNbColumns(nb_cols);
		
		// taille des colonnes
		double col_size = root.attribute("colsize").toDouble(&ok);
		if (ok) border_and_inset.setColumnsWidth(col_size);
		
		// retrocompatibilite : les schemas enregistres avec la 0.1 ont un attribut "height"
		if (root.hasAttribute("rows") && root.hasAttribute("rowsize")) {
			// nombre de lignes
			int nb_rows = root.attribute("rows").toInt(&ok);
			if (ok) border_and_inset.setNbRows(nb_rows);
			
			// taille des lignes
			double row_size = root.attribute("rowsize").toDouble(&ok);
			if (ok) border_and_inset.setRowsHeight(row_size);
		} else {
			// hauteur du schema
			double height = root.attribute("height").toDouble(&ok);
			if (ok) border_and_inset.setDiagramHeight(height);
		}
		
		// affichage des lignes et colonnes
		border_and_inset.displayColumns(root.attribute("displaycols") != "false");
		border_and_inset.displayRows(root.attribute("displayrows") != "false");
		
		border_and_inset.adjustInsetToColumns();
		
		// repere le permier element "defaultconductor"
		for (QDomNode node = root.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
			QDomElement elmts = node.toElement();
			if(elmts.isNull() || elmts.tagName() != "defaultconductor") continue;
			defaultConductorProperties.fromXml(elmts);
			break;
		}
		
	}
	
	// si la racine n'a pas d'enfant : le chargement est fini (schema vide)
	if (root.firstChild().isNull()) {
		write(document);
		return(true);
	}
	
	// chargement de tous les elements du fichier XML
	QList<Element *> added_elements;
	QHash<int, Terminal *> table_adr_id;
	foreach (QDomElement e, QET::findInDomElement(root, "elements", "element")) {
		if (!Element::valideXml(e)) continue;
		
		// cree un element dont le type correspond a l'id type
		QString type_id = e.attribute("type");
		ElementsLocation element_location = ElementsLocation(type_id);
		if (type_id.startsWith("embed://")) element_location.setProject(project_);
		
		CustomElement *nvel_elmt = new CustomElement(element_location);
		if (nvel_elmt -> isNull()) {
			QString debug_message = QString("Diagram::fromXml() : Le chargement de la description de l'element %1 a echoue avec le code d'erreur %2").arg(element_location.path()).arg(nvel_elmt -> state());
			qDebug() << qPrintable(debug_message);
			delete nvel_elmt;
			
			qDebug() << "Diagram::fromXml() : Utilisation d'un GhostElement en lieu et place de cet element.";
			nvel_elmt = new GhostElement(element_location);
		}
		
		// charge les caracteristiques de l'element
		if (nvel_elmt -> fromXml(e, table_adr_id)) {
			// ajout de l'element au schema et a la liste des elements ajoutes
			addElement(nvel_elmt);
			added_elements << nvel_elmt;
		} else {
			delete nvel_elmt;
			qDebug() << "Diagram::fromXml() : Le chargement des parametres d'un element a echoue";
		}
	}
	
	// chargement de tous les textes du fichiers XML
	QList<DiagramTextItem *> added_texts;
	foreach (QDomElement f, QET::findInDomElement(root, "inputs", "input")) {
		DiagramTextItem *dti = new DiagramTextItem(0, this);
		dti -> fromXml(f);
		addDiagramTextItem(dti);
		added_texts << dti;
	}
	
	// gere la translation des nouveaux elements et texte si celle-ci est demandee
	if (position != QPointF()) {
		// determine quel est le coin superieur gauche du rectangle entourant les elements ajoutes
		qreal minimum_x = 0, minimum_y = 0;
		bool init = false;
		QList<QGraphicsItem *> added_items;
		foreach (Element *added_element, added_elements) added_items << added_element;
		foreach (DiagramTextItem *added_text, added_texts) added_items << added_text;
		foreach (QGraphicsItem *item, added_items) {
			QPointF csg = item -> mapToScene(item -> boundingRect()).boundingRect().topLeft();
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
		foreach (Element *added_element, added_elements) {
			added_element -> setPos(added_element -> pos().x() + diff_x, added_element -> pos().y() + diff_y);
		}
		foreach (DiagramTextItem *added_text, added_texts) {
			added_text -> setPos(added_text -> pos().x() + diff_x, added_text -> pos().y() + diff_y);
		}
	}
	
	// chargement de tous les Conducteurs du fichier XML
	QList<Conductor *> added_conductors;
	foreach (QDomElement f, QET::findInDomElement(root, "conductors", "conductor")) {
		if (!Conductor::valideXml(f)) continue;
		// verifie que les bornes que le conducteur relie sont connues
		int id_p1 = f.attribute("terminal1").toInt();
		int id_p2 = f.attribute("terminal2").toInt();
		if (table_adr_id.contains(id_p1) && table_adr_id.contains(id_p2)) {
			// pose le conducteur... si c'est possible
			Terminal *p1 = table_adr_id.value(id_p1);
			Terminal *p2 = table_adr_id.value(id_p2);
			if (p1 != p2) {
				bool can_add_conductor = true;
				bool cia = ((Element *)p2 -> parentItem()) -> internalConnections();
				if (!cia) {
					foreach(QGraphicsItem *item, p2 -> parentItem() -> children()) {
						if (item == p1) can_add_conductor = false;
					}
				}
				if (can_add_conductor) {
					Conductor *c = new Conductor(table_adr_id.value(id_p1), table_adr_id.value(id_p2), 0, this);
					c -> fromXml(f);
					added_conductors << c;
				}
			}
		} else qDebug() << "Diagram::fromXml() : Le chargement du conducteur" << id_p1 << id_p2 << "a echoue";
	}
	
	// remplissage des listes facultatives
	if (content_ptr != NULL) {
		content_ptr -> elements         = added_elements;
		content_ptr -> conductorsToMove = added_conductors;
		content_ptr -> textFields       = added_texts;
	}
	
	write();
	return(true);
}

/**
	Enregistre le schema XML dans son document XML interne et emet le signal
	written().
*/
void Diagram::write() {
	qDebug() << qPrintable(QString("Diagram::write() : saving changes from diagram \"%1\" [%2]").arg(title()).arg(QET::pointerString(this)));
	write(toXml().documentElement());
	undoStack().setClean();
}

/**
	Enregistre un element XML dans son document XML interne et emet le signal
	written().
	@param element xml a enregistrer
*/
void Diagram::write(const QDomElement &element) {
	xml_document.clear();
	xml_document.appendChild(xml_document.importNode(element, true));
	emit(written());
}

/**
	@return true si la fonction write a deja ete appele (pour etre plus exact :
	si le document XML utilise en interne n'est pas vide), false sinon
*/
bool Diagram::wasWritten() const {
	return(!xml_document.isNull());
}

/**
	@return le schema en XML tel qu'il doit etre enregistre dans le fichier projet
	@param xml_doc document XML a utiliser pour creer l'element
*/
QDomElement Diagram::writeXml(QDomDocument &xml_doc) const {
	// si le schema n'a pas ete enregistre explicitement, on n'ecrit rien
	if (!wasWritten()) return(QDomElement());
	
	QDomElement diagram_elmt = xml_document.documentElement();
	QDomNode new_node = xml_doc.importNode(diagram_elmt, true);
	return(new_node.toElement());
}

/**
	Ajoute un element sur le schema
	@param element Element a ajouter
*/
void Diagram::addElement(Element *element) {
	if (!element || isReadOnly()) return;
	
	// ajoute l'element au schema
	if (element -> scene() != this) {
		addItem(element);
	}
	
	// surveille les modifications de ses champs de texte
	foreach(ElementTextItem *eti, element -> texts()) {
		connect(
			eti,
			SIGNAL(diagramTextChanged(DiagramTextItem *, const QString &, const QString &)),
			this,
			SLOT(diagramTextChanged(DiagramTextItem *, const QString &, const QString &))
		);
	}
}

/**
	Ajoute un conducteur sur le schema
	@param conductor Conducteur a ajouter
*/
void Diagram::addConductor(Conductor *conductor) {
	if (!conductor || isReadOnly()) return;
	
	// ajoute le conducteur au schema
	if (conductor -> scene() != this) {
		addItem(conductor);
		conductor -> terminal1 -> addConductor(conductor);
		conductor -> terminal2 -> addConductor(conductor);
	}
}

/**
	Aoute un champ de texte independant sur le schema
	@param dti Champ de texte a ajouter
*/
void Diagram::addDiagramTextItem(DiagramTextItem *dti) {
	if (!dti || isReadOnly()) return;
	
	// ajoute le champ de texte au schema
	if (dti -> scene() != this) {
		addItem(dti);
	}
	
	// surveille les modifications apportees au champ de texte
	connect(
		dti,
		SIGNAL(diagramTextChanged(DiagramTextItem *, const QString &, const QString &)),
		this,
		SLOT(diagramTextChanged(DiagramTextItem *, const QString &, const QString &))
	);
}

/**
	Enleve un element du schema
	@param element Element a enlever
*/
void Diagram::removeElement(Element *element) {
	if (!element || isReadOnly()) return;
	
	// enleve l'element au schema
	removeItem(element);
	
	// arrete la surveillance des modifications de ses champs de texte
	foreach(ElementTextItem *eti, element -> texts()) {
		disconnect(
			eti,
			SIGNAL(diagramTextChanged(DiagramTextItem *, const QString &, const QString &)),
			this,
			SLOT(diagramTextChanged(DiagramTextItem *, const QString &, const QString &))
		);
	}
}

/**
	Enleve un conducteur du schema
	@param conductor Conducteur a enlever
*/
void Diagram::removeConductor(Conductor *conductor) {
	if (!conductor || isReadOnly()) return;
	
	// detache le conducteur sans le detruire
	conductor -> terminal1 -> removeConductor(conductor);
	conductor -> terminal2 -> removeConductor(conductor);
	
	// enleve le conducteur du schema
	removeItem(conductor);
}

/**
	Enleve un champ de texte independant du schema
	@param dti Champ de texte a enlever
*/
void Diagram::removeDiagramTextItem(DiagramTextItem *dti) {
	if (!dti || isReadOnly()) return;
	
	// enleve le champ de texte au schema
	removeItem(dti);
	
	// arrete la surveillance des modifications apportees au champ de texte
	disconnect(
		dti,
		SIGNAL(diagramTextChanged(DiagramTextItem *, const QString &, const QString &)),
		this,
		SLOT(diagramTextChanged(DiagramTextItem *, const QString &, const QString &))
	);
}

/**
	Gere le fait qu'un texte du schema ait ete modifie
	@param text_item Texte modifie
	@param old_text Ancien texte
	@param new_text Nouveau texte
*/
void Diagram::diagramTextChanged(DiagramTextItem *text_item, const QString &old_text, const QString &new_text) {
	if (!text_item) return;
	undo_stack -> push(new ChangeDiagramTextCommand(text_item, old_text, new_text));
}

/**
	Selectionne tous les objets du schema
*/
void Diagram::selectAll() {
	if (items().isEmpty()) return;
	
	blockSignals(true);
	foreach(QGraphicsItem *qgi, items()) qgi -> setSelected(true);
	blockSignals(false);
	emit(selectionChanged());
}

/**
	Deslectionne tous les objets selectionnes
*/
void Diagram::deselectAll() {
	if (items().isEmpty()) return;
	
	clearSelection();
}

/**
	Inverse l'etat de selection de tous les objets du schema
*/
void Diagram::invertSelection() {
	if (items().isEmpty()) return;
	
	blockSignals(true);
	foreach (QGraphicsItem *item, items()) item -> setSelected(!item -> isSelected());
	blockSignals(false);
	emit(selectionChanged());
}

/**
	@return Le rectangle (coordonnees par rapport a la scene) delimitant le bord du schema
*/
QRectF Diagram::border() const {
	return(
		QRectF(
			margin,
			margin,
			border_and_inset.borderWidth(),
			border_and_inset.borderHeight()
		)
	);
}

/**
	@return le titre du cartouche
*/
QString Diagram::title() const {
	return(border_and_inset.title());
}

/**
	@return la liste des elements de ce schema
*/
QList<CustomElement *> Diagram::customElements() const {
	QList<CustomElement *> elements_list;
	foreach(QGraphicsItem *qgi, items()) {
		if (CustomElement *elmt = qgraphicsitem_cast<CustomElement *>(qgi)) {
			elements_list << elmt;
		}
	}
	return(elements_list);
}

/**
	Oublie la liste des elements et conducteurs en mouvement
*/
void Diagram::invalidateMovedElements() {
	if (!moved_elements_fetched) return;
	moved_elements_fetched = false;
	elements_to_move.clear();
	conductors_to_move.clear();
	conductors_to_update.clear();
	texts_to_move.clear();
}

/**
	Reconstruit la liste des elements et conducteurs en mouvement
*/
void Diagram::fetchMovedElements() {
	// recupere les elements deplaces
	foreach (QGraphicsItem *item, selectedItems()) {
		if (Element *elmt = qgraphicsitem_cast<Element *>(item)) {
			elements_to_move << elmt;
		} else if (DiagramTextItem *t = qgraphicsitem_cast<DiagramTextItem *>(item)) {
			if (!t -> parentItem()) texts_to_move << t;
		}
	}
	
	// pour chaque element deplace, determine les conducteurs qui seront modifies
	foreach(Element *elmt, elements_to_move) {
		foreach(Terminal *terminal, elmt -> terminals()) {
			foreach(Conductor *conductor, terminal -> conductors()) {
				Terminal *other_terminal;
				if (conductor -> terminal1 == terminal) {
					other_terminal = conductor -> terminal2;
				} else {
					other_terminal = conductor -> terminal1;
				}
				// si les deux elements du conducteur sont deplaces
				if (elements_to_move.contains(static_cast<Element *>(other_terminal -> parentItem()))) {
					conductors_to_move << conductor;
				} else {
					conductors_to_update.insert(conductor, terminal);
				}
			}
		}
	}
	moved_elements_fetched = true;
}

/**
	Deplace les elements, conducteurs et textes selectionnes en gerant au
	mieux les conducteurs (seuls les conducteurs dont un seul des elements
	est deplace sont recalcules, les autres sont deplaces).
	@param diff Translation a effectuer
	@param dontmove QGraphicsItem (optionnel) a ne pas deplacer ; note : ce
	parametre ne concerne que les elements et les champs de texte.
*/
void Diagram::moveElements(const QPointF &diff, QGraphicsItem *dontmove) {
	// inutile de deplacer les autres elements s'il n'y a pas eu de mouvement concret
	if (diff.isNull()) return;
	
	current_movement += diff;
	
	// deplace les elements selectionnes
	foreach(Element *element, elementsToMove()) {
		if (dontmove != NULL && element == dontmove) continue;
		element -> setPos(element -> pos() + diff);
	}
	
	// deplace certains conducteurs
	foreach(Conductor *conductor, conductorsToMove()) {
		conductor -> setPos(conductor -> pos() + diff);
	}
	
	// recalcule les autres conducteurs
	const QHash<Conductor *, Terminal *> &conductors_modify = conductorsToUpdate();
	foreach(Conductor *conductor, conductors_modify.keys()) {
		conductor -> updateWithNewPos(QRectF(), conductors_modify[conductor], conductors_modify[conductor] -> amarrageConductor());
	}
	
	// deplace les champs de texte
	foreach(DiagramTextItem *dti, textsToMove()) {
		if (dontmove != NULL && dti == dontmove) continue;
		dti -> setPos(dti -> pos() + diff);
	}
}

/**
	Permet de savoir si un element est utilise sur un schema
	@param location Emplacement d'un element
	@return true si l'element location est utilise sur ce schema, false sinon
*/
bool Diagram::usesElement(const ElementsLocation &location) {
	foreach(CustomElement *element, customElements()) {
		if (element -> location() == location) {
			return(true);
		}
	}
	return(false);
}

/**
	Definit s'il faut afficher ou non les bornes
	@param dt true pour afficher les bornes, false sinon
*/
void Diagram::setDrawTerminals(bool dt) {
	foreach(QGraphicsItem *qgi, items()) {
		if (Terminal *t = qgraphicsitem_cast<Terminal *>(qgi)) {
			t -> setVisible(dt);
		}
	}
}

/**
	@return la liste des conducteurs selectionnes sur le schema
*/
QSet<Conductor *> Diagram::selectedConductors() const {
	QSet<Conductor *> conductors_set;
	foreach(QGraphicsItem *qgi, selectedItems()) {
		if (Conductor *c = qgraphicsitem_cast<Conductor *>(qgi)) {
			conductors_set << c;
		}
	}
	return(conductors_set);
}

/// @return true si le presse-papier semble contenir un schema
bool Diagram::clipboardMayContainDiagram() {
	QString clipboard_text = QApplication::clipboard() -> text().trimmed();
	bool may_be_diagram = clipboard_text.startsWith("<diagram") && clipboard_text.endsWith("</diagram>");
	return(may_be_diagram);
}

/**
	@return le projet auquel ce schema appartient ou 0 s'il s'agit d'un schema
	independant.
*/
QETProject *Diagram::project() const {
	return(project_);
}

/**
	@param project le nouveau projet auquel ce schema appartient ou 0 s'il
	s'agit d'un schema independant. Indiquer 0 pour rendre ce schema independant.
*/
void Diagram::setProject(QETProject *project) {
	project_ = project;
}

/**
	@return true si le schema est en lecture seule
*/
bool Diagram::isReadOnly() const {
	return(read_only_);
}

/**
	@param read_only true pour passer le schema en lecture seule, false sinon
*/
void Diagram::setReadOnly(bool read_only) {
	if (read_only_ != read_only) {
		read_only_ = read_only;
		emit(readOnlyChanged(read_only_));
	}
}

/**
	@return Le contenu du schema. Les conducteurs sont tous places dans
	conductorsToMove.
*/
DiagramContent Diagram::content() const {
	DiagramContent dc;
	foreach(QGraphicsItem *qgi, items()) {
		if (Element *e = qgraphicsitem_cast<Element *>(qgi)) {
			dc.elements << e;
		} else if (DiagramTextItem *dti = qgraphicsitem_cast<DiagramTextItem *>(qgi)) {
			dc.textFields << dti;
		} else if (Conductor *c = qgraphicsitem_cast<Conductor *>(qgi)) {
			dc.conductorsToMove << c;
		}
	}
	return(dc);
}

/**
	@return le contenu selectionne du schema.
*/
DiagramContent Diagram::selectedContent() {
	invalidateMovedElements();
	DiagramContent dc;
	dc.elements           = elementsToMove().toList();
	dc.textFields         = textsToMove().toList();
	dc.conductorsToMove   = conductorsToMove().toList();
	dc.conductorsToUpdate = conductorsToUpdate();
	
	// recupere les conducteurs selectionnes isoles (= non deplacables mais supprimables)
	foreach(QGraphicsItem *qgi, items()) {
		if (Conductor *c = qgraphicsitem_cast<Conductor *>(qgi)) {
			if (
				c -> isSelected() &&\
				!c -> terminal1 -> parentItem() -> isSelected() &&\
				!c -> terminal2 -> parentItem() -> isSelected()
			) {
				dc.otherConductors << c;
			}
		}
	}
	invalidateMovedElements();
	return(dc);
}
