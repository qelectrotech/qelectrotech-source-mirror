/*
	Copyright 2006-2021 The QElectroTech Team
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
#include "../autoNum/assignvariables.h"
#include "../conductorproperties.h"

#include <QGraphicsPathItem>

class ConductorProfile;
class ConductorSegmentProfile;
class Diagram;
class Terminal;
class ConductorSegment;
class ConductorTextItem;
class Element;
class QETDiagramEditor;
class NumerotationContext;
class QetGraphicsHandlerItem;

typedef QPair<QPointF, Qt::Corner> ConductorBend;
typedef QHash<Qt::Corner, ConductorProfile> ConductorProfilesGroup;
/**
	This class represents a conductor, i.e. a wire between two element
	terminals.
*/
class Conductor : public QGraphicsObject
{
	Q_OBJECT

	Q_PROPERTY(QPointF pos READ pos WRITE setPos)
	Q_PROPERTY(int animPath READ fakePath WRITE updatePathAnimate)
	Q_PROPERTY(ConductorProperties properties READ properties WRITE setProperties)
	Q_PROPERTY(autonum::sequentialNumbers sequenceNum READ sequenceNum WRITE setSequenceNum)

	signals:
		void propertiesChange();
	
	public:
		Conductor(Terminal *, Terminal *);
		~Conductor() override;

		bool isValid() const;
	
	private:
		Conductor(const Conductor &);

	public:
		enum { Type = UserType + 1001 };
		enum Highlight { None, Normal, Alert };

		Terminal *terminal1;
		Terminal *terminal2;
	
	public:
		/**
			@brief type
			Enable the use of qgraphicsitem_cast
			to safely cast a QGraphicsItem into a conductor.
			@return the QGraphicsItem type
		*/
		int type() const override { return Type; }
		Diagram *diagram() const;
		ConductorTextItem *textItem() const;
		void updatePath(const QRectF & = QRectF());

		//This method do nothing, it's only made to be used with Q_PROPERTY
		//It's used to anim the path when is change
		void updatePathAnimate(const int = 1) {updatePath();}
		int fakePath() {return 1;}

		void paint(
				QPainter *,
				const QStyleOptionGraphicsItem *,
				QWidget *) override;
		QRectF boundingRect() const override;
		QPainterPath shape() const override;
		virtual QPainterPath nearShape() const;
		qreal length() const;
		ConductorSegment *middleSegment();
		QPointF posForText(Qt::Orientations &flag);
		void refreshText();
		void setPath(const QPainterPath &path);
		QPainterPath path() const;

	public:
		static bool valideXml (QDomElement &);
		bool fromXml (QDomElement &);
		QDomElement toXml (
				QDomDocument &,
				QHash<Terminal *,
				int> &) const;
	private:
		bool pathFromXml(const QDomElement &);

	public:
		QVector <QPointF> handlerPoints() const;
		const QList<ConductorSegment *> segmentsList() const;

		void setPropertyToPotential(
				const ConductorProperties &property,
				bool only_text = false);
		void setProperties(const ConductorProperties &property);
		ConductorProperties properties() const;

		void setProfile(const ConductorProfile &, Qt::Corner);
		ConductorProfile profile(Qt::Corner) const;
		void setProfiles(const ConductorProfilesGroup &);
		ConductorProfilesGroup profiles() const;
		void calculateTextItemPosition();
		virtual Highlight highlight() const;
		virtual void setHighlighted(Highlight);
		QSet<Conductor *> relatedPotentialConductors(
				const bool all_diagram = true,
				QList <Terminal *> *t_list=nullptr);
		QETDiagramEditor* diagramEditor() const;
		void editProperty ();

		autonum::sequentialNumbers sequenceNum () const
		{return m_autoNum_seq;}
		autonum::sequentialNumbers& rSequenceNum()
		{return m_autoNum_seq;}
		void setSequenceNum(const autonum::sequentialNumbers& sn);

        QList<QPointF> junctions() const;

	private:
		void setUpConnectionForFormula(
				QString old_formula, QString new_formula);
		autonum::sequentialNumbers m_autoNum_seq;

	public:
		void setFreezeLabel(bool freeze);
	
	public slots:
		void displayedTextChanged();
	
	protected:
		void mouseDoubleClickEvent(
				QGraphicsSceneMouseEvent *event) override;
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(
				QGraphicsSceneMouseEvent *event) override;
		void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
		QVariant itemChange(
				GraphicsItemChange, const QVariant &) override;
		bool sceneEventFilter(
				QGraphicsItem *watched, QEvent *event) override;

	private:
		void adjusteHandlerPos();
		
		void handlerMousePressEvent(
				QetGraphicsHandlerItem *qghi,
				QGraphicsSceneMouseEvent *event);
		void handlerMouseMoveEvent(
				QetGraphicsHandlerItem *qghi,
				QGraphicsSceneMouseEvent *event);
		void handlerMouseReleaseEvent(
				QetGraphicsHandlerItem *qghi,
				QGraphicsSceneMouseEvent *event);
		void addHandler();
		void removeHandler();
		
		
		QVector<QetGraphicsHandlerItem *> m_handler_vector;
		int m_vector_index = -1;
		bool m_mouse_over;
			/// Functional properties
		ConductorProperties m_properties;
			/// Text input for non simple, non-singleline conductors
		ConductorTextItem *m_text_item;
			/// Segments composing the conductor
		ConductorSegment *segments;
			/// Attributes related to mouse interaction
		bool m_moving_segment;
		int moved_point;
		qreal m_previous_z_value;
		ConductorSegment *m_moved_segment;
		QPointF before_mov_text_pos_;
			/// Whether the conductor was manually modified by users
		bool modified_path;
			/// Whether the current profile should be saved as soon as possible
		bool has_to_save_profile;
			/// conductor profile: "photography" of what the conductor is supposed to look
			/// like - there is one profile per kind of traject
		ConductorProfilesGroup conductor_profiles;
			/// Define whether and how the conductor should be highlighted
		Highlight must_highlight_;
		bool m_valid;
		bool m_freeze_label = false;

			/// QPen et QBrush objects used to draw conductors
		static QPen conductor_pen;
		static QBrush conductor_brush;
		static bool pen_and_brush_initialized;
		QPainterPath m_path;
	
	private:
		void segmentsToPath();
		void saveProfile(bool = true);
		void generateConductorPath(const QPointF &, Qet::Orientation, const QPointF &, Qet::Orientation);
		void updateConductorPath(const QPointF &, Qet::Orientation, const QPointF &, Qet::Orientation);
		uint segmentsCount(QET::ConductorSegmentType = QET::Both) const;
		QList<QPointF> segmentsToPoints() const;
		QList<ConductorBend> bends() const;

		void pointsToSegments(const QList<QPointF>&);
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
