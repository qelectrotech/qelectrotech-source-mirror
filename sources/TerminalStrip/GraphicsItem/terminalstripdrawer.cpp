/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "terminalstripdrawer.h"
#include <QPainter>
#include <QHash>

namespace TerminalStripDrawer {

/**
 * @brief TerminalStripDrawer::TerminalStripDrawer
 * @param strip
 * @param pattern
 */
TerminalStripDrawer::TerminalStripDrawer(QSharedPointer<AbstractTerminalStripInterface> strip,
										 QSharedPointer<TerminalStripLayoutPattern> layout) :
	m_strip { strip },
	m_pattern { layout }
{}

void TerminalStripDrawer::setStrip(QSharedPointer<AbstractTerminalStripInterface> strip)
{
	m_strip = strip;
}

/**
 * @brief TerminalStripDrawer::paint
 * @param painter
 */
void TerminalStripDrawer::paint(QPainter *painter)
{
	if (m_strip && m_pattern)
	{
			//To draw text, QPainter need a Qrect. Instead of create an instance
			//for each text, we re-use the same instance of QRect.
		QRect text_rect;
		painter->save();

		auto pen_{painter->pen()};
		pen_.setColor(Qt::black);
		pen_.setWidth(1);

		auto brush_ = painter->brush();
		brush_.setColor(Qt::white);

		painter->setPen(pen_);
		painter->setBrush(brush_);

		if (m_preview_draw)
		{
			painter->save();
			painter->setPen(Qt::blue);
			painter->drawRect(boundingRect());
			painter->restore();
		}

			//Draw header
		painter->drawRect(m_pattern->m_header_rect);

			//Draw the header text
		painter->save();

		if (m_pattern->m_header_text_orientation == Qt::Horizontal)
		{
			text_rect.setRect(0,m_pattern->m_header_rect.y(),m_pattern->m_header_rect.width(),m_pattern->m_header_rect.height());
		}
		else
		{
			painter->translate(m_pattern->m_header_rect.bottomLeft());
			painter->rotate(270);
			text_rect.setRect(0,0,m_pattern->m_header_rect.height(),m_pattern->m_header_rect.width());
		}

		const auto text_{m_strip->installation() + " " + m_strip->location() + " " + m_strip->name()};
		painter->drawText(text_rect, text_, m_pattern->headerTextOption());
		painter->restore();

			//Move painter pos to next drawing
		painter->translate(m_pattern->m_header_rect.width(),0);

		int x_offset{m_pattern->m_header_rect.width()};

			//Draw spacer
		painter->drawRect(m_pattern->m_spacer_rect);
			//Move painter pos to next drawing
		painter->translate(m_pattern->m_spacer_rect.width(),0);
		x_offset += m_pattern->m_spacer_rect.width();


			//Draw terminals
		const auto terminals_text_rect{m_pattern->m_terminals_text_rect};
		const auto terminals_text_orientation{m_pattern->m_terminals_text_orientation};
		const auto terminals_text_option{m_pattern->terminalsTextOption()};
		QRect terminal_rect;

		QHash<QUuid, QVector<QPointF>> bridges_anchor_points;

			//Loop over physical terminals
		for (const auto &physical_t : m_strip->physicalTerminal())
		{
				//Get the good offset according to how many level have the current physical terminal
			const QVector<QSharedPointer<AbstractRealTerminalInterface>> real_terminal_vector{physical_t->realTerminals()};
			const auto real_t_count{real_terminal_vector.size()};
			const auto offset_{4 - real_t_count};

				//Loop over real terminals
			for (auto i=0 ; i<real_t_count ; ++i)
			{
				const auto index_ = offset_ + i;
				if (index_ >= 4) {
					break;
				}

				terminal_rect = m_pattern->m_terminal_rect[index_];
					//Draw terminal rect
				painter->drawRect(terminal_rect);
					//Draw a stronger line if the current terminal have level
					//and the current level is the first
				if (real_t_count > 1 && i == 0)
				{
					painter->save();
					pen_ = painter->pen();
					pen_.setWidth(4);
					pen_.setCapStyle(Qt::FlatCap);
					painter->setPen(pen_);
					const auto p1 { terminal_rect.topLeft() };
						//We can't use terminal_rect.bottomLeft for p2 because
						//the returned value deviate from the true value
						//(see Qt documentation about QRect)
					const QPoint p2 { p1.x(), p1.y() + terminal_rect.height() };
					painter->drawLine(p1, p2);
					painter->restore();
				}

					//Draw text
				painter->save();
				if (terminals_text_orientation[index_] == Qt::Horizontal)
				{
					text_rect = terminals_text_rect[index_];
				}
				else
				{
					const auto rect_{terminals_text_rect[index_]};
					painter->translate(rect_.bottomLeft());
					painter->rotate(270);
					text_rect.setRect(0, 0, rect_.height(), terminal_rect.width());
				}

				const auto shared_real_terminal{real_terminal_vector[i]};
				painter->drawText(text_rect,
								  shared_real_terminal ? shared_real_terminal->label() : QLatin1String(),
								  terminals_text_option[index_]);

				if (m_preview_draw)
				{
					painter->setPen(Qt::blue);
					painter->drawRect(text_rect);
				}

				painter->restore();

					//Add bridge anchor
				if (shared_real_terminal->isBridged())
				{
					painter->save();
					if (QScopedPointer<AbstractBridgeInterface> bridge_ {
						shared_real_terminal->bridge() })
					{
						const auto x_anchor{terminal_rect.width()/2};
						const auto y_anchor {m_pattern->m_bridge_point_y_offset[index_]};
						const auto radius_anchor{m_pattern->m_bridge_point_d/2};

						painter->setBrush(Qt::SolidPattern);
						painter->drawEllipse(QPointF(x_anchor, y_anchor),
											 radius_anchor, radius_anchor);

						auto anchor_points{bridges_anchor_points.value(bridge_->uuid())};
						anchor_points.append(QPointF(x_offset + x_anchor, y_anchor));
						bridges_anchor_points.insert(bridge_->uuid(), anchor_points);
					}
					painter->restore();
				}

					//Move painter pos to next drawing
				painter->translate(terminal_rect.width(),0);
				x_offset += terminal_rect.width();
			}
		}
		painter->restore();

			//Draw the bridges
		for (const auto &points_ : qAsConst(bridges_anchor_points))
		{
			painter->save();
			auto pen_{painter->pen()};
			pen_.setWidth(2);
			painter->setPen(pen_);
			painter->drawPolyline(QPolygonF{points_});
			painter->restore();
		}
	}
}

QRectF TerminalStripDrawer::boundingRect() const
{
	return QRect{0, 0, width(), height()};;
}

void TerminalStripDrawer::setLayout(QSharedPointer<TerminalStripLayoutPattern> layout)
{
	m_pattern = layout;
}

bool TerminalStripDrawer::haveLayout() const
{
	return !m_pattern.isNull();
}

void TerminalStripDrawer::setPreviewDraw(bool draw) {
	m_preview_draw = draw;
}

int TerminalStripDrawer::height() const
{
	if (m_pattern)
	{
		auto height_{m_pattern->m_header_rect.y() + m_pattern->m_header_rect.height()};

		height_ = std::max(height_, m_pattern->m_spacer_rect.y() + m_pattern->m_spacer_rect.height());

		for (const auto &rect : m_pattern->m_terminal_rect) {
			height_ = std::max(height_, rect.y() + rect.height());
		}

		return height_;
	}

	return 0;
}

int TerminalStripDrawer::width() const
{
	if (m_pattern)
	{
		int width_{m_pattern->m_header_rect.width() + m_pattern->m_spacer_rect.width()};

		if (m_strip)
		{
			//Loop over physical terminals
			for (const auto &physical_t : m_strip->physicalTerminal())
			{
				//Get the good offset according to how many level have the current physical terminal
				const QVector<QSharedPointer<AbstractRealTerminalInterface>> real_terminal_vector{physical_t->realTerminals()};
				const auto real_t_count{real_terminal_vector.size()};
				const auto offset_{4 - real_t_count};

				//Loop over real terminals
				for (auto i=0 ; i<real_t_count ; ++i)
				{
					const auto index_ = offset_ + i;
					if (index_ >= 4) {
						break;
					}

					width_ += m_pattern->m_terminal_rect[index_].width();
				}
			}
		}

		return width_;
	}

	return 0;
}

} //End namespace TerminalStripDrawer
