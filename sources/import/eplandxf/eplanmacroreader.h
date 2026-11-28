/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef EPLANMACROREADER_H
#define EPLANMACROREADER_H

#include "eplanelementbuilder.h"

#include <QStringList>

/**
	@brief Reads the schematic of an EPLAN window macro (.ema, "EplanPxfRoot"
	XML) as manufacturers ship it inside their EDZ parts.

	Of the macro's variants the multi-line schematics (MacroFuncType 1) are
	read, one at a time: lines (O31), polylines (O34), rectangles (O89,
	filled when EPLAN fills them), arcs (O32), ellipses (O90), texts (O30),
	the drawings instances place (O144 -> O5), connection points (O17 with a connection designation and an
	optional description such as "IN") and black-box frames (O17 with a
	frame) with their identifying letters and function text. Coordinates
	stay in EPLAN mm with y up, as EplanElementBuilder expects them.

	Pictures (manufacturer logos) and objects of other kinds are left out and
	listed in skipped().
*/
/// One variant of a macro: a representation type (MacroFuncType: 1
/// multi-line, 2 single-line, 4 overview, 6 panel layout) and its variant
/// (0 = A, 1 = B ...)
struct EplanMacroVariant
{
	int func_type = 0;
	int id = 0;
	QString description;   ///< one language, line breaks as " - "
	int pins = 0;          ///< connection points
};

class EplanMacroReader
{
	public:
		/**
			Reads the @p index-th multi-line schematic variant (0 = variant A).
			Macros may carry several: the whole device and parts of it, such
			as the power supply or one rack layout per base unit.
		*/
		bool read(const QString &path, int index = 0);
		/// number of multi-line schematic variants of the macro
		static int variantCount(const QString &path);
		/// what the read variant shows ("Power Supply", "Rack layout A"),
		/// from its description; may be empty
		QString variantName() const { return m_variant_name; }
		/**
			A macro with several variants per representation type, as the
			macro packages of a manufacturer hold them (a PLC card: one
			variant per channel, one with all channels, the overview):
			load() it once, then readVariant() each of variants(). Other
			than read(), this also takes the graphics the macro places
			(O5 placed by O144, with its scale and rotation), free lines,
			arcs, rectangles and texts.
		*/
		bool load(const QByteArray &macro);
		QVector<EplanMacroVariant> variants() const;
		bool readVariant(int func_type, int variant_id);
		/**
			Reads the 2D panel layout variant (MacroFuncType 6) instead: the
			front view the manufacturer draws as sub-objects of a graphic
			(lines S31, arcs S32, rectangles S89, texts S30), in mm, y up.
			Fails when the macro has no such variant or it holds no drawing.
		*/
		bool readLayout(const QString &path);
		QString errorString() const { return m_error; }
		EplanSymbolData symbol() const { return m_symbol; }
		/// identifying letters of the black box ("T"), empty when it has none
		QString deviceLetters() const { return m_letters; }
		/// macro description, EPLAN multilingual text resolved to one language
		QString description() const { return m_description; }
		QStringList skipped() const { return m_skipped; }

	private:
		void addFunction(const QDomElement &function);

		QDomDocument m_doc;   ///< the macro of load()
		EplanSymbolData m_symbol;
		QString m_letters;
		QString m_description;
		QString m_variant_name;
		QStringList m_skipped;
		QString m_error;
};

#endif // EPLANMACROREADER_H
