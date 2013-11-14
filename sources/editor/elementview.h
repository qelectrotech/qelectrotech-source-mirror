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
#ifndef ELEMENT_VIEW_H
#define ELEMENT_VIEW_H
#include <QGraphicsView>
#include "elementscene.h"

/**
	This class provides a widget to render an ElementScene instance, i.e. the
	edition class for electrical elements.
*/
class ElementView : public QGraphicsView {
	Q_OBJECT
	friend class PastePartsCommand;
	
	// constructors, destructor
	public:
	ElementView(ElementScene *, QWidget * = 0);
	virtual ~ElementView();
	
	private:
	ElementView(const ElementView &);
	
	// methods
	public:
	ElementScene *scene() const;
	void setScene(ElementScene *);
	QRectF viewedSceneRect() const;
	void contextMenuEvent(QContextMenuEvent *event);
	
	protected:
	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void wheelEvent(QWheelEvent *);
	virtual void drawBackground(QPainter *, const QRectF &);
	
	private:
	QRectF applyMovement(const QRectF &, const QPointF &);
	
	public slots:
	void setVisualisationMode();
	void setSelectionMode();
	void zoomIn();
	void zoomOut();
	void zoomFit();
	void zoomReset();
	void adjustSceneRect();
	void resetSceneRect ();
	void cut();
	void copy();
	void paste();
	void pasteInArea();
	
	signals:
	/// Signal emitted after the mode changed
	void modeChanged();
	
	private slots:
	void getPasteArea(const QRectF &);
	ElementContent pasteAreaDefined(const QRectF &);
	ElementContent paste(const QPointF &);
	ElementContent paste(const QDomDocument &, const QPointF &);
	ElementContent pasteWithOffset(const QDomDocument &);
	
	// attributes
	private:
	ElementScene *scene_;
	QString to_paste_in_area_;
	int offset_paste_count_;
	QPointF start_top_left_corner_;
	QPointF reference_view_;
	QPointF center_view_;
	bool is_moving_view_;               ///< Indicate whether the visualisation mode has been enabled due to mouse/keyboard interactions
};
#endif
