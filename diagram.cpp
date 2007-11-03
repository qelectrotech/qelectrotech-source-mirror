#include <math.h>
#include "qetapp.h"
#include "conductor.h"
#include "customelement.h"
#include "diagram.h"
#include "exportdialog.h"
#include "diagramcommands.h"

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
	qgi_manager(this),
	draw_terminals(true)
{
	setBackgroundBrush(Qt::white);
	conductor_setter = new QGraphicsLineItem(0, 0);
	conductor_setter -> setZValue(1000000);
	QPen t;
	t.setColor(Qt::black);
	t.setWidthF(1.5);
	t.setStyle(Qt::DashLine);
	conductor_setter -> setPen(t);
	conductor_setter -> setLine(QLineF(QPointF(0.0, 0.0), QPointF(0.0, 0.0)));
	connect(this, SIGNAL(selectionChanged()), this, SLOT(slot_checkSelectionEmptinessChange()));
}

/**
	Destructeur
*/
Diagram::~Diagram() {
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
	QGraphicsScene::keyPressEvent(e);
}

/**
	Gere les relachements de touches du clavier
	@param e QKeyEvent decrivant l'evenement clavier
*/
void Diagram::keyReleaseEvent(QKeyEvent *e) {
	// detecte le relachement d'une touche de direction ( = deplacement d'elements)
	if (
		(e -> key() == Qt::Key_Left || e -> key() == Qt::Key_Right  ||\
		 e -> key() == Qt::Key_Up    || e -> key() == Qt::Key_Down) &&\
		!current_movement.isNull()  && !e -> isAutoRepeat()
	) {
		// cree un objet d'annulation pour le mouvement qui vient de se finir
		undoStack().push(
			new MoveElementsCommand(
				this,
				elementsToMove(),
				conductorsToMove(),
				conductorsToUpdate(),
				textsToMove(),
				current_movement
			)
		);
		invalidateMovedElements();
		current_movement = QPointF();
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
		racine.setAttribute("cols",    border_and_inset.nbColumn());
		racine.setAttribute("colsize", border_and_inset.columnsWidth());
		racine.setAttribute("height",  border_and_inset.columnsHeight());
		racine.setAttribute("version", QET::version);
		
		// type de conducteur par defaut
		QDomElement default_conductor = document.createElement("defaultconductor");
		defaultConductorProperties.toXml(document, default_conductor);
		racine.appendChild(default_conductor);
	}
	document.appendChild(racine);
	
	// si le schema ne contient pas d'element (et donc pas de conducteurs), on retourne de suite le document XML
	if (items().isEmpty()) return(document);
	
	// creation de trois listes : une qui contient les elements, une qui contient les conducteurs, une qui contient les champs de texte
	QList<Element *> list_elements;
	QList<Conductor *> list_conductors;
	QList<DiagramTextItem *> list_texts;
	
	// Determine les elements a « XMLiser »
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
	Importe le diagram decrit dans un document XML. Si une position est
	precisee, les elements importes sont positionnes de maniere a ce que le
	coin superieur gauche du plus petit rectangle pouvant les entourant tous
	(le bounding rect) soit a cette position.
	@param document Le document XML a analyser
	@param position La position du diagram importe
	@param consider_informations Si vrai, les informations complementaires
	(auteur, titre, ...) seront prises en compte
	@param added_elements si ce pointeur vers une liste d'elements n'est pas
	NULL, il sera rempli avec les elements ajoutes au schema par le fromXml
	@param added_conductors si ce pointeur vers une liste de conducteurs n'est
	pas NULL, il sera rempli avec les conducteurs ajoutes au schema par le
	fromXml
	@return true si l'import a reussi, false sinon
*/
bool Diagram::fromXml(QDomDocument &document, QPointF position, bool consider_informations, QList<Element *> *added_elements_ptr, QList<Conductor *> *added_conductors_ptr, QList<DiagramTextItem *> *added_texts_ptr) {
	QDomElement root = document.documentElement();
	// le premier element doit etre un schema
	if (root.tagName() != "diagram") return(false);
	
	// verifie basiquement que la version actuelle est capable de lire ce fichier
	if (root.hasAttribute("version")) {
		bool conv_ok;
		qreal diagram_version = root.attribute("version").toDouble(&conv_ok);
		if (conv_ok && QET::version.toDouble() < diagram_version) {
			QMessageBox::warning(
				0,
				tr("Avertissement"),
				tr("Ce document semble avoir \351t\351 enregistr\351 avec une "
				"version ult\351rieure de QElectroTech. Il est possible que "
				"l'ouverture de tout ou partie de ce document \351choue.")
			);
		}
	}
	
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
		
		// hauteur du schema
		double height = root.attribute("height").toDouble(&ok);
		if (ok) border_and_inset.setColumnsHeight(height);
		
		// repere le permier element "defaultconductor"
		for (QDomNode node = root.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
			QDomElement elmts = node.toElement();
			if(elmts.isNull() || elmts.tagName() != "defaultconductor") continue;
			defaultConductorProperties.fromXml(elmts);
			break;
		}
		
	}
	
	// si la racine n'a pas d'enfant : le chargement est fini (schema vide)
	if (root.firstChild().isNull()) return(true);
	
	// chargement de tous les elements du fichier XML
	QList<Element *> added_elements;
	QHash<int, Terminal *> table_adr_id;
	foreach (QDomElement e, QET::findInDomElement(root, "elements", "element")) {
		if (!Element::valideXml(e)) continue;
		
		// cree un element dont le type correspond à l'id type
		QString type_id = e.attribute("type");
		QString chemin_fichier = QETApp::realPath(type_id);
		CustomElement *nvel_elmt = new CustomElement(chemin_fichier);
		if (nvel_elmt -> isNull()) {
			QString debug_message = QString("Le chargement de la description de l'element %1 a echoue avec le code d'erreur %2").arg(chemin_fichier).arg(nvel_elmt -> etat());
			delete nvel_elmt;
			qDebug(debug_message.toLatin1().data());
			continue;
		}
		
		// charge les caracteristiques de l'element
		if (nvel_elmt -> fromXml(e, table_adr_id)) {
			// ajout de l'element au schema et a la liste des elements ajoutes
			addItem(nvel_elmt);
			added_elements << nvel_elmt;
		} else {
			delete nvel_elmt;
			qDebug("Le chargement des parametres d'un element a echoue");
		}
	}
	
	// chargement de tous les textes du fichiers XML
	QList<DiagramTextItem *> added_texts;
	foreach (QDomElement f, QET::findInDomElement(root, "inputs", "input")) {
		DiagramTextItem *dti = new DiagramTextItem(0, this);
		dti -> fromXml(f);
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
				bool cia = ((Element *)p2 -> parentItem()) -> connexionsInternesAcceptees();
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
		} else qDebug() << "Le chargement du conductor" << id_p1 << id_p2 << "a echoue";
	}
	
	// remplissage des listes facultatives
	if (added_elements_ptr   != NULL) *added_elements_ptr   = added_elements;
	if (added_conductors_ptr != NULL) *added_conductors_ptr = added_conductors;
	if (added_texts_ptr      != NULL) *added_texts_ptr      = added_texts;
	
	return(true);
}

/**
	Verifie si la selection est passe d'un etat ou elle est vide a un etat ou
	elle ne l'est pas, et inversement. Si c'est le cas, le signal
	EmptinessChanged() est emis.
*/
void Diagram::slot_checkSelectionEmptinessChange() {
	static bool selection_was_empty = true;
	bool selection_is_empty = selectedItems().isEmpty();
	if (selection_was_empty != selection_is_empty) {
		emit(selectionEmptinessChanged());
		selection_was_empty = selection_is_empty;
	}
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

/// oublie la liste des elements et conducteurs en mouvement
void Diagram::invalidateMovedElements() {
	if (!moved_elements_fetched) return;
	moved_elements_fetched = false;
	elements_to_move.clear();
	conductors_to_move.clear();
	conductors_to_update.clear();
	texts_to_move.clear();
}

/// reconstruit la liste des elements et conducteurs en mouvement
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
