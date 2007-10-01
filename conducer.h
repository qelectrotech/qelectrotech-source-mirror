#ifndef CONDUCTEUR_H
#define CONDUCTEUR_H
#include <QtGui>
#include "terminal.h"
#include "conducerprofile.h"
#include "diagramtextitem.h"
class ConducerSegment;
class Element;
/**
	Cette classe represente les proprietes specifiques a un conducteur unifilaire
*/
class SingleLineProperties {
	public:
	SingleLineProperties();
	virtual ~SingleLineProperties();
	void setPhasesCount(int);
	unsigned short int phasesCount();
	void draw(QPainter *, QET::ConducerSegmentType, const QRectF &);
	void toXml(QDomDocument &, QDomElement &) const;
	void fromXml(QDomElement &);
	bool hasGround;
	bool hasNeutral;
	private:
	unsigned short int phases;
	void drawGround (QPainter *, QET::ConducerSegmentType, QPointF, qreal);
	void drawNeutral(QPainter *, QET::ConducerSegmentType, QPointF, qreal);
};

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
	/// caracteristiques des conducteurs unifilaires
	SingleLineProperties singleLineProperties;
	
	// methodes
	public:
	virtual int type() const { return Type; }
	void destroy();
	bool isDestroyed() const { return(destroyed); }
	Diagram *diagram() const;
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
	void setProfile(const ConducerProfile &);
	void setSingleLine(bool);
	bool isSingleLine() const;
	
	protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *);
	
	private:
	/// booleen indiquant si le fil est encore valide
	bool destroyed;
	/// booleen indiquant le mode du conducteur : unifilaire ou non
	bool is_single_line;
	/// champ de texte editable pour les conducteurs non unifilaires
	DiagramTextItem *text_item;
	/// Segments composant le conducteur
	ConducerSegment *segments;
	/// Attributs lies aux manipulations a la souris
	QPointF press_point;
	bool moving_point;
	bool moving_segment;
	int moved_point;
	qreal previous_z_value;
	ConducerSegment *moved_segment;
	/// booleen indiquant si le conducteur a ete modifie manuellement par l'utilisateur
	bool modified_path;
	/// booleen indiquant s'il faut sauver le profil courant au plus tot
	bool has_to_save_profile;
	/// profil du conducteur : "photo" de ce a quoi le conducteur doit ressembler
	ConducerProfile conducer_profile;
	/// QPen et QBrush utilises pour dessiner les conducteurs
	static QPen conducer_pen;
	static QBrush conducer_brush;
	static bool pen_and_brush_initialized;
	
	private:
	void segmentsToPath();
	void saveProfile(bool = true);
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
