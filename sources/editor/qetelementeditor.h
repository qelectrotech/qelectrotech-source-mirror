/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "qetmainwindow.h"
#include "qet.h"
#include "elementscene.h"
#include "elementslocation.h"


class ElementItemEditor;
class ElementView;

/**
	This class represents an element editor, allowing users to draw, change and
	configure a particular electrical element.
*/
class QETElementEditor : public QETMainWindow {
	Q_OBJECT

	// constructor, destructor
	public:
		QETElementEditor(QWidget * = nullptr);
		~QETElementEditor() override;
	private:
		QETElementEditor(const QETElementEditor &);
		void setupActions();
		void setupMenus();
		void setupInterface();

	// attributes
	private:
		/// whether the editor is "read-only"
		bool read_only;
		/// menus
		QMenu *file_menu, *edit_menu, *paste_from_menu, *display_menu, *tools_menu;
		/// view widget for the editing scene
		ElementView *m_view;
		/// editing scene
		ElementScene *m_elmt_scene;
		/// container for widgets dedicated to primitive edition
		QDockWidget *m_tools_dock;
		/// Stack of widgets for tools_dock
		QStackedWidget *m_tools_dock_stack;
		/// label displayed when several primitives are selected
		QLabel *m_default_informations;
		/// Hash associating primitive names with their matching edition widget
		QHash<QString, ElementItemEditor *> m_editors;
		/// container for the undo list
		QDockWidget *m_undo_dock;
		/// Container for the list of existing primitives
		QDockWidget *m_parts_dock;
		/// List of primitives
		QListWidget *m_parts_list;
		/// actions for the "file" menu
		QAction *new_element, *open, *open_dxf, *open_file, *save, *save_as, *save_as_file, *reload, *quit;
		/// actions for the "edit" menu
		QAction *selectall, *deselectall, *inv_select;
		QAction *cut, *copy, *paste, *paste_in_area, *paste_from_file, *paste_from_elmt;
		QAction *undo, *redo;
		QAction *edit_delete, *edit_size_hs, *edit_names, *edit_author, *m_edit_properties;
		/// toolbars
		QToolBar *parts_toolbar, *main_toolbar, *view_toolbar, *depth_toolbar, *element_toolbar;
		/// Action group
		QActionGroup *parts, *m_zoom_ag, *m_depth_action_group;
		/// minimum window title
		QString min_title;
		/// filename of the currently edited element
		QString filename_;
		/// location of the currently edited element
		ElementsLocation location_;
		/// whether the currently edited element comes from a file or a location
		bool opened_from_file;

	// methods
	public:
		void setNames(const NamesList &);
		void setLocation(const ElementsLocation &);
		ElementsLocation location() const;
		void setFileName(const QString &);
		QString fileName() const;
		void setReadOnly(bool);
		bool isReadOnly() const;
		void fromFile(const QString &);
		void fromLocation(const ElementsLocation &);
		bool toFile(const QString &);
		bool toLocation(const ElementsLocation &location);
		bool isEditing(const ElementsLocation &);
		bool isEditing(const QString &);
		ElementScene *elementScene() const;
		void readSettings();
		void writeSettings();
		static QPointF pasteOffset();
		static QString getOpenElementFileName(QWidget * = nullptr, const QString & = QString());
		void contextMenu(QPoint p, QList<QAction *> actions = QList<QAction*>());

	signals:
		void saveToLocation(ElementsLocation loc);

	protected:
		void closeEvent(QCloseEvent *) override;
		void firstActivation(QEvent *) override;

	private:
		bool canClose();
		QWidget *clearToolsDock();
		void copyAndPasteXml(const QDomDocument &);
	
	public slots:
		void addLine();
		void addRect();
		void addEllipse();
		void addPolygon();
		void addArc();
		void addText();
		void addTerminal();
		void addDynamicTextField();
		void UncheckAddPrimitive();

		void slot_new();
		void slot_open();
		void slot_openDxf();
		void slot_openFile();
		void openRecentFile(const QString &);
		void openElement(const QString &);
		void slot_reload();
		bool slot_save();
		bool slot_saveAs();
		bool slot_saveAsFile();
		void slot_setRubberBandToView();
		void slot_setNoDragToView();
		void slot_updateInformations();
		void slot_updateMenus();
		void slot_updateTitle();
		void slot_createPartsList();
		void slot_updatePartsList();
		void slot_updateSelectionFromPartsList();
		bool checkElement();
		void pasteFromFile();
		void pasteFromElement();
		void updateCurrentPartEditor();
};

/**
	@param nameslist the new list of names for the currently edited element
*/
inline void QETElementEditor::setNames(const NamesList &nameslist) {
	m_elmt_scene -> setNames(nameslist);
}

/**
	@return the location of the currently edited element
*/
inline ElementsLocation QETElementEditor::location() const {
	return(location_);
}

/**
	@return the filename of the currently edited element
*/
inline QString QETElementEditor::fileName() const {
	return(filename_);
}

/**
	@return the editing scene
*/
inline ElementScene *QETElementEditor::elementScene() const {
	return(m_elmt_scene);
}

#endif
