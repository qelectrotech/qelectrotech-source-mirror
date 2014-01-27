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
#include "qettabbar.h"
#include <QWheelEvent>

/**
	Constructeur
	@param parent QWidget parent
*/
QETTabBar::QETTabBar(QWidget *parent) :
	QTabBar(parent),
	no_more_tabs_(true),
	movable_tabs_(false),
	moved_tab_(-1)
{
}

/**
	Destructeur
*/
QETTabBar::~QETTabBar() {
}

/**
	Active l'onglet suivant si possible
*/
void QETTabBar::activateNextTab() {
	int count_ = count();
	if (count_ < 2) return;
	
	int current_index = currentIndex();
	if (current_index == count_ - 1) {
		setCurrentIndex(0);
	} else {
		setCurrentIndex(current_index + 1);
	}
}

/**
	Active l'onglet precedent si possible
*/
void QETTabBar::activatePreviousTab() {
	int count_ = count();
	if (count_ < 2) return;
	
	int current_index = currentIndex();
	if (!current_index) {
		setCurrentIndex(count_ - 1);
	} else {
		setCurrentIndex(current_index - 1);
	}
}

/**
	@param movable true pour que les onglets soient deplacables, false sinon
*/
void QETTabBar::setMovable(bool movable) {
#if QT_VERSION < 0x040500
	movable_tabs_ = movable;
#else
	QTabBar::setMovable(movable);
#endif
}

/**
	@return true si les onglets sont deplacables, false sinon
*/
bool QETTabBar::isMovable() const {
#if QT_VERSION < 0x040500
	return(movable_tabs_);
#else
	return(QTabBar::isMovable());
#endif
}

/**
	@return true si les onglets sont dessines de maniere verticale
*/
bool QETTabBar::isVertical() const {
	int current_shape = shape();
	return(
		current_shape == QTabBar::RoundedEast    ||
		current_shape == QTabBar::RoundedWest    ||
		current_shape == QTabBar::TriangularEast ||
		current_shape == QTabBar::TriangularWest
	);
}

/**
	@return true si les onglets sont dessines de maniere horizontale
*/
bool QETTabBar::isHorizontal() const {
	int current_shape = shape();
	return(
		current_shape == QTabBar::RoundedNorth    ||
		current_shape == QTabBar::RoundedSouth    ||
		current_shape == QTabBar::TriangularNorth ||
		current_shape == QTabBar::TriangularSouth
	);
}

/**
	Gere l'insertion d'un onglet
	@param index indice de l'onglet insere
*/
void QETTabBar::tabInserted(int index) {
	QTabBar::tabInserted(index);
	if (no_more_tabs_) {
		emit(firstTabInserted());
	}
	no_more_tabs_ = false;
}

/**
	Gere le retrait d'un onglet
	@param index indice de l'onglet enleve
*/
void QETTabBar::tabRemoved(int index) {
	QTabBar::tabRemoved(index);
	if (!count()) {
		emit(lastTabRemoved());
		no_more_tabs_ = true;
	}
}

/**
	Gere les evenements rollette sur cette barre d'onglets
	@param event Evenement rollette
*/
void QETTabBar::wheelEvent(QWheelEvent *event) {
	int num_degrees = event -> delta() / 8;
	int num_steps = qAbs(num_degrees / 15);
	
	if (num_degrees <= 0) {
		// passe a l'onglet suivant
		for (int i = 0 ; i < num_steps ; ++ i) activateNextTab();
	} else {
		// passe a l'onglet precedent
		for (int i = 0 ; i < num_steps ; ++ i) activatePreviousTab();
	}
	event -> accept();
}

/**
	@param event Objet decrivant l'evenement souris
*/
void QETTabBar::mousePressEvent(QMouseEvent *event) {
	QTabBar::mousePressEvent(event);
	if (movable_tabs_) {
		if (event -> button() == Qt::LeftButton) {
			// retient l'onglet deplace et la position a laquelle le mouvement debute
			moved_tab_ = tabForPressedPosition(event -> pos());
			press_point_ = event -> pos();
		}
	}
}

/**
	@param event Objet decrivant l'evenement souris
*/
void QETTabBar::mouseMoveEvent(QMouseEvent *event) {
	QTabBar::mouseMoveEvent(event);
	
	// gere le deplacement d'onglets si celui-ci est active
	if (movable_tabs_ && moved_tab_ != -1) {
		// repere l'onglet de "destination"
		int dest_tab = tabForMovedPosition(event -> pos());
		
		// verifie s'il faut deplacer l'onglet puis le deplace
		if (mustMoveTab(moved_tab_, dest_tab, event -> pos())) {
			moveTab(moved_tab_, dest_tab);
			moved_tab_ = dest_tab;
			event -> accept();
		}
	}
}

/**
	@param event Objet decrivant l'evenement souris
*/
void QETTabBar::mouseReleaseEvent(QMouseEvent *event) {
	QTabBar::mouseReleaseEvent(event);
	moved_tab_ = -1;
}

/**
	@param event Objet decrivant l'evenement souris
*/
void QETTabBar::mouseDoubleClickEvent(QMouseEvent *event) {
	QTabBar::mouseDoubleClickEvent(event);
	int clicked_tab = tabForPressedPosition(event -> pos());
	emit(tabDoubleClicked(clicked_tab));
}

