/*
 C opyright 2006-2026 The QEle*ctroTech Team
 This file is part of QElectroTech.
 */
#ifndef DIAGRAMEVENTADDMACRO_H
#define DIAGRAMEVENTADDMACRO_H

#include "../ElementsCollection/elementslocation.h"
#include "diagrameventinterface.h"

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

private:
	bool loadMacro();
	void addMacro(QPointF final_pos);

private:
	ElementsLocation m_location;
	QDomDocument m_macro_doc;
	QGraphicsPixmapItem *m_preview_item;
	QPointer<QStatusBar> m_status_bar;
};

#endif // DIAGRAMEVENTADDMACRO_H
