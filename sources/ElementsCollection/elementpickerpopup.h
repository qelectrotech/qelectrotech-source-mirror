/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ELEMENTPICKERPOPUP_H
#define ELEMENTPICKERPOPUP_H

#include "elementslocation.h"
#include "../shortcutbarsettings.h"

#include <QFrame>

class ElementsCollectionWidget;
class QLineEdit;
class QListView;
class QStandardItemModel;
class QLabel;
class QAction;
class QHBoxLayout;
class QListWidget;
class QListWidgetItem;
class QSizeGrip;
class QToolButton;
class QVBoxLayout;

/**
	@brief A cursor-anchored element picker.

	Opens where the mouse is, focused on its search field: type to filter,
	Enter to place, Esc to close. It is a recall tool, as opposed to the
	Collections dock, which is a browsing tool.

	It deliberately does not build its own ElementsCollectionModel. Loading the
	collection is already the slow part of startup and a second copy would
	double it, so the picker asks the dock's widget to run the query -- see
	ElementsCollectionWidget::rankedSearch(). That also means the two always
	agree on results and ranking.
*/
class ElementPickerPopup : public QFrame
{
	Q_OBJECT

	public:
		explicit ElementPickerPopup(ElementsCollectionWidget *source,
					    QWidget *parent = nullptr);

		void popUpAt(const QPoint &global_pos);
		void popUpShortcutBar(const QPoint &global_pos,
				      ShortcutBarSettings::Context context);

	signals:
			/// Emitted when the user picks an element; the popup has closed
		void elementChosen(const ElementsLocation &location);

	protected:
		void keyPressEvent(QKeyEvent *event) override;
		void closeEvent(QCloseEvent *event) override;
		bool eventFilter(QObject *watched, QEvent *event) override;

	private:
		void runSearch();
		void chooseCurrent();
		void showPalette();
		void show(const QPoint &global_pos);
		void setCommands(const QStringList &ids);
		void layoutTiles(int bar_width);
		QAction *commandAction(const QString &id) const;
		QListWidgetItem *barItem(const QString &id, bool icon_only) const;
		void startCustomising();
		void fillCustomising(const QStringList &ids);
		void finishCustomising(bool save);
		void runSymbolSearch();
		void setPickerVisible(bool visible);
		void keepOnScreen(const QPoint &global_pos);
		int loadPaletteDir(const QString &dir_path, const QString &prefix,
				   int depth);

		ElementsCollectionWidget *m_source = nullptr;
		QLineEdit *m_search = nullptr;
		QListView *m_view = nullptr;
		QStandardItemModel *m_model = nullptr;
		QLabel *m_hint = nullptr;
		QWidget *m_commands = nullptr;
		QHBoxLayout *m_commands_layout = nullptr;
			/// The bar's tiles, commands and elements, in order
		QList<QToolButton *> m_tiles;
		QVBoxLayout *m_tile_rows = nullptr;
		QSizeGrip *m_grip = nullptr;
			/// The size grip is being dragged, from this x at this width
		bool m_grip_active = false;
		int m_grip_press_x = 0;
		int m_grip_press_width = 0;
			/// Opened as the shortcut bar, as opposed to the plain picker
		bool m_bar_mode = false;
		ShortcutBarSettings::Context m_context = ShortcutBarSettings::Canvas;
			/// Customising the bar in place
		bool m_customising = false;
		QWidget *m_editor = nullptr;
		QListWidget *m_edit_row = nullptr;
		QListWidget *m_edit_available = nullptr;
			/// Element search, for pinning elements to the bar
		QWidget *m_edit_symbols_box = nullptr;
		QLineEdit *m_edit_symbols_search = nullptr;
		QListWidget *m_edit_symbols = nullptr;
		bool m_palette_mode = true;
};

#endif // ELEMENTPICKERPOPUP_H
