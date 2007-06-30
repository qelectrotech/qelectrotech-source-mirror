#ifndef CUSTOM_ELEMENT_EDITOR_H
#define CUSTOM_ELEMENT_EDITOR_H
#include <QtGui>
#include "editorscene.h"
#include "orientationset.h"
class CustomElementEditor : public QMainWindow {
	Q_OBJECT
	
	// constructeur, destructeur
	public:
	CustomElementEditor(QWidget * = 0);
	virtual ~CustomElementEditor();
	private:
	CustomElementEditor(const CustomElementEditor &);
	
	// attributs
	private:
	/// booleen indiquant si l'editeur est en mode "lecture seule" ou non
	bool read_only;
	/// menus
	QMenu *file_menu, *edit_menu, *display_menu, *tools_menu, *help_menu;
	/// vue sur la scene d'edition
	QGraphicsView *ce_view;
	/// scene d'edition
	EditorScene *ce_scene;
	/// container pour les widgets d'edition des parties
	QDockWidget *tools_dock;
	/// actions du menu fichier
	QAction *new_element, *open, *save, *save_as, *quit;
	/// actions du menu edition
	QAction *selectall, *deselectall, *inv_select;
	QAction *edit_delete, *edit_size, *edit_hotspot, *edit_names, *edit_ori;
	/// barre d'outils
	QToolBar *parts_toolbar;
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
	
	protected:
	void closeEvent(QCloseEvent *);
	
	private:
	void setupActions();
	void setupMenus();
	void setupInterface();
	bool close();
	
	public slots:
	void slot_new();
	void slot_open();
	bool slot_save();
	bool slot_saveAs();
	void slot_quit(QCloseEvent * = NULL);
	void slot_setRubberBandToView();
	void slot_setNoDragToView();
	void slot_setNormalMode();
	void slot_updateInformations();
	void slot_updateMenus();
	void xmlPreview();
};

inline void CustomElementEditor::setSize(const QSize &siz) {
	ce_scene -> setWidth(siz.width());
	ce_scene -> setHeight(siz.height());
}

inline QSize CustomElementEditor::size() const {
	return(
		QSize(
			ce_scene -> width(),
			ce_scene -> height()
		)
	);
}

inline void CustomElementEditor::setHotspot(const QPoint &hs) {
	ce_scene -> setHotspot(hs);
}

inline QPoint CustomElementEditor::hotspot() const {
	return(ce_scene -> hotspot());
}

inline void CustomElementEditor::setNames(const NamesList &nameslist) {
	ce_scene -> setNames(nameslist);
}

inline void CustomElementEditor::setOrientations(const OrientationSet &orientation_set) {
	ce_scene -> setOrientations(orientation_set);
}

inline OrientationSet CustomElementEditor::orientations() const {
	return(ce_scene -> orientations());
}

inline void CustomElementEditor::setFileName(const QString &fn) {
	setWindowTitle(min_title + " - " + fn);
	_filename = fn;
}

inline QString CustomElementEditor::fileName() const {
	return(_filename);
}

#endif
