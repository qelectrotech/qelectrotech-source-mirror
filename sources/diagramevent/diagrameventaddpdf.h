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
#ifndef DIAGRAMEVENTADDPDF_H
#define DIAGRAMEVENTADDPDF_H

// PDF page import needs the QtPdf module (QET_HAS_QTPDF, set by CMake
// only when QtPdf is present AND Qt >= 6.4, for QPdfDocument::pagePointSize()).
#ifdef QET_HAS_QTPDF

#include "diagrameventinterface.h"

class Diagram;
class DiagramImageItem;

/**
	@brief The DiagramEventAddPdf class
	This diagram event handles the adding of a PDF page as an image in a diagram.
	The selected PDF page is rendered to a QImage at 150 DPI and added as a
	DiagramImageItem. The placement, rotation and scaling behavior is identical
	to the existing image import.
*/
class DiagramEventAddPdf : public DiagramEventInterface
{
	Q_OBJECT

	public:
		DiagramEventAddPdf(Diagram *diagram);
		~DiagramEventAddPdf() override;

		void mousePressEvent       (QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent        (QGraphicsSceneMouseEvent *event) override;
		void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event) override;
		void wheelEvent            (QGraphicsSceneWheelEvent *event) override;

		bool isNull() const;

	private:
		void openDialog();

		DiagramImageItem *m_image;
		bool m_is_added;
};

#endif // QET_HAS_QTPDF

#endif // DIAGRAMEVENTADDPDF_H
