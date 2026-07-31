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
#ifndef PLCLINKWIDGET_H
#define PLCLINKWIDGET_H

#include "abstractelementpropertieseditorwidget.h"

#include <QHash>
#include <QPointer>
#include <QSet>

class QTreeWidgetItem;
class QTreeWidget;
class QLineEdit;
class QPushButton;
class QLabel;
class Element;

/**
	@brief The PlcLinkWidget class
	Provides a dedicated widget for linking a PLC-Slave to a PLC-Master IO entry.
	Displays all PLC-Masters as expandable items, each showing their IO entries.
	Each IO entry can only be linked to one slave. Already-linked entries are
	shown as struck-through and greyed out.
*/
class PlcLinkWidget : public AbstractElementPropertiesEditorWidget
{
	Q_OBJECT

	public:
		explicit PlcLinkWidget(Element *elmt, QWidget *parent = nullptr);
		~PlcLinkWidget() override = default;

		void setElement(Element *element) override;
		void apply() override;
		QUndoCommand *associatedUndo() const override;
		QString title() const override;

	public slots:
		void updateUi() override;

	private:
		void buildPlcTree();
		void hideButtons();
		void showButtons();

	private slots:
		void on_m_search_field_textEdited(const QString &text);
		void on_m_tree_widget_customContextMenuRequested(const QPoint &pos);
		void on_m_unlink_pb_clicked();
		void on_m_show_this_pb_clicked();

	private:
		QLabel *m_label{nullptr};
		QPushButton *m_unlink_pb{nullptr};
		QPushButton *m_show_this_pb{nullptr};
		QLineEdit *m_search_field{nullptr};
		QTreeWidget *m_tree_widget{nullptr};
		QLabel *m_hidden_masters_label{nullptr};

		// Maps IO child items to (master, io_index)
		struct PlcIoEntry {
			QPointer<Element> master;
			int ioIndex = -1;
		};
		QHash<QTreeWidgetItem*, PlcIoEntry> m_io_entry_hash;

		Element *m_element_to_link = nullptr;
		int m_pending_io_index = -1;
};

#endif // PLCLINKWIDGET_H
