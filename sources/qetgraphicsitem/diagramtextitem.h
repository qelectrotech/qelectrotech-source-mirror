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
#ifndef DIAGRAM_TEXT_ITEM_H
#define DIAGRAM_TEXT_ITEM_H

#include <QGraphicsTextItem>
#include <QFont>
#include <QSharedPointer>
#include <QVariantMap>

class Diagram;
class QDomElement;
class QDomDocument;
class TextResizeHandles;

/**
	This class represents a selectable, movable and editable text field on a
	diagram.
	@see QGraphicsItem::GraphicsItemFlags
*/
class DiagramTextItem : public QGraphicsTextItem
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
	Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)
	Q_PROPERTY(QPointF anchorPos READ anchorPos WRITE setAnchorPos)
	Q_PROPERTY(Qt::Alignment alignmentAtAnchor READ alignment WRITE setAlignmentAtAnchor NOTIFY alignmentChanged)
	Q_PROPERTY(QString plainText READ toPlainText WRITE setPlainText)
	Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
	
	
	signals:
		void colorChanged(QColor color);
		void alignmentChanged(Qt::Alignment alignment);
		void textEdited(const QString &old_str, const QString &new_str);
		void fontChanged(QFont font);

	public:
		DiagramTextItem(QGraphicsItem * = nullptr);
		DiagramTextItem(const QString &, QGraphicsItem * = nullptr);

	private:
		void build();
	
	public:
		enum { Type = UserType + 1004 };
		int type() const override { return Type; }

		Diagram *diagram() const;
        // Language display never changes the original editable/source document.
        virtual QString languageId16() const { return property("languageId16").toString(); }
        void refreshLanguage16();
        void refreshLanguageSiblings23();
        bool hasLanguageDisplay16() const;
        QString displayedText16() const;
        bool isTranslationMode21() const;
        bool languageHidden21() const;
        QPointF sourcePosition21() const;
        QPointF languageOffset22(Qt::Alignment *resolved = nullptr) const;
        QVariantMap languageVariant16() const;
        int languageLineCount16() const;
        const QTextDocument *languageDocument16() const;
        QRectF boundingRect() const override;
		virtual void fromXml(const QDomElement &) = 0;
		virtual QDomElement toXml(QDomDocument &) const;
		void edit();

		QPointF mapMovementToScene    (const QPointF &) const;
		QPointF mapMovementFromScene  (const QPointF &) const;
		QPointF mapMovementToParent   (const QPointF &) const;
		QPointF mapMovementFromParent (const QPointF &) const;

		void setFont(const QFont &font);

		void setColor(const QColor& color);
		QColor color() const;

		void setNoEditable(bool e = true) {m_no_editable = e;}
		
		void setAlignment(const Qt::Alignment &alignment);
		Qt::Alignment alignment() const;
		bool m_block_alignment = false;

		QPointF anchorPos() const;
		void setAnchorPos(const QPointF &anchor);
		void setAlignmentAtAnchor(const Qt::Alignment &alignment);
		
		QRectF frameRect() const;
		QRectF writtenRect() const;
		QPainterPath shape() const override;
		bool contains(const QPointF &point) const override;
	
	void setHtml(const QString &text);
		void setPlainText(const QString &text);
		bool isHtml() const;

	protected:
		void refreshTextResizeHandles();
		TextResizeHandles *textResizeHandles() const {return m_resize_handles;}

		void paint(QPainter *,
			   const QStyleOptionGraphicsItem *,
			   QWidget *) override;
		void focusInEvent(QFocusEvent *) override;
		void focusOutEvent(QFocusEvent *) override;
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

		void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event) override;
		void mousePressEvent       (QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent        (QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent     (QGraphicsSceneMouseEvent *event) override;
		bool finishAdditiveSelection25(QGraphicsSceneMouseEvent *event);

		void hoverEnterEvent(QGraphicsSceneHoverEvent *) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override;
		void hoverMoveEvent(QGraphicsSceneHoverEvent *) override;

		virtual void applyRotation(const qreal &);
		void prepareAlignment();
		void finishAlignment();

		virtual bool textResizeHandlesWanted() const;
		virtual bool hasUserTextWidth() const {return false;}
		bool isEditing() const;
		void removeTextResizeHandles();

	
	protected:
		bool
		m_mouse_hover = false,
		m_first_move = true,
		m_additive_text_press25 = false,
		m_toggle_text25 = false,
		m_no_editable,
		m_is_html = false,
		m_non_left_alignment = false;

		QString
		m_previous_html_text,
		m_previous_text;
		
		QPointF m_mouse_to_origin_movement;
		
	private:
		void pushResizeCommand(qreal old_width, qreal new_width,
							   QPointF old_pos, QPointF new_pos);

		void ensureLanguageDocument16() const;
        QPointF calculateLanguageOffset23(Qt::Alignment *resolved) const;
        mutable QPointF m_language_offset23;
        mutable Qt::Alignment m_language_alignment23;
        mutable QVariantMap m_language_variant23;
        mutable bool m_language_hidden23 = false;
        bool m_language_refreshing23 = false;

        mutable QString m_language_key16;
        mutable QSharedPointer<QTextDocument> m_language_document16;
        QMetaObject::Connection m_language_connection16;
		QRectF m_alignment_rect;
		TextResizeHandles *m_resize_handles = nullptr;
		Qt::Alignment m_alignment = (Qt::AlignTop | Qt::AlignLeft);
};
#endif
