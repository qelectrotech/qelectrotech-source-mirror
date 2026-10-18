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
#ifndef MODELTERMINALDATA_H
#define MODELTERMINALDATA_H

#include <QColor>
#include <QString>
#include "../../qetgraphicsitem/element.h"
#include "../realterminal.h"

struct modelRealTerminalData
{
		static modelRealTerminalData data(const QSharedPointer<RealTerminal> &real_t)
		{
			modelRealTerminalData mrtd;
			if (!real_t.isNull())
			{
				mrtd.level_ = real_t->level();
				mrtd.label_ = real_t->label();
				mrtd.Xref_ = real_t->Xref();
				mrtd.cable_ = real_t->cable();
				mrtd.cable_wire = real_t->cableWire();
				mrtd.conductor_ = real_t->conductor();
				mrtd.led_ = real_t->isLed();
				mrtd.shield_ = real_t->isShield();
				mrtd.separator_ = real_t->hasSeparatorAfter();
				mrtd.color_ = real_t->color();
				mrtd.stripe_color_ = real_t->stripeColor();
				mrtd.section_ = real_t->section();
				mrtd.width_mm_ = real_t->widthMm();
				mrtd.catalog_entry_ = real_t->catalogEntry();
				mrtd.type_ = real_t->type();
				mrtd.function_ = real_t->function();
				mrtd.element_ = real_t->element();
				mrtd.real_terminal = real_t.toWeakRef();
				mrtd.bridged_ = real_t->isBridged();
				mrtd.spare_ = real_t->isSpare();
			}

			return mrtd;
		}

		int level_ = -1;
		QString label_;
		QString Xref_;
		QString cable_;
		QString cable_wire;
		QString conductor_;
		bool led_ = false;
		bool shield_ = false;
		bool separator_ = false;	//A separator plate is mounted after the terminal
		QColor color_;		//Invalid = no color set by the user
		QColor stripe_color_;	//Second color of a two-tone color, invalid if plain
		qreal section_ = 0;	//In mm2, 0 = no section set by the user
		qreal width_mm_ = 0;	//In mm, 0 = no width set by the user
		QString catalog_entry_;	//Terminal chosen in the catalog, empty if the width was typed
		bool bridged_ = false;
		bool spare_ = false;	//Spare terminal, on no folio (RealTerminal::isSpare())

		ElementData::TerminalType type_ = ElementData::TerminalType::TTGeneric;
		ElementData::TerminalFunction function_ = ElementData::TerminalFunction::TFGeneric;
		QPointer<Element> element_;

		QWeakPointer<RealTerminal> real_terminal;
};

struct modelPhysicalTerminalData
{
		QVector<modelRealTerminalData> real_data;
		int pos_ = -1;
		QUuid uuid_;
};

inline bool operator == (const modelPhysicalTerminalData &data_1, const modelPhysicalTerminalData &data_2) {
	return data_1.uuid_ == data_2.uuid_;
}

#endif // MODELTERMINALDATA_H
