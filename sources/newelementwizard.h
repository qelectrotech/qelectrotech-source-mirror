/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef NEW_ELEMENT_WIZARD_H
#define NEW_ELEMENT_WIZARD_H
#include <QtGui>
class ElementsCategoriesWidget;
class ElementsCategory;
class NamesListWidget;
class OrientationSetWidget;
class HotspotEditor;
class QFileNameEdit;
/**
	Cette classe represente un dialogue qui permet a l'utilisateur de specifier
	les premiers parametres de l'element qu'il va construire.
	Ces parametres sont :
	  - la categorie dans laquelle il faut enregistrer l'element
	  - le nom du fichier dans lequel il faut enregistrer l'element
	  - les noms de cet element
	  - les dimensions
	  - la position du hotspot
	  - les orientations possibles
*/
class NewElementWizard : public QWizard {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	NewElementWizard(QWidget * = 0, Qt::WindowFlags = 0);
	virtual ~NewElementWizard();
	
	private:
	NewElementWizard(const NewElementWizard &);
	
	// methodes
	public:
	ElementsCategory *selectedCategory() const;
	bool preselectCategory(ElementsCategory *);
	
	// attributs
	private:
	enum WizardState { Category, Filename, Names, Dimensions, Orientations };
	ElementsCategoriesWidget *categories_list;
	QFileNameEdit *qle_filename;
	NamesListWidget *element_names;
	OrientationSetWidget *orientation_set;
	HotspotEditor *hotspot_editor;
	WizardState current_state;
	QString chosen_file;
	ElementsCategory *chosen_category;
	
	// methodes
	private:
	QWizardPage *buildStep1();
	QWizardPage *buildStep2();
	QWizardPage *buildStep3();
	QWizardPage *buildStep4();
	QWizardPage *buildStep5();
	bool validStep1();
	bool validStep2();
	bool validateCurrentPage();
	void createNewElement();
};
#endif
