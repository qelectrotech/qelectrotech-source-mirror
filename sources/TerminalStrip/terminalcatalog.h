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
#ifndef TERMINALCATALOG_H
#define TERMINALCATALOG_H

#include <QLocale>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * @brief The TerminalCatalog namespace
 * Data shared between the terminal strip editor and the element editor :
 * usual sections (mm2) and widths (mm) of the terminals sold by the manufacturers.
 * Only the values found in the catalogs of the manufacturers are listed,
 * any other value can be typed by the user.
 * For Legrand and ABB Entrelec the value is the pitch ("pas") of the catalog.
 */
namespace TerminalCatalog
{
	/**
	 * How the wire is clamped in the terminal.
	 * Used by the realistic drawing of the terminal strip.
	 */
	enum class Clamp {
		Screw,	///< Screw clamp (Phoenix Contact UT, Weidmüller WDU...)
		Spring,	///< Spring-cage clamp, opened with a screwdriver (Phoenix Contact ST)
		PushIn	///< Push-in clamp (WAGO TOPJOB S)
	};

	/**
	 * Width (mm) of a terminal sold by a manufacturer.
	 * For ABB Entrelec the value is the spacing (pitch) given by the catalog.
	 */
	struct WidthEntry { double mm; const char *label; double section; const char *manufacturer; Clamp clamp; };

	inline const QVector<double> &sections()
	{
		static const QVector<double> sections_ {0.5, 0.75, 1, 1.5, 2.5, 4, 6, 10, 16, 25, 35, 50, 70, 95, 120, 150, 185, 240, 300};
		return sections_;
	}

	/**
	 * @brief awgForSection
	 * @param mm2 : a section in mm2
	 * @return the nearest AWG / kcmil size, as text ("14", "1/0", "250 kcmil"),
	 * or an empty string if the section has no usual equivalent.
	 * The correspondence is approximate (the usual table of the manufacturers),
	 * the two systems don't have the same steps.
	 */
	inline QString awgForSection(double mm2)
	{
		struct Awg { double mm2; const char *awg; };
		static const Awg table_[] {
			{0.5,  "20"},      {0.75, "18"},      {1,   "17"},
			{1.5,  "16"},      {2.5,  "14"},      {4,   "12"},
			{6,    "10"},      {10,   "8"},       {16,  "6"},
			{25,   "4"},       {35,   "2"},       {50,  "1/0"},
			{70,   "2/0"},     {95,   "3/0"},     {120, "250 kcmil"},
			{150,  "300 kcmil"}, {185, "350 kcmil"}, {240, "500 kcmil"},
			{300,  "600 kcmil"}
		};
		for (const auto &entry : table_) {
			if (qAbs(entry.mm2 - mm2) < 0.0001) {
				return QString::fromLatin1(entry.awg);
			}
		}
		return QString();
	}

	/**
	 * @brief sectionText
	 * @param mm2 : a section in mm2
	 * @param unit_text : the text of the section with its unit, for example "2,5 mm²"
	 * @return @a unit_text followed by the AWG size if there is one : "2,5 mm² (AWG 14)"
	 */
	inline QString sectionText(double mm2, const QString &unit_text)
	{
		const auto awg_ = awgForSection(mm2);
		if (awg_.isEmpty()) {
			return unit_text;
		}
		return awg_.contains(QLatin1String("kcmil"))
				? unit_text + QStringLiteral(" (%1)").arg(awg_)
				: unit_text + QStringLiteral(" (AWG %1)").arg(awg_);
	}

