/*
	Copyright 2006-2026 The QElectroTech Team
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
#ifndef QETELEMENTEDITOR_H
#define QETELEMENTEDITOR_H

#include "../../ElementsCollection/elementslocation.h"

#include <QCloseEvent>
#include <QMainWindow>

class ElementScene;
class QActionGroup;
class ElementItemEditor;
class ElementView;
class QListWidget;
class QStackedWidget;
class QLabel;

namespace Ui {
	class QETElementEditor;
}

class QETElementEditor : public QMainWindow
{
		Q_OBJECT
	signals:
		void saveToLocation(ElementsLocation loc);

	public:
		explicit QETElementEditor(QWidget *parent = nullptr);
		~QETElementEditor() override;

		void contextMenu(QPoint p, QList<QAction *> actions = QList<QAction *>());
		void setNames(const NamesList &name_list);
		void setLocation(const ElementsLocation &location);
		ElementsLocation location() const;
		void setFileName(const QString &file_name);
		QString fileName() const;
		void setReadOnly(bool ro);
		bool isReadOnly() const;
		void fromFile(const QString &filepath);
		bool toFile(const QString &filepath);
		void fromLocation(const ElementsLocation &location);
		bool toLocation(const ElementsLocation &location);
		bool isEditing(const ElementsLocation &location);
		bool isEditing(const QString &filepath);
		ElementScene *elementScene() const;
		ElementView *elementView() const;
		static QPointF pasteOffset();
		static QString getOpenElementFileName(QWidget *parent = nullptr, const QString &dir = QString());
		void updateTitle();
		void fillPartsList();
		void UncheckAddPrimitive();
		void updateCurrentPartEditor();
		void updateInformations();
		void updatePartsList();
		void updateSelectionFromPartsList();
		void openElement(const QString &filepath);
		bool checkElement();

	protected:
		bool event(QEvent *event) override;
		void closeEvent(QCloseEvent *) override;


	private slots:
		bool on_m_save_action_triggered();
		bool on_m_save_as_action_triggered();
		void on_m_select_all_act_triggered();
		void on_m_edit_element_properties_action_triggered();
		void on_m_new_action_triggered();
		void on_m_open_action_triggered();
		void on_m_open_from_file_action_triggered();
		bool on_m_save_as_file_action_triggered();
		void on_m_reload_action_triggered();
		void on_m_quit_action_triggered();
		void on_m_deselect_all_action_triggered();
		void on_m_cut_action_triggered();
		void on_m_copy_action_triggered();
		void on_m_paste_action_triggered();
		void on_m_paste_in_area_action_triggered();
		void on_m_paste_from_file_action_triggered();
		void on_m_paste_from_element_action_triggered();
		void on_m_revert_selection_action_triggered();
		void on_m_delete_action_triggered();
		void on_m_edit_names_action_triggered();
		void on_m_edit_author_action_triggered();
		void on_m_zoom_in_action_triggered();
		void on_m_zoom_out_action_triggered();
		void on_m_zoom_fit_best_action_triggered();
		void on_m_zoom_original_action_triggered();
		void on_m_about_qet_action_triggered();
		void on_m_online_manual_triggered();
		void on_m_youtube_action_triggered();
		void on_m_donate_action_triggered();
		void on_m_about_qt_action_triggered();
		void on_m_import_dxf_triggered();
		void on_m_import_scaled_element_triggered();

	private:
		bool canClose();
		void readSettings();
		void writeSettings() const;
		void setupActions();
		void updateAction();
		void setupConnection();
		void initGui();
		QWidget *clearToolsDock();
		void copyAndPasteXml(const QDomDocument &xml_document);

	private:
		Ui::QETElementEditor *ui;

		bool
			m_read_only        = false,
			m_opened_from_file = false,
			m_first_activation = true;

		ElementScene *m_elmt_scene = nullptr;

		QActionGroup
			*m_add_part_action_grp = nullptr,
			*m_depth_action_group  = nullptr;

		QList<QAction *> m_context_menu_action_list;

		QAction
			*m_undo_action = nullptr,
			*m_redo_action = nullptr;


			/// Hash associating primitive names with their matching edition widget
		QHash<QString, ElementItemEditor *> m_editors;

		ElementsLocation m_location;

		QString
			m_file_name,
			m_min_title;

		ElementView *m_view = nullptr;

		QListWidget *m_parts_list = nullptr;

		QStackedWidget *m_tools_dock_stack = nullptr;

		QLabel *m_default_informations = nullptr;

};

#endif // QETELEMENTEDITOR_H
