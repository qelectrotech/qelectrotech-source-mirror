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
#ifndef GESTURESCONFIGPAGE_H
#define GESTURESCONFIGPAGE_H

#include "configpage.h"
#include "../../shortcutbarsettings.h"

#include <QHash>
#include <QStringList>

class QComboBox;
class QLabel;
class QListWidget;

/**
	@brief The gesture ring as an editor: one slot per direction, clockwise
	from the top. Click a slot to choose it; drop a command from the
	command list on a slot to put it there; Delete empties the chosen slot.
*/
class GestureRingEditor : public QWidget
{
		Q_OBJECT

	public:
		explicit GestureRingEditor(QWidget *parent = nullptr);

		void setIds(const QStringList &ids);
		QStringList ids() const { return m_ids; }
		void setDirections(int directions);
		void setDescriptions(const QHash<QString, QString> &descriptions);
		int currentSlot() const { return m_current; }
		void setCurrentSlot(int slot);
		int slotAt(const QPoint &pos) const;
		void place(const QString &id);
		void clearCurrent();

		QSize sizeHint() const override;

	signals:
		void edited();
		void currentSlotChanged(int slot);

	protected:
		void paintEvent(QPaintEvent *event) override;
		void mousePressEvent(QMouseEvent *event) override;
		void keyPressEvent(QKeyEvent *event) override;
		void dragEnterEvent(QDragEnterEvent *event) override;
		void dragMoveEvent(QDragMoveEvent *event) override;
		void dropEvent(QDropEvent *event) override;

	private:
		QString idAt(int slot) const;
		QPointF slotCenter(int slot) const;

		QStringList m_ids;
		QHash<QString, QString> m_descriptions;
		int m_directions = 8;
		int m_current = 0;
};

/**
	@brief The GesturesConfigPage class
	Choose 4 or 8 directions for the mouse gesture ring, and which command
	sits in each direction for each selection context. A context the user
	does not edit keeps following the shortcut bar. Saved by applyConf().
*/
class GesturesConfigPage : public ConfigPage
{
		Q_OBJECT

	public:
		explicit GesturesConfigPage(QWidget *parent = nullptr);

		void applyConf() override;
		QString title() const override;
		QIcon icon() const override;

	private:
		void showContext();
		void placeSelected();
		void followBar();
		void updateState();

		QComboBox *m_directions;
		QComboBox *m_context;
		QListWidget *m_available;
		GestureRingEditor *m_ring;
		QLabel *m_slot_name;
		QLabel *m_state;
		ShortcutBarSettings::Context m_shown = ShortcutBarSettings::Canvas;
		QHash<int, QStringList> m_pending;
		QHash<int, bool> m_custom;
		QHash<QString, QString> m_descriptions;
};

#endif // GESTURESCONFIGPAGE_H
