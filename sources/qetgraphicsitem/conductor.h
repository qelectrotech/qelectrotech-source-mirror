/*
	Copyright 2006-2015 The QElectroTech Team
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

#include "conductorproperties.h"
#include <QGraphicsPathItem>
#include "QetGraphicsItemModeler/qetgraphicshandlerutility.h"

class ConductorProfile;
class ConductorSegmentProfile;
class Diagram;
class Terminal;
class ConductorSegment;
class ConductorTextItem;
class Element;
class QETDiagramEditor;
typedef QPair<QPointF, Qt::Corner> ConductorBend;
typedef QHash<Qt::Corner, ConductorProfile> ConductorProfilesGroup;
/**
	This class represents a conductor, i.e. a wire between two element
	terminals.
*/
class Conductor : public QObject, public QGraphicsPathItem
{
	Q_OBJECT

	Q_PROPERTY(QPointF pos READ pos WRITE setPos)
	Q_PROPERTY(int animPath READ fakePath WRITE updatePathAnimate)
	Q_PROPERTY(ConductorProperties properties READ properties WRITE setProperties)

	signals:
		void propertiesChange();
	
		// constructors, destructor
	public:
		Conductor(Terminal *, Terminal *);
		virtual ~Conductor();

		bool isValid() const;
	
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
	Diagram *diagram() const;
	ConductorTextItem *textItem() const;
	void updatePath(const QRectF & = QRectF());

	//This method do nothing, it's only made to be used with Q_PROPERTY
	//It's used to anim the path when is change
	void updatePathAnimate(const int = 1) {updatePath();}
	int fakePath() {return 1;}

	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	QRectF boundingRect() const;
	virtual QPainterPath shape() const;
	virtual QPainterPath nearShape() const;
	qreal length() const;
	ConductorSegment *middleSegment();
	QPointF posForText(Qt::Orientations &flag);
	QString text() const;
	void setText(const QString &);

	public:
		static bool valideXml (QDomElement &);
		bool        fromXml   (QDomElement &);
		QDomElement toXml     (QDomDocument &, QHash<Terminal *, int> &) const;
	private:
		bool pathFromXml(const QDomElement &);

	public:
		QVector <QPointF> handlerPoints() const;
		const QList<ConductorSegment *> segmentsList() const;
		void setProperties(const ConductorProperties &properties);
		ConductorProperties properties() const;
		void setProfile(const ConductorProfile &, Qt::Corner);
		ConductorProfile profile(Qt::Corner) const;
		void setProfiles(const ConductorProfilesGroup &);
		ConductorProfilesGroup profiles() const;
		void calculateTextItemPosition();
		virtual Highlight highlight() const;
		virtual void setHighlighted(Highlight);
		QSet<Conductor *> relatedPotentialConductors(const bool all_diagram = true, QList <Terminal *> *t_list=0);
		QETDiagramEditor* diagramEditor() const;
		void editProperty ();
	
	public slots:
	void displayedTextChanged();
	
	protected:
		virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
		virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
		virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
		virtual QVariant itemChange(GraphicsItemChange, const QVariant &);
	
	bool				bMouseOver;

	private:
		QetGraphicsHandlerUtility m_handler;
	/// Functional properties
	ConductorProperties properties_;
	/// Text input for non simple, non-singleline conductors
	ConductorTextItem *text_item;
	/// Segments composing the conductor
	ConductorSegment *segments;
	/// Attributs related to mouse interaction
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
	static bool pen_and_brush_initialized;
	/// Define whether and how the conductor should be highlighted
	Highlight must_highlight_;
	bool m_valid;
	
	private:
	void segmentsToPath();
	void saveProfile(bool = true);
	void generateConductorPath(const QPointF &, Qet::Orientation, const QPointF &, Qet::Orientation);
	void updateConductorPath(const QPointF &, Qet::Orientation, const QPointF &, Qet::Orientation);
	uint segmentsCount(QET::ConductorSegmentType = QET::Both) const;
	QList<QPointF> segmentsToPoints() const;
	QList<ConductorBend> bends() const;
	QList<QPointF> junctions() const;
	void pointsToSegments(QList<QPointF>);
	Qt::Corner currentPathType() const;
	void deleteSegments();
	static int getCoeff(const qreal &, const qreal &);
	static int getSign(const qreal &);
	QHash<ConductorSegmentProfile *, qreal> shareOffsetBetweenSegments(const qreal &offset, const QList<ConductorSegmentProfile *> &, const qreal & = 0.01) const;
	static QPointF extendTerminal(const QPointF &, Qet::Orientation, qreal = 9.0);
	static Qt::Corner movementType(const QPointF &, const QPointF &);
	static QPointF movePointIntoPolygon(const QPointF &, const QPainterPath &);
};

Conductor * longuestConductorInPotential (Conductor *conductor, bool all_diagram = false);
QList <Conductor *> relatedConductors (const Conductor *conductor);


//return true if @a is between or at @b and @c.
template <typename T>
bool isBetween (const T a, const T b, const T c) {
	return (b <= c)? (a >= b && a <= c) : (a <= b && a >= c);
}

#endif
