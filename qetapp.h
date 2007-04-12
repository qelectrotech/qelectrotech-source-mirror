#ifndef QETAPP_H
#define QETAPP_H
#include <QtGui>
class DiagramView;
class ElementsPanelWidget;
/**
	Cette classe represente la fenetre principale de QElectroTech et,
	ipso facto, la plus grande partie de l'interface graphique de QElectroTech.
	Il s'agit d'un objet QMainWindow avec un QWorkSpace contenant des objets
	« Diagram » en guise de widget central et un « Panel d'Appareils » en guise
	de widget « Dock ».
*/
class QETApp : public QMainWindow {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	QETApp(QWidget * = 0);
	virtual ~QETApp();
	
	private:
	QETApp(const QETApp &);
	
	// methodes
	public:
	void closeEvent(QCloseEvent *);
	void addDiagramView(DiagramView *);
	static QString commonElementsDir();
	static QString customElementsDir();
	static QString configDir();
	static QString languagesPath();
	static QString realPath(QString &);
	static QString symbolicPath(QString &);
	
	protected:
	void actions();
	
	private:
	DiagramView *diagramEnCours() const;
	void menus();
	void toolbar();
	
	public slots:
	void systray(QSystemTrayIcon::ActivationReason);
	void systrayReduce();
	void systrayRestore();
	void quit(QCloseEvent * = NULL);
	void toggleFullScreen();
	void aPropos();
	void dialog_print();
	void dialog_export();
	bool dialogue_enregistrer_sous();
	bool enregistrer();
	bool nouveau();
	bool ouvrir();
	bool fermer();
	void slot_editInfos();
	void slot_couper();
	void slot_copier();
	void slot_coller();
	void slot_zoomPlus();
	void slot_zoomMoins();
	void slot_zoomFit();
	void slot_zoomReset();
	void slot_selectAll();
	void slot_selectNothing();
	void slot_selectInvert();
	void slot_supprimer();
	void slot_pivoter();
	void slot_setSelectionMode();
	void slot_setVisualisationMode();
	void slot_updateActions();
	void slot_updateMenuFenetres();
	void slot_addColumn();
	void slot_removeColumn();
	void slot_expand();
	void slot_shrink();
	
	// attributs
	protected:
	// Actions faisables au travers de menus dans l'application QElectroTech
	QAction *mode_selection;
	QAction *mode_visualise;
	QAction *nouveau_fichier;
	QAction *ouvrir_fichier;
	QAction *fermer_fichier;
	QAction *enr_fichier;
	QAction *enr_fichier_sous;
	QAction *importer;
	QAction *exporter;
	QAction *imprimer;
	QAction *quitter_qet;
	QAction *annuler;
	QAction *refaire;
	QAction *couper;
	QAction *copier;
	QAction *coller;
	QAction *sel_tout;
	QAction *sel_rien;
	QAction *sel_inverse;
	QAction *supprimer;
	QAction *selectionner;
	QAction *pivoter;
	QAction *infos_diagram;
	QAction *add_column;
	QAction *remove_column;
	QAction *expand_diagram;
	QAction *shrink_diagram;
	QAction *poser_fil;
	QAction *reduce_appli;
	QAction *restore_appli;
	QAction *zoom_avant;
	QAction *zoom_arriere;
	QAction *zoom_adapte;
	QAction *zoom_reset;
	QAction *a_propos_de_qet;
	QAction *a_propos_de_qt;
	QAction *configurer;
	QAction *entrer_pe;
	QAction *sortir_pe;
	QAction *f_mosaique;
	QAction *f_cascade;
	QAction *f_reorganise;
	QAction *f_prec;
	QAction *f_suiv;
	QAction *menu_systray_masquer_restaurer;
	
	private:
	QWorkspace workspace;
	QSignalMapper windowMapper;
	/// Dock pour le Panel d'Appareils
	QDockWidget *qdw_pa;
	/// Panel d'Appareils
	ElementsPanelWidget *pa;
	/// Elements de menus pour l'icone du systray
	QMenu *menu_systray;
	QAction *systray_masquer;
	QAction * config_fullscreen;
	QMenu *menu_fenetres;
	/// Icone dans le systray
	QSystemTrayIcon *qsti;
	/// Geometrie de la fenetre principale
	QByteArray wg;
	QToolBar *barre_outils;
};
#endif
