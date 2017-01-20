/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "qtextorientationwidget.h"

/**
	Constructeur
	Par defaut, ce widget met en valeur les angles multiples de 45 degres
	et presente un texte oriente a 0 degre, avec la police par defaut de
	l'application. Le texte affiche est 
	@param parent Widget parent
*/
QTextOrientationWidget::QTextOrientationWidget(QWidget *parent) :
	QWidget(parent),
	squares_interval_(45.0),
	current_orientation_(0.0),
	display_text_(true),
	must_highlight_angle_(false),
	read_only_(false)
{
	// chaines par defaut
	text_size_hash_.insert(tr("Ex.",     "Short example string"),  -1);
	text_size_hash_.insert(tr("Exemple", "Longer example string"), -1);
	
	// definit la politique de gestion de la taille de ce widget :
	// on prefere la sizeHint()
	QSizePolicy size_policy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	// on souhaite conserver le rapport entre sa hauteur et sa largeur
	size_policy.setHeightForWidth(true);
	setSizePolicy(size_policy);
	
	// suivi de la souris : permet de recevoir les evenements relatifs aux
	// mouvement de la souris sans que l'utilisateur n'ait a cliquer
	setMouseTracking(true);
}

/**
	Destructeur
*/
QTextOrientationWidget::~QTextOrientationWidget() {
}

/**
	@param angle la nouvelle orientation / le nouvel angle selectionne(e)
	0 degre correspond a un texte horizontal, de gauche a droite
	90 degres correspondent a un texte vertical de haut en bas
*/
void QTextOrientationWidget::setOrientation(const double &angle) {
	current_orientation_ = angle;
	update();
}

/**
	@return l'orientation / l'angle actuellement selectionne(e)
	0 degre correspond a un texte horizontal, de gauche a droite
	90 degres correspondent a un texte vertical de haut en bas
*/
double QTextOrientationWidget::orientation() const {
	return(current_orientation_);
}

/**
	Definit la police de caracteres a utiliser pour le texte affiche
	@param font Une police de caracteres
*/
void QTextOrientationWidget::setFont(const QFont &font) {
	text_font_ = font;
	
	// invalide le cache contenant les longueurs des textes a disposition
	foreach(QString text, text_size_hash_.keys()) {
		text_size_hash_[text] = -1;
	}
}

/**
	@return la police utilisee pour le texte affiche
*/
QFont QTextOrientationWidget::font() const {
	return(text_font_);
}

/**
	@param display_text true pour afficher un texte, false sinon
*/
void QTextOrientationWidget::setDisplayText(bool display_text) {
	display_text_ = display_text;
}

/**
	@return la police utilisee pour le texte affiche
*/
bool QTextOrientationWidget::textDisplayed() const {
	return(display_text_);
}

/**
	@param texts_list Une liste de chaines de caracteres utilisables par le
	widget afin d'afficher un texte en guise d'exemple. Le widget choisit la
	chaine la plus appropriee en fonction de sa taille.
	Note : la liste fournie ne doit pas etre vide. Utilisez setDisplayText si
	vous ne voulez plus afficher de texte.
*/
void QTextOrientationWidget::setUsableTexts(const QStringList &texts_list) {
	if (texts_list.isEmpty()) return;
	
	// on oublie les anciennes chaines
	foreach(QString text, text_size_hash_.keys()) {
		// il faut oublier les anciennes chaines
		if (!texts_list.contains(text)) {
			text_size_hash_.remove(text);
		}
	}
	
	// on ajoute les nouvelles, sans les calculer (on met -1 en guise de longueur)
	foreach(QString text, texts_list) {
		if (!text_size_hash_.contains(text)) {
			text_size_hash_[text] = -1;
		}
	}
}

/**
	@return la liste des chaines dont le widget dispose pour afficher un texte
*/
QStringList QTextOrientationWidget::usableTexts() const {
	return(text_size_hash_.keys());
}

/**
	@return true si le widget est en mode "lecture seule", false sinon
*/
bool QTextOrientationWidget::isReadOnly() const {
	return(read_only_);
}

/**
	@param ro true pour passer le widget en mode "lecture seule", false sinon
*/
void QTextOrientationWidget::setReadOnly(bool ro) {
	read_only_ = ro;
}

/**
	@return la taille recommandee pour ce widget
*/
QSize QTextOrientationWidget::sizeHint() const {
	return(QSize(50, 50));
}

/**
	@param w une largeur donnee
	@return la hauteur preferee pour une largeur donnee
	Pour ce widget : retourne la largeur fournie afin de maintenir le widget carre
*/
int QTextOrientationWidget::heightForWidth(int w) const {
	return(w);
}

