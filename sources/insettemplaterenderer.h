/*
	Copyright 2006-2010 Xavier Guerrin
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
#ifndef INSET_TEMPLATE_RENDERER_H
#define INSET_TEMPLATE_RENDERER_H
#include <QPicture>
#include "diagramcontext.h"
class InsetTemplate;
class InsetTemplateRenderer : public QObject {
	Q_OBJECT
	
	public:
	InsetTemplateRenderer(QObject * = 0);
	virtual ~InsetTemplateRenderer();
	const InsetTemplate *insetTemplate() const;
	void setInsetTemplate(const InsetTemplate *);
	void setContext(const DiagramContext &context);
	int height() const;
	void render(QPainter *, int);
	
	private:
	void renderToQPicture(int);
	void invalidateRenderedTemplate();
	
	private:
	const InsetTemplate *inset_template_;
	QPicture rendered_template_;
	DiagramContext context_;
	int last_known_inset_width_;
};
#endif
