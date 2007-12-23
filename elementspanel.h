/*
	Copyright 2006-2007 Xavier Guerrin
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
#ifndef PANELAPPAREILS_H
#define PANELAPPAREILS_H
#include <QtGui>
#include "qetdiagrameditor.h"
/**
	Cette classe represente le panel d'appareils (en tant qu'element
	graphique) dans lequel l'utilisateur choisit les composants de
	son choix et les depose sur le schema par drag'n drop.
*/
class ElementsPanel : public QTreeWidget {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	ElementsPanel(QWidget * = 0);
	virtual ~ElementsPanel();
	
	private:
	ElementsPanel(const ElementsPanel &);
	
	// methodes
	public:
	bool selectedItemIsAnElement() const;
	bool selectedItemIsACategory() const;
	
	private:
	void addFile(QTreeWidgetItem *, QString);
	void addDir(QTreeWidgetItem *, QString, QString = QString());
	QFileInfo selectedFile() const;
	void launchElementEditor(const QString &);
	void launchCategoryEditor(const QString &);
	void saveExpandedCategories();
	QTreeWidgetItem *findFile(const QString &) const;
	QStringList expanded_directories;
	QString last_selected_item;
	
	public slots:
	void slot_doubleClick(QTreeWidgetItem *, int);
	void dragMoveEvent(QDragMoveEvent *);
	void dropEvent(QDropEvent *);
	void startDrag(Qt::DropActions);
	void reload();
	void editCategory();
	void editElement();
	void deleteCategory();
	void deleteElement();
};
#endif
