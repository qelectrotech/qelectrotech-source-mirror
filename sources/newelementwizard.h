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
#ifndef NEW_ELEMENT_WIZARD_H
#define NEW_ELEMENT_WIZARD_H

#include <QWizard>
#include "elementslocation.h"

class NamesListWidget;
class QFileNameEdit;
class QTreeView;

/**
	This class provides a wizard dialog enabling users to to specify the basic
	parameters of the electrical elements they intend to draw.
	
	These parameters include:
	  - the category the element should be saved to
	  - the filename the element should be saved to
	  - localized names
*/
class NewElementWizard : public QWizard {
	Q_OBJECT
	
	// constructors, destructor
	public:
	NewElementWizard(QWidget * = 0, Qt::WindowFlags = 0);
	virtual ~NewElementWizard();
	
	private:
	NewElementWizard(const NewElementWizard &);
	
	// attributes
	private:
	enum WizardState { Category, Filename, Names };
	QFileNameEdit *m_qle_filename;
	NamesListWidget *m_names_list;
	QString m_chosen_file;
	QTreeView *m_tree_view = nullptr;
	ElementsLocation m_chosen_location;
	
	// methods
	private:
	QWizardPage *buildStep1();
	QWizardPage *buildStep2();
	QWizardPage *buildStep3();
	bool validStep1();
	bool validStep2();
	bool validateCurrentPage();
	void createNewElement();
};
#endif
