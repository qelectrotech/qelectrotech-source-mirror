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
#ifndef CUSTOM_ELEMENT_EDITOR_H
#define CUSTOM_ELEMENT_EDITOR_H
#include <QtGui>
#include "qetmainwindow.h"
#include "qet.h"
#include "elementscene.h"
#include "orientationset.h"
#include "elementslocation.h"
class ElementItemEditor;
class ElementView;
/**
	Cette classe represente un editeur d'element. Elle permet a l'utilisateur
	de dessiner, modifier et parametrer un element electrique. Le dessin se
	fait par ajout de parties (Part).
*/
class QETElementEditor : public QETMainWindow {
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
	QMenu *file_menu, *edit_menu, *paste_from_menu, *display_menu, *tools_menu;
	/// vue sur la scene d'edition
	ElementView *ce_view;
	/// scene d'edition
	ElementScene *ce_scene;
	/// container pour les widgets d'edition des parties
	QDockWidget *tools_dock;
	/// Pile de widgets pour tools_dock
	QStackedWidget *tools_dock_stack_;
	/// label affiche lors de la selection de plusieurs elements
	QLabel *default_informations;
	/// Hash permettant d'acceder aux widgets d'editions disponibles
	QHash<QString, ElementItemEditor *> editors_;
	/// ScrollArea pour le DockWidget affichant des infos sur la partie selectionnee
	QScrollArea *tools_dock_scroll_area_;
	/// container pour la liste des annulations
	QDockWidget *undo_dock;
	/// Container pour la liste des parties
	QDockWidget *parts_dock;
	/// Liste des parties
	QListWidget *parts_list;
	/// actions du menu fichier
	QAction *new_element, *open, *open_file, *save, *save_as, *save_as_file, *reload, *quit;
	/// actions du menu edition
	QAction *selectall, *deselectall, *inv_select;
	QAction *cut, *copy, *paste, *paste_in_area, *paste_from_file, *paste_from_elmt;
	QAction *undo, *redo;
	QAction *edit_delete, *edit_size_hs, *edit_names, *edit_ori, *edit_author;
	QAction *edit_raise, *edit_lower, *edit_backward, *edit_forward;
	/// actions du menu affichage
	QAction *zoom_in, *zoom_out, *zoom_fit, *zoom_reset;
	/// barres d'outils
	QToolBar *parts_toolbar, *main_toolbar, *view_toolbar, *depth_toolbar, *element_toolbar;
	/// actions de la barre d'outils
	QActionGroup *parts;
	QAction *move, *add_line, *add_circle, *add_rectangle, *add_ellipse, *add_polygon, *add_text;
	QAction *add_arc, *add_terminal, *add_textfield;
	/// titre minimal
	QString min_title;
	/// Nom de fichier de l'element edite
	QString filename_;
	/// Emplacement de l'element edite
	ElementsLocation location_;
	/// booleen indiquant si l'element en cours d'edition provient d'un fichier ou d'un emplacement
	bool opened_from_file;
	
	// methodes
	public:
	void setSize(const QSize &);
	QSize size() const;
	void setHotspot(const QPoint &);
	QPoint hotspot() const;
	void setNames(const NamesList &);
	void setOrientations(const OrientationSet &orientation_set);
	OrientationSet orientations() const;
	void setLocation(const ElementsLocation &);
	ElementsLocation location() const;
	void setFileName(const QString &);
	QString fileName() const;
	void setReadOnly(bool);
	bool isReadOnly() const;
	void fromFile(const QString &);
	void fromLocation(const ElementsLocation &);
	bool toFile(const QString &);
	bool toLocation(const ElementsLocation &);
	bool isEditing(const ElementsLocation &);
	bool isEditing(const QString &);
	ElementScene *elementScene() const;
	void readSettings();
	void writeSettings();
	static QPointF pasteOffset();
	static QET::OrientedMovement pasteMovement();
	static QString getOpenElementFileName(QWidget * = 0, const QString & = QString());
	
	protected:
	void closeEvent(QCloseEvent *);
	virtual void firstActivation(QEvent *);
	
	private:
	void setupActions();
	void setupMenus();
	void setupInterface();
	bool canClose();
	QWidget *clearToolsDock();
	void copyAndPasteXml(const QDomDocument &);
	
	public slots:
	void slot_new();
	void slot_open();
	void slot_openFile();
	void openRecentFile(const QString &);
	void openElement(const QString &);
	void slot_reload();
	bool slot_save();
	bool slot_saveAs();
	bool slot_saveAsFile();
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
	bool checkElement();
	void pasteFromFile();
	void pasteFromElement();
	void updateCurrentPartEditor();
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
	@return l'emplacement de l'element edite
*/
inline ElementsLocation QETElementEditor::location() const {
	return(location_);
}

/**
	@return le nom de fichier de l'element edite
*/
inline QString QETElementEditor::fileName() const {
	return(filename_);
}

/**
	@return la scene d'edition de l'element
*/
inline ElementScene *QETElementEditor::elementScene() const {
	return(ce_scene);
}

#endif
