#ifndef CONDUCTOR_H
#define CONDUCTOR_H
#include <QtGui>
#include "terminal.h"
#include "conductorprofile.h"
#include "diagramtextitem.h"
class ConductorSegment;
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
	void draw(QPainter *, QET::ConductorSegmentType, const QRectF &);
	void toXml(QDomDocument &, QDomElement &) const;
	void fromXml(QDomElement &);
	bool hasGround;
	bool hasNeutral;
	private:
	unsigned short int phases;
	void drawGround (QPainter *, QET::ConductorSegmentType, QPointF, qreal);
	void drawNeutral(QPainter *, QET::ConductorSegmentType, QPointF, qreal);
};

/**
	Cette classe represente un conducteur. Un conducteur relie deux bornes d'element.
*/
class Conductor : public QGraphicsPathItem {
	
	// constructeurs, destructeur
	public:
	Conductor(Terminal *, Terminal *, Element * = 0, QGraphicsScene * = 0);
	virtual ~Conductor();
	
	private:
	Conductor(const Conductor &);
	
	// attributs
	public:
	enum { Type = UserType + 1001 };
	enum ConductorType { Simple, Single, Multi };
	
	/// premiere borne a laquelle le fil est rattache
	Terminal *terminal1;
	/// deuxieme borne a laquelle le fil est rattache
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
	ConductorSegment *middleSegment();
	QString text() const;
	void setText(const QString &);
	static bool valideXml(QDomElement &);
	bool fromXml(QDomElement &);
	QDomElement toXml(QDomDocument &, QHash<Terminal *, int> &) const;
	const QList<ConductorSegment *> segmentsList() const;
	void setProfile(const ConductorProfile &);
	ConductorProfile profile() const;
	ConductorType conductorType() const;
	void setConductorType(ConductorType);
	
	protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *);
	
	private:
	/// booleen indiquant si le fil est encore valide
	bool destroyed;
	/// enum indiquant le mode du conducteur : simple, unifilaire ou non
	ConductorType type_;
	/// champ de texte editable pour les conducteurs non unifilaires
	DiagramTextItem *text_item;
	/// segments composant le conducteur
	ConductorSegment *segments;
	/// attributs lies aux manipulations a la souris
	QPointF press_point;
	bool moving_point;
	bool moving_segment;
	int moved_point;
	qreal previous_z_value;
	ConductorSegment *moved_segment;
	/// booleen indiquant si le conducteur a ete modifie manuellement par l'utilisateur
	bool modified_path;
	/// booleen indiquant s'il faut sauver le profil courant au plus tot
	bool has_to_save_profile;
	/// profil du conducteur : "photo" de ce a quoi le conducteur doit ressembler
	ConductorProfile conductor_profile;
	/// QPen et QBrush utilises pour dessiner les conducteurs
	static QPen conductor_pen;
	static QBrush conductor_brush;
	static bool pen_and_brush_initialized;
	
	private:
	void segmentsToPath();
	void saveProfile(bool = true);
	void priv_calculeConductor(const QPointF &, QET::Orientation, const QPointF &, QET::Orientation);
	void priv_modifieConductor(const QPointF &, QET::Orientation, const QPointF &, QET::Orientation);
	uint nbSegments(QET::ConductorSegmentType = QET::Both) const;
	QList<QPointF> segmentsToPoints() const;
	void pointsToSegments(QList<QPointF>);
	bool hasClickedOn(QPointF, QPointF) const;
	void calculateTextItemPosition();
	static int getCoeff(const qreal &, const qreal &);
	static int getSign(const qreal &);
	QHash<ConductorSegmentProfile *, qreal> shareOffsetBetweenSegments(const qreal &offset, const QList<ConductorSegmentProfile *> &, const qreal & = 0.01) const;
	static QPointF extendTerminal(const QPointF &, QET::Orientation, qreal = 12.0);
	static qreal conductor_bound(qreal, qreal, qreal, qreal = 0.0);
	static qreal conductor_bound(qreal, qreal, bool);
	static QString typeToString(ConductorType);
};
#endif
