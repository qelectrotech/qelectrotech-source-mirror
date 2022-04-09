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
#ifndef TERMINALSTRIPTREEWIDGET_H
#define TERMINALSTRIPTREEWIDGET_H

#include <QTreeWidget>
#include <QUuid>

/**
 * @brief The TerminalStripTreeWidget class
 * Derived class use to implement custom drag and drop
 */
class TerminalStripTreeWidget : public QTreeWidget
{
        Q_OBJECT
	public :
		enum TreeWidgetType{
			Root,
			Terminal,
			FreeTerminal,
			Installation,
			Location,
			Strip
		};

			//Role used for data in QTreeWidgetItem
		static constexpr int UUID_USER_ROLE{Qt::UserRole + 1};

	signals:
			/**
			 * @brief terminalAddedToStrip
			 * Signal emited when a terminal is moved from free terminal to a terminals trip
			 */
		void terminalAddedToStrip(QUuid terminal_uuid, QUuid strip_uuid);
			/**
			 * @brief terminalMovedFromStripToStrip
			 * Signam emitted when a terminal is moved from from a terminal stip to another one
			 */
		void terminalMovedFromStripToStrip(QUuid terminal_uuid, QUuid old_strip_uuid, QUuid new_strip_uuid);
			/**
			 * @brief terminalRemovedFromStrip
			 * Signal emitted when a terminal is moved from a terminal strip to free terminal
			 */
		void terminalRemovedFromStrip(QUuid terminal_uuid, QUuid old_strip_uuid);


    public:
        TerminalStripTreeWidget(QWidget *parent = nullptr);

	protected:
		QStringList mimeTypes() const override;
		void startDrag(Qt::DropActions supportedActions) override;
		void dragMoveEvent(QDragMoveEvent *event) override;
		void dropEvent(QDropEvent *event) override;
		Qt::DropActions supportedDropActions() const override;
};

#endif // TERMINALSTRIPTREEWIDGET_H
