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
#ifndef CONDUCTOR_H
#define CONDUCTOR_H
#include <QtGui>
#include "terminal.h"
#include "conductorprofile.h"
#include "conductorproperties.h"
class ConductorSegment;
class ConductorTextItem;
class Element;
typedef QPair<QPointF, Qt::Corner> ConductorBend;
typedef QHash<Qt::Corner, ConductorProfile> ConductorProfilesGroup;
/**
	Cette classe represente un conducteur. Un conducteur relie deux bornes d'element.
*/
class Conductor : public QObject, public QGraphicsPathItem {
	
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	Conductor(Terminal *, Terminal *, Diagram * = 0);
	virtual ~Conductor();
	
	private:
	Conductor(const Conductor &);
	
	// attributs
	public:
	enum { Type = UserType + 1001 };
	enum Highlight { None, Normal, Alert };
	
	/// premiere borne a laquelle le fil est rattache
	Terminal *terminal1;
	/// deuxieme borne a laquelle le fil est rattache
	Terminal *terminal2;
	
	// methodes
	public:
	/**
		permet de caster un QGraphicsItem en Conductor avec qgraphicsitem_cast
		@return le type de QGraphicsItem
	*/
	virtual int type() const { return Type; }
	void destroy();
	/// @return true si ce conducteur est detruit
	bool isDestroyed() const { return(destroyed); }
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
	/// caracteristiques du conducteur
	ConductorProperties properties_;
	/// booleen indiquant si le fil est encore valide
	bool destroyed;
	/// champ de texte editable pour les conducteurs non unifilaires
	ConductorTextItem *text_item;
	/// segments composant le conducteur
	ConductorSegment *segments;
	/// attributs lies aux manipulations a la souris
	QPointF press_point;
	bool moving_point;
	bool moving_segment;
	int moved_point;
	qreal previous_z_value;
	ConductorSegment *moved_segment;
	QPointF before_mov_text_pos_;
	/// booleen indiquant si le conducteur a ete modifie manuellement par l'utilisateur
	bool modified_path;
	/// booleen indiquant s'il faut sauver le profil courant au plus tot
	bool has_to_save_profile;
	/// profil du conducteur : "photo" de ce a quoi le conducteur doit ressembler - il y a un profil par type de trajet
	ConductorProfilesGroup conductor_profiles;
	/// QPen et QBrush utilises pour dessiner les conducteurs
	static QPen conductor_pen;
	static QBrush conductor_brush;
	static QBrush square_brush;
	static bool pen_and_brush_initialized;
	/// facteur de taille du carre de saisie du segment
	qreal segments_squares_scale_;
	/// Definit la facon dont le conducteur doit etre mis en evidence
	Highlight must_highlight_;
	
	private:
	void segmentsToPath();
	void saveProfile(bool = true);
	void priv_calculeConductor(const QPointF &, QET::Orientation, const QPointF &, QET::Orientation);
	void priv_modifieConductor(const QPointF &, QET::Orientation, const QPointF &, QET::Orientation);
	uint nbSegments(QET::ConductorSegmentType = QET::Both) const;
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
};
#endif
