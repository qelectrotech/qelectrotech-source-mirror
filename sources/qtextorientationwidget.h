/*
	Copyright 2006-2014 The QElectroTech Team
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
	This class provides a visual representation of a text orientation.
*/
class QTextOrientationWidget : public QWidget {
	Q_OBJECT
	
	// constructors, destructor
	public:
	QTextOrientationWidget(QWidget * = 0);
	virtual ~QTextOrientationWidget();
	private:
	QTextOrientationWidget(const QTextOrientationWidget &);
	QTextOrientationWidget &operator=(const QTextOrientationWidget &);
	
	// methods
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
	
	public slots:
	void setOrientation(const double &);
	
	protected:
	virtual QSize sizeHint () const;
	int heightForWidth(int) const;
	virtual void paintEvent(QPaintEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	
	signals:
	/**
		Signal emitted when users specify an orientation by clicking the widget.
	*/
	void orientationChanged(double);
	
	// attributes
	private:
	/// Interval between commonly used angles (represented by squares), in degrees
	double squares_interval_;
	/// current angle
	double current_orientation_;
	/// Whether to display an example text
	bool display_text_;
	/// Font used to render the example text
	QFont text_font_;
	/// Associate available example texts with their length (in pixels)
	QHash<QString, qreal> text_size_hash_;
	/// Specific angle to be highlighted
	double highlight_angle_;
	/// Whether to highlight a specific angle
	bool must_highlight_angle_;
	/// Whether this widget is read only
	bool read_only_;
	
	private:
	QString getMostUsableStringForRadius(const qreal &);
	void generateTextSizeHash();
	bool positionIsASquare(const QPointF &, double * = 0);
};
#endif
