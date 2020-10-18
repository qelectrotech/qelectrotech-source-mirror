/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "styleeditor.h"
#include "customelementgraphicpart.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "qeticons.h"
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLabel>

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param p La partie a editer
	@param parent le Widget parent
*/
StyleEditor::StyleEditor(QETElementEditor *editor, CustomElementGraphicPart *p, QWidget *parent) :
	ElementItemEditor(editor, parent),
	part(p)
{
	// couleur
	outline_color = new QComboBox(this);
	outline_color -> addItem(QET::Icons::ColorHTMLGrayBlack, tr("Noir", "element part color"), CustomElementGraphicPart::BlackColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteWhite, tr("Blanc", "element part color"), CustomElementGraphicPart::WhiteColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenLime, tr("Vert", "element part color"), CustomElementGraphicPart::GreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLRedRed, tr("Rouge", "element part color"), CustomElementGraphicPart::RedColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueBlue, tr("Bleu", "element part color"), CustomElementGraphicPart::BlueColor);
	outline_color -> addItem(QET::Icons::ColorGray, tr("Gris", "element part color"), CustomElementGraphicPart::GrayColor);
	outline_color -> addItem(QET::Icons::ColorBrown, tr("Marron", "element part color"), CustomElementGraphicPart::BrunColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowYellow, tr("Jaune", "element part color"), CustomElementGraphicPart::YellowColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanCyan, tr("Cyan", "element part color"), CustomElementGraphicPart::CyanColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleMagenta, tr("Magenta", "element part color"), CustomElementGraphicPart::MagentaColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGraySilver, tr("Gris clair", "element part color"), CustomElementGraphicPart::LightgrayColor);
	outline_color -> addItem(QET::Icons::ColorOrange, tr("Orange", "element part color"), CustomElementGraphicPart::OrangeColor);
	outline_color -> addItem(QET::Icons::ColorPurple, tr("Violet", "element part color"), CustomElementGraphicPart::PurpleColor);
	outline_color -> insertSeparator(outline_color -> count());
	outline_color -> addItem(QET::Icons::ColorHTMLPinkPink, tr("Pink : Pink", "element part color"), CustomElementGraphicPart::HTMLPinkPinkColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPinkLightPink, tr("Pink : LightPink", "element part color"), CustomElementGraphicPart::HTMLPinkLightPinkColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPinkHotPink, tr("Pink : HotPink", "element part color"), CustomElementGraphicPart::HTMLPinkHotPinkColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPinkDeepPink, tr("Pink : DeepPink", "element part color"), CustomElementGraphicPart::HTMLPinkDeepPinkColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPinkPaleVioletRed, tr("Pink : PaleVioletRed", "element part color"), CustomElementGraphicPart::HTMLPinkPaleVioletRedColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPinkMediumVioletRed, tr("Pink : MediumVioletRed", "element part color"), CustomElementGraphicPart::HTMLPinkMediumVioletRedColor);
	outline_color -> addItem(QET::Icons::ColorHTMLRedLightSalmon, tr("Red : LightSalmon", "element part color"), CustomElementGraphicPart::HTMLRedLightSalmonColor);
	outline_color -> addItem(QET::Icons::ColorHTMLRedSalmon, tr("Red : Salmon", "element part color"), CustomElementGraphicPart::HTMLRedSalmonColor);
	outline_color -> addItem(QET::Icons::ColorHTMLRedDarkSalmon, tr("Red : DarkSalmon", "element part color"), CustomElementGraphicPart::HTMLRedDarkSalmonColor);
	outline_color -> addItem(QET::Icons::ColorHTMLRedLightCoral, tr("Red : LightCoral", "element part color"), CustomElementGraphicPart::HTMLRedLightCoralColor);
	outline_color -> addItem(QET::Icons::ColorHTMLRedIndianRed, tr("Red : IndianRed", "element part color"), CustomElementGraphicPart::HTMLRedIndianRedColor);
	outline_color -> addItem(QET::Icons::ColorHTMLRedCrimson, tr("Red : Crimson", "element part color"), CustomElementGraphicPart::HTMLRedCrimsonColor);
	outline_color -> addItem(QET::Icons::ColorHTMLRedFirebrick, tr("Red : Firebrick", "element part color"), CustomElementGraphicPart::HTMLRedFirebrickColor);
	outline_color -> addItem(QET::Icons::ColorHTMLRedDarkRed, tr("Red : DarkRed", "element part color"), CustomElementGraphicPart::HTMLRedDarkRedColor);
	outline_color -> addItem(QET::Icons::ColorHTMLRedRed, tr("Red : Red", "element part color"), CustomElementGraphicPart::HTMLRedRedColor);
	outline_color -> addItem(QET::Icons::ColorHTMLOrangeOrangeRed, tr("Orange : OrangeRed", "element part color"), CustomElementGraphicPart::HTMLOrangeOrangeRedColor);
	outline_color -> addItem(QET::Icons::ColorHTMLOrangeTomato, tr("Orange : Tomato", "element part color"), CustomElementGraphicPart::HTMLOrangeTomatoColor);
	outline_color -> addItem(QET::Icons::ColorHTMLOrangeCoral, tr("Orange : Coral", "element part color"), CustomElementGraphicPart::HTMLOrangeCoralColor);
	outline_color -> addItem(QET::Icons::ColorHTMLOrangeDarkOrange, tr("Orange : DarkOrange", "element part color"), CustomElementGraphicPart::HTMLOrangeDarkOrangeColor);
	outline_color -> addItem(QET::Icons::ColorHTMLOrangeOrange, tr("Orange : Orange", "element part color"), CustomElementGraphicPart::HTMLOrangeOrangeColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowYellow, tr("Yellow : Yellow", "element part color"), CustomElementGraphicPart::HTMLYellowYellowColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowLightYellow, tr("Yellow : LightYellow", "element part color"), CustomElementGraphicPart::HTMLYellowLightYellowColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowLemonChiffon, tr("Yellow : LemonChiffon", "element part color"), CustomElementGraphicPart::HTMLYellowLemonChiffonColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowLightGoldenrodYellow, tr("Yellow : LightGoldenrodYellow", "element part color"), CustomElementGraphicPart::HTMLYellowLightGoldenrodYellowColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowPapayaWhip, tr("Yellow : PapayaWhip", "element part color"), CustomElementGraphicPart::HTMLYellowPapayaWhipColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowMoccasin, tr("Yellow : Moccasin", "element part color"), CustomElementGraphicPart::HTMLYellowMoccasinColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowPeachPuff, tr("Yellow : PeachPuff", "element part color"), CustomElementGraphicPart::HTMLYellowPeachPuffColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowPaleGoldenrod, tr("Yellow : PaleGoldenrod", "element part color"), CustomElementGraphicPart::HTMLYellowPaleGoldenrodColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowKhaki, tr("Yellow : Khaki", "element part color"), CustomElementGraphicPart::HTMLYellowKhakiColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowDarkKhaki, tr("Yellow : DarkKhaki", "element part color"), CustomElementGraphicPart::HTMLYellowDarkKhakiColor);
	outline_color -> addItem(QET::Icons::ColorHTMLYellowGold, tr("Yellow : Gold", "element part color"), CustomElementGraphicPart::HTMLYellowGoldColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownCornsilk, tr("Brown : Cornsilk", "element part color"), CustomElementGraphicPart::HTMLBrownCornsilkColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownBlanchedAlmond, tr("Brown : BlanchedAlmond", "element part color"), CustomElementGraphicPart::HTMLBrownBlanchedAlmondColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownBisque, tr("Brown : Bisque", "element part color"), CustomElementGraphicPart::HTMLBrownBisqueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownNavajoWhite, tr("Brown : NavajoWhite", "element part color"), CustomElementGraphicPart::HTMLBrownNavajoWhiteColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownWheat, tr("Brown : Wheat", "element part color"), CustomElementGraphicPart::HTMLBrownWheatColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownBurlywood, tr("Brown : Burlywood", "element part color"), CustomElementGraphicPart::HTMLBrownBurlywoodColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownTan, tr("Brown : Tan", "element part color"), CustomElementGraphicPart::HTMLBrownTanColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownRosyBrown, tr("Brown : RosyBrown", "element part color"), CustomElementGraphicPart::HTMLBrownRosyBrownColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownSandyBrown, tr("Brown : SandyBrown", "element part color"), CustomElementGraphicPart::HTMLBrownSandyBrownColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownGoldenrod, tr("Brown : Goldenrod", "element part color"), CustomElementGraphicPart::HTMLBrownGoldenrodColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownDarkGoldenrod, tr("Brown : DarkGoldenrod", "element part color"), CustomElementGraphicPart::HTMLBrownDarkGoldenrodColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownPeru, tr("Brown : Peru", "element part color"), CustomElementGraphicPart::HTMLBrownPeruColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownChocolate, tr("Brown : Chocolate", "element part color"), CustomElementGraphicPart::HTMLBrownChocolateColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownSaddleBrown, tr("Brown : SaddleBrown", "element part color"), CustomElementGraphicPart::HTMLBrownSaddleBrownColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownSienna, tr("Brown : Sienna", "element part color"), CustomElementGraphicPart::HTMLBrownSiennaColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownBrown, tr("Brown : Brown", "element part color"), CustomElementGraphicPart::HTMLBrownBrownColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBrownMaroon, tr("Brown : Maroon", "element part color"), CustomElementGraphicPart::HTMLBrownMaroonColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenDarkOliveGreen, tr("Green : DarkOliveGreen", "element part color"), CustomElementGraphicPart::HTMLGreenDarkOliveGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenOlive, tr("Green : Olive", "element part color"), CustomElementGraphicPart::HTMLGreenOliveColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenOliveDrab, tr("Green : OliveDrab", "element part color"), CustomElementGraphicPart::HTMLGreenOliveDrabColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenYellowGreen, tr("Green : YellowGreen", "element part color"), CustomElementGraphicPart::HTMLGreenYellowGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenLimeGreen, tr("Green : LimeGreen", "element part color"), CustomElementGraphicPart::HTMLGreenLimeGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenLime, tr("Green : Lime", "element part color"), CustomElementGraphicPart::HTMLGreenLimeColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenLawnGreen, tr("Green : LawnGreen", "element part color"), CustomElementGraphicPart::HTMLGreenLawnGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenChartreuse, tr("Green : Chartreuse", "element part color"), CustomElementGraphicPart::HTMLGreenChartreuseColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenGreenYellow, tr("Green : GreenYellow", "element part color"), CustomElementGraphicPart::HTMLGreenGreenYellowColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenSpringGreen, tr("Green : SpringGreen", "element part color"), CustomElementGraphicPart::HTMLGreenSpringGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenMediumSpringGreen, tr("Green : MediumSpringGreen", "element part color"), CustomElementGraphicPart::HTMLGreenMediumSpringGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenLightGreen, tr("Green : LightGreen", "element part color"), CustomElementGraphicPart::HTMLGreenLightGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenPaleGreen, tr("Green : PaleGreen", "element part color"), CustomElementGraphicPart::HTMLGreenPaleGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenDarkSeaGreen, tr("Green : DarkSeaGreen", "element part color"), CustomElementGraphicPart::HTMLGreenDarkSeaGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenMediumAquamarine, tr("Green : MediumAquamarine", "element part color"), CustomElementGraphicPart::HTMLGreenMediumAquamarineColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenMediumSeaGreen, tr("Green : MediumSeaGreen", "element part color"), CustomElementGraphicPart::HTMLGreenMediumSeaGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenSeaGreen, tr("Green : SeaGreen", "element part color"), CustomElementGraphicPart::HTMLGreenSeaGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenForestGreen, tr("Green : ForestGreen", "element part color"), CustomElementGraphicPart::HTMLGreenForestGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenGreen, tr("Green : Green", "element part color"), CustomElementGraphicPart::HTMLGreenGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGreenDarkGreen, tr("Green : DarkGreen", "element part color"), CustomElementGraphicPart::HTMLGreenDarkGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanAqua, tr("Cyan : Aqua", "element part color"), CustomElementGraphicPart::HTMLCyanAquaColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanCyan, tr("Cyan : Cyan", "element part color"), CustomElementGraphicPart::HTMLCyanCyanColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanLightCyan, tr("Cyan : LightCyan", "element part color"), CustomElementGraphicPart::HTMLCyanLightCyanColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanPaleTurquoise, tr("Cyan : PaleTurquoise", "element part color"), CustomElementGraphicPart::HTMLCyanPaleTurquoiseColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanAquamarine, tr("Cyan : Aquamarine", "element part color"), CustomElementGraphicPart::HTMLCyanAquamarineColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanTurquoise, tr("Cyan : Turquoise", "element part color"), CustomElementGraphicPart::HTMLCyanTurquoiseColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanMediumTurquoise, tr("Cyan : MediumTurquoise", "element part color"), CustomElementGraphicPart::HTMLCyanMediumTurquoiseColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanDarkTurquoise, tr("Cyan : DarkTurquoise", "element part color"), CustomElementGraphicPart::HTMLCyanDarkTurquoiseColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanLightSeaGreen, tr("Cyan : LightSeaGreen", "element part color"), CustomElementGraphicPart::HTMLCyanLightSeaGreenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanCadetBlue, tr("Cyan : CadetBlue", "element part color"), CustomElementGraphicPart::HTMLCyanCadetBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanDarkCyan, tr("Cyan : DarkCyan", "element part color"), CustomElementGraphicPart::HTMLCyanDarkCyanColor);
	outline_color -> addItem(QET::Icons::ColorHTMLCyanTeal, tr("Cyan : Teal", "element part color"), CustomElementGraphicPart::HTMLCyanTealColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueLightSteelBlue, tr("Blue : LightSteelBlue", "element part color"), CustomElementGraphicPart::HTMLBlueLightSteelBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBluePowderBlue, tr("Blue : PowderBlue", "element part color"), CustomElementGraphicPart::HTMLBluePowderBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueLightBlue, tr("Blue : LightBlue", "element part color"), CustomElementGraphicPart::HTMLBlueLightBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueSkyBlue, tr("Blue : SkyBlue", "element part color"), CustomElementGraphicPart::HTMLBlueSkyBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueLightSkyBlue, tr("Blue : LightSkyBlue", "element part color"), CustomElementGraphicPart::HTMLBlueLightSkyBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueDeepSkyBlue, tr("Blue : DeepSkyBlue", "element part color"), CustomElementGraphicPart::HTMLBlueDeepSkyBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueDodgerBlue, tr("Blue : DodgerBlue", "element part color"), CustomElementGraphicPart::HTMLBlueDodgerBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueCornflowerBlue, tr("Blue : CornflowerBlue", "element part color"), CustomElementGraphicPart::HTMLBlueCornflowerBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueSteelBlue, tr("Blue : SteelBlue", "element part color"), CustomElementGraphicPart::HTMLBlueSteelBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueRoyalBlue, tr("Blue : RoyalBlue", "element part color"), CustomElementGraphicPart::HTMLBlueRoyalBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueBlue, tr("Blue : Blue", "element part color"), CustomElementGraphicPart::HTMLBlueBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueMediumBlue, tr("Blue : MediumBlue", "element part color"), CustomElementGraphicPart::HTMLBlueMediumBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueDarkBlue, tr("Blue : DarkBlue", "element part color"), CustomElementGraphicPart::HTMLBlueDarkBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueNavy, tr("Blue : Navy", "element part color"), CustomElementGraphicPart::HTMLBlueNavyColor);
	outline_color -> addItem(QET::Icons::ColorHTMLBlueMidnightBlue, tr("Blue : MidnightBlue", "element part color"), CustomElementGraphicPart::HTMLBlueMidnightBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleLavender, tr("Purple : Lavender", "element part color"), CustomElementGraphicPart::HTMLPurpleLavenderColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleThistle, tr("Purple : Thistle", "element part color"), CustomElementGraphicPart::HTMLPurpleThistleColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurplePlum, tr("Purple : Plum", "element part color"), CustomElementGraphicPart::HTMLPurplePlumColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleViolet, tr("Purple : Violet", "element part color"), CustomElementGraphicPart::HTMLPurpleVioletColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleOrchid, tr("Purple : Orchid", "element part color"), CustomElementGraphicPart::HTMLPurpleOrchidColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleFuchsia, tr("Purple : Fuchsia", "element part color"), CustomElementGraphicPart::HTMLPurpleFuchsiaColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleMagenta, tr("Purple : Magenta", "element part color"), CustomElementGraphicPart::HTMLPurpleMagentaColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleMediumOrchid, tr("Purple : MediumOrchid", "element part color"), CustomElementGraphicPart::HTMLPurpleMediumOrchidColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleMediumPurple, tr("Purple : MediumPurple", "element part color"), CustomElementGraphicPart::HTMLPurpleMediumPurpleColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleBlueViolet, tr("Purple : BlueViolet", "element part color"), CustomElementGraphicPart::HTMLPurpleBlueVioletColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleDarkViolet, tr("Purple : DarkViolet", "element part color"), CustomElementGraphicPart::HTMLPurpleDarkVioletColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleDarkOrchid, tr("Purple : DarkOrchid", "element part color"), CustomElementGraphicPart::HTMLPurpleDarkOrchidColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleDarkMagenta, tr("Purple : DarkMagenta", "element part color"), CustomElementGraphicPart::HTMLPurpleDarkMagentaColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurplePurple, tr("Purple : Purple", "element part color"), CustomElementGraphicPart::HTMLPurplePurpleColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleIndigo, tr("Purple : Indigo", "element part color"), CustomElementGraphicPart::HTMLPurpleIndigoColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleDarkSlateBlue, tr("Purple : DarkSlateBlue", "element part color"), CustomElementGraphicPart::HTMLPurpleDarkSlateBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleSlateBlue, tr("Purple : SlateBlue", "element part color"), CustomElementGraphicPart::HTMLPurpleSlateBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLPurpleMediumSlateBlue, tr("Purple : MediumSlateBlue", "element part color"), CustomElementGraphicPart::HTMLPurpleMediumSlateBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteWhite, tr("White : White", "element part color"), CustomElementGraphicPart::HTMLWhiteWhiteColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteSnow, tr("White : Snow", "element part color"), CustomElementGraphicPart::HTMLWhiteSnowColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteHoneydew, tr("White : Honeydew", "element part color"), CustomElementGraphicPart::HTMLWhiteHoneydewColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteMintCream, tr("White : MintCream", "element part color"), CustomElementGraphicPart::HTMLWhiteMintCreamColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteAzure, tr("White : Azure", "element part color"), CustomElementGraphicPart::HTMLWhiteAzureColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteAliceBlue, tr("White : AliceBlue", "element part color"), CustomElementGraphicPart::HTMLWhiteAliceBlueColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteGhostWhite, tr("White : GhostWhite", "element part color"), CustomElementGraphicPart::HTMLWhiteGhostWhiteColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteWhiteSmoke, tr("White : WhiteSmoke", "element part color"), CustomElementGraphicPart::HTMLWhiteWhiteSmokeColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteSeashell, tr("White : Seashell", "element part color"), CustomElementGraphicPart::HTMLWhiteSeashellColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteBeige, tr("White : Beige", "element part color"), CustomElementGraphicPart::HTMLWhiteBeigeColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteOldLace, tr("White : OldLace", "element part color"), CustomElementGraphicPart::HTMLWhiteOldLaceColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteFloralWhite, tr("White : FloralWhite", "element part color"), CustomElementGraphicPart::HTMLWhiteFloralWhiteColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteIvory, tr("White : Ivory", "element part color"), CustomElementGraphicPart::HTMLWhiteIvoryColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteAntiqueWhite, tr("White : AntiqueWhite", "element part color"), CustomElementGraphicPart::HTMLWhiteAntiqueWhiteColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteLinen, tr("White : Linen", "element part color"), CustomElementGraphicPart::HTMLWhiteLinenColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteLavenderBlush, tr("White : LavenderBlush", "element part color"), CustomElementGraphicPart::HTMLWhiteLavenderBlushColor);
	outline_color -> addItem(QET::Icons::ColorHTMLWhiteMistyRose, tr("White : MistyRose", "element part color"), CustomElementGraphicPart::HTMLWhiteMistyRoseColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGrayGainsboro, tr("Gray : Gainsboro", "element part color"), CustomElementGraphicPart::HTMLGrayGainsboroColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGrayLightGray, tr("Gray : LightGray", "element part color"), CustomElementGraphicPart::HTMLGrayLightGrayColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGraySilver, tr("Gray : Silver", "element part color"), CustomElementGraphicPart::HTMLGraySilverColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGrayDarkGray, tr("Gray : DarkGray", "element part color"), CustomElementGraphicPart::HTMLGrayDarkGrayColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGrayGray, tr("Gray : Gray", "element part color"), CustomElementGraphicPart::HTMLGrayGrayColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGrayDimGray, tr("Gray : DimGray", "element part color"), CustomElementGraphicPart::HTMLGrayDimGrayColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGrayLightSlateGray, tr("Gray : LightSlateGray", "element part color"), CustomElementGraphicPart::HTMLGrayLightSlateGrayColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGraySlateGray, tr("Gray : SlateGray", "element part color"), CustomElementGraphicPart::HTMLGraySlateGrayColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGrayDarkSlateGray, tr("Gray : DarkSlateGray", "element part color"), CustomElementGraphicPart::HTMLGrayDarkSlateGrayColor);
	outline_color -> addItem(QET::Icons::ColorHTMLGrayBlack, tr("Gray : Black", "element part color"), CustomElementGraphicPart::HTMLGrayBlackColor);
	outline_color -> insertSeparator(outline_color -> count());
	outline_color -> addItem(tr("Aucun", "element part color"), CustomElementGraphicPart::NoneColor);

	// style
	line_style = new QComboBox(this);
	line_style -> addItem(tr("Normal",       "element part line style"), CustomElementGraphicPart::NormalStyle);
	line_style -> addItem(tr("Tiret",        "element part line style"), CustomElementGraphicPart::DashedStyle);
	line_style -> addItem(tr("Pointillé", "element part line style"), CustomElementGraphicPart::DottedStyle);
	line_style -> addItem(tr("Traits et points", "element part line style"), CustomElementGraphicPart::DashdottedStyle);
	//normal_style -> setChecked(true);

	// epaisseur
	size_weight = new QComboBox(this);
	size_weight -> addItem(tr("Nulle", "element part weight"),  CustomElementGraphicPart::NoneWeight);
	size_weight -> addItem(tr("Fine", "element part weight"),  CustomElementGraphicPart::ThinWeight);
	size_weight -> addItem(tr("Normale", "element part weight"),  CustomElementGraphicPart::NormalWeight);
	size_weight -> addItem(tr("Forte", "element part weight"),  CustomElementGraphicPart::UltraWeight);
	size_weight -> addItem(tr("Élevé", "element part weight"),  CustomElementGraphicPart::BigWeight);

	// remplissage
	filling_color = new QComboBox (this);
	filling_color -> addItem(tr("Aucun", "element part filling"), CustomElementGraphicPart::NoneFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGrayBlack, tr("Noir", "element part filling"), CustomElementGraphicPart::BlackFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteWhite, tr("Blanc", "element part filling"), CustomElementGraphicPart::WhiteFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenLime, tr("Vert", "element part filling"), CustomElementGraphicPart::GreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLRedRed, tr("Rouge", "element part filling"), CustomElementGraphicPart::RedFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueBlue, tr("Bleu", "element part filling"), CustomElementGraphicPart::BlueFilling);
	filling_color -> addItem(QET::Icons::ColorGray, tr("Gris", "element part filling"), CustomElementGraphicPart::GrayFilling);
	filling_color -> addItem(QET::Icons::ColorBrown, tr("Marron", "element part filling"), CustomElementGraphicPart::BrunFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowYellow, tr("Jaune", "element part filling"), CustomElementGraphicPart::YellowFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanCyan, tr("Cyan", "element part filling"), CustomElementGraphicPart::CyanFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleMagenta, tr("Magenta", "element part filling"), CustomElementGraphicPart::MagentaFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGraySilver, tr("Gris clair", "element part filling"), CustomElementGraphicPart::LightgrayFilling);
	filling_color -> addItem(QET::Icons::ColorOrange, tr("Orange", "element part filling"), CustomElementGraphicPart::OrangeFilling);
	filling_color -> addItem(QET::Icons::ColorPurple, tr("Violet", "element part filling"), CustomElementGraphicPart::PurpleFilling);
	filling_color -> insertSeparator(filling_color -> count());
	filling_color -> addItem(QET::Icons::ColorHTMLPinkPink, tr("Pink : Pink", "element part filling"), CustomElementGraphicPart::HTMLPinkPinkFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPinkLightPink, tr("Pink : LightPink", "element part filling"), CustomElementGraphicPart::HTMLPinkLightPinkFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPinkHotPink, tr("Pink : HotPink", "element part filling"), CustomElementGraphicPart::HTMLPinkHotPinkFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPinkDeepPink, tr("Pink : DeepPink", "element part filling"), CustomElementGraphicPart::HTMLPinkDeepPinkFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPinkPaleVioletRed, tr("Pink : PaleVioletRed", "element part filling"), CustomElementGraphicPart::HTMLPinkPaleVioletRedFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPinkMediumVioletRed, tr("Pink : MediumVioletRed", "element part filling"), CustomElementGraphicPart::HTMLPinkMediumVioletRedFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLRedLightSalmon, tr("Red : LightSalmon", "element part filling"), CustomElementGraphicPart::HTMLRedLightSalmonFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLRedSalmon, tr("Red : Salmon", "element part filling"), CustomElementGraphicPart::HTMLRedSalmonFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLRedDarkSalmon, tr("Red : DarkSalmon", "element part filling"), CustomElementGraphicPart::HTMLRedDarkSalmonFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLRedLightCoral, tr("Red : LightCoral", "element part filling"), CustomElementGraphicPart::HTMLRedLightCoralFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLRedIndianRed, tr("Red : IndianRed", "element part filling"), CustomElementGraphicPart::HTMLRedIndianRedFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLRedCrimson, tr("Red : Crimson", "element part filling"), CustomElementGraphicPart::HTMLRedCrimsonFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLRedFirebrick, tr("Red : Firebrick", "element part filling"), CustomElementGraphicPart::HTMLRedFirebrickFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLRedDarkRed, tr("Red : DarkRed", "element part filling"), CustomElementGraphicPart::HTMLRedDarkRedFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLRedRed, tr("Red : Red", "element part filling"), CustomElementGraphicPart::HTMLRedRedFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLOrangeOrangeRed, tr("Orange : OrangeRed", "element part filling"), CustomElementGraphicPart::HTMLOrangeOrangeRedFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLOrangeTomato, tr("Orange : Tomato", "element part filling"), CustomElementGraphicPart::HTMLOrangeTomatoFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLOrangeCoral, tr("Orange : Coral", "element part filling"), CustomElementGraphicPart::HTMLOrangeCoralFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLOrangeDarkOrange, tr("Orange : DarkOrange", "element part filling"), CustomElementGraphicPart::HTMLOrangeDarkOrangeFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLOrangeOrange, tr("Orange : Orange", "element part filling"), CustomElementGraphicPart::HTMLOrangeOrangeFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowYellow, tr("Yellow : Yellow", "element part filling"), CustomElementGraphicPart::HTMLYellowYellowFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowLightYellow, tr("Yellow : LightYellow", "element part filling"), CustomElementGraphicPart::HTMLYellowLightYellowFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowLemonChiffon, tr("Yellow : LemonChiffon", "element part filling"), CustomElementGraphicPart::HTMLYellowLemonChiffonFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowLightGoldenrodYellow, tr("Yellow : LightGoldenrodYellow", "element part filling"), CustomElementGraphicPart::HTMLYellowLightGoldenrodYellowFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowPapayaWhip, tr("Yellow : PapayaWhip", "element part filling"), CustomElementGraphicPart::HTMLYellowPapayaWhipFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowMoccasin, tr("Yellow : Moccasin", "element part filling"), CustomElementGraphicPart::HTMLYellowMoccasinFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowPeachPuff, tr("Yellow : PeachPuff", "element part filling"), CustomElementGraphicPart::HTMLYellowPeachPuffFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowPaleGoldenrod, tr("Yellow : PaleGoldenrod", "element part filling"), CustomElementGraphicPart::HTMLYellowPaleGoldenrodFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowKhaki, tr("Yellow : Khaki", "element part filling"), CustomElementGraphicPart::HTMLYellowKhakiFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowDarkKhaki, tr("Yellow : DarkKhaki", "element part filling"), CustomElementGraphicPart::HTMLYellowDarkKhakiFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLYellowGold, tr("Yellow : Gold", "element part filling"), CustomElementGraphicPart::HTMLYellowGoldFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownCornsilk, tr("Brown : Cornsilk", "element part filling"), CustomElementGraphicPart::HTMLBrownCornsilkFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownBlanchedAlmond, tr("Brown : BlanchedAlmond", "element part filling"), CustomElementGraphicPart::HTMLBrownBlanchedAlmondFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownBisque, tr("Brown : Bisque", "element part filling"), CustomElementGraphicPart::HTMLBrownBisqueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownNavajoWhite, tr("Brown : NavajoWhite", "element part filling"), CustomElementGraphicPart::HTMLBrownNavajoWhiteFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownWheat, tr("Brown : Wheat", "element part filling"), CustomElementGraphicPart::HTMLBrownWheatFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownBurlywood, tr("Brown : Burlywood", "element part filling"), CustomElementGraphicPart::HTMLBrownBurlywoodFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownTan, tr("Brown : Tan", "element part filling"), CustomElementGraphicPart::HTMLBrownTanFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownRosyBrown, tr("Brown : RosyBrown", "element part filling"), CustomElementGraphicPart::HTMLBrownRosyBrownFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownSandyBrown, tr("Brown : SandyBrown", "element part filling"), CustomElementGraphicPart::HTMLBrownSandyBrownFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownGoldenrod, tr("Brown : Goldenrod", "element part filling"), CustomElementGraphicPart::HTMLBrownGoldenrodFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownDarkGoldenrod, tr("Brown : DarkGoldenrod", "element part filling"), CustomElementGraphicPart::HTMLBrownDarkGoldenrodFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownPeru, tr("Brown : Peru", "element part filling"), CustomElementGraphicPart::HTMLBrownPeruFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownChocolate, tr("Brown : Chocolate", "element part filling"), CustomElementGraphicPart::HTMLBrownChocolateFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownSaddleBrown, tr("Brown : SaddleBrown", "element part filling"), CustomElementGraphicPart::HTMLBrownSaddleBrownFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownSienna, tr("Brown : Sienna", "element part filling"), CustomElementGraphicPart::HTMLBrownSiennaFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownBrown, tr("Brown : Brown", "element part filling"), CustomElementGraphicPart::HTMLBrownBrownFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBrownMaroon, tr("Brown : Maroon", "element part filling"), CustomElementGraphicPart::HTMLBrownMaroonFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenDarkOliveGreen, tr("Green : DarkOliveGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenDarkOliveGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenOlive, tr("Green : Olive", "element part filling"), CustomElementGraphicPart::HTMLGreenOliveFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenOliveDrab, tr("Green : OliveDrab", "element part filling"), CustomElementGraphicPart::HTMLGreenOliveDrabFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenYellowGreen, tr("Green : YellowGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenYellowGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenLimeGreen, tr("Green : LimeGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenLimeGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenLime, tr("Green : Lime", "element part filling"), CustomElementGraphicPart::HTMLGreenLimeFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenLawnGreen, tr("Green : LawnGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenLawnGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenChartreuse, tr("Green : Chartreuse", "element part filling"), CustomElementGraphicPart::HTMLGreenChartreuseFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenGreenYellow, tr("Green : GreenYellow", "element part filling"), CustomElementGraphicPart::HTMLGreenGreenYellowFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenSpringGreen, tr("Green : SpringGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenSpringGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenMediumSpringGreen, tr("Green : MediumSpringGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenMediumSpringGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenLightGreen, tr("Green : LightGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenLightGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenPaleGreen, tr("Green : PaleGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenPaleGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenDarkSeaGreen, tr("Green : DarkSeaGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenDarkSeaGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenMediumAquamarine, tr("Green : MediumAquamarine", "element part filling"), CustomElementGraphicPart::HTMLGreenMediumAquamarineFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenMediumSeaGreen, tr("Green : MediumSeaGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenMediumSeaGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenSeaGreen, tr("Green : SeaGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenSeaGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenForestGreen, tr("Green : ForestGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenForestGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenGreen, tr("Green : Green", "element part filling"), CustomElementGraphicPart::HTMLGreenGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGreenDarkGreen, tr("Green : DarkGreen", "element part filling"), CustomElementGraphicPart::HTMLGreenDarkGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanAqua, tr("Cyan : Aqua", "element part filling"), CustomElementGraphicPart::HTMLCyanAquaFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanCyan, tr("Cyan : Cyan", "element part filling"), CustomElementGraphicPart::HTMLCyanCyanFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanLightCyan, tr("Cyan : LightCyan", "element part filling"), CustomElementGraphicPart::HTMLCyanLightCyanFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanPaleTurquoise, tr("Cyan : PaleTurquoise", "element part filling"), CustomElementGraphicPart::HTMLCyanPaleTurquoiseFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanAquamarine, tr("Cyan : Aquamarine", "element part filling"), CustomElementGraphicPart::HTMLCyanAquamarineFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanTurquoise, tr("Cyan : Turquoise", "element part filling"), CustomElementGraphicPart::HTMLCyanTurquoiseFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanMediumTurquoise, tr("Cyan : MediumTurquoise", "element part filling"), CustomElementGraphicPart::HTMLCyanMediumTurquoiseFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanDarkTurquoise, tr("Cyan : DarkTurquoise", "element part filling"), CustomElementGraphicPart::HTMLCyanDarkTurquoiseFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanLightSeaGreen, tr("Cyan : LightSeaGreen", "element part filling"), CustomElementGraphicPart::HTMLCyanLightSeaGreenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanCadetBlue, tr("Cyan : CadetBlue", "element part filling"), CustomElementGraphicPart::HTMLCyanCadetBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanDarkCyan, tr("Cyan : DarkCyan", "element part filling"), CustomElementGraphicPart::HTMLCyanDarkCyanFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLCyanTeal, tr("Cyan : Teal", "element part filling"), CustomElementGraphicPart::HTMLCyanTealFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueLightSteelBlue, tr("Blue : LightSteelBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueLightSteelBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBluePowderBlue, tr("Blue : PowderBlue", "element part filling"), CustomElementGraphicPart::HTMLBluePowderBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueLightBlue, tr("Blue : LightBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueLightBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueSkyBlue, tr("Blue : SkyBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueSkyBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueLightSkyBlue, tr("Blue : LightSkyBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueLightSkyBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueDeepSkyBlue, tr("Blue : DeepSkyBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueDeepSkyBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueDodgerBlue, tr("Blue : DodgerBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueDodgerBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueCornflowerBlue, tr("Blue : CornflowerBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueCornflowerBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueSteelBlue, tr("Blue : SteelBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueSteelBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueRoyalBlue, tr("Blue : RoyalBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueRoyalBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueBlue, tr("Blue : Blue", "element part filling"), CustomElementGraphicPart::HTMLBlueBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueMediumBlue, tr("Blue : MediumBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueMediumBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueDarkBlue, tr("Blue : DarkBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueDarkBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueNavy, tr("Blue : Navy", "element part filling"), CustomElementGraphicPart::HTMLBlueNavyFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLBlueMidnightBlue, tr("Blue : MidnightBlue", "element part filling"), CustomElementGraphicPart::HTMLBlueMidnightBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleLavender, tr("Purple : Lavender", "element part filling"), CustomElementGraphicPart::HTMLPurpleLavenderFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleThistle, tr("Purple : Thistle", "element part filling"), CustomElementGraphicPart::HTMLPurpleThistleFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurplePlum, tr("Purple : Plum", "element part filling"), CustomElementGraphicPart::HTMLPurplePlumFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleViolet, tr("Purple : Violet", "element part filling"), CustomElementGraphicPart::HTMLPurpleVioletFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleOrchid, tr("Purple : Orchid", "element part filling"), CustomElementGraphicPart::HTMLPurpleOrchidFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleFuchsia, tr("Purple : Fuchsia", "element part filling"), CustomElementGraphicPart::HTMLPurpleFuchsiaFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleMagenta, tr("Purple : Magenta", "element part filling"), CustomElementGraphicPart::HTMLPurpleMagentaFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleMediumOrchid, tr("Purple : MediumOrchid", "element part filling"), CustomElementGraphicPart::HTMLPurpleMediumOrchidFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleMediumPurple, tr("Purple : MediumPurple", "element part filling"), CustomElementGraphicPart::HTMLPurpleMediumPurpleFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleBlueViolet, tr("Purple : BlueViolet", "element part filling"), CustomElementGraphicPart::HTMLPurpleBlueVioletFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleDarkViolet, tr("Purple : DarkViolet", "element part filling"), CustomElementGraphicPart::HTMLPurpleDarkVioletFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleDarkOrchid, tr("Purple : DarkOrchid", "element part filling"), CustomElementGraphicPart::HTMLPurpleDarkOrchidFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleDarkMagenta, tr("Purple : DarkMagenta", "element part filling"), CustomElementGraphicPart::HTMLPurpleDarkMagentaFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurplePurple, tr("Purple : Purple", "element part filling"), CustomElementGraphicPart::HTMLPurplePurpleFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleIndigo, tr("Purple : Indigo", "element part filling"), CustomElementGraphicPart::HTMLPurpleIndigoFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleDarkSlateBlue, tr("Purple : DarkSlateBlue", "element part filling"), CustomElementGraphicPart::HTMLPurpleDarkSlateBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleSlateBlue, tr("Purple : SlateBlue", "element part filling"), CustomElementGraphicPart::HTMLPurpleSlateBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLPurpleMediumSlateBlue, tr("Purple : MediumSlateBlue", "element part filling"), CustomElementGraphicPart::HTMLPurpleMediumSlateBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteWhite, tr("White : White", "element part filling"), CustomElementGraphicPart::HTMLWhiteWhiteFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteSnow, tr("White : Snow", "element part filling"), CustomElementGraphicPart::HTMLWhiteSnowFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteHoneydew, tr("White : Honeydew", "element part filling"), CustomElementGraphicPart::HTMLWhiteHoneydewFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteMintCream, tr("White : MintCream", "element part filling"), CustomElementGraphicPart::HTMLWhiteMintCreamFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteAzure, tr("White : Azure", "element part filling"), CustomElementGraphicPart::HTMLWhiteAzureFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteAliceBlue, tr("White : AliceBlue", "element part filling"), CustomElementGraphicPart::HTMLWhiteAliceBlueFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteGhostWhite, tr("White : GhostWhite", "element part filling"), CustomElementGraphicPart::HTMLWhiteGhostWhiteFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteWhiteSmoke, tr("White : WhiteSmoke", "element part filling"), CustomElementGraphicPart::HTMLWhiteWhiteSmokeFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteSeashell, tr("White : Seashell", "element part filling"), CustomElementGraphicPart::HTMLWhiteSeashellFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteBeige, tr("White : Beige", "element part filling"), CustomElementGraphicPart::HTMLWhiteBeigeFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteOldLace, tr("White : OldLace", "element part filling"), CustomElementGraphicPart::HTMLWhiteOldLaceFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteFloralWhite, tr("White : FloralWhite", "element part filling"), CustomElementGraphicPart::HTMLWhiteFloralWhiteFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteIvory, tr("White : Ivory", "element part filling"), CustomElementGraphicPart::HTMLWhiteIvoryFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteAntiqueWhite, tr("White : AntiqueWhite", "element part filling"), CustomElementGraphicPart::HTMLWhiteAntiqueWhiteFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteLinen, tr("White : Linen", "element part filling"), CustomElementGraphicPart::HTMLWhiteLinenFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteLavenderBlush, tr("White : LavenderBlush", "element part filling"), CustomElementGraphicPart::HTMLWhiteLavenderBlushFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLWhiteMistyRose, tr("White : MistyRose", "element part filling"), CustomElementGraphicPart::HTMLWhiteMistyRoseFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGrayGainsboro, tr("Gray : Gainsboro", "element part filling"), CustomElementGraphicPart::HTMLGrayGainsboroFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGrayLightGray, tr("Gray : LightGray", "element part filling"), CustomElementGraphicPart::HTMLGrayLightGrayFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGraySilver, tr("Gray : Silver", "element part filling"), CustomElementGraphicPart::HTMLGraySilverFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGrayDarkGray, tr("Gray : DarkGray", "element part filling"), CustomElementGraphicPart::HTMLGrayDarkGrayFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGrayGray, tr("Gray : Gray", "element part filling"), CustomElementGraphicPart::HTMLGrayGrayFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGrayDimGray, tr("Gray : DimGray", "element part filling"), CustomElementGraphicPart::HTMLGrayDimGrayFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGrayLightSlateGray, tr("Gray : LightSlateGray", "element part filling"), CustomElementGraphicPart::HTMLGrayLightSlateGrayFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGraySlateGray, tr("Gray : SlateGray", "element part filling"), CustomElementGraphicPart::HTMLGraySlateGrayFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGrayDarkSlateGray, tr("Gray : DarkSlateGray", "element part filling"), CustomElementGraphicPart::HTMLGrayDarkSlateGrayFilling);
	filling_color -> addItem(QET::Icons::ColorHTMLGrayBlack, tr("Gray : Black", "element part filling"), CustomElementGraphicPart::HTMLGrayBlackFilling);
	filling_color -> insertSeparator(filling_color -> count());
	filling_color -> addItem(tr("Lignes Horizontales", "element part filling"), CustomElementGraphicPart::HorFilling);
	filling_color -> addItem(tr("Lignes Verticales", "element part filling"), CustomElementGraphicPart::VerFilling);
	filling_color -> addItem(tr("Hachures gauche", "element part filling"), CustomElementGraphicPart::BdiagFilling);
	filling_color -> addItem(tr("Hachures droite", "element part filling"), CustomElementGraphicPart::FdiagFilling);

	// antialiasing
	antialiasing = new QCheckBox(tr("Antialiasing"));

	updateForm();

	auto main_layout = new QVBoxLayout();
	main_layout -> setContentsMargins(0,0,0,0);

	main_layout -> addWidget(new QLabel("<u>" + tr("Apparence :") + "</u> "));

	outline_color->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	filling_color->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	auto grid_layout = new QGridLayout(this);
	grid_layout->addWidget(new QLabel(tr("Contour :")), 0,0, Qt::AlignRight);
	grid_layout->addWidget(outline_color, 0, 1);
	grid_layout->addWidget(new QLabel(tr("Remplissage :")), 0, 2, Qt::AlignRight);
	grid_layout->addWidget(filling_color, 0, 3);
	grid_layout->addWidget(new QLabel(tr("Style :")), 1, 0, Qt::AlignRight);
	grid_layout->addWidget(line_style, 1, 1);
	grid_layout->addWidget(new QLabel(tr("Épaisseur :")), 1, 2, Qt::AlignRight);
	grid_layout->addWidget(size_weight, 1 ,3);
	main_layout->addItem(grid_layout);

	main_layout -> addWidget(antialiasing);

	main_layout -> addSpacing(10);
	main_layout -> addWidget(new QLabel("<u>" + tr("Géométrie :") + "</u> "));
	setLayout(main_layout);
}

/// Destructeur
StyleEditor::~StyleEditor()
{
}

/// Update antialiasing with undo command
void StyleEditor::updatePartAntialiasing()
{
	makeUndo(tr("style antialiasing"), "antialias", antialiasing -> isChecked());
}

/// Update color with undo command
void StyleEditor::updatePartColor()
{
	makeUndo(tr("style couleur"),"color", outline_color->itemData(outline_color -> currentIndex()));
}

/// Update style with undo command
void StyleEditor::updatePartLineStyle()
{
	makeUndo(tr("style ligne"), "line_style", line_style->itemData(line_style -> currentIndex()));
}

/// Update weight with undo command
void StyleEditor::updatePartLineWeight()
{
	makeUndo(tr("style epaisseur"), "line_weight", size_weight->itemData(size_weight -> currentIndex()));
}

/// Update color filling with undo command
void StyleEditor::updatePartFilling()
{
	makeUndo(tr("style remplissage"), "filling", filling_color->itemData(filling_color -> currentIndex()));
}

/**
	@brief StyleEditor::updateForm
	Update the edition form according to the value of edited part(s)
*/
void StyleEditor::updateForm()
{
	if (!part && m_part_list.isEmpty()) return;
	activeConnections(false);

	if (part)
	{
		antialiasing  ->setChecked(part -> antialiased());
		outline_color ->removeItem(13); //Remove the separator for set the good index at the line below
		outline_color ->setCurrentIndex(part->color());
		outline_color ->insertSeparator(13);
		line_style    ->setCurrentIndex(part -> lineStyle());
		size_weight   ->setCurrentIndex(part -> lineWeight());
		filling_color ->removeItem(14); //Remove the separator for set the good index at the line below
		filling_color ->setCurrentIndex(part -> filling());
		filling_color ->insertSeparator(14);
	}
	else if (m_part_list.size())
	{
		CustomElementGraphicPart *first_part = m_part_list.first();
		antialiasing -> setChecked(first_part -> antialiased());
		outline_color -> setCurrentIndex(first_part -> color());
		line_style    -> setCurrentIndex(first_part -> lineStyle());
		size_weight   -> setCurrentIndex(first_part -> lineWeight());
		filling_color -> setCurrentIndex(first_part -> filling());

		for (auto cegp : m_part_list)
		{
			if (first_part -> antialiased() != cegp -> antialiased()) antialiasing -> setChecked(false);
			if (first_part -> color()       != cegp -> color())      outline_color -> setCurrentIndex(-1);
			if (first_part -> lineStyle()   != cegp -> lineStyle())  line_style    -> setCurrentIndex(-1);
			if (first_part -> lineWeight()  != cegp -> lineWeight()) size_weight   -> setCurrentIndex(-1);
			if (first_part -> filling()     != cegp -> filling())    filling_color -> setCurrentIndex(-1);
		}
	}

	activeConnections(true);
}

/**
	@brief StyleEditor::setPart
	Set the part to edit by this editor.
	Note : editor can accept or refuse to edit a part
	@param new_part : part to edit
	@return  true if editor accept to edit this CustomElementPart otherwise false
*/
bool StyleEditor::setPart(CustomElementPart *new_part) {
	m_part_list.clear();
	m_cep_list.clear();

	if (!new_part)
	{
		part = nullptr;
		return(true);
	}

	if (CustomElementGraphicPart *part_graphic = dynamic_cast<CustomElementGraphicPart *>(new_part))
	{
		part = part_graphic;
		m_cep_list.append(part_graphic);
		updateForm();
		return(true);
	}

	return(false);
}

/**
	@brief StyleEditor::setParts
	Set several parts to edit by this editor.
	Note : editor can accept or refuse to edit several parts.
	@param part_list
	@return true if every customeElementPart stored in part_list can
	be edited by this part editor, otherwise return false
	(see StyleEditor::isStyleEditable)
*/
bool StyleEditor::setParts(QList<CustomElementPart *> part_list)
{
	if (part_list.isEmpty()) return false;
	if (part_list.size() == 1) return setPart(part_list.first());

	part = nullptr;
	m_part_list.clear();
	m_cep_list.clear();

	if (!isStyleEditable(part_list)) return false;

	foreach (CustomElementPart *cep, part_list)
	{
		if (CustomElementGraphicPart *cegp = dynamic_cast<CustomElementGraphicPart *>(cep))
			m_part_list << cegp;
		else
			return false;
	}

	foreach (CustomElementGraphicPart *cegp, m_part_list)
		m_cep_list << cegp;

	updateForm();
	return true;
}

/**
	@return la primitive actuellement editee, ou 0 si ce widget n'en edite pas
*/
CustomElementPart *StyleEditor::currentPart() const
{
	return(part);
}

QList<CustomElementPart*> StyleEditor::currentParts() const
{
	return m_cep_list;
}

/**
	@brief StyleEditor::isStyleEditable
	@param cep_list
	@return true if all of the content of cep_list
	can be edited by style editor, else return false.
*/
bool StyleEditor::isStyleEditable(QList<CustomElementPart *> cep_list)
{
	QStringList str;
	str << "arc" << "ellipse" << "line" << "polygon" << "rect";

    for (CustomElementPart *cep: cep_list)
		if (!str.contains(cep -> xmlName()))
			return false;

	return true;
}

/**
	Active ou desactive les connexionx signaux/slots entre les widgets internes.
	@param active true pour activer les connexions, false pour les desactiver
*/
void StyleEditor::activeConnections(bool active) {
	if (active) {
		connect (outline_color, SIGNAL(activated(int)), this, SLOT(updatePartColor()));
		connect(line_style,        SIGNAL(activated(int)), this, SLOT(updatePartLineStyle()));
		connect(size_weight,       SIGNAL(activated(int)), this, SLOT(updatePartLineWeight()));
		connect(filling_color, SIGNAL(activated(int)), this, SLOT(updatePartFilling()));
		connect(antialiasing, SIGNAL(stateChanged(int)),  this, SLOT(updatePartAntialiasing()));
	} else {
		disconnect(outline_color, SIGNAL(activated(int)), this, SLOT(updatePartColor()));
		disconnect(line_style,        SIGNAL(activated(int)), this, SLOT(updatePartLineStyle()));
		disconnect(size_weight,       SIGNAL(activated(int)), this, SLOT(updatePartLineWeight()));
		disconnect(filling_color, SIGNAL(activated(int)), this, SLOT(updatePartFilling()));
		disconnect(antialiasing, SIGNAL(stateChanged(int)),  this, SLOT(updatePartAntialiasing()));
	}
}

void StyleEditor::makeUndo(const QString &undo_text, const char *property_name, const QVariant &new_value)
{
	QPropertyUndoCommand *undo = nullptr;
	if (part && (new_value != part->property(property_name)))
	{
		undo = new QPropertyUndoCommand(part, property_name, part->property(property_name), new_value);
		undo->setText(undo_text);
		undoStack().push(undo);
		return;
	}
	else if (!m_part_list.isEmpty())
	{
		foreach (CustomElementGraphicPart *cegp, m_part_list)
		{
			if (!undo)
			{
				undo = new QPropertyUndoCommand(cegp, property_name, cegp->property(property_name), new_value);
				undo->setText(undo_text);
			}
			else
				new QPropertyUndoCommand(cegp, property_name, cegp->property(property_name), new_value, undo);
		}
		undoStack().push(undo);
	}
}
