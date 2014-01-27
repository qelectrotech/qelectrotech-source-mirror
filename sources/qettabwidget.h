/*
	Copyright 2006-2013 The QElectroTech Team
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
#ifndef QET_TAB_WIDGET_H
#define QET_TAB_WIDGET_H
#include <QTabWidget>
class QETTabBar;
/**
	This class behaves like a QTabWidget except it uses a QETTAbBar to manage its
	tabs. It also transmits the lastTabRemoved() and firstTabInserted() signals.
	@see QETTabBar
*/
class QETTabWidget : public QTabWidget {
	Q_OBJECT
	
	// constructors, destructor
	public:
	QETTabWidget(QWidget * = 0);
	virtual~QETTabWidget();
	
	private:
	QETTabWidget(const QETTabWidget &);
	
	// methods
	public:
	void setMovable(bool);
	bool isMovable() const;
	void moveTab(int, int);
	QETTabBar *tabBar() const;
	
	protected:
	void wheelEvent(QWheelEvent *);
	
	signals:
	void lastTabRemoved();
	void firstTabInserted();
	void tabMoved(int, int);
	void tabDoubleClicked(int);
	
	// attributes
	private:
	QETTabBar *tab_bar_;
};
#endif
