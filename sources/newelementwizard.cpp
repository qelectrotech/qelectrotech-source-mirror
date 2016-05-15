/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "newelementwizard.h"
#include "nameslistwidget.h"
#include "qetelementeditor.h"
#include "qfilenameedit.h"
#include "qetmessagebox.h"
#include "elementscollectionmodel.h"
#include "elementcollectionitem.h"

/**
	Constructeur
	@param parent QWidget parent de ce dialogue
	@param f flags pour le dialogue
*/
NewElementWizard::NewElementWizard(QWidget *parent, Qt::WindowFlags f) :
	QWizard(parent, f)
{
	setOptions(options() & ~QWizard::NoCancelButton);

#ifdef Q_OS_WIN
	setWizardStyle(QWizard::AeroStyle);
#elif defined(Q_OS_MAC)
	setWizardStyle(QWizard::MacStyle);
#endif

	setPixmap(LogoPixmap, QPixmap(":/ico/256x256/qelectrotech.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	setWindowTitle(tr("Créer un nouvel élément : Assistant", "window title"));
	setButtonText(QWizard::NextButton, tr("&Suivant >"));
	addPage(buildStep1());
	addPage(buildStep2());
	addPage(buildStep3());
	setFixedSize(705, 325);
}

/**
	Destructeur
*/
NewElementWizard::~NewElementWizard() {
}

/**
 * @brief NewElementWizard::preselectedLocation
 * Select item in the tree view represented by location,
 * @param location
 */
void NewElementWizard::preselectedLocation(const ElementsLocation &location)
{
	QModelIndex index = m_model->index(location);
	if (index.isValid()) {
		m_tree_view->scrollTo(index);
		m_tree_view->setCurrentIndex(index);
	}
}

/**
 * @brief NewElementWizard::buildStep1
 * @return
 */
QWizardPage *NewElementWizard::buildStep1()
{
	QWizardPage *page = new QWizardPage();
	page -> setProperty("WizardState", Category);
	page -> setTitle(tr("Étape 1/3 : Catégorie parente", "wizard page title"));
	page -> setSubTitle(tr("Sélectionnez une catégorie dans laquelle enregistrer le nouvel élément.", "wizard page subtitle"));
	QVBoxLayout *layout = new QVBoxLayout();

	m_tree_view = new QTreeView(this);

	m_model = new ElementsCollectionModel(m_tree_view);
	m_model->hideElement();
	m_model->addCustomCollection();

	m_tree_view->setModel(m_model);
	m_tree_view->setHeaderHidden(true);
	m_tree_view->setAnimated(true);
	layout->addWidget(m_tree_view);
	
	page->setLayout(layout);
	return(page);
}

/**
 * @brief NewElementWizard::buildStep2
 * @return
 */
QWizardPage *NewElementWizard::buildStep2() {
	QWizardPage *page = new QWizardPage();
	page -> setProperty("WizardState", Filename);
	page -> setTitle(tr("Étape 2/3 : Nom du fichier", "wizard page title"));
	page -> setSubTitle(tr("Indiquez le nom du fichier dans lequel enregistrer le nouvel élément.", "wizard page subtitle"));
	QVBoxLayout *layout = new QVBoxLayout();
	
	m_qle_filename = new QFileNameEdit(tr("nouvel_element"));
	m_qle_filename -> selectAll();
	QLabel *explication2 = new QLabel(tr("Vous n'êtes pas obligé de préciser l'extension *.elmt. Elle sera ajoutée automatiquement."));
	explication2 -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	explication2 -> setWordWrap(true);
	layout -> addWidget(m_qle_filename);
	layout -> addWidget(explication2);
	layout -> addSpacing(100);
	
	page -> setLayout(layout);
	return(page);
}

/**
 * @brief NewElementWizard::buildStep3
 * @return
 */
QWizardPage *NewElementWizard::buildStep3() {
	QWizardPage *page = new QWizardPage();
	page -> setProperty("WizardState", Names);
	page -> setTitle(tr("Étape 3/3 : Noms de l'élément", "wizard page title"));
	page -> setSubTitle(tr("Indiquez le ou les noms de l'élément.", "wizard page subtitle"));
	QVBoxLayout *layout = new QVBoxLayout();
	
	m_names_list = new NamesListWidget();
	NamesList hash_name;
	hash_name.addName(QLocale::system().name().left(2), tr("Nom du nouvel élément", "default name when creating a new element"));
	m_names_list -> setNames(hash_name);
	layout -> addWidget(m_names_list);
	
	page -> setLayout(layout);
	return(page);
}

/**
 * @brief NewElementWizard::validateCurrentPage
 * @return true if the current step is valid
 */
bool NewElementWizard::validateCurrentPage() {
	WizardState wizard_state = static_cast<WizardState>(currentPage() -> property("WizardState").toInt());
	if      (wizard_state == Category) return(validStep1());
	else if (wizard_state == Filename) return(validStep2());
	else if (wizard_state == Names) {
		// must have one name minimum
		if (m_names_list -> checkOneName())
			createNewElement();
		return true;
	}
	else return(true);
}

/**
 * @brief NewElementWizard::validStep1
 * Valid the setp 1
 * @return trie if the step is valid.
 */
bool NewElementWizard::validStep1()
{
		//They must be one directory selected
	bool step1_ok = false;

	QModelIndex index = m_tree_view->currentIndex();
	if (index.isValid()) {

		ElementCollectionItem *eci = static_cast<ElementCollectionItem*>(index.internalPointer());
		if (eci && eci->isDir()) {
			ElementsLocation loc(eci->collectionPath());
			if (loc.exist()) {
				m_chosen_location = loc;
				step1_ok = true;
			}
		}
	}

	if (!step1_ok) {
		QET::QetMessageBox::critical(parentWidget(),
									 tr("Erreur", "message box title"),
									 tr("Vous devez sélectionner une catégorie.", "message box content"));
	}

	return(step1_ok);
}

/**
 * @brief NewElementWizard::validStep2
 * Valid the step 2
 * @return true if step is valid
 */
bool NewElementWizard::validStep2() {
	QString file_name = m_qle_filename -> text();

	if (file_name.isEmpty()) {
		QET::QetMessageBox::critical(this,
									 tr("Erreur", "message box title"),
									 tr("Vous devez entrer un nom de fichier", "message box content"));
		return false;
	}

	if (!file_name.endsWith(".elmt")) {
		file_name += ".elmt";
	}

	ElementsLocation loc_ = m_chosen_location;
	loc_.addToPath(file_name);
	if (loc_.exist()) {
		QET::QetMessageBox::critical(this,
									 tr("Erreur", "message box title"),
									 tr("Un élément portant le même nom existe déjà"));
		return false;
	}

	m_chosen_file = file_name;
	return(true);
}

/**
 * @brief NewElementWizard::createNewElement
 * Lauch an element editor for create the new element
 */
void NewElementWizard::createNewElement() {
	QETElementEditor *edit_new_element = new QETElementEditor(parentWidget());
	edit_new_element -> setNames(m_names_list -> names());

	ElementsLocation loc_ = m_chosen_location;
	loc_.addToPath(m_chosen_file);
	edit_new_element -> setLocation(loc_);
	edit_new_element -> show();
}
