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
#ifndef QETSETTINGS_H
#define QETSETTINGS_H

#include <Qt>
#include <QColor>
#include <QStringList>

/**
 *Interface to use QSettings in QElectroTech
 *Prefer use this namespace instead of directly QSettings
 *It's notably useful when some QSettings value are used in
 *different part of QElectroTech source code
 */
namespace QetSettings
{
	void setHdpiScaleFactorRoundingPolicy(const QString &policy_str);
	void setHdpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy policy);
	Qt::HighDpiScaleFactorRoundingPolicy hdpiScaleFactorRoundingPolicy(
			Qt::HighDpiScaleFactorRoundingPolicy default_policy = Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

	bool scriptingEnabled();
	void setScriptingEnabled(bool enabled);
	bool scriptingForcedByEnvironment();
	bool qetdbEnabled();
	void setQetdbEnabled(bool enabled);
	bool workingCopyRecovery();
	void setWorkingCopyRecovery(bool enabled);
	bool liveAssistantEnabled();
	void setLiveAssistantEnabled(bool enabled);
	bool liveSkipStartWarning();
	void setLiveSkipStartWarning(bool skip);
	bool liveAskFirst();
	void setLiveAskFirst(bool ask);

		// Whether symbols with one label in one place are one device for the
		// duplicate check (off by default).
	bool duplicateLabelsByDesignation();
	void setDuplicateLabelsByDesignation(bool by_designation);

		// Whether labels differing only in letter case (K1, k1) are distinct
		// for the duplicate check.
	bool duplicateLabelsCaseSensitive();
	void setDuplicateLabelsCaseSensitive(bool sensitive);
		// Whether a wire number need only be unique within its folio's
		// plant (=) for the duplicate check. Off by default.
	bool duplicateLabelsSignalsByPlant();
	void setDuplicateLabelsSignalsByPlant(bool by_plant);

		// Whether a new folio takes the group and sub-group of the folio
		// before it. Off by default.
	bool newFolioTakesPreviousGroup();
	void setNewFolioTakesPreviousGroup(bool takes);
		// Whether placing a macro asks for its device labels, proposing the
		// next free ones. Off by default. A macro with a '?' always asks.
	bool askLabelsPlacingMacro();
	void setAskLabelsPlacingMacro(bool ask);

	/**
		Whether a terminal strip draws the wires which end at it: one
		stub per terminal which has a wire of its own, going up from the
		strip like its cables go down, naming where the wire lands and
		saying what it is made of. Off by default, so that the projects
		drawn before it existed look the same.
	*/
	bool stripWiresShown();
	void setStripWiresShown(bool shown);

	/**
		Whether a terminal strip is sorted by the label of its terminals
		("X1:2" before "X1:10") instead of the way the plan is drawn
		(folio by folio, then left to right). Used when a strip is made
		by the automatic assignment and by "Automatic position". Off by
		default: the plan order, which keeps N and PE where they are
		drawn.
	*/
	bool stripSortedByLabel();
	void setStripSortedByLabel(bool by_label);

	/**
		@brief Free-text drawing conventions for this installation (grid,
		flow direction, routing, tagging, grouping, ...), written once by
		the user and read by every AI assistant that connects -- live or
		headless -- through qet-assistant.json and qet.houseStyle().
		Empty when never set.
	*/
	QString houseStyle();
	void setHouseStyle(const QString &text);

	/**
		@brief The sheet (folio) background the diagram editors draw.

		Both halves are needed, not just the colour: with custom == false
		the sheet follows the system ("Couleur systeme") and the views are
		free to invert the lightness on a dark palette, while the very same
		#ffffff with custom == true is an explicit "always white" that
		must not be inverted. Storing only the colour would collapse the
		two on the next start.
	*/
	struct SheetBackground
	{
		QColor color;
		bool custom = false;
		/// Yellow symbols and red conductors on the sheet instead of one
		/// light ink (PaletteGraphicsView::setColoredInk()).
		bool colored_ink = false;
	};

	SheetBackground sheetBackground();
	void setSheetBackground(const SheetBackground &background);

	QStringList sheetBackgroundRecentColors();
	void setSheetBackgroundRecentColors(const QStringList &colors);
}

#endif // QETSETTINGS_H