	inline const QVector<WidthEntry> &widthEntries()
	{
		static const QVector<WidthEntry> entries_ {
			{4.2,  "Phoenix Contact ST 1,5", 1.5, "Phoenix Contact", Clamp::Spring},
			{5.0,  "ABB Entrelec MA 2.5/5", 2.5, "ABB Entrelec", Clamp::Screw},
			{5.1,  "Weidmüller WDU 2.5", 2.5, "Weidmüller", Clamp::Screw},
			{5.2,  "WAGO 2002 (TOPJOB S)", 2.5, "WAGO", Clamp::PushIn},
			{5.2,  "Phoenix Contact ST 2,5 / ST 2,5-QUATTRO", 2.5, "Phoenix Contact", Clamp::Spring},
			{6.0,  "ABB Entrelec M 4/6", 4.0, "ABB Entrelec", Clamp::Screw},
			{6.0,  "Legrand Viking 3 (4 mm²)", 4.0, "Legrand", Clamp::Screw},
			{6.1,  "Weidmüller WDU 4", 4.0, "Weidmüller", Clamp::Screw},
			{6.2,  "Phoenix Contact UT 4", 4.0, "Phoenix Contact", Clamp::Screw},
			{6.2,  "Phoenix Contact ST 4", 4.0, "Phoenix Contact", Clamp::Spring},
			{7.5,  "WAGO 2006 (TOPJOB S)", 6.0, "WAGO", Clamp::PushIn},
			{7.9,  "Weidmüller WDU 6", 6.0, "Weidmüller", Clamp::Screw},
			{8.0,  "ABB Entrelec M 6/8", 6.0, "ABB Entrelec", Clamp::Screw},
			{8.0,  "ABB Entrelec M 4/8", 4.0, "ABB Entrelec", Clamp::Screw},
			{8.2,  "Phoenix Contact UT 6", 6.0, "Phoenix Contact", Clamp::Screw},
			{10.0, "ABB Entrelec M 10/10", 10.0, "ABB Entrelec", Clamp::Screw},
			{10.2, "Phoenix Contact UT 10", 10.0, "Phoenix Contact", Clamp::Screw},
			{11.9, "Weidmüller WDU 16/ZA", 16.0, "Weidmüller", Clamp::Screw},
			{12.0, "ABB Entrelec M 16/12", 16.0, "ABB Entrelec", Clamp::Screw},
			{12.0, "Legrand Viking 3 (16 mm²)", 16.0, "Legrand", Clamp::Screw},
			{12.2, "Phoenix Contact UT 16", 16.0, "Phoenix Contact", Clamp::Screw},
			{16.0, "ABB Entrelec M 35/16", 35.0, "ABB Entrelec", Clamp::Screw},
			{16.0, "Phoenix Contact UT 35", 35.0, "Phoenix Contact", Clamp::Screw},
			{16.0, "Weidmüller WDU 35N", 35.0, "Weidmüller", Clamp::Screw},
			{26.0, "ABB Entrelec M 35/26.FF", 35.0, "ABB Entrelec", Clamp::Screw},
			{31.0, "ABB Entrelec M 70/31.AF", 70.0, "ABB Entrelec", Clamp::Screw},
			{35.0, "ABB Entrelec M 120/35.F", 120.0, "ABB Entrelec", Clamp::Screw},
			{42.0, "ABB Entrelec M 300/42.FF", 300.0, "ABB Entrelec", Clamp::Screw}
		};
		return entries_;
	}
	/// Manufacturers of the catalog, in the order of the first entry.
	inline QStringList manufacturers()
	{
		QStringList list_;
		for (const auto &entry : widthEntries()) {
			const auto name_ = QString::fromUtf8(entry.manufacturer);
			if (!list_.contains(name_)) {
				list_ << name_;
			}
		}
		return list_;
	}

	/**
	 * @brief entryByLabel
	 * @param label : the label of an entry, as stored in the element
	 * informations (RealTerminal::catalogEntryInfoKey())
	 * @return the entry of the catalog with this label, or nullptr
	 */
	inline const WidthEntry *entryByLabel(const QString &label)
	{
		if (label.isEmpty()) {
			return nullptr;
		}
		for (const auto &entry : widthEntries()) {
			if (label == QString::fromUtf8(entry.label)) {
				return &entry;
			}
		}
		return nullptr;
	}

	/**
	 * @brief modelName
	 * @param entry
	 * @return the label of @a entry without the name of its manufacturer :
	 * "ST 4" for "Phoenix Contact ST 4"
	 */
	inline QString modelName(const WidthEntry &entry)
	{
		const auto label_ = QString::fromUtf8(entry.label);
		const auto manufacturer_ = QString::fromUtf8(entry.manufacturer);
		if (label_.startsWith(manufacturer_ + QLatin1Char(' '))) {
			return label_.mid(manufacturer_.size() + 1);
		}
		return label_;
	}
}

#endif // TERMINALCATALOG_H
