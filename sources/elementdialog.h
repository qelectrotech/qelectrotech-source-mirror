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
#ifndef ELEMENT_DIALOG_H
#define ELEMENT_DIALOG_H

#include <QDialog>
#include "elementslocation.h"

class QDialogButtonBox;
class QFileNameEdit;
class QTreeView;
/**
	This class provides several dialogs to select an element or a category
	(e.g. new or existing, for opening or for saving...).
*/
class ElementDialog : public QDialog
{
	Q_OBJECT
	// enumerations
	/**
		This enum represents the available configurations for the required dialog
	*/
	enum {
		OpenElement  = 0, ///< The dialog should open an element
		SaveElement  = 1, ///< The dialog should select an element for saving
		OpenCategory = 2, ///< The dialog should open a category
		SaveCategory = 3  ///< The dialog should select a category for saving
	};
	
		// constructors, destructor
	public:
		ElementDialog(uint = ElementDialog::OpenElement, QWidget *parent = nullptr);
	private:
		ElementDialog(const ElementDialog &);

	public:
		ElementsLocation location() const;

	private:
		void setUpWidget();
		void setUpConnection();
		void indexClicked(const QModelIndex &index);
		void updateWidget();
		void checkCurrentLocation();
		void checkAccept();
	
		// attributes
	private:
		uint m_mode;
		ElementsLocation m_location;
		QDialogButtonBox *m_buttons_box = nullptr;
		QFileNameEdit *m_text_field     = nullptr;
		QTreeView *m_tree_view          = nullptr;

	public:
		static ElementsLocation getOpenElementLocation(QWidget *parent = nullptr);
		static ElementsLocation getSaveElementLocation(QWidget *parent = nullptr);
	private:
		static ElementsLocation execConfiguredDialog(int, QWidget *parent = nullptr);

};
#endif
