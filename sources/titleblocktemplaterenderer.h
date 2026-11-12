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
#ifndef TITLEBLOCK_TEMPLATE_RENDERER_H
#define TITLEBLOCK_TEMPLATE_RENDERER_H
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPicture>
#include <QTransform>
#include "diagramcontext.h"

class ProjectLanguages;
class TitleBlockTemplate;
/**
	@brief The TitleBlockTemplateRenderer class
*/
class TitleBlockTemplateRenderer : public QObject
{
	Q_OBJECT
	
	public:
		TitleBlockTemplateRenderer(QObject * = nullptr);
		~TitleBlockTemplateRenderer() override;
		
		const TitleBlockTemplate *titleBlockTemplate() const;
		void setTitleBlockTemplate(const TitleBlockTemplate *);
		
		void setContext(const DiagramContext &context);
		DiagramContext context()const;
		void setLanguages(const ProjectLanguages *languages);
		DiagramContext shownContext() const;
		
		int height() const;
		void render(QPainter *, int);
			/// Forget the title block recorded for the screen
		void invalidateCache();
		void renderDxf(QRectF &, int, QString &, int,
			       const QTransform & = QTransform());

	private:
		const TitleBlockTemplate *m_titleblock_template;
		DiagramContext m_context;
		const ProjectLanguages *m_languages = nullptr;

			/// The title block drawn on screen, recorded once and played
			/// on every paint until what it is drawn from changes
		QPicture m_cache;
		bool m_cache_valid = false;
		const TitleBlockTemplate *m_cache_template = nullptr;
		DiagramContext m_cache_context;
		int m_cache_width = -1;
		QColor m_cache_background;
		QColor m_cache_pen;
		QFont m_cache_font;
		QPainter::RenderHints m_cache_hints;
};
#endif
