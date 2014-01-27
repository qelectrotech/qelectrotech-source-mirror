/*
	Copyright 2006-2013 The QElectroTech Team
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
#include <QPicture>
#include "diagramcontext.h"
class TitleBlockTemplate;
class TitleBlockTemplateRenderer : public QObject {
	Q_OBJECT
	
	public:
	TitleBlockTemplateRenderer(QObject * = 0);
	virtual ~TitleBlockTemplateRenderer();
	const TitleBlockTemplate *titleBlockTemplate() const;
	void setTitleBlockTemplate(const TitleBlockTemplate *);
	void setContext(const DiagramContext &context);
	int height() const;
	void render(QPainter *, int);
	void renderDxf(QRectF &, int, QString &, int);
	void invalidateRenderedTemplate();
	void setUseCache(bool);
	bool useCache() const;
	
	private:
	void renderToQPicture(int);
	
	private:
	const TitleBlockTemplate *titleblock_template_;
	bool use_cache_;
	QPicture rendered_template_;
	DiagramContext context_;
	int last_known_titleblock_width_;
};
#endif
