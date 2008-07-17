/*
	Copyright 2006-2008 Xavier Guerrin
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
#ifndef QET_DIAGRAM_EDITOR_H
#define QET_DIAGRAM_EDITOR_H
#include <QtGui>
#include "insetproperties.h"
class DiagramView;
class ElementsPanelWidget;
/**
	Cette classe represente la fenetre principale de QElectroTech et,
	ipso facto, la plus grande partie de l'interface graphique de QElectroTech.
	Il s'agit d'un objet QMainWindow avec un QWorkSpace contenant des objets
	« DiagramView » en guise de widget central et un « Panel d'Elements » en
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
	static InsetProperties defaultInsetProperties();
	
	protected:
	void actions();
	
	private:
	DiagramView *currentDiagram() const;
	void menus();
	void toolbar();
	
	public slots:
	void toggleFullScreen();
	void configureQET();
	void aboutQET();
	void printDialog();
	void exportDialog();
	bool saveAsDialog();
	bool save();
	bool newDiagram();
	bool openDiagram();
	bool openAndAddDiagram(const QString &);
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
	void readSettings();
	void writeSettings();
	
	// attributs
	public:
	// Actions faisables au travers de menus dans l'application QElectroTech
	QActionGroup *grp_visu_sel;  ///< Groupe d'actions pour les modes (edition et visualisation)
	QAction *mode_selection;     ///< Passe en mode edition
	QAction *mode_visualise;     ///< Passe en mode visualisation
	QAction *new_file;           ///< Cree un nouveau schema
	QAction *open_file;          ///< OUvre un fichier
	QAction *close_file;         ///< Ferme le fichier
	QAction *save_file;          ///< Enregistre le fichier
	QAction *save_file_sous;     ///< Enregistrer le fichier sous un nom donne
	QAction *import_diagram;     ///< Importe un schema existant (non implemente)
	QAction *export_diagram;     ///< Exporte le schema sous forme d'image
	QAction *print;              ///< Imprime le schema
	QAction *quit_editor;        ///< Quitte l'editeur de schema
	QAction *undo;               ///< Annule la derniere action
	QAction *redo;               ///< Refait une action annulee
	QAction *cut;                ///< Coupe la selection dans le presse-papier
	QAction *copy;               ///< Copie la selection dans le presse-papier
	QAction *paste;              ///< Colle le contenu du presse-papier sur le schema
	QAction *select_all;         ///< Selectionne tout
	QAction *select_nothing;     ///< Deselectionne tout
	QAction *select_invert;      ///< Inverse la selection
	QAction *delete_selection;   ///< Supprime la selection
	QAction *rotate_selection;   ///< Pivote les elements selectionnes
	QAction *conductor_prop;     ///< Lance le dialogue d'edition du conducteur selectionne
	QAction *conductor_reset;    ///< Reinitialise les conducteurs selectionnes
	QAction *conductor_default;  ///< Lance le dialogue d'edition des conducteurs par defaut
	QAction *infos_diagram;      ///< Lance le dialogue d'edition des proprietes du schema
	QAction *add_text;           ///< Passe en mode "Ajout de texte"
	QAction *add_column;         ///< Ajoute une colonne au schema
	QAction *remove_column;      ///< Enleve une colonne du schema
	QAction *expand_diagram;     ///< Augmente la hauteur du schema
	QAction *shrink_diagram;     ///< Reduit la hauteur du schema
	QAction *zoom_in;            ///< Zoome avant
	QAction *zoom_out;           ///< Zoome arriere
	QAction *zoom_fit;           ///< Ajuste le zoom de facon a voir l'integralite des elements
	QAction *zoom_reset;         ///< Remet le zoom 1:1
	QAction *about_qet;          ///< Lance le dialogue "A propos de QElectroTech"
	QAction *about_qt;           ///< Lance le dialogue "A propos de Qt"
	QAction *configure;          ///< Lance le dialogue de configuration de QElectroTech
	QAction *fullscreen_on;      ///< Passe en mode plein ecran
	QAction *fullscreen_off;     ///< Sort du mode plein ecran
	QAction *tile_window;        ///< Affiche les fenetre MDI en mosaique
	QAction *cascade_window;     ///< Affiche les fenetres MDI en cascade
	QAction *arrange_window;     ///< Reorganise les fenetres MDI
	QAction *prev_window;        ///< Affiche la fenetre MDI precedente
	QAction *next_window;        ///< Affiche la fenetre MDI suivante
	
	private:
	QWorkspace workspace;
	QSignalMapper windowMapper;
	/// Dossier a utiliser pour Fichier > ouvrir
	QDir open_dialog_dir;
	/// Dock pour le Panel d'Elements
	QDockWidget *qdw_pa;
	/// Dock pour la liste des annulations
	QDockWidget *qdw_undo;
	/// Panel d'Elements
	ElementsPanelWidget *pa;
	QMenu *windows_menu;
	QToolBar *main_bar;
	QToolBar *view_bar;
	QToolBar *diagram_bar;
	QUndoGroup undo_group;
};
#endif
