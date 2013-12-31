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
#ifndef CONDUCTOR_H
#define CONDUCTOR_H
#include <QtGui>
#include "terminal.h"
#include "conductorprofile.h"
#include "conductorproperties.h"
#include "qetdiagrameditor.h"
class ConductorSegment;
class ConductorTextItem;
class Element;
typedef QPair<QPointF, Qt::Corner> ConductorBend;
typedef QHash<Qt::Corner, ConductorProfile> ConductorProfilesGroup;
/**
	This class represents a conductor, i.e. a wire between two element
	terminals.
*/
class Conductor : public QObject, public QGraphicsPathItem {
	
	Q_OBJECT
	
	// constructors, destructor
	public:
	Conductor(Terminal *, Terminal *, Diagram * = 0);
	virtual ~Conductor();
	
	private:
	Conductor(const Conductor &);
	
	// attributes
	public:
	enum { Type = UserType + 1001 };
	enum Highlight { None, Normal, Alert };
	
	/// First terminal the wire is attached to
	Terminal *terminal1;
	/// Second terminal the wire is attached to
	Terminal *terminal2;
	
	// methods
	public:
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a
		Conductor.
		@return the QGraphicsItem type
	*/
	virtual int type() const { return Type; }
	void destroy();
	/// @return true if this conductor is destroyed
	bool isDestroyed() const { return(destroyed_); }
	Diagram *diagram() const;
	ConductorTextItem *textItem() const;
	void updatePath(const QRectF & = QRectF());
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	QRectF boundingRect() const;
	virtual QPainterPath shape() const;
	virtual qreal nearDistance() const;
	virtual QPainterPath nearShape() const;
	virtual QPainterPath variableShape(const qreal &) const;
	virtual bool isNearConductor(const QPointF &);
	qreal length();
	ConductorSegment *middleSegment();
	bool containsPoint(const QPointF &) const;
	QString text() const;
	void setText(const QString &);
	static bool valideXml(QDomElement &);
	bool fromXml(QDomElement &);
	QDomElement toXml(QDomDocument &, QHash<Terminal *, int> &) const;
	const QList<ConductorSegment *> segmentsList() const;
	void setProperties(const ConductorProperties &);
	ConductorProperties properties() const;
	void setProfile(const ConductorProfile &, Qt::Corner);
	ConductorProfile profile(Qt::Corner) const;
	void setProfiles(const ConductorProfilesGroup &);
	ConductorProfilesGroup profiles() const;
	void readProperties();
	void adjustTextItemPosition();
	virtual Highlight highlight() const;
	virtual void setHighlighted(Highlight);
	void autoText();
	QSet<Conductor *> relatedPotentialConductors(QList <Terminal *> *t_list=0);
	QETDiagramEditor* diagramEditor() const;
	
	public slots:
	void displayedTextChanged();
	
	protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *);
	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *);
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *);
	virtual QVariant itemChange(GraphicsItemChange, const QVariant &);
	
	private:
	/// Functional properties
	ConductorProperties properties_;
	/// Whether this conductor is still valid
	bool destroyed_;
	/// Text input for non simple, non-singleline conductors
	ConductorTextItem *text_item;
	/// Segments composing the conductor
	ConductorSegment *segments;
	/// Attributs related to mouse interaction
	QPointF press_point;
	bool moving_point;
	bool moving_segment;
	int moved_point;
	qreal previous_z_value;
	ConductorSegment *moved_segment;
	QPointF before_mov_text_pos_;
	/// Whether the conductor was manually modified by users
	bool modified_path;
	/// Whether the current profile should be saved as soon as possible
	bool has_to_save_profile;
	/// conductor profile: "photography" of what the conductor is supposed to look
	/// like - there is one profile per kind of traject
	ConductorProfilesGroup conductor_profiles;
	/// QPen et QBrush objects used to draw conductors
	static QPen conductor_pen;
	static QBrush conductor_brush;
	static QBrush square_brush;
	static bool pen_and_brush_initialized;
	/// Scale factor to render square used to move segments
	qreal segments_squares_scale_;
	/// Define whether and how the conductor should be highlighted
	Highlight must_highlight_;
	
	private:
	void segmentsToPath();
	void saveProfile(bool = true);
	void generateConductorPath(const QPointF &, QET::Orientation, const QPointF &, QET::Orientation);
	void updateConductorPath(const QPointF &, QET::Orientation, const QPointF &, QET::Orientation);
	uint segmentsCount(QET::ConductorSegmentType = QET::Both) const;
	QList<QPointF> segmentsToPoints() const;
	QSet<Conductor *> relatedConductors() const;
	QList<ConductorBend> bends() const;
	QList<QPointF> junctions() const;
	void pointsToSegments(QList<QPointF>);
	bool hasClickedOn(QPointF, QPointF) const;
	void calculateTextItemPosition();
	Qt::Corner currentPathType() const;
	void deleteSegments();
	static int getCoeff(const qreal &, const qreal &);
	static int getSign(const qreal &);
	QHash<ConductorSegmentProfile *, qreal> shareOffsetBetweenSegments(const qreal &offset, const QList<ConductorSegmentProfile *> &, const qreal & = 0.01) const;
	static QPointF extendTerminal(const QPointF &, QET::Orientation, qreal = 9.0);
	static qreal conductor_bound(qreal, qreal, qreal, qreal = 0.0);
	static qreal conductor_bound(qreal, qreal, bool);
	static Qt::Corner movementType(const QPointF &, const QPointF &);
	static QPointF movePointIntoPolygon(const QPointF &, const QPainterPath &);
	Terminal * relatedPotentialTerminal (Terminal *);
};
#endif
