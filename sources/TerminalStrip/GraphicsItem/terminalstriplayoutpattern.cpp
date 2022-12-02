/*
	Copyright 2006-2022 The QElectroTech Team
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
#include "terminalstriplayoutpattern.h"
#include <QDebug>

//#include "../physicalterminal.h"
//#include "../realterminal.h"
//#include "../terminalstrip.h"
//#include "../terminalstripbridge.h"

//#include <QPainter>

///**
// * @brief TerminalStripDrawer::TerminalStripDrawer
// * @param strip
// * @param pattern
// */
//TerminalStripDrawer::TerminalStripDrawer(QPointer<TerminalStrip> strip) :
//	m_strip(strip)
//{}

///**
// * @brief TerminalStripDrawer::paint
// * @param painter
// */
//void TerminalStripDrawer::paint(QPainter *painter)
//{
//	if (m_strip)
//	{
//			//To draw text, QPainter need a Qrect. Instead of create an instance
//			//for each text, we re-use the same instance of QRect.
//		QRect text_rect;
//		painter->save();

//		auto pen_{painter->pen()};
//		pen_.setColor(Qt::black);
//		pen_.setWidth(1);

//		auto brush_ = painter->brush();
//		brush_.setColor(Qt::white);

//		painter->setPen(pen_);
//		painter->setBrush(brush_);

//			//Draw header
//		painter->drawRect(m_pattern.m_header_rect);

//			//Draw the header text
//		painter->save();

//		if (m_pattern.m_header_text_orientation == Qt::Horizontal)
//		{
//			text_rect.setRect(0,m_pattern.m_header_rect.y(),m_pattern.m_header_rect.width(),m_pattern.m_header_rect.height());
//		}
//		else
//		{
//			painter->translate(m_pattern.m_header_rect.bottomLeft());
//			painter->rotate(270);
//			text_rect.setRect(0,0,m_pattern.m_header_rect.height(),m_pattern.m_header_rect.width());
//		}

//		const auto text_{m_strip->installation() + " " + m_strip->location() + " " + m_strip->name()};
//		painter->drawText(text_rect, text_, m_pattern.headerTextOption());
//		painter->restore();

//			//Move painter pos to next drawing
//		painter->translate(m_pattern.m_header_rect.width(),0);

//		int x_offset{m_pattern.m_header_rect.width()};

//			//Draw spacer
//		painter->drawRect(m_pattern.m_spacer_rect);
//			//Move painter pos to next drawing
//		painter->translate(m_pattern.m_spacer_rect.width(),0);
//		x_offset += m_pattern.m_spacer_rect.width();


//			//Draw terminals
//		const auto terminals_text_rect{m_pattern.m_terminals_text_rect};
//		const auto terminals_text_orientation{m_pattern.m_terminals_text_orientation};
//		const auto terminals_text_option{m_pattern.terminalsTextOption()};
//		QRect terminal_rect;

//		QHash<QUuid, QVector<QPointF>> bridges_anchor_points;

//			//Loop over physical terminals
//		for (const auto &physical_t : m_strip->physicalTerminal())
//		{
//				//Get the good offset according to how many level have the current physical terminal
//			const QVector<QSharedPointer<RealTerminal>> real_terminal{physical_t->realTerminals()};
//			const auto real_t_count{real_terminal.size()};
//			const auto offset_{4 - real_t_count};

//				//Loop over real terminals
//			for (auto i=0 ; i<real_t_count ; ++i)
//			{
//				const auto index_ = offset_ + i;
//				if (index_ >= 4) {
//					break;
//				}

//				terminal_rect = m_pattern.m_terminal_rect[index_];
//					//Draw terminal rect
//				painter->drawRect(terminal_rect);

//					//Draw text
//				painter->save();
//				if (terminals_text_orientation[index_] == Qt::Horizontal)
//				{
//					text_rect = terminals_text_rect[index_];
//				}
//				else
//				{
//					const auto rect_{terminals_text_rect[index_]};
//					painter->translate(rect_.bottomLeft());
//					painter->rotate(270);
//					text_rect.setRect(0, 0, rect_.height(), rect_.width());
//				}

//				const auto shared_real_terminal{real_terminal[i]};
//				painter->drawText(text_rect,
//								  shared_real_terminal ? shared_real_terminal->label() : QLatin1String(),
//								  terminals_text_option[index_]);
//				painter->restore();