/**
	@param src_tab Index de l'onglet de depart
	@param dst_tab Index de l'onglet de destination
	@param pos Position de la souris dans le cadre du deplacement de l'onglet
	@return true s'il faut deplacer l'onglet src_tab a la place de l'onglet
	dst_tab.
	Cette methode 
*/
bool QETTabBar::mustMoveTab(int src_tab, int dst_tab, const QPoint &pos) const {
	// les onglets sources et cibles doivent etre valides et differents
	if (src_tab == -1 || dst_tab == -1) return(false);
	if (src_tab == dst_tab) return(false);
	
	/*
		A ce stade, le deplacement est possible mais selon la position du
		pointeur de la souris, on peut assister a des deplacements prematures
		d'onglets, rendant l'interface plus difficilement utilisable.
		On s'assure donc que le curseur est assez "loin" pour eviter ces
		problemes.
	*/
	// recupere les rectangles representant les onglets
	QRect source_rect = tabRect(src_tab);
	QRect target_rect = tabRect(dst_tab);
	
	if (isHorizontal()) {
		if (layoutDirection() == Qt::LeftToRight && source_rect.x() < target_rect.x()) {
			source_rect.moveRight(target_rect.right());
		} else {
			source_rect.moveLeft(target_rect.left());
		}
	} else {
		if (source_rect.y() < target_rect.y()) {
			source_rect.moveBottom(target_rect.bottom());
		} else {
			source_rect.moveTop(target_rect.top());
		}
	}
	return(posMatchesTabRect(source_rect, pos));
}

/**
	Deplace un onglet.
	@param src_tab Index de l'onglet de depart
	@param dst_tab Index de l'onglet de destination
*/
void QETTabBar::moveTab(int src_tab, int dst_tab) {
#if QT_VERSION < 0x040500
	// sauvegarde les caracteristiques de l'onglet deplace
	QIcon    old_tab_icon      = tabIcon(src_tab);
	QVariant old_tab_data      = tabData(src_tab);
	QString  old_tab_text      = tabText(src_tab);
	QColor   old_tab_textcolor = tabTextColor(src_tab);
	QString  old_tab_tooltip   = tabToolTip(src_tab);
	QString  old_tab_whatsthis = tabWhatsThis(src_tab);
	
	// si la QETTabBar est utilise dans un QTabWidget (ou une classe en
	// derivant), elle lui delegue le deplacement de l'onglet
	if (QTabWidget *qtabwidget = qobject_cast<QTabWidget *>(parent())) {
		QWidget *old_tab_widget = qtabwidget -> widget(src_tab);
		qtabwidget -> removeTab(src_tab);
		qtabwidget -> insertTab(dst_tab, old_tab_widget, old_tab_text);
		qtabwidget -> setCurrentIndex(dst_tab);
	} else {
		removeTab(src_tab);
		insertTab(dst_tab, old_tab_text);
		setCurrentIndex(dst_tab);
	}
	
	// remet en place les caracteristiques de l'onglet deplace
	setTabIcon     (dst_tab, old_tab_icon     );
	setTabData     (dst_tab, old_tab_data     );
	setTabTextColor(dst_tab, old_tab_textcolor);
	setTabToolTip  (dst_tab, old_tab_tooltip  );
	setTabWhatsThis(dst_tab, old_tab_whatsthis);
	
	// signale le deplacement de l'onglet
	emit(tabMoved(src_tab, dst_tab));
#else
	QTabBar::moveTab(src_tab, dst_tab);
#endif
}

/**
	@param pos Position
	@return l'index de l'onglet correspondant a la position pos, ou -1 si aucun
	onglet ne correspond.
*/
int QETTabBar::tabForPressedPosition(const QPoint &pos) {
	for (int tab_index = 0 ; tab_index < count() ; ++ tab_index) {
		if (tabRect(tab_index).contains(pos)) return(tab_index);
	}
	return(-1);
}

/**
	@param pos Position
	@return l'index de l'onglet correspondant a la position pos lors d'un
	deplacement d'onglet, ou -1 si aucun onglet ne correspond. Cette methode ne
	prend en compte que l'abscisse ou que l'ordonnee de la position en fonction
	de l'orientation des onglets.
*/
int QETTabBar::tabForMovedPosition(const QPoint &pos) {
	for (int tab_index = 0 ; tab_index < count() ; ++ tab_index) {
		if (posMatchesTabRect(tabRect(tab_index), pos)) return(tab_index);
	}
	return(-1);
}

/**
	@param rect Un rectangle cense representer un onglet
	@param pos Une position
	@return true si la position pos correspond a ce rectangle.
	Cette methode ne prend en compte que l'abscisse ou que
	l'ordonnee de la position en fonction de l'orientation des onglets.
*/
bool QETTabBar::posMatchesTabRect(const QRect &rect, const QPoint &pos) const {
	if (isVertical()) {
		// les onglets sont disposes de maniere verticale : on prend en compte l'ordonnee
		if (pos.y() >= rect.y() && pos.y() < rect.y() + rect.height()) return(true);
	} else {
		// les onglets sont disposes de maniere horizontale : on prend en compte l'abscisse
		if (pos.x() >= rect.x() && pos.x() < rect.x() + rect.width()) return(true);
	}
	return(false);
}
