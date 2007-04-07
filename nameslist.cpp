#include "nameslist.h"

/**
	Constructeur
	@param parent QWidget parent de la liste de noms
*/
NamesList::NamesList(QWidget *parent) : QWidget(parent) {
	QVBoxLayout *names_list_layout = new QVBoxLayout();
	setLayout(names_list_layout);
	
	tree_names = new QTreeWidget();
	tree_names -> setColumnCount(2);
	QStringList headers;
	headers << tr("Langue") << tr("Nom");
	tree_names -> setHeaderLabels(headers);
	
	button_add_line = new QPushButton(tr("Ajouter une ligne"));
	connect(button_add_line, SIGNAL(released()), this, SLOT(addLine()));
	
	names_list_layout -> addWidget(tree_names);
	names_list_layout -> addWidget(button_add_line);
}

/**
	Destructeur
*/
NamesList::~NamesList() {
	
}

/**
	Ajoute une ligne a l'editeur
*/
void NamesList::addLine() {
	clean();
	QTreeWidgetItem *qtwi = new QTreeWidgetItem();
	qtwi -> setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	tree_names -> addTopLevelItem(qtwi);
}

/**
	Verifie qu'il y a au moins un nom
*/
bool NamesList::checkOneName() {
	updateHash();
	if (!hash_names.size()) {
		QMessageBox::critical(
			this,
			tr("La cat\351gorie doit avoir au moins un nom."),
			tr("Vous devez entrer au moins un nom pour la cat\351gorie.")
		);
		return(false);
	}
	return(true);
}

/**
	Lit les noms valides dans hash_names
*/
void NamesList::updateHash() {
	hash_names.clear();
	int names_count = tree_names -> topLevelItemCount();
	for (int i = 0 ; i < names_count ; ++ i) {
		QString lang  = tree_names -> topLevelItem(i) -> text(0);
		QString value = tree_names -> topLevelItem(i) -> text(1);
		if (lang != "" && value != "") hash_names.insert(lang, value);
	}
}

/**
	Nettoie la liste des noms en enlevant les lignes vides
*/
void NamesList::clean() {
	int names_count = tree_names -> topLevelItemCount() - 1;
	for (int i = names_count ; i >= 0 ; -- i) {
		if (
			tree_names -> topLevelItem(i) -> text(0) == QString() &&\
			tree_names -> topLevelItem(i) -> text(1) == QString()
		) {
			tree_names -> takeTopLevelItem(i);
		}
	}
}

/**
	@return Les noms entres dans la Names List
*/
QHash<QString, QString> NamesList::names() {
	updateHash();
	return(hash_names);
}

/**
	Specifie les noms que la liste doit afficher
	@param provided_names Hash des noms a afficher
*/
void NamesList::setNames(QHash<QString, QString> &provided_names) {
	foreach(QString lang, provided_names.keys()) {
		QString value = provided_names[lang];
		if (lang == "" || value == "") continue;
		QStringList values;
		values << lang << value;
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(values);
		qtwi -> setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		tree_names -> addTopLevelItem(qtwi);
	}
}
