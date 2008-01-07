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
#ifndef CUSTOM_ELEMENT_EDITOR_H
#define CUSTOM_ELEMENT_EDITOR_H
#include <QtGui>
#include "elementscene.h"
#include "orientationset.h"
class ElementView;
/**
	Cette classe represente un editeur d'element. Elle permet a l'utilisateur
	de dessiner, modifier et parametrer un element electrique. Le dessin se
	fait par ajout de parties (Part).
*/
class QETElementEditor : public QMainWindow {
	Q_OBJECT
	
	// constructeur, destructeur
	public:
	QETElementEditor(QWidget * = 0);
	virtual ~QETElementEditor();
	private:
	QETElementEditor(const QETElementEditor &);
	
	// attributs
	private:
	/// booleen indiquant si l'editeur est en mode "lecture seule" ou non
	bool read_only;
	/// menus
	QMenu *file_menu, *edit_menu, *display_menu, *tools_menu, *help_menu;
	/// vue sur la scene d'edition
	ElementView *ce_view;
	/// scene d'edition
	ElementScene *ce_scene;
	/// container pour les widgets d'edition des parties
	QDockWidget *tools_dock;
	/// container pour la liste des annulations
	QDockWidget *undo_dock;
	/// Container pour la liste des parties
	QDockWidget *parts_dock;
	/// Liste des parties
	QListWidget *parts_list;
	/// actions du menu fichier
	QAction *new_element, *open, *save, *save_as, *reload, *quit;
	/// actions du menu edition
	QAction *selectall, *deselectall, *inv_select;
	QAction *undo, *redo;
	QAction *zoom_in, *zoom_out, *zoom_fit, *zoom_reset;
	QAction *edit_delete, *edit_size_hs, *edit_names, *edit_ori;
	QAction *edit_raise, *edit_lower, *edit_backward, *edit_forward;
	/// barres d'outils
	QToolBar *parts_toolbar, *main_toolbar, *view_toolbar, *depth_toolbar, *element_toolbar;
	/// actions de la barre d'outils
	QActionGroup *parts;
	QAction *move, *add_line, *add_circle, *add_ellipse, *add_polygon, *add_text;
	QAction *add_arc, *add_terminal, *add_textfield;
	/// label affiche lors de la selection de plusieurs elements
	QLabel *default_informations;
	/// titre minimal
	QString min_title;
	/// Nom de fichier
	QString _filename;
	
	// methodes
	public:
	void setSize(const QSize &);
	QSize size() const;
	void setHotspot(const QPoint &);
	QPoint hotspot() const;
	void setNames(const NamesList &);
	void setOrientations(const OrientationSet &orientation_set);
	OrientationSet orientations() const;
	void setFileName(const QString &);
	QString fileName() const;
	void setReadOnly(bool);
	bool isReadOnly() const;
	void fromFile(const QString &);
	bool toFile(const QString &);
	ElementScene *elementScene() const;
	void readSettings();
	void writeSettings();
	
	protected:
	void closeEvent(QCloseEvent *);
	
	private:
	void setupActions();
	void setupMenus();
	void setupInterface();
	bool canClose();
	
	public slots:
	void slot_new();
	void slot_open();
	void slot_reload();
	bool slot_save();
	bool slot_saveAs();
	void slot_setRubberBandToView();
	void slot_setNoDragToView();
	void slot_setNormalMode();
	void slot_updateInformations();
	void slot_updateMenus();
	void slot_updateTitle();
	void slot_createPartsList();
	void slot_updatePartsList();
	void slot_updateSelectionFromPartsList();
	void xmlPreview();
};

/**
	@param siz La nouvelle taille de l'element edite
*/
inline void QETElementEditor::setSize(const QSize &siz) {
	ce_scene -> setWidth(siz.width());
	ce_scene -> setHeight(siz.height());
}

/**
	@return la taille de l'element edite
*/
inline QSize QETElementEditor::size() const {
	return(
		QSize(
			ce_scene -> width(),
			ce_scene -> height()
		)
	);
}

/**
	@param hs Le nouveau point de saisie de l'element edite
*/
inline void QETElementEditor::setHotspot(const QPoint &hs) {
	ce_scene -> setHotspot(hs);
}

/**
	@return le point de saisie de l'element edite
*/
inline QPoint QETElementEditor::hotspot() const {
	return(ce_scene -> hotspot());
}

/**
	@param nameslist le nouvel ensemble de noms de l'element edite
*/
inline void QETElementEditor::setNames(const NamesList &nameslist) {
	ce_scene -> setNames(nameslist);
}

/**
	@param orientation_set le nouvel ensemble d'orientations de l'element edite
*/
inline void QETElementEditor::setOrientations(const OrientationSet &orientation_set) {
	ce_scene -> setOrientations(orientation_set);
}

/**
	@return le nouvel ensemble d'orientations de l'element edite
*/
inline OrientationSet QETElementEditor::orientations() const {
	return(ce_scene -> orientations());
}

/**
	@param fn Le nouveau nom de fichier de l'element edite
*/
inline void QETElementEditor::setFileName(const QString &fn) {
	_filename = fn;
	slot_updateTitle();
}

/**
	@return le nomde fichier de l'element edite
*/
inline QString QETElementEditor::fileName() const {
	return(_filename);
}

/**
	@return la scene d'edition de l'element
*/
inline ElementScene *QETElementEditor::elementScene() const {
	return(ce_scene);
}

#endif
