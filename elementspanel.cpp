#include "elementspanel.h"
#include "elementscategory.h"
#include "elementscategoryeditor.h"
#include "customelement.h"
#include "customelementeditor.h"

/**
	Constructeur
	@param parent Le QWidget parent du panel d'appareils
*/
ElementsPanel::ElementsPanel(QWidget *parent) :  QTreeWidget(parent) {
	
	// selection unique
	setSelectionMode(QAbstractItemView::SingleSelection);
	setColumnCount(1);
	header() -> hide();
	
	// drag'n drop autorise
	setDragEnabled(true);
	setAcceptDrops(false);
	setDropIndicatorShown(false);
	
	// taille des elements
	setIconSize(QSize(50, 50));
	
	// charge les collections
	reload();
	
	// force du noir sur une alternance de blanc (comme le schema) et de gris
	// clair, avec du blanc sur bleu pas trop fonce pour la selection
	QPalette qp = palette();
	qp.setColor(QPalette::Text,            Qt::black);
	qp.setColor(QPalette::Base,            Qt::white);
	qp.setColor(QPalette::AlternateBase,   QColor("#e8e8e8"));
	qp.setColor(QPalette::Highlight,       QColor("#678db2"));
	qp.setColor(QPalette::HighlightedText, Qt::white);
	setPalette(qp);
	
	// double-cliquer sur un element permet de l'editer
	connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(slot_doubleClick(QTreeWidgetItem *, int)));
}

/**
	Destructeur
*/
ElementsPanel::~ElementsPanel() {
}

/**
	Gere le mouvement lors d'un drag'n drop
*/
void ElementsPanel::dragMoveEvent(QDragMoveEvent */*e*/) {
}

/**
	Gere le depot lors d'un drag'n drop
*/
void ElementsPanel::dropEvent(QDropEvent */*e*/) {
}

/**
	Gere le debut des drag'n drop
	@param supportedActions Les actions supportees
	@todo virer les lignes type «if ("tel appareil") construire TelAppareil» => trouver un moyen d'automatiser ca
 */
void ElementsPanel::startDrag(Qt::DropActions /*supportedActions*/) {
	// recupere le nom du fichier decrivant l'element
	QString nom_fichier = currentItem() -> data(0, 42).toString();
	if (nom_fichier == QString()) return;
	
	// objet QDrag pour realiser le drag'n drop
	QDrag *drag = new QDrag(this);
	
	// donnees qui seront transmises par le drag'n drop
	QMimeData *mimeData = new QMimeData();
	
	// appareil temporaire pour fournir un apercu
	int etat;
	Element *appar = new CustomElement(nom_fichier, 0, 0, &etat);
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
	Methode privee permettant d'ajouter un dossier au panel d'appareils
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere l'element
	@param dossier Chemin absolu du dossier a inserer
	@param nom Parametre facultatif permettant de forcer le nom du dossier.
	S'il n'est pas precise, la fonction ouvre le fichier qet_directory situe
	dans le dossier et y lit le nom du dossier ; si ce fichier n'existe pas ou
	est invalide, la fonction utilise le nom du dossier.
*/
void ElementsPanel::addDir(QTreeWidgetItem *qtwi_parent, QString adr_dossier, QString nom) {
	ElementsCategory category(adr_dossier);
	if (!category.exists()) return;
	
	// recupere le nom de la categorie
	QString nom_categorie = (nom != QString()) ? nom : category.name();
	
	// creation du QTreeWidgetItem representant le dossier
	QTreeWidgetItem *qtwi_dossier = new QTreeWidgetItem(qtwi_parent, QStringList(nom_categorie));
	QLinearGradient t(0, 0, 200, 0);
	t.setColorAt(0, QColor("#e8e8e8"));
	t.setColorAt(1, QColor("#ffffff"));
	qtwi_dossier -> setBackground(0, QBrush(t));
	qtwi_dossier -> setExpanded(true);
	qtwi_dossier -> setData(0, 42, adr_dossier);
	
	// ajout des sous-categories / sous-dossiers
	QStringList dossiers = category.entryList(QStringList(), QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name);
	foreach(QString dossier, dossiers) addDir(qtwi_dossier, adr_dossier + dossier + "/");
	
	// ajout des elements / fichiers
	QStringList fichiers = category.entryList(QStringList("*.elmt"), QDir::Files, QDir::Name);
	foreach(QString fichier, fichiers) addFile(qtwi_dossier, adr_dossier + fichier);
}

/**
	Methode privee permettant d'ajouter un element au panel d'appareils
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere l'element
	@param fichier Chemin absolu du fichier XML decrivant l'element a inserer
*/
void ElementsPanel::addFile(QTreeWidgetItem *qtwi_parent, QString fichier) {
	QString whats_this = tr("Ceci est un \351l\351ment que vous pouvez ins\351rer dans votre sch\351ma par cliquer-d\351placer");
	QString tool_tip = tr("Cliquer-d\351posez cet \351l\351ment sur le sch\351ma pour ins\351rer un \351l\351ment ");
	int etat;
	CustomElement *elmt_perso = new CustomElement(fichier, 0, 0, &etat);
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

/**
	Recharge l'arbre des elements
*/
void ElementsPanel::reload() {
	// vide l'arbre
	while (takeTopLevelItem(0));
	
	// chargement des elements de la collection QET
	addDir(invisibleRootItem(), QETApp::commonElementsDir(), tr("Collection QET"));
	
	// chargement des elements de la collection utilisateur
	addDir(invisibleRootItem(), QETApp::customElementsDir(), tr("Collection utilisateur"));
}

void ElementsPanel::slot_doubleClick(QTreeWidgetItem *qtwi, int) {
	// recupere le fichier ou le dossier correspondant au QTreeWidgetItem
	QString filename = qtwi -> data(0, 42).toString();
	
	// le fichier doit exister
	QFileInfo infos_file(filename);
	if (!infos_file.exists()) return;
	
	
	if (infos_file.isFile()) {
		// il s'agit d'un element
		CustomElementEditor *cee = new CustomElementEditor();
		cee -> fromFile(filename);
		cee -> show();
	} else if (infos_file.isDir()) {
		// il s'agit d'une categorie
		ElementsCategory c(filename);
		ElementsCategoryEditor ece(filename, true);
		if (ece.exec() == QDialog::Accepted) reload();
	}
}
