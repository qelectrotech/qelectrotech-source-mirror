/*
	Copyright 2026 Trovo Tech Solutions
	This file is part of a custom feature set built on QElectroTech.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef WIRESPEC_H
#define WIRESPEC_H

#include <QString>
#include <QStringList>
#include <QVector>

/**
	@brief The WireSpec struct
	One record of the wire / cable catalogue. Mirrors the SolidWorks Electrical
	wire/cable manager fields observed in the PRT500 schematic set
	(W31 Brown_1x0.5mm², W57 Brown_1x16, W63 CAN_Cable_4x0.5mm², ...).

	A single-core wire is described by one WireSpec with numCores == 1.
	A multi-core cable is one WireSpec with numCores > 1 and one entry per
	core in coreColors (IEC 60757 colour names).

	The label helpers produce the strings consumed by the Terminal Strip
	module columns (Conductor / Cable / CableWire) so the catalogue can act
	as the data source that enriches a generated terminal strip page.
*/
struct WireSpec
{
		// --- Catalogue identity ---
		QString wireId;                 ///< Primary key, e.g. "W31"
		QString manufacturerName;       ///< e.g. "Lapp Group"
		QString manufacturerPartNo;     ///< e.g. "ÖLFLEX 100 BK"
		QString supplierName;           ///< e.g. "RS Components"
		QString supplierPartNo;         ///< e.g. "724-1652"

		// --- Physical / electrical ---
		double  crossSectionMm2 = 0.0;  ///< Conductor cross-section, mm²
		double  outerDiaMm      = 0.0;  ///< Cable outer diameter, mm
		double  insulationDiaMm = 0.0;  ///< Single-core insulation diameter, mm
		int     numCores        = 1;    ///< 1 = single wire, >1 = multi-core cable
		/// Per-core colours. coreColors[i] holds that core's colours as
		/// {base, tracer1, tracer2} — 1 to 3 IEC 60757 names (tracers optional).
		QVector<QStringList> coreColors;
		bool    hasShield       = false;
		QString shieldType;             ///< "Braid" / "Foil" / "Both"
		int     voltageRatingV  = 0;
		int     tempRatingC     = 0;
		bool    isFlexible      = false;
		QString colorPrimary;           ///< Primary colour (IEC 60757)
		QString notes;

		bool isValid() const { return !wireId.isEmpty(); }
		bool isCable() const { return numCores > 1; }

		/// Flat list of every core colour (base + tracers), for swatch display.
		QStringList allCoreColors() const
		{
			QStringList out;
			for (const QStringList &core : coreColors)
				out += core;
			return out;
		}

		/// Best single colour to represent this wire: the explicit primary
		/// colour if set, else the first core's base colour. Used for the
		/// conductor colour and the catalogue picker swatch.
		QString effectiveColor() const
		{
			if (!colorPrimary.isEmpty())
				return colorPrimary;
			for (const QStringList &core : coreColors)
				if (!core.isEmpty())
					return core.first();
			return QString();
		}

		/**
			@brief specLabel
			Produces the SWE-style spec string used in terminal strip pages,
			e.g. "Brown_1x0.5mm²" or "CAN_Cable_4x0.5mm²".
			@param familyName : optional cable family prefix (e.g. "CAN_Cable").
			When empty the primary colour is used as the prefix.
		*/
		QString specLabel(const QString &familyName = QString()) const
		{
			const QString prefix = familyName.isEmpty() ? colorPrimary : familyName;
			QString cs = QString::number(crossSectionMm2);
			return QStringLiteral("%1_%2x%3mm²")
					.arg(prefix.isEmpty() ? QStringLiteral("Wire") : prefix)
					.arg(numCores)
					.arg(cs);
		}

		bool operator==(const WireSpec &o) const { return wireId == o.wireId; }
		bool operator!=(const WireSpec &o) const { return !(*this == o); }
};

#endif // WIRESPEC_H
