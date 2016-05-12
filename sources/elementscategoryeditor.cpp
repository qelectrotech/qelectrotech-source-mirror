/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "elementscategoryeditor.h"
#include "nameslistwidget.h"
#include "qet.h"
#include "qfilenameedit.h"
#include "qetmessagebox.h"
#include "elementcollectionhandler.h"

/**
 * @brief ElementsCategoryEditor::ElementsCategoryEditor
 * Constructor
 * @param location : location of the category to edit, or parent directory/category for the creation of a new category
 * @param edit : true = edit mode, false = creation mode
 * @param parent : parent widget
 */
ElementsCategoryEditor::ElementsCategoryEditor(const ElementsLocation &location, bool edit, QWidget *parent) :
	QDialog(parent),
	m_edit_mode(edit),
	m_location(location)
{
	setUpWidget();

	if (m_location.isElement()) {
		QET::QetMessageBox::warning(this,
									tr("L'item n'est pas une catégorie", "message box title"),
									tr("L'item demandé n'est pas une categrie. Abandon.", "message box content"));
		return;
	}

	if (!location.exist()) {
		QET::QetMessageBox::warning(this,
									tr("Catégorie inexistante", "message box title"),
									tr("La catégorie demandée n'existe pas. Abandon.", "message box content"));
		return;
	}
	
	if (m_edit_mode) {
		setWindowTitle(tr("Éditer une catégorie", "window title"));
		connect(m_buttons, SIGNAL(accepted()), this, SLOT(acceptUpdate()));
		
		m_names_list -> setNames(m_location.nameList());
		m_file_line_edit -> setText(m_location.fileSystemPath());
		m_file_line_edit -> setReadOnly(true);
	} else {
		setWindowTitle(tr("Créer une nouvelle catégorie", "window title"));
		connect(m_buttons, SIGNAL(accepted()), this, SLOT(acceptCreation()));
		
		NamesList cat_names;
		cat_names.addName(QLocale::system().name().left(2), tr("Nom de la nouvelle catégorie", "default name when creating a new category"));
		m_names_list -> setNames(cat_names);
	}

		//Location is ReadOnly
	if (!m_location.isWritable()) {
		QET::QetMessageBox::warning(
			this,
			tr("Édition en lecture seule", "message box title"),
			tr("Vous n'avez pas les privilèges nécessaires pour modifier cette catégorie. Elle sera donc ouverte en lecture seule.", "message box content")
		);
		m_names_list -> setReadOnly(true);
		m_file_line_edit -> setReadOnly(true);
	}
}

/**
 * @brief ElementsCategoryEditor::~ElementsCategoryEditor
 * Destructor
 */
ElementsCategoryEditor::~ElementsCategoryEditor() {
}

/**
 * @brief ElementsCategoryEditor::setUpWidget
 */
void ElementsCategoryEditor::setUpWidget()
{
	QVBoxLayout *editor_layout = new QVBoxLayout();
	setLayout(editor_layout);
	
	m_names_list = new NamesListWidget();
	m_file_name = new QLabel(tr("Nom interne : "));
	m_file_line_edit = new QFileNameEdit();
	
	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
	
	QHBoxLayout *internal_name_layout = new QHBoxLayout();
	internal_name_layout -> addWidget(m_file_name);
	internal_name_layout -> addWidget(m_file_line_edit);
	
	editor_layout -> addLayout(internal_name_layout);
	editor_layout -> addWidget(new QLabel(tr("Vous pouvez spécifier un nom par langue pour la catégorie.")));
	editor_layout -> addWidget(m_names_list);
	editor_layout -> addWidget(m_buttons);
}

/**
 * @brief ElementsCategoryEditor::acceptCreation
 * Valid the creation of the category
 */
void ElementsCategoryEditor::acceptCreation()
{
	if (!m_location.isWritable()) {
		QDialog::accept();
	}
	
		//there must be at least one name
	if (!m_names_list -> checkOneName()) {
		return;
	}
	
		//User must enter a directorie name
	if (!m_file_line_edit -> isValid()) {
		QET::QetMessageBox::critical(this,
									 tr("Nom interne manquant", "message box title"),
									 tr("Vous devez spécifier un nom interne.", "message box content"));
		return;
	}
	QString dirname = m_file_line_edit -> text();
	

		//Check if dirname already exist.
	ElementsLocation created_location = m_location;
	created_location.addToPath(dirname);

	if (created_location.exist()) {
		QET::QetMessageBox::critical(this,
									 tr("Nom interne déjà utilisé", "message box title"),
									 tr("Le nom interne que vous avez choisi est déjà utilisé "
										"par une catégorie existante. Veuillez en choisir un autre.",
										"message box content"));
		return;
	}

	ElementCollectionHandler ech_;
	NamesList nl = m_names_list->names();
	ElementsLocation loc = ech_.createDir(m_location, dirname, nl);
	if (loc.isNull()) {
		QET::QetMessageBox::critical(this,
									 tr("Erreur", "message box title"),
									 tr("Impossible de créer la catégorie", "message box content"));
		return;
	}
	
	QDialog::accept();
}

/**
 * @brief ElementsCategoryEditor::acceptUpdate
 * Valid the update of the category
 */
void ElementsCategoryEditor::acceptUpdate()
{
	if (!m_location.isWritable()) {
		QDialog::accept();
	}
	
		//There must be at least one name
	if (!m_names_list -> checkOneName()) {
		return;
	}
	
	ElementCollectionHandler ech;

	if (ech.setNames(m_location, m_names_list->names())){
		QDialog::accept();
	}
	else {
		return;
	}
}
