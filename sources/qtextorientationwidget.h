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
#ifndef Q_TEXT_ORIENTATION_WIDGET_H
#define Q_TEXT_ORIENTATION_WIDGET_H
#include <QtGui>
/**
	Cette classe permet de representer graphiquement l'orientation d'un texte.
*/
class QTextOrientationWidget : public QWidget {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	QTextOrientationWidget(QWidget * = 0);
	virtual ~QTextOrientationWidget();
	private:
	QTextOrientationWidget(const QTextOrientationWidget &);
	QTextOrientationWidget &operator=(const QTextOrientationWidget &);
	
	// methodes publiques
	public:
	double orientation() const;
	void setFont(const QFont &);
	QFont font() const;
	void setDisplayText(bool);
	bool textDisplayed() const;
	void setUsableTexts(const QStringList &);
	QStringList usableTexts() const;
	bool isReadOnly() const;
	void setReadOnly(bool);
	
	// slots publics
	public slots:
	void setOrientation(const double &);
	
	protected:
	virtual QSize sizeHint () const;
	int heightForWidth(int) const;
	virtual void paintEvent(QPaintEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	
	// signaux
	signals:
	/**
		Signal emis lorsque l'utilisateur specifie une orientation en cliquant
		sur le widget
	*/
	void orientationChanged(double);
	
	// attributs prives
	private:
	/// Intervalle entre les petits angles privilegies, en degres
	double squares_interval_;
	/// angle represente
	double current_orientation_;
	/// Booleen indiquant s'il faut afficher ou non un texte
	bool display_text_;
	/// Police utilisee pour le texte affiche
	QFont text_font_;
	/// Hash associant les textes disponible a leur longueur en pixels
	QHash<QString, qreal> text_size_hash_;
	/// Angle specifique a mettre en valeur
	double highlight_angle_;
	/// Booleen indiquant s'il faut mettre en valeur un des angles
	bool must_highlight_angle_;
	/// Booleen indiquant si le widget est en mode "lecture seule" ou non
	bool read_only_;
	
	// methodes privees
	private:
	QString getMostUsableStringForRadius(const qreal &);
	void generateTextSizeHash();
	bool positionIsASquare(const QPointF &, double * = 0);
};
#endif
