#ifndef CONDUCTEUR_H
#define CONDUCTEUR_H
#include <QtGui>
#include "terminal.h"
#include "conducerprofile.h"
class ConducerSegment;
class Element;
/**
	Cette classe represente un conducteur. Un conducteur relie deux bornes d'element.
*/
class Conducer : public QGraphicsPathItem {
	
	// constructeurs, destructeur
	public:
	Conducer(Terminal *, Terminal *, Element * = 0, QGraphicsScene * = 0);
	virtual ~Conducer();
	
	private:
	Conducer(const Conducer &);
	
	// attributs
	public:
	enum { Type = UserType + 1001 };
	
	///Premiere borne a laquelle le fil est rattache
	Terminal *terminal1;
	///Deuxieme borne a laquelle le fil est rattache
	Terminal *terminal2;
	
	private:
	/// booleen indiquant si le fil est encore valide
	bool destroyed;
	QGraphicsTextItem *text_item;
	ConducerSegment *segments;
	QPointF press_point;
	bool moving_point;
	bool moving_segment;
	int moved_point;
	qreal previous_z_value;
	ConducerSegment *moved_segment;
	bool modified_path;
	bool has_to_save_profile;
	ConducerProfile conducer_profile;
	static QPen conducer_pen;
	static QBrush conducer_brush;
	static bool pen_and_brush_initialized;
	
	// methodes
	public:
	virtual int type() const { return Type; }
	void destroy();
	bool isDestroyed() const { return(destroyed); }
	void updateWithNewPos(const QRectF &, const Terminal *, const QPointF &);
	void update(const QRectF & = QRectF());
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	QRectF boundingRect() const;
	virtual QPainterPath shape() const;
	qreal length();
	ConducerSegment *middleSegment();
	static bool valideXml(QDomElement &);
	bool fromXml(QDomElement &);
	QDomElement toXml(QDomDocument &, QHash<Terminal *, int> &) const;
	const QList<ConducerSegment *> segmentsList() const;
	
	protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	void hoverMoveEvent(QGraphicsSceneHoverEvent *);
	
	private:
	void segmentsToPath();
	void saveProfile();
	void priv_calculeConducer(const QPointF &, QET::Orientation, const QPointF &, QET::Orientation);
	void priv_modifieConducer(const QPointF &, QET::Orientation, const QPointF &, QET::Orientation);
	uint nbSegments(QET::ConducerSegmentType = QET::Both) const;
	QList<QPointF> segmentsToPoints() const;
	void pointsToSegments(QList<QPointF>);
	bool hasClickedOn(QPointF, QPointF) const;
	void calculateTextItemPosition();
	static int getCoeff(const qreal &, const qreal &);
	static int getSign(const qreal &);
	QHash<ConducerSegmentProfile *, qreal> shareOffsetBetweenSegments(const qreal &offset, const QList<ConducerSegmentProfile *> &, const qreal & = 0.01) const;
	static QPointF extendTerminal(const QPointF &, QET::Orientation, qreal = 12.0);
	static qreal conducer_bound(qreal, qreal, qreal, qreal = 0.0);
	static qreal conducer_bound(qreal, qreal, bool);
};
#endif