/**
	Effectue le rendu du widget
	@param event Evenement decrivant la demande de rendu du widget
*/
void QTextOrientationWidget::paintEvent(QPaintEvent *event) {
	Q_UNUSED(event);
	
	// rectangle de travail avec son centre et son rayon
	QRect drawing_rectangle(QPoint(0, 0), size());
	drawing_rectangle.adjust(5, 5, -5, -5);
	
	QPointF drawing_rectangle_center(drawing_rectangle.center());
	qreal drawing_rectangle_radius = drawing_rectangle.width() / 2.0;
	
	QPainter p;
	p.begin(this);
	
	p.setRenderHint(QPainter::Antialiasing,     true);
	p.setRenderHint(QPainter::TextAntialiasing, true);
	
	// cercle gris a fond jaune
	p.setPen(QPen(QBrush(QColor("#9FA8A8")), 2.0));
	p.setBrush(QBrush(QColor("#ffffaa")));
	p.drawEllipse(drawing_rectangle);
	
	// ligne rouge indiquant l'angle actuel
	p.setPen(QPen(QBrush(Qt::red), 1.0));
	p.translate(drawing_rectangle_center);
	p.rotate(current_orientation_);
	p.drawLine(QLineF(QPointF(), QPointF(drawing_rectangle_radius, 0.0)));
	
	// texte optionnel
	if (display_text_) {
		// determine le texte a afficher
		QString chosen_text = getMostUsableStringForRadius(drawing_rectangle_radius);
		if (!chosen_text.isEmpty()) {
			p.resetTransform();
			p.setPen(Qt::black);
			p.setFont(text_font_);
			p.translate(drawing_rectangle_center);
			p.rotate(current_orientation_);
			p.drawText(QPoint(), chosen_text);
		}
	}
	
	// carres verts a fond vert
	qreal squares_size = size().width() / 15.0;
	qreal square_offset = - squares_size / 2.0;
	QRectF square_qrect = QRect(square_offset, square_offset, squares_size, squares_size);
	p.setPen(Qt::NoPen);
	p.setBrush(QBrush(QColor("#248A34")));
	for (double drawing_angle = 0.0 ; drawing_angle < 360.0 ; drawing_angle += squares_interval_) {
		if (must_highlight_angle_ && highlight_angle_ == drawing_angle && underMouse()) {
			p.setBrush(QBrush(QColor("#43FF5F")));
		}
		p.resetTransform();
		p.translate(drawing_rectangle_center);
		p.rotate(drawing_angle);
		p.translate(drawing_rectangle_radius - 1.0, 0.0);
		p.rotate(-45.0);
		p.drawRect(square_qrect);
		if (must_highlight_angle_ && highlight_angle_ == drawing_angle) {
			p.setBrush(QBrush(QColor("#248A34")));
		}
	}
	
	p.end();
}

/**
	Gere les mouvements de la souris sur ce widget
	@param event Evenement decrivant le mouvement de la souris
*/
void QTextOrientationWidget::mouseMoveEvent(QMouseEvent *event) {
	if (read_only_) return;
	
	bool drawn_angle_hovered = positionIsASquare(event -> localPos(), &highlight_angle_);
	
	if (must_highlight_angle_ != drawn_angle_hovered) {
		must_highlight_angle_ = drawn_angle_hovered;
		update();
	}
}

/**
	Gere les relachements de la souris sur ce widget
	@param event Evenement decrivant le relachement de la souris
*/
void QTextOrientationWidget::mouseReleaseEvent(QMouseEvent *event) {
	if (read_only_) return;
	
	double clicked_angle;
	bool drawn_angle_clicked = positionIsASquare(event -> localPos(), &clicked_angle);
	
	if (drawn_angle_clicked) {
		setOrientation(clicked_angle);
		emit(orientationChanged(clicked_angle));
		must_highlight_angle_ = false;
		update();
	}
}

/**
	@param radius Rayon du cercle qui limitera le rendu du texte
	@return la chaine la plus appropriee en fonction de la taille du widget.
*/
QString QTextOrientationWidget::getMostUsableStringForRadius(const qreal &radius) {
	// s'assure que l'on connait la longueur de chaque texte a disposition
	generateTextSizeHash();
	
	// recupere les longueurs a disposition
	QList<qreal> available_lengths = text_size_hash_.values();
	// trie les longueurs par ordre croissant
	qSort(available_lengths.begin(), available_lengths.end());
	// recherche la position ou l'on insererait le rayon
	QList<qreal>::const_iterator i = qUpperBound(available_lengths, radius);
	
	// la valeur precedent cette position est donc celle qui nous interesse
	if (i == available_lengths.begin()) {
		// nous sommes au debut de la liste - nous ne pouvons donc pas afficher de chaine
		return(QString());
	} else {
		-- i;
		qreal final_length = *i;
		QString final_string = text_size_hash_.keys(final_length).first();
		return(final_string);
	}
}

/**
	S'assure que le hash associant les textes utilisables a leur taille soit
	correctement rempli.
*/
void QTextOrientationWidget::generateTextSizeHash() {
	QFontMetrics font_metrics(text_font_);
	foreach(QString text, text_size_hash_.keys()) {
		if (text_size_hash_[text] == -1) {
			text_size_hash_[text] = font_metrics.boundingRect(text).width();
		}
	}
}

/**
	Determine si une position donnee correspond a un des carres representant un
	angle pertinent.
	@param pos Position donnee
	@param angle_value_ptr Si different de 0, le double pointe par ce parametre
	vaudra l'angle pertinent concerne
*/
bool QTextOrientationWidget::positionIsASquare(const QPointF &pos, double *angle_value_ptr) {
	// rectangle de travail avec son centre et son rayon
	QRect drawing_rectangle(QPoint(0, 0), size());
	drawing_rectangle.adjust(5, 5, -5, -5);
	
	QPointF drawing_rectangle_center(drawing_rectangle.center());
	qreal drawing_rectangle_radius = drawing_rectangle.width() / 2.0;
	
	qreal squares_size = size().width() / 15.0;
	qreal square_offset = - squares_size / 2.0;
	QRectF square_qrect = QRect(square_offset, square_offset, squares_size, squares_size);
	
	for (double drawing_angle = 0.0 ; drawing_angle < 360.0 ; drawing_angle += squares_interval_) {
		QTransform transform = QTransform()
			.translate(drawing_rectangle_center.x(), drawing_rectangle_center.y())
			.rotate(drawing_angle)
			.translate(drawing_rectangle_radius - 1.0, 0.0)
			.rotate(-45.0);
		
		QRectF mapped_rectangle = transform.mapRect(square_qrect);
		if (mapped_rectangle.contains(pos)) {
			if (angle_value_ptr) *angle_value_ptr = drawing_angle;
			return(true);
		}
	}
	
	return(false);
}
