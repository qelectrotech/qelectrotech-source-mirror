#ifndef QETAPP_H
	#define QETAPP_H
	#include <QtGui>
	class SchemaVue;
	class PanelAppareils;
	/**
		Cette classe represente la fenetre principale de QElectroTech et,
		ipso facto, la plus grande partie de l'interface graphique de QElectroTech.
		Il s'agit d'un objet QMainWindow avec un objet « Schema » en guise de widget central
		et un « Panel d'Appareils » en guise de widget « Dock ».
	*/
	class QETApp : public QMainWindow {
		Q_OBJECT
		public:
		QETApp(QWidget *parent=0);
		void closeEvent(QCloseEvent * event );
		void addSchemaVue(SchemaVue *);
		
		public slots:
		void systray(QSystemTrayIcon::ActivationReason raison);
		void systrayReduire();
		void systrayRestaurer();
		void quitter();
		void toggleFullScreen();
		void toggleAntialiasing();
		void aPropos();
		void dialogue_imprimer();
		void dialogue_exporter();
		bool dialogue_enregistrer_sous();
		bool enregistrer();
		bool nouveau();
		bool ouvrir();
		bool fermer();
		
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
		QAction *poser_fil;
		QAction *masquer_appli;
		QAction *restaurer_appli;
		QAction *zoom_avant;
		QAction *zoom_arriere;
		QAction *zoom_adapte;
		QAction *zoom_reset;
		QAction *a_propos_de_qet;
		QAction *a_propos_de_qt;
		QAction *configurer;
		QAction *entrer_pe;
		QAction *sortir_pe;
		QAction *toggle_aa;
		QAction *f_mosaique;
		QAction *f_cascade;
		QAction *f_reorganise;
		QAction *f_prec;
		QAction *f_suiv;
		
		void actions();
		// menus variables
		QAction *menu_systray_masquer_restaurer;
		
		private:
		QWorkspace workspace;
		SchemaVue *schemaEnCours();
		QSignalMapper windowMapper;
		/// Dock pour le Panel d'Appareils
		QDockWidget *qdw_pa;
		/// Panel d'Appareils
		PanelAppareils *pa;
		/// Elements de menus pour l'icone du systray
		QMenu *menu_systray;
		QAction *systray_masquer;
		QAction * config_fullscreen;
		QAction *systray_quitter;
		QMenu *menu_fenetres;
		/// Icone dans le systray
		QSystemTrayIcon *qsti;
		/// Geometrie de la fenetre principale
		QByteArray wg;
		void menus();
		void toolbar();
		QToolBar *barre_outils;
		
		private slots:
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
	};
#endif
