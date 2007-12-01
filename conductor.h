/*
	Copyright 2006-2007 Xavier Guerrin
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
#include "diagramtextitem.h"
class ConductorSegment;
class Element;
typedef QPair<QPointF, Qt::Corner> ConductorBend;
typedef QHash<Qt::Corner, ConductorProfile> ConductorProfilesGroup;
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
	
	/// premiere borne a laquelle le fil est rattache
	Terminal *terminal1;
	/// deuxieme borne a laquelle le fil est rattache
	Terminal *terminal2;
	
	// methodes
	public:
	virtual int type() const { return Type; }
	void destroy();
	/// @return true si ce conducteur est detruit
	bool isDestroyed() const { return(destroyed); }
	Diagram *diagram() const;
	void updateWithNewPos(const QRectF &, const Terminal *, const QPointF &);
	void update(const QRectF & = QRectF());
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	QRectF boundingRect() const;
	virtual QPainterPath shape() const;
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
	
	protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *);
	
	private:
	/// caracteristiques du conducteur
	ConductorProperties properties_;
	/// booleen indiquant si le fil est encore valide
	bool destroyed;
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
	/// profil du conducteur : "photo" de ce a quoi le conducteur doit ressembler - il y a un profil par type de trajet
	ConductorProfilesGroup conductor_profiles;
	/// QPen et QBrush utilises pour dessiner les conducteurs
	static QPen conductor_pen;
	static QBrush conductor_brush;
	static QBrush square_brush;
	static bool pen_and_brush_initialized;
	
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
	static QPointF extendTerminal(const QPointF &, QET::Orientation, qreal = 12.0);
	static qreal conductor_bound(qreal, qreal, qreal, qreal = 0.0);
	static qreal conductor_bound(qreal, qreal, bool);
	static Qt::Corner movementType(const QPointF &, const QPointF &);
};
#endif
