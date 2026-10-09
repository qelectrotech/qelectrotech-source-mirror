/*
	Copyright 2006-2026 The QElectroTech Team
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

#ifndef CABINETLAYOUTREFERENCEITEM_H
#define CABINETLAYOUTREFERENCEITEM_H

#include "qetgraphicsitem.h"

#include <QFont>
#include <QPen>
#include <QPointer>
#include <QUuid>
#include <QSvgRenderer>
#include <QScopedPointer>
#include <QFontMetricsF>

class QETProject;
class Element;

/**
	@brief The CabinetLayoutReferenceItem class
	A scale-accurate box representing, on a cabinet layout
	("disposition des armoires") folio, the real-world footprint of a
	device placed elsewhere in the project -- front view (width x
	height) or side view (depth x height), depending on the folio's
	own Diagram::cabinetLayoutView() setting.

	Unlike the earlier CabinetLayoutBoxFactory/Element-based approach,
	this is a dedicated, lightweight top-level diagram item type (see
	Diagram::toXml()/fromXml(), which already dispatches over several
	such types -- Conductor, IndependentTextItem, QetShapeItem, etc. --
	via item->type()), not a library symbol:
	- No .elmt file, no ElementsLocation, no project-embedded
	  collection entry is involved in creating one.
	- No auto-numbering/formula machinery -- the label always comes
	  directly from the referenced source element, never
	  auto-generated.
	- Not tied to Element's fixed, author-time symbol geometry: its
	  size is entirely computed from the source device's
	  width/height/depth and the folio's scale.

	Linking works exactly like Master/Slave elements
	(@see Element::initLink(), SlaveElement::linkToElement()): the
	source is looked up once, via ElementProvider (a direct search of
	live Element objects already in the project's diagrams, not a SQL
	round-trip -- QET's own established pattern for "resolve a stored
	UUID reference once the whole project has finished loading").
	There is no automatic retry after that; a reference whose source
	can't be found stays orphaned (@see isOrphaned()) until the next
	full reload (closing and reopening the project), matching how
	Element::initLink() itself only ever attempts once per load.

	Because of that "only once" contract, what's needed to still show
	something sensible for an orphaned reference -- the source's last
	known label and real-world width/height in mm -- is cached
	directly in this item's own toXml()/fromXml(), rather than only
	looked up live. Once successfully linked, this cache is kept
	genuinely live via Element::elementInfoChange, so a save always
	persists the current state, not a stale snapshot.
*/

class CabinetLayoutReferenceItem : public QetGraphicsItem
{
	Q_OBJECT
	Q_PROPERTY(QPen pen READ pen WRITE setPen)
	Q_PROPERTY(QBrush brush READ brush WRITE setBrush)
	Q_PROPERTY(qreal textRotation READ textRotation WRITE setTextRotation)
	Q_PROPERTY(QFont font READ font WRITE setFont)
	Q_PROPERTY(QString displayedInfoKey READ displayedInfoKey WRITE setDisplayedInfoKey)

	public:
		explicit CabinetLayoutReferenceItem(const QUuid &source_element_uuid,
											 bool is_side_view,
											 QGraphicsItem *parent = nullptr);
		~CabinetLayoutReferenceItem() override;

		enum { Type = UserType + 1012 };
		int type() const override { return Type; }
		void setPos(const QPointF &p) override;

		QRectF boxRect() const;
		QRectF boundingRect() const override;
		void fitFontSizeToBox();
		void paint(QPainter *painter,
				   const QStyleOptionGraphicsItem *options,
				   QWidget *widget = nullptr) override;

		QDomElement toXml(QDomDocument &document) const;
		bool fromXml(const QDomElement &dom_element);

		QUuid sourceElementUuid() const { return m_source_element_uuid; }
		bool isSideView() const { return m_is_side_view; }
		bool isOrphaned() const { return m_is_orphaned; }

		QPen pen() const { return m_box_pen; }
		void setPen(const QPen &pen) { m_box_pen = pen; update(); }
		QBrush brush() const { return m_box_brush; }
		void setBrush(const QBrush &brush) { m_box_brush = brush; update(); }
		qreal textRotation() const { return m_text_rotation; }
		void setTextRotation(qreal angle) { m_text_rotation = angle; update(); }
		QFont font() const { return m_font; }
		void setFont(const QFont &font) { m_font = font; update(); }
		QString displayedInfoKey() const { return m_displayed_info_key; }
		void setDisplayedInfoKey(const QString &key);
		bool hasLayoutPicture() const { return m_has_layout_picture; }

