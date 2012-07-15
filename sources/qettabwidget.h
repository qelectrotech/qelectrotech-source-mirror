/*
	Copyright 2006-2012 Xavier Guerrin
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
	Cette classe se comporte comme un QTabWidget a ceci pres qu'elle utilise
	un QETTAbBar pour gerer ses onglets.
	Elle transmet aussi ses signaux lastTabRemoved() et firstTabInserted().
	@see QETTabBar
*/
class QETTabWidget : public QTabWidget {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	QETTabWidget(QWidget * = 0);
	virtual~QETTabWidget();
	
	private:
	QETTabWidget(const QETTabWidget &);
	
	// methodes
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
	
	// attributs
	private:
	QETTabBar *tab_bar_;
};
#endif
