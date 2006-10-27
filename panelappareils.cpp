#include "panelappareils.h"
#include "contacteur.h"
#include "del.h"
#include "entree.h"
#include "elementperso.h"

/**
	Constructeur
	@param parent Le QWidget parent du panel d'appareils
	@todo : definir une classe heritant de QListWidgetItem et automatiser tout ca
*/
PanelAppareils::PanelAppareils(QWidget *parent) :  QListWidget(parent) {
	
	// selection unique
	setSelectionMode(QAbstractItemView::SingleSelection);
	
	// drag'n drop autorise
	setDragEnabled(true);
	setAcceptDrops(false);
	setDropIndicatorShown(false);
	
	// style, mouvement et taille des elements
	setIconSize(QSize(50, 50));
	setMovement(QListView::Free);
	setViewMode(QListView::ListMode);
	
	// donnees
	/*Element *del = new DEL(0,0);
	Element *contacteur = new Contacteur(0,0);
	Element *entree = new Entree(0, 0);*/
	
	QListWidgetItem *qlwi;
	QString whats_this = tr("Ceci est un \351l\351ment que vous pouvez ins\351rer dans votre sch\351ma par cliquer-d\351placer");
	QString tool_tip = tr("Cliquer-d\351posez cet \351l\351ment sur le sch\351ma pour ins\351rer un \351l\351ment ");
	
	// remplissage de la liste
	QDir dossier_elements("elements/");
	QStringList filtres;
	filtres << "*.elmt";
	QStringList fichiers = dossier_elements.entryList(filtres, QDir::Files, QDir::Name);
	foreach(QString fichier, fichiers) {
		int etat;
		ElementPerso *elmt_perso = new ElementPerso(fichier, 0, 0, &etat);
		if (etat != 0) {
			qDebug() << "Le chargement du composant" << fichier << "a echoue avec le code d'erreur" << etat;
			continue;
		}
		qlwi = new QListWidgetItem(QIcon(elmt_perso -> pixmap()), elmt_perso -> nom(), this);
		qlwi -> setStatusTip(tool_tip + "\253 " + elmt_perso -> nom() + " \273");
		qlwi -> setToolTip(elmt_perso -> nom());
		qlwi -> setWhatsThis(whats_this);
		qlwi -> setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
		qlwi -> setData(42, fichier);
	}
	
	// force du noir sur une alternance de blanc (comme le schema) et de bleu clair
	QPalette qp = palette();
	setAlternatingRowColors(true);
	qp.setColor(QPalette::Text, Qt::black);
	qp.setColor(QPalette::Base, Qt::white);
	//qp.setColor(QPalette::AlternateBase, QColor(240, 255, 255));
	setPalette(qp);
}

/**
	Gere le mouvement lors d'un drag'n drop
*/
void PanelAppareils::dragMoveEvent(QDragMoveEvent */*e*/) {
}

/**
	Gere le depot lors d'un drag'n drop
*/
void PanelAppareils::dropEvent(QDropEvent */*e*/) {
}

/**
	Gere le debut des drag'n drop
	@param supportedActions Les actions supportees
	@todo virer les lignes type «if ("tel appareil") construire TelAppareil» => trouver un moyen d'automatiser ca
 */
void PanelAppareils::startDrag(Qt::DropActions /*supportedActions*/) {
	// objet QDrag pour realiser le drag'n drop
	QDrag *drag = new QDrag(this);
	
	// donnees qui seront transmises par le drag'n drop
	QMimeData *mimeData = new QMimeData();
	
	// appareil temporaire pour fournir un apercu
	Element *appar;
	int etat;
	QString nom_fichier = currentItem() -> data(42).toString();
	appar = new ElementPerso(nom_fichier, 0, 0, &etat);
	if (etat != 0) {
		delete appar;
		return;
	}
	
	mimeData -> setText(nom_fichier);
	drag -> setMimeData(mimeData);
	
	// accrochage d'une pixmap representant l'appareil au pointeur
	drag -> setPixmap(appar -> pixmap());
	drag -> setHotSpot(appar -> hotspot());
	
	// realisation du drag'n drop
	drag -> start(Qt::CopyAction);
	
	// suppression de l'appareil temporaire
	delete appar;
}
