#include "panelappareils.h"
#include "elementperso.h"

/**
	Constructeur
	@param parent Le QWidget parent du panel d'appareils
*/
PanelAppareils::PanelAppareils(QWidget *parent) :  QTreeWidget(parent) {
	
	// selection unique
	setSelectionMode(QAbstractItemView::SingleSelection);
	setColumnCount(1);
	setHeaderLabel("Elements");
	
	// drag'n drop autorise
	setDragEnabled(true);
	setAcceptDrops(false);
	setDropIndicatorShown(false);
	
	// taille des elements
	setIconSize(QSize(50, 50));
	
	QTreeWidgetItem *elmts_qet = new QTreeWidgetItem(this, QStringList(tr("Collection QET")));
	QTreeWidgetItem *elmts_perso = new QTreeWidgetItem(this, QStringList(tr("Collection utilisateur")));
	
	// remplissage de la liste
	QDir dossier_elmts_qet(QETApp::commonElementsDir());
	QDir dossier_elmts_perso(QETApp::customElementsDir());
	QStringList filtres("*.elmt");
	QStringList fichiers1 = dossier_elmts_qet.entryList(filtres, QDir::Files, QDir::Name);
	foreach(QString fichier, fichiers1) ajouterFichier(elmts_qet, QETApp::commonElementsDir()+fichier);
	QStringList fichiers2 = dossier_elmts_perso.entryList(filtres, QDir::Files, QDir::Name);
	foreach(QString fichier, fichiers2) ajouterFichier(elmts_perso, QETApp::customElementsDir()+fichier);
	
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
	// recupere le nom du fichier decrivant l'element
	QString nom_fichier = currentItem() -> data(0, 42).toString();
	if (nom_fichier == QString()) return;
	
	// objet QDrag pour realiser le drag'n drop
	QDrag *drag = new QDrag(this);
	
	// donnees qui seront transmises par le drag'n drop
	QMimeData *mimeData = new QMimeData();
	
	// appareil temporaire pour fournir un apercu
	int etat;
	Element *appar = new ElementPerso(nom_fichier, 0, 0, &etat);
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

/**
	Methode privee permettant d'ajouter un element au panel d'appareils
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere l'element
	@param fichier Chemin absolu du fichier XML decrivant l'element a inserer
*/
void PanelAppareils::ajouterFichier(QTreeWidgetItem *qtwi_parent, QString fichier) {
	QString whats_this = tr("Ceci est un \351l\351ment que vous pouvez ins\351rer dans votre sch\351ma par cliquer-d\351placer");
	QString tool_tip = tr("Cliquer-d\351posez cet \351l\351ment sur le sch\351ma pour ins\351rer un \351l\351ment ");
	int etat;
	ElementPerso *elmt_perso = new ElementPerso(fichier, 0, 0, &etat);
	if (etat != 0) {
		qDebug() << "Le chargement du composant" << fichier << "a echoue avec le code d'erreur" << etat;
		return;
	}
	QTreeWidgetItem *qtwi = new QTreeWidgetItem(qtwi_parent, QStringList(elmt_perso -> nom()));
	qtwi -> setStatusTip(0, tool_tip + "\253 " + elmt_perso -> nom() + " \273");
	qtwi -> setToolTip(0, elmt_perso -> nom());
	qtwi -> setWhatsThis(0, whats_this);
	qtwi -> setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
	qtwi -> setIcon(0, QIcon(elmt_perso -> pixmap()));
	qtwi -> setData(0, 42, fichier);
}