//					//Add bridge anchor
//				if (shared_real_terminal->isBridged())
//				{
//					painter->save();
//					if (const auto bridge_ = shared_real_terminal->bridge())
//					{
//						const auto anchor_center{m_pattern.m_bridge_point_d/2};
//						painter->setBrush(Qt::SolidPattern);
//						painter->drawEllipse(QPointF{terminal_rect.width()/2, m_pattern.m_bridge_point_y_offset[index_]},
//											 anchor_center,
//											 anchor_center);

//						auto anchor_points{bridges_anchor_points.value(bridge_->uuid())};
//						anchor_points.append(QPointF{x_offset + terminal_rect.width()/2,
//													 m_pattern.m_bridge_point_y_offset[index_]});
//						bridges_anchor_points.insert(bridge_->uuid(), anchor_points);
//					}
//					painter->restore();
//				}

//					//Move painter pos to next drawing
//				painter->translate(terminal_rect.width(),0);
//				x_offset += terminal_rect.width();
//			}
//		}

//		painter->restore();

//			//Draw the bridges
//		for (const auto &points_ : qAsConst(bridges_anchor_points))
//		{
//			painter->save();
//			auto pen_{painter->pen()};
//			pen_.setWidth(2);
//			painter->setPen(pen_);
//			painter->drawPolyline(QPolygonF{points_});
//			painter->restore();
//		}
//	}
//}

//QRectF TerminalStripDrawer::boundingRect() const
//{
//	return QRect{0, 0, width(), height()};;
//}

//int TerminalStripDrawer::height() const
//{
//	auto height_{m_pattern.m_header_rect.y() + m_pattern.m_header_rect.height()};

//	height_ = std::max(height_, m_pattern.m_spacer_rect.y() + m_pattern.m_spacer_rect.height());

//	for (const auto &rect : m_pattern.m_terminal_rect) {
//		height_ = std::max(height_, rect.y() + rect.height());
//	}

//	return height_;
//}

//int TerminalStripDrawer::width() const
//{
//	int width_{m_pattern.m_header_rect.width() + m_pattern.m_spacer_rect.width()};

//	if (m_strip)
//	{
//			//Loop over physical terminals
//		for (const auto &physical_t : m_strip->physicalTerminal())
//		{
//			//Get the good offset according to how many level have the current physical terminal
//			const QVector<QSharedPointer<RealTerminal>> real_terminal{physical_t->realTerminals()};
//			const auto real_t_count{real_terminal.size()};
//			const auto offset_{4 - real_t_count};

//			//Loop over real terminals
//			for (auto i=0 ; i<real_t_count ; ++i)
//			{
//				const auto index_ = offset_ + i;
//				if (index_ >= 4) {
//					break;
//				}

//				width_ += m_pattern.m_terminal_rect[index_].width();
//			}
//		}
//	}

//	return width_;
//}

TerminalStripLayoutPattern::TerminalStripLayoutPattern()
{
	updateHeaderTextOption();
	updateTerminalsTextOption();
}

void TerminalStripLayoutPattern::setHeaderTextAlignment(const Qt::Alignment &alignment)
{
	m_header_text_alignment = alignment;
	updateHeaderTextOption();
}

Qt::Alignment TerminalStripLayoutPattern::headerTextAlignment() const
{
	return m_header_text_alignment;
}

QTextOption TerminalStripLayoutPattern::headerTextOption() const {
	return m_header_text_option;
}

void TerminalStripLayoutPattern::setTerminalsTextAlignment(const QVector<Qt::Alignment> &alignment)
{
	m_terminals_text_alignment = alignment;
	updateTerminalsTextOption();
}

QVector<Qt::Alignment> TerminalStripLayoutPattern::terminalsTextAlignment() const
{
	return m_terminals_text_alignment;
}

QVector<QTextOption> TerminalStripLayoutPattern::terminalsTextOption() const
{
	return m_terminals_text_option;
}

void TerminalStripLayoutPattern::updateHeaderTextOption()
{
	m_header_text_option.setAlignment(m_header_text_alignment);
	m_header_text_option.setWrapMode(QTextOption::WordWrap);
}

void TerminalStripLayoutPattern::updateTerminalsTextOption()
{
	if (m_terminals_text_option.size() ==
		m_terminals_text_alignment.size())
	{
		for (auto i = 0 ; i<m_terminals_text_option.size() ; ++i)
		{
			m_terminals_text_option[i].setAlignment(m_terminals_text_alignment.at(i));
			m_terminals_text_option[i].setWrapMode(QTextOption::WordWrap);
		}
	}
	else {
		qDebug() << "TerminalStripLayoutPattern::updateTerminalsTextOption() : Wrong vector size";
	}
}
