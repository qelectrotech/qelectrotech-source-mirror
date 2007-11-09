#ifndef QET_DIAGRAM_EDITOR_H
#define QET_DIAGRAM_EDITOR_H
#include <QtGui>
class DiagramView;
class ElementsPanelWidget;
/**
	Cette classe represente la fenetre principale de QElectroTech et,
	ipso facto, la plus grande partie de l'interface graphique de QElectroTech.
	Il s'agit d'un objet QMainWindow avec un QWorkSpace contenant des objets
	« DiagramView » en guise de widget central et un « Panel d'Appareils » en
	guise de widget « Dock ».
*/
class QETDiagramEditor : public QMainWindow {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	QETDiagramEditor(const QStringList & = QStringList(), QWidget * = 0);
	virtual ~QETDiagramEditor();
	
	private:
	QETDiagramEditor(const QETDiagramEditor &);
	
	// methodes
	public:
	void closeEvent(QCloseEvent *);
	void addDiagramView(DiagramView *);
	
	protected:
	void actions();
	
	private:
	DiagramView *currentDiagram() const;
	void menus();
	void toolbar();
	
	public slots:
	void toggleFullScreen();
	void aboutQET();
	void printDialog();
	void exportDialog();
	bool saveAsDialog();
	bool save();
	bool newDiagram();
	bool openDiagram();
	bool closeDiagram();
	void slot_editInfos();
	void slot_cut();
	void slot_copy();
	void slot_paste();
	void slot_zoomIn();
	void slot_zoomOut();
	void slot_zoomFit();
	void slot_zoomReset();
	void slot_selectAll();
	void slot_selectNothing();
	void slot_selectInvert();
	void slot_delete();
	void slot_rotate();
	void slot_setSelectionMode();
	void slot_setVisualisationMode();
	void slot_updateActions();
	void slot_updateModeActions();
	void slot_updateComplexActions();
	void slot_updatePasteAction();
	void slot_updateWindowsMenu();
	void slot_addColumn();
	void slot_removeColumn();
	void slot_expand();
	void slot_shrink();
	void slot_editConductor();
	void slot_resetConductors();
	void slot_editDefaultConductors();
	void slot_addText();
	
	// attributs
	public:
	// Actions faisables au travers de menus dans l'application QElectroTech
	QActionGroup *grp_visu_sel;
	QAction *mode_selection;
	QAction *mode_visualise;
	QAction *new_file;
	QAction *open_file;
	QAction *close_file;
	QAction *save_file;
	QAction *save_file_sous;
	QAction *import_diagram;
	QAction *export_diagram;
	QAction *print;
	QAction *quit_editor;
	QAction *undo;
	QAction *redo;
	QAction *cut;
	QAction *copy;
	QAction *paste;
	QAction *select_all;
	QAction *select_nothing;
	QAction *select_invert;
	QAction *delete_selection;
	QAction *rotate_selection;
	QAction *conductor_prop;
	QAction *conductor_reset;
	QAction *conductor_default;
	QAction *infos_diagram;
	QAction *add_text;
	QAction *add_column;
	QAction *remove_column;
	QAction *expand_diagram;
	QAction *shrink_diagram;
	QAction *zoom_in;
	QAction *zoom_out;
	QAction *zoom_fit;
	QAction *zoom_reset;
	QAction *about_qet;
	QAction *about_qt;
	QAction *configure;
	QAction *fullscreen_on;
	QAction *fullscreen_off;
	QAction *tile_window;
	QAction *cascade_window;
	QAction *arrange_window;
	QAction *prev_window;
	QAction *next_window;
	
	private:
	QWorkspace workspace;
	QSignalMapper windowMapper;
	/// Dossier a utiliser pour Fichier > ouvrir
	QDir open_dialog_dir;
	/// Dock pour le Panel d'Appareils
	QDockWidget *qdw_pa;
	/// Dock pour la liste des annulations
	QDockWidget *qdw_undo;
	/// Panel d'Appareils
	ElementsPanelWidget *pa;
	QMenu *windows_menu;
	QToolBar *main_bar;
	QToolBar *view_bar;
	QToolBar *diagram_bar;
	QUndoGroup undo_group;
};
#endif
