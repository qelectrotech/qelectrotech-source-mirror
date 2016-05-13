/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef QET_MAIN_WINDOW_H
#define QET_MAIN_WINDOW_H
#include <QMainWindow>
#include <QHash>
/**
	This is the base class for the main top-level windows within
	QElectroTech.
*/
class QETMainWindow : public QMainWindow {
	Q_OBJECT
	
	// constructor, destructor
	public:
	QETMainWindow(QWidget * = 0, Qt::WindowFlags = 0);
	virtual ~QETMainWindow();
	
	// methods
	protected:
	void initCommonActions();
	void initCommonMenus();
	void insertMenu(QMenu *, QMenu *, bool = true);
	QAction *actionForMenu(QMenu *);
	
	protected:
	virtual bool event(QEvent *);
	virtual void dragEnterEvent(QDragEnterEvent *e);
	virtual void dropEvent(QDropEvent *e);
	virtual void firstActivation(QEvent *);
	
	// slots
	public slots:
	void toggleFullScreen();
	void updateFullScreenAction();
	void checkToolbarsmenu();
	
	// attributes
	protected:
	QAction *configure_action_;              ///< Launch the QElectroTech configuration dialog
	QAction *fullscreen_action_;             ///< Toggle full screen
	QAction *whatsthis_action_;              ///< Toggle "What's this" mode
	QAction *about_qet_;                     ///< Launch the "About QElectroTech" dialog
	QAction *about_qt_;                      ///< launch the "About Qt" dialog
	QMenu *settings_menu_;                   ///< Settings menu
	QMenu *help_menu_;                       ///< Help menu
	QMenu *display_toolbars_;                ///< Show/hide toolbars/docks
	QHash<QMenu *, QAction *> menu_actions_; ///< Store actions retrieved when inserting menus
	bool first_activation_;                  ///< Used to detect whether the window is activated for the first time
};
#endif
