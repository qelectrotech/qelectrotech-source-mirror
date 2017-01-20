/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "nameslistwidget.h"
#include "qetmessagebox.h"
#include "qeticons.h"

/**
	Constructeur
	@param parent QWidget parent de la liste de noms
*/
NamesListWidget::NamesListWidget(QWidget *parent) : QWidget(parent), read_only(false) {
	QVBoxLayout *names_list_layout = new QVBoxLayout();
	setLayout(names_list_layout);
	
	tree_names = new QTreeWidget();
	tree_names -> setRootIsDecorated(false);
	tree_names -> setColumnCount(2);
	QStringList headers;
	headers << tr("Langue") << tr("Texte");
	tree_names -> setHeaderLabels(headers);
	tree_names -> setWhatsThis(
		tr(
			"Cette liste vous permet de saisir un court texte de façon à ce"
			" qu'il soit traduisible dans d'autres langues. Pour ce faire, elle"
			" associe des codes ISO 639-1 de langues (ex. : \"fr\" pour"
			" français) aux traductions du texte en question dans ces"
			" mêmes langues.",
			"\"What's this\" tip"
		)
	);
	
	button_add_line = new QPushButton(QET::Icons::Add, tr("Ajouter une ligne"));
	button_add_line -> setWhatsThis(
		tr(
			"Ce bouton permet d'ajouter une association langue / traduction "
			"dans la liste ci-dessus.",
			"\"What's this\" tip"
		)
	);
	connect(button_add_line, SIGNAL(released()), this, SLOT(addLine()));
	
	names_list_layout -> addWidget(tree_names);
	names_list_layout -> addWidget(button_add_line);
}

/**
	Destructeur
*/
NamesListWidget::~NamesListWidget() {
}

/**
	Ajoute une ligne a l'editeur
*/
void NamesListWidget::addLine() {
	clean();
	if (read_only) return;
	QTreeWidgetItem *qtwi = new QTreeWidgetItem();
	qtwi -> setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	tree_names -> addTopLevelItem(qtwi);
 	tree_names -> setCurrentItem(qtwi);
    	tree_names -> editItem(qtwi);
}

/**
	Verifie qu'il y a au moins un nom
*/
bool NamesListWidget::checkOneName() {
	updateHash();
	if (!hash_names.count()) {
		QET::QetMessageBox::critical(
			this,
			tr("Il doit y avoir au moins un nom.", "message box title"),
			tr("Vous devez entrer au moins un nom.", "message box content")
		);
		return(false);
	}
	return(true);
}

/**
	Lit les noms valides dans hash_names
*/
void NamesListWidget::updateHash() {
	hash_names.clearNames();
	int names_count = tree_names -> topLevelItemCount();
	for (int i = 0 ; i < names_count ; ++ i) {
		QString lang  = tree_names -> topLevelItem(i) -> text(0);
		QString value = tree_names -> topLevelItem(i) -> text(1);
		if (lang != "" && value != "") hash_names.addName(lang, value);
	}
}

/**
	Nettoie la liste des noms en enlevant les lignes vides
*/
void NamesListWidget::clean() {
	int names_count = tree_names -> topLevelItemCount() - 1;
	for (int i = names_count ; i >= 0 ; -- i) {
		if (
			tree_names -> topLevelItem(i) -> text(0).isEmpty() &&\
			tree_names -> topLevelItem(i) -> text(1).isEmpty()
		) {
			tree_names -> takeTopLevelItem(i);
		}
	}
}

/**
	@return Les noms entres dans la Names List
*/
NamesList NamesListWidget::names() {
	updateHash();
	return(hash_names);
}

/**
	Definit les noms que le widget doit afficher
*/
void NamesListWidget::setNames(const NamesList &provided_names) {
	foreach(QString lang, provided_names.langs()) {
		QString value = provided_names[lang];
		QStringList values;
		values << lang << value;
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(values);
		if (!read_only) qtwi -> setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		tree_names -> addTopLevelItem(qtwi);
		tree_names -> sortItems(0, Qt::AscendingOrder);
	}
}

/**
	Verifie qu'il y a au moins un nom de saisi - si c'est le cas, le signal
	imputChecked() est emis.
*/
void NamesListWidget::check() {
	if (checkOneName()) emit(inputChecked());
}

/**
	Definit le mode d'edition du widget
	@param ro true pour que la liste de noms soit en lecture seule, false sinon
*/
void NamesListWidget::setReadOnly(bool ro) {
	read_only = ro;
	int names_count = tree_names -> topLevelItemCount() - 1;
	for (int i = names_count ; i >= 0 ; -- i) {
		Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		if (!read_only) flags |= Qt::ItemIsEditable;
		tree_names -> topLevelItem(i) -> setFlags(flags);
	}
	button_add_line -> setEnabled(!read_only);
}

/// @return true si la liste de noms est en lecture seule, false sinon
bool NamesListWidget::isReadOnly() const {
	return(read_only);
}