		/**
			@brief linkToSource
			Attempts, once, to find the live Element matching
			sourceElementUuid() (via ElementProvider -- a direct search
			of the project's already-loaded diagrams, no database
			involved) and connect to it for live updates. Called from
			Diagram::refreshContents() (matching where
			Element::initLink() itself is called from) after a fresh
			project load, and directly at drop time when a new
			reference is created interactively (@see
			DiagramEventAddCabinetLayoutReference), since the source is
			always already live in the scene in that case.

			On success: caches the source's current label/width/height,
			connects to its elementInfoChange (further edits stay live)
			and to its destruction (so this reference becomes orphaned
			again if the source is later deleted), and clears
			isOrphaned().

			On failure: leaves the item showing its last cached
			label/size (@see toXml()/fromXml()) and marks isOrphaned().
			Does not remove the item -- an orphaned reference is a
			visible, actionable state, not silently dropped, and the
			user may still want to delete it manually or wait for the
			source to reappear (e.g. undoing its deletion) before the
			next reload.
			@param project the project to search for the source element
		*/
		void linkToSource(QETProject *project);

		/**
			@brief jumpToSource
			Selects sourceElementUuid()'s element on its own folio and
			scrolls that folio's view to make it visible, switching the
			active MDI subwindow if the source element lives on a
			different folio than this reference item. No-op if the
			source element can no longer be found.
		*/
		void jumpToSource() const;

		/**
			@brief existsReferenceFor
			@param project the project to search across all its folios
			@param source_element_uuid the device to look for
			@param is_side_view which view (front/side) to check --
			a device may legitimately have one reference of each kind
			(e.g. a front-view box on one folio and a side-view box on
			another), just not two of the same kind.
			@return true if a CabinetLayoutReferenceItem for this exact
			(source_element_uuid, is_side_view) combination already
			exists somewhere in the project. Used both to reject a
			second placement attempt (@see
			DiagramEventAddCabinetLayoutReference) and, later, to mark
			an already-placed device in the source tree
			(@see CabinetLayoutSourceModel).
		*/
		static bool existsReferenceFor(QETProject *project,
										const QUuid &source_element_uuid,
										bool is_side_view);

	protected:
		void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

	private slots:
		/**
			@brief onSourceInfoChanged
			Connected to the linked source Element's elementInfoChange.
			Re-reads label/width/height/depth directly from the
			(already-held) source element -- no database round-trip --
			and recomputes this item's cached values and on-screen size.
		*/
		void onSourceInfoChanged();

		/**
			@brief onSourceDestroyed
			Connected to the linked source Element's destroyed(). Marks
			this reference orphaned again; its last cached label/size
			stay visible (@see isOrphaned()).
		*/
		void onSourceDestroyed();

	private:
		void applyElementData(Element *element);
		void applyLayoutPicture(Element *element);
		void recomputeBoxSize();
		void drawBox(QPainter *painter) const;
		void drawLabel(QPainter *painter) const;

		QUuid m_source_element_uuid;
		bool m_is_side_view;
		bool m_is_orphaned = true;

		QPointer<Element> m_source_element;

		QString m_label;
		qreal m_real_width_mm  = 0.0;
		qreal m_real_height_mm = 0.0;

		qreal m_box_width  = 100;
		qreal m_box_height = 100;

		QPen m_box_pen { QColor(Qt::black), 1.0 };
		QBrush m_box_brush { QColor(200, 200, 200, 65) };
		QPen m_text_pen { QColor(Qt::black), 1.0 };
		qreal m_text_rotation = 270;
		QString m_displayed_info_key = QStringLiteral("label");
		QFont m_font { QStringLiteral("Sans Serif"), 7 };

		QScopedPointer<QSvgRenderer> m_layout_svg_renderer;
		bool m_has_layout_picture = false;
};

#endif // CABINETLAYOUTREFERENCEITEM_H