/*
 C opyright 2006-2026 The QEle*ctroTech Team
 This file is part of QElectroTech.
 */
#ifndef DIAGRAMEVENTADDMACRO_H
#define DIAGRAMEVENTADDMACRO_H

#include <ElementsCollection/elementslocation.h>
#include "diagrameventinterface.h"
#include "macrolabels.h"

#include <functional>

#include <QDomDocument>
#include <QGraphicsPixmapItem>

class QStatusBar;

/**
 * @brief The DiagramEventAddMacro class
 */
class DiagramEventAddMacro : public DiagramEventInterface
{
	Q_OBJECT

public:
	DiagramEventAddMacro(const ElementsLocation &location, Diagram *diagram, QPointF pos = QPointF(0,0));
	~DiagramEventAddMacro() override;

	void mouseMoveEvent        (QGraphicsSceneMouseEvent *event) override;
	void mousePressEvent       (QGraphicsSceneMouseEvent *event) override;
	void mouseReleaseEvent     (QGraphicsSceneMouseEvent *event) override;
	void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event) override;
	void keyPressEvent         (QKeyEvent *event) override;
	void init() override;
		/// Who is asked for the labels and texts; a dialog by default.
		/// False places nothing.
	void setAsker(std::function<bool(QList<MacroLabels::Field> &)> asker) {m_asker = std::move(asker);}

private:
	bool loadMacro();
	bool askLabels(QDomElement &diagram);
	void addMacro(QPointF final_pos);

private:
	ElementsLocation m_location;
	QDomDocument m_macro_doc;
	QGraphicsPixmapItem *m_preview_item;
	QPointer<QStatusBar> m_status_bar;
	std::function<bool(QList<MacroLabels::Field> &)> m_asker;
	QPointF m_items_top_left;	// top-left of bounding rect of top-level items in the macro (for correct placement offset)
};

#endif // DIAGRAMEVENTADDMACRO_H
