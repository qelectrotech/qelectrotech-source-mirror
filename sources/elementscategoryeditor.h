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
#ifndef ELEMENTS_CATEGORY_EDITOR_H
#define ELEMENTS_CATEGORY_EDITOR_H

#include <QDialog>
#include "elementslocation.h"

class NamesListWidget;
class QFileNameEdit;
class QDialogButtonBox;
class QLabel;

/**
 * @brief The ElementsCategoryEditor class
 * This class provides a dialog to edit an existing category or create a new one.
 */
class ElementsCategoryEditor : public QDialog
{
	Q_OBJECT

	public:
		ElementsCategoryEditor(const ElementsLocation &location, bool edit = true, QWidget *parent = nullptr);
		virtual ~ElementsCategoryEditor();

		ElementsLocation createdLocation() const;
	
	private:
		ElementsCategoryEditor(const ElementsCategoryEditor &);
	
	private:
		QDialogButtonBox *m_buttons;
		NamesListWidget *m_names_list;
		QLabel *m_file_name;
		QFileNameEdit *m_file_line_edit;
		bool m_edit_mode;
		ElementsLocation m_location;
		ElementsLocation m_created_location;

	private:
		void setUpWidget();
	
	public slots:
		void acceptCreation();
		void acceptUpdate();
};
#endif
