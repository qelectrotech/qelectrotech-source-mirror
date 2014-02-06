#ifndef DIAGRAMFOLIOLIST_H
#define DIAGRAMFOLIOLIST_H

#include "diagram.h"

class DiagramFolioList : public Diagram
{
	public:
	DiagramFolioList(QObject *parent = 0) : Diagram(parent) {}
	virtual ~DiagramFolioList() {}

	protected:
	void drawBackground(QPainter *, const QRectF &);

	private:
	void fillRow(QPainter *, const QRectF &, QString, QString, QString, QString);

};

#endif // DIAGRAMFOLIOLIST_H
