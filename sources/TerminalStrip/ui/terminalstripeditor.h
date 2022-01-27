/*
	Copyright 2006-2021 The QElectroTech Team
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
#ifndef TERMINALSTRIPEDITOR_H
#define TERMINALSTRIPEDITOR_H

#include <QDialog>

#include "terminalstripmodel.h"

namespace Ui {
	class TerminalStripEditor;
}

class QETProject;
class TerminalStrip;
class QTreeWidgetItem;
class TerminalElement;
class QAbstractButton;

/**
 * @brief The TerminalStripEditor class
 * Main dialog used to edit terminal strip
 * of a project
 */
class TerminalStripEditor : public QDialog
{
		Q_OBJECT

	public:
		explicit TerminalStripEditor(QETProject *project, QWidget *parent = nullptr);
		~TerminalStripEditor() override;

	private:
		void setUpUndoConnections();
		void buildTree();
		QTreeWidgetItem* addTerminalStrip(TerminalStrip *terminal_strip);
		void addFreeTerminal();
		void setCurrentStrip(TerminalStrip *strip_);
		void spanMultiLevelTerminals();
		void selectionChanged();
		QSize setUpBridgeCellWidth();
		TerminalStripModel::Column isSingleColumnSelected() const;
		QPair<TerminalStripModel::Column, QVector<modelRealTerminalData>> singleColumnData() const;

	private slots:
		void on_m_add_terminal_strip_pb_clicked();
		void on_m_remove_terminal_strip_pb_clicked();
		void on_m_reload_pb_clicked();
		void on_m_terminal_strip_tw_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
		void on_m_dialog_button_box_clicked(QAbstractButton *button);
		void on_m_auto_ordering_pb_clicked();
		void on_m_group_terminals_pb_clicked();
		void on_m_ungroup_pb_clicked();
		void on_m_level_sb_valueChanged(int arg1);
		void on_m_type_cb_activated(int index);
		void on_m_function_cb_activated(int index);
		void on_m_led_cb_activated(int index);
		void on_m_bridge_terminals_pb_clicked();
		void on_m_unbridge_terminals_pb_clicked();
		void on_m_bridge_color_cb_activated(const QColor &col);

	private:
		Ui::TerminalStripEditor *ui;
		QETProject *m_project = nullptr;

		QHash<QTreeWidgetItem *, TerminalStrip *> m_item_strip_H;
		QHash<QUuid, QPointer<TerminalElement>> m_uuid_terminal_H;
		QHash<QUuid, QPointer<TerminalStrip>> m_uuid_strip_H;
		TerminalStrip *m_current_strip = nullptr;
		TerminalStripModel *m_model = nullptr;
};

#endif // TERMINALSTRIPEDITOR_H
