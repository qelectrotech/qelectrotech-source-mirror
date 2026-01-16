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
#include "customelementgraphicpart.h"

#include "../../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../elementscene.h"

#include <QRegularExpression>

/**
	@brief CustomElementGraphicPart::CustomElementGraphicPart
	Default constructor.
	By default, item is selectable, send geometry change (Qt > 4.6),
	accept mouse left button and accept hover event
	@param editor QETElement editor that belong this.
	@param parent
*/
CustomElementGraphicPart::CustomElementGraphicPart(QETElementEditor *editor,
						   QGraphicsItem *parent) :
	QGraphicsObject (parent),
	CustomElementPart(editor),
	m_hovered (false),
	_linestyle(NormalStyle),
	_lineweight(NormalWeight),
	_filling(NoneFilling),
	_color(BlackColor),
	_antialiased(false)
{
	setFlags(QGraphicsItem::ItemIsSelectable
		 | QGraphicsItem::ItemIsMovable
		 | QGraphicsItem::ItemSendsGeometryChanges);
	setAcceptHoverEvents(true);
}

/**
	@brief CustomElementGraphicPart::~CustomElementGraphicPart
	Destructor
*/
CustomElementGraphicPart::~CustomElementGraphicPart() {}

/**
	@brief CustomElementGraphicPart::drawCross
	Draw a cross at pos center
	@param center : center of cross
	@param painter : painter to use for draw cross,
	the painter state is restored at end of this method.
*/
void CustomElementGraphicPart::drawCross(const QPointF &center,
					 QPainter *painter)
{
	painter -> save();
	painter -> setRenderHint(QPainter::Antialiasing, false);
	painter -> setPen((painter -> brush().color()
			   == QColor(Qt::black)
			   && painter -> brush().isOpaque())
			  ? Qt::yellow
			  : Qt::blue);
	painter -> drawLine(QLineF(center.x() - 2.0,
				   center.y(),
				   center.x() + 2.0,
				   center.y()));
	painter -> drawLine(QLineF(center.x(),
				   center.y() - 2.0,
				   center.x(),
				   center.y() + 2.0));
	painter -> restore();
}

/**
	@brief CustomElementGraphicPart::setLineStyle
	Set line style to ls
	@param ls
*/
void CustomElementGraphicPart::setLineStyle(const LineStyle ls)
{
	if (_linestyle == ls) return;
	_linestyle = ls;
	update();
}

/**
	@brief CustomElementGraphicPart::setLineWeight
	Set line weight to lw
	@param lw
*/
void CustomElementGraphicPart::setLineWeight(const LineWeight lw)
{
	if (_lineweight == lw) return;
	_lineweight = lw;
	update();
}

/**
	@brief CustomElementGraphicPart::penWeight
	@return the weight of pen
*/
qreal CustomElementGraphicPart::penWeight() const
{
	if (_lineweight == NoneWeight || _lineweight == ThinWeight) return 0;
	else if (_lineweight == NormalWeight) return 1;
	else if (_lineweight == UltraWeight)  return 2;
	else if (_lineweight == BigWeight)    return 5;
	return 1;
}

/**
	@brief CustomElementGraphicPart::setFilling
	Set filling to f
	@param f
*/
void CustomElementGraphicPart::setFilling(const Filling f)
{
	if (_filling == f) return;
	_filling = f;
	update();
}

/**
	@brief CustomElementGraphicPart::setColor
	Set color to c
	@param c
*/
void CustomElementGraphicPart::setColor(const Color c)
{
	if (_color == c) return;
	_color = c;
	update();
}

/**
	@brief CustomElementGraphicPart::setAntialiased
	Set antialias to b
	@param b
*/
void CustomElementGraphicPart::setAntialiased(const bool b)
{
	if (_antialiased == b) return;
	_antialiased = b;
	update();
}

/**
	@brief CustomElementGraphicPart::stylesToXml
	Write the current style to xml element.
	The style are stored like this:
	name-of-style:value;name-of-style:value
	Each style separate by ; and name-style/value are separate by :
	@param qde : QDOmElement used to write the style.
*/
void CustomElementGraphicPart::stylesToXml(QDomElement &qde) const
{
	QString css_like_styles;

	css_like_styles += "line-style:";
	if      (_linestyle == DashedStyle)     css_like_styles += "dashed";
	else if (_linestyle == DottedStyle)     css_like_styles += "dotted";
	else if (_linestyle == DashdottedStyle) css_like_styles += "dashdotted";
	else if (_linestyle == NormalStyle)     css_like_styles += "normal";

	css_like_styles += ";line-weight:";
	if      (_lineweight == NoneWeight)   css_like_styles += "none";
	else if (_lineweight == ThinWeight)   css_like_styles += "thin";
	else if (_lineweight == NormalWeight) css_like_styles += "normal";
	else if (_lineweight == UltraWeight)  css_like_styles += "hight";
	else if (_lineweight == BigWeight)    css_like_styles += "eleve";

	static const QMap<Filling, QString> filling_map = {
		{NoneFilling, "none"},
		{BlackFilling, "black"},
		{WhiteFilling, "white"},
		{GreenFilling, "green"},
		{BlueFilling, "blue"},
		{RedFilling, "red"},
		{GrayFilling, "gray"},
		{BrunFilling, "brun"},
		{YellowFilling, "yellow"},
		{CyanFilling, "cyan"},
		{MagentaFilling, "magenta"},
		{LightgrayFilling, "lightgray"},
		{OrangeFilling, "orange"},
		{PurpleFilling, "purple"},
		{HTMLPinkPinkFilling, "HTMLPinkPink"},
		{HTMLPinkLightPinkFilling, "HTMLPinkLightPink"},
		{HTMLPinkHotPinkFilling, "HTMLPinkHotPink"},
		{HTMLPinkDeepPinkFilling, "HTMLPinkDeepPink"},
		{HTMLPinkPaleVioletRedFilling, "HTMLPinkPaleVioletRed"},
		{HTMLPinkMediumVioletRedFilling, "HTMLPinkMediumVioletRed"},
		{HTMLRedLightSalmonFilling, "HTMLRedLightSalmon"},
		{HTMLRedSalmonFilling, "HTMLRedSalmon"},
		{HTMLRedDarkSalmonFilling, "HTMLRedDarkSalmon"},
		{HTMLRedLightCoralFilling, "HTMLRedLightCoral"},
		{HTMLRedIndianRedFilling, "HTMLRedIndianRed"},
		{HTMLRedCrimsonFilling, "HTMLRedCrimson"},
		{HTMLRedFirebrickFilling, "HTMLRedFirebrick"},
		{HTMLRedDarkRedFilling, "HTMLRedDarkRed"},
		{HTMLRedRedFilling, "HTMLRedRed"},
		{HTMLOrangeOrangeRedFilling, "HTMLOrangeOrangeRed"},
		{HTMLOrangeTomatoFilling, "HTMLOrangeTomato"},
		{HTMLOrangeCoralFilling, "HTMLOrangeCoral"},
		{HTMLOrangeDarkOrangeFilling, "HTMLOrangeDarkOrange"},
		{HTMLOrangeOrangeFilling, "HTMLOrangeOrange"},
		{HTMLYellowYellowFilling, "HTMLYellowYellow"},
		{HTMLYellowLightYellowFilling, "HTMLYellowLightYellow"},
		{HTMLYellowLemonChiffonFilling, "HTMLYellowLemonChiffon"},
		{HTMLYellowLightGoldenrodYellowFilling, "HTMLYellowLightGoldenrodYellow"},
		{HTMLYellowPapayaWhipFilling, "HTMLYellowPapayaWhip"},
		{HTMLYellowMoccasinFilling, "HTMLYellowMoccasin"},
		{HTMLYellowPeachPuffFilling, "HTMLYellowPeachPuff"},
		{HTMLYellowPaleGoldenrodFilling, "HTMLYellowPaleGoldenrod"},
		{HTMLYellowKhakiFilling, "HTMLYellowKhaki"},
		{HTMLYellowDarkKhakiFilling, "HTMLYellowDarkKhaki"},
		{HTMLYellowGoldFilling, "HTMLYellowGold"},
		{HTMLBrownCornsilkFilling, "HTMLBrownCornsilk"},
		{HTMLBrownBlanchedAlmondFilling, "HTMLBrownBlanchedAlmond"},
		{HTMLBrownBisqueFilling, "HTMLBrownBisque"},
		{HTMLBrownNavajoWhiteFilling, "HTMLBrownNavajoWhite"},
		{HTMLBrownWheatFilling, "HTMLBrownWheat"},
		{HTMLBrownBurlywoodFilling, "HTMLBrownBurlywood"},
		{HTMLBrownTanFilling, "HTMLBrownTan"},
		{HTMLBrownRosyBrownFilling, "HTMLBrownRosyBrown"},
		{HTMLBrownSandyBrownFilling, "HTMLBrownSandyBrown"},
		{HTMLBrownGoldenrodFilling, "HTMLBrownGoldenrod"},
		{HTMLBrownDarkGoldenrodFilling, "HTMLBrownDarkGoldenrod"},
		{HTMLBrownPeruFilling, "HTMLBrownPeru"},
		{HTMLBrownChocolateFilling, "HTMLBrownChocolate"},
		{HTMLBrownSaddleBrownFilling, "HTMLBrownSaddleBrown"},
		{HTMLBrownSiennaFilling, "HTMLBrownSienna"},
		{HTMLBrownBrownFilling, "HTMLBrownBrown"},
		{HTMLBrownMaroonFilling, "HTMLBrownMaroon"},
		{HTMLGreenDarkOliveGreenFilling, "HTMLGreenDarkOliveGreen"},
		{HTMLGreenOliveFilling, "HTMLGreenOlive"},
		{HTMLGreenOliveDrabFilling, "HTMLGreenOliveDrab"},
		{HTMLGreenYellowGreenFilling, "HTMLGreenYellowGreen"},
		{HTMLGreenLimeGreenFilling, "HTMLGreenLimeGreen"},
		{HTMLGreenLimeFilling, "HTMLGreenLime"},
		{HTMLGreenLawnGreenFilling, "HTMLGreenLawnGreen"},
		{HTMLGreenChartreuseFilling, "HTMLGreenChartreuse"},
		{HTMLGreenGreenYellowFilling, "HTMLGreenGreenYellow"},
		{HTMLGreenSpringGreenFilling, "HTMLGreenSpringGreen"},
		{HTMLGreenMediumSpringGreenFilling, "HTMLGreenMediumSpringGreen"},
		{HTMLGreenLightGreenFilling, "HTMLGreenLightGreen"},
		{HTMLGreenPaleGreenFilling, "HTMLGreenPaleGreen"},
		{HTMLGreenDarkSeaGreenFilling, "HTMLGreenDarkSeaGreen"},
		{HTMLGreenMediumAquamarineFilling, "HTMLGreenMediumAquamarine"},
		{HTMLGreenMediumSeaGreenFilling, "HTMLGreenMediumSeaGreen"},
		{HTMLGreenSeaGreenFilling, "HTMLGreenSeaGreen"},
		{HTMLGreenForestGreenFilling, "HTMLGreenForestGreen"},
		{HTMLGreenGreenFilling, "HTMLGreenGreen"},
		{HTMLGreenDarkGreenFilling, "HTMLGreenDarkGreen"},
		{HTMLCyanAquaFilling, "HTMLCyanAqua"},
		{HTMLCyanCyanFilling, "HTMLCyanCyan"},
		{HTMLCyanLightCyanFilling, "HTMLCyanLightCyan"},
		{HTMLCyanPaleTurquoiseFilling, "HTMLCyanPaleTurquoise"},
		{HTMLCyanAquamarineFilling, "HTMLCyanAquamarine"},
		{HTMLCyanTurquoiseFilling, "HTMLCyanTurquoise"},
		{HTMLCyanMediumTurquoiseFilling, "HTMLCyanMediumTurquoise"},
		{HTMLCyanDarkTurquoiseFilling, "HTMLCyanDarkTurquoise"},
		{HTMLCyanLightSeaGreenFilling, "HTMLCyanLightSeaGreen"},
		{HTMLCyanCadetBlueFilling, "HTMLCyanCadetBlue"},
		{HTMLCyanDarkCyanFilling, "HTMLCyanDarkCyan"},
		{HTMLCyanTealFilling, "HTMLCyanTeal"},
		{HTMLBlueLightSteelBlueFilling, "HTMLBlueLightSteelBlue"},
		{HTMLBluePowderBlueFilling, "HTMLBluePowderBlue"},
		{HTMLBlueLightBlueFilling, "HTMLBlueLightBlue"},
		{HTMLBlueSkyBlueFilling, "HTMLBlueSkyBlue"},
		{HTMLBlueLightSkyBlueFilling, "HTMLBlueLightSkyBlue"},
		{HTMLBlueDeepSkyBlueFilling, "HTMLBlueDeepSkyBlue"},
		{HTMLBlueDodgerBlueFilling, "HTMLBlueDodgerBlue"},
		{HTMLBlueCornflowerBlueFilling, "HTMLBlueCornflowerBlue"},
		{HTMLBlueSteelBlueFilling, "HTMLBlueSteelBlue"},
		{HTMLBlueRoyalBlueFilling, "HTMLBlueRoyalBlue"},
		{HTMLBlueBlueFilling, "HTMLBlueBlue"},
		{HTMLBlueMediumBlueFilling, "HTMLBlueMediumBlue"},
		{HTMLBlueDarkBlueFilling, "HTMLBlueDarkBlue"},
		{HTMLBlueNavyFilling, "HTMLBlueNavy"},
		{HTMLBlueMidnightBlueFilling, "HTMLBlueMidnightBlue"},
		{HTMLPurpleLavenderFilling, "HTMLPurpleLavender"},
		{HTMLPurpleThistleFilling, "HTMLPurpleThistle"},
		{HTMLPurplePlumFilling, "HTMLPurplePlum"},
		{HTMLPurpleVioletFilling, "HTMLPurpleViolet"},
		{HTMLPurpleOrchidFilling, "HTMLPurpleOrchid"},
		{HTMLPurpleFuchsiaFilling, "HTMLPurpleFuchsia"},
		{HTMLPurpleMagentaFilling, "HTMLPurpleMagenta"},
		{HTMLPurpleMediumOrchidFilling, "HTMLPurpleMediumOrchid"},
		{HTMLPurpleMediumPurpleFilling, "HTMLPurpleMediumPurple"},
		{HTMLPurpleBlueVioletFilling, "HTMLPurpleBlueViolet"},
		{HTMLPurpleDarkVioletFilling, "HTMLPurpleDarkViolet"},
		{HTMLPurpleDarkOrchidFilling, "HTMLPurpleDarkOrchid"},
		{HTMLPurpleDarkMagentaFilling, "HTMLPurpleDarkMagenta"},
		{HTMLPurplePurpleFilling, "HTMLPurplePurple"},
		{HTMLPurpleIndigoFilling, "HTMLPurpleIndigo"},
		{HTMLPurpleDarkSlateBlueFilling, "HTMLPurpleDarkSlateBlue"},
		{HTMLPurpleSlateBlueFilling, "HTMLPurpleSlateBlue"},
		{HTMLPurpleMediumSlateBlueFilling, "HTMLPurpleMediumSlateBlue"},
		{HTMLWhiteWhiteFilling, "HTMLWhiteWhite"},
		{HTMLWhiteSnowFilling, "HTMLWhiteSnow"},
		{HTMLWhiteHoneydewFilling, "HTMLWhiteHoneydew"},
		{HTMLWhiteMintCreamFilling, "HTMLWhiteMintCream"},
		{HTMLWhiteAzureFilling, "HTMLWhiteAzure"},
		{HTMLWhiteAliceBlueFilling, "HTMLWhiteAliceBlue"},
		{HTMLWhiteGhostWhiteFilling, "HTMLWhiteGhostWhite"},
		{HTMLWhiteWhiteSmokeFilling, "HTMLWhiteWhiteSmoke"},
		{HTMLWhiteSeashellFilling, "HTMLWhiteSeashell"},
		{HTMLWhiteBeigeFilling, "HTMLWhiteBeige"},
		{HTMLWhiteOldLaceFilling, "HTMLWhiteOldLace"},
		{HTMLWhiteFloralWhiteFilling, "HTMLWhiteFloralWhite"},
		{HTMLWhiteIvoryFilling, "HTMLWhiteIvory"},
		{HTMLWhiteAntiqueWhiteFilling, "HTMLWhiteAntiqueWhite"},
		{HTMLWhiteLinenFilling, "HTMLWhiteLinen"},
		{HTMLWhiteLavenderBlushFilling, "HTMLWhiteLavenderBlush"},
		{HTMLWhiteMistyRoseFilling, "HTMLWhiteMistyRose"},
		{HTMLGrayGainsboroFilling, "HTMLGrayGainsboro"},
		{HTMLGrayLightGrayFilling, "HTMLGrayLightGray"},
		{HTMLGraySilverFilling, "HTMLGraySilver"},
		{HTMLGrayDarkGrayFilling, "HTMLGrayDarkGray"},
		{HTMLGrayGrayFilling, "HTMLGrayGray"},
		{HTMLGrayDimGrayFilling, "HTMLGrayDimGray"},
		{HTMLGrayLightSlateGrayFilling, "HTMLGrayLightSlateGray"},
		{HTMLGraySlateGrayFilling, "HTMLGraySlateGray"},
		{HTMLGrayDarkSlateGrayFilling, "HTMLGrayDarkSlateGray"},
		{HTMLGrayBlackFilling, "HTMLGrayBlack"},
		{HorFilling, "hor"},
		{VerFilling, "ver"},
		{BdiagFilling, "bdiag"},
		{FdiagFilling, "fdiag"}};

	css_like_styles += ";filling:";

	auto filling = filling_map.find(_filling);
	if (filling != filling_map.end()) { css_like_styles += filling.value(); }

	static const QMap<Color, QString> color_map = {
		{WhiteColor, "white"},
		{BlackColor, "black"},
		{GreenColor, "green"},
		{RedColor, "red"},
		{BlueColor, "blue"},
		{GrayColor, "gray"},
		{BrunColor, "brun"},
		{YellowColor, "yellow"},
		{CyanColor, "cyan"},
		{MagentaColor, "magenta"},
		{LightgrayColor, "lightgray"},
		{OrangeColor, "orange"},
		{PurpleColor, "purple"},
		{HTMLPinkPinkColor, "HTMLPinkPink"},
		{HTMLPinkLightPinkColor, "HTMLPinkLightPink"},
		{HTMLPinkHotPinkColor, "HTMLPinkHotPink"},
		{HTMLPinkDeepPinkColor, "HTMLPinkDeepPink"},
		{HTMLPinkPaleVioletRedColor, "HTMLPinkPaleVioletRed"},
		{HTMLPinkMediumVioletRedColor, "HTMLPinkMediumVioletRed"},
		{HTMLRedLightSalmonColor, "HTMLRedLightSalmon"},
		{HTMLRedSalmonColor, "HTMLRedSalmon"},
		{HTMLRedDarkSalmonColor, "HTMLRedDarkSalmon"},
		{HTMLRedLightCoralColor, "HTMLRedLightCoral"},
		{HTMLRedIndianRedColor, "HTMLRedIndianRed"},
		{HTMLRedCrimsonColor, "HTMLRedCrimson"},
		{HTMLRedFirebrickColor, "HTMLRedFirebrick"},
		{HTMLRedDarkRedColor, "HTMLRedDarkRed"},
		{HTMLRedRedColor, "HTMLRedRed"},
		{HTMLOrangeOrangeRedColor, "HTMLOrangeOrangeRed"},
		{HTMLOrangeTomatoColor, "HTMLOrangeTomato"},
		{HTMLOrangeCoralColor, "HTMLOrangeCoral"},
		{HTMLOrangeDarkOrangeColor, "HTMLOrangeDarkOrange"},
		{HTMLOrangeOrangeColor, "HTMLOrangeOrange"},
		{HTMLYellowYellowColor, "HTMLYellowYellow"},
		{HTMLYellowLightYellowColor, "HTMLYellowLightYellow"},
		{HTMLYellowLemonChiffonColor, "HTMLYellowLemonChiffon"},
		{HTMLYellowLightGoldenrodYellowColor, "HTMLYellowLightGoldenrodYellow"},
		{HTMLYellowPapayaWhipColor, "HTMLYellowPapayaWhip"},
		{HTMLYellowMoccasinColor, "HTMLYellowMoccasin"},
		{HTMLYellowPeachPuffColor, "HTMLYellowPeachPuff"},
		{HTMLYellowPaleGoldenrodColor, "HTMLYellowPaleGoldenrod"},
		{HTMLYellowKhakiColor, "HTMLYellowKhaki"},
		{HTMLYellowDarkKhakiColor, "HTMLYellowDarkKhaki"},
		{HTMLYellowGoldColor, "HTMLYellowGold"},
		{HTMLBrownCornsilkColor, "HTMLBrownCornsilk"},
		{HTMLBrownBlanchedAlmondColor, "HTMLBrownBlanchedAlmond"},
		{HTMLBrownBisqueColor, "HTMLBrownBisque"},
		{HTMLBrownNavajoWhiteColor, "HTMLBrownNavajoWhite"},
		{HTMLBrownWheatColor, "HTMLBrownWheat"},
		{HTMLBrownBurlywoodColor, "HTMLBrownBurlywood"},
		{HTMLBrownTanColor, "HTMLBrownTan"},
		{HTMLBrownRosyBrownColor, "HTMLBrownRosyBrown"},
		{HTMLBrownSandyBrownColor, "HTMLBrownSandyBrown"},
		{HTMLBrownGoldenrodColor, "HTMLBrownGoldenrod"},
		{HTMLBrownDarkGoldenrodColor, "HTMLBrownDarkGoldenrod"},
		{HTMLBrownPeruColor, "HTMLBrownPeru"},
		{HTMLBrownChocolateColor, "HTMLBrownChocolate"},
		{HTMLBrownSaddleBrownColor, "HTMLBrownSaddleBrown"},
		{HTMLBrownSiennaColor, "HTMLBrownSienna"},
		{HTMLBrownBrownColor, "HTMLBrownBrown"},
		{HTMLBrownMaroonColor, "HTMLBrownMaroon"},
		{HTMLGreenDarkOliveGreenColor, "HTMLGreenDarkOliveGreen"},
		{HTMLGreenOliveColor, "HTMLGreenOlive"},
		{HTMLGreenOliveDrabColor, "HTMLGreenOliveDrab"},
		{HTMLGreenYellowGreenColor, "HTMLGreenYellowGreen"},
		{HTMLGreenLimeGreenColor, "HTMLGreenLimeGreen"},
		{HTMLGreenLimeColor, "HTMLGreenLime"},
		{HTMLGreenLawnGreenColor, "HTMLGreenLawnGreen"},
		{HTMLGreenChartreuseColor, "HTMLGreenChartreuse"},
		{HTMLGreenGreenYellowColor, "HTMLGreenGreenYellow"},
		{HTMLGreenSpringGreenColor, "HTMLGreenSpringGreen"},
		{HTMLGreenMediumSpringGreenColor, "HTMLGreenMediumSpringGreen"},
		{HTMLGreenLightGreenColor, "HTMLGreenLightGreen"},
		{HTMLGreenPaleGreenColor, "HTMLGreenPaleGreen"},
		{HTMLGreenDarkSeaGreenColor, "HTMLGreenDarkSeaGreen"},
		{HTMLGreenMediumAquamarineColor, "HTMLGreenMediumAquamarine"},
		{HTMLGreenMediumSeaGreenColor, "HTMLGreenMediumSeaGreen"},
		{HTMLGreenSeaGreenColor, "HTMLGreenSeaGreen"},
		{HTMLGreenForestGreenColor, "HTMLGreenForestGreen"},
		{HTMLGreenGreenColor, "HTMLGreenGreen"},
		{HTMLGreenDarkGreenColor, "HTMLGreenDarkGreen"},
		{HTMLCyanAquaColor, "HTMLCyanAqua"},
		{HTMLCyanCyanColor, "HTMLCyanCyan"},
		{HTMLCyanLightCyanColor, "HTMLCyanLightCyan"},
		{HTMLCyanPaleTurquoiseColor, "HTMLCyanPaleTurquoise"},
		{HTMLCyanAquamarineColor, "HTMLCyanAquamarine"},
		{HTMLCyanTurquoiseColor, "HTMLCyanTurquoise"},
		{HTMLCyanMediumTurquoiseColor, "HTMLCyanMediumTurquoise"},
		{HTMLCyanDarkTurquoiseColor, "HTMLCyanDarkTurquoise"},
		{HTMLCyanLightSeaGreenColor, "HTMLCyanLightSeaGreen"},
		{HTMLCyanCadetBlueColor, "HTMLCyanCadetBlue"},
		{HTMLCyanDarkCyanColor, "HTMLCyanDarkCyan"},
		{HTMLCyanTealColor, "HTMLCyanTeal"},
		{HTMLBlueLightSteelBlueColor, "HTMLBlueLightSteelBlue"},
		{HTMLBluePowderBlueColor, "HTMLBluePowderBlue"},
		{HTMLBlueLightBlueColor, "HTMLBlueLightBlue"},
		{HTMLBlueSkyBlueColor, "HTMLBlueSkyBlue"},
		{HTMLBlueLightSkyBlueColor, "HTMLBlueLightSkyBlue"},
		{HTMLBlueDeepSkyBlueColor, "HTMLBlueDeepSkyBlue"},
		{HTMLBlueDodgerBlueColor, "HTMLBlueDodgerBlue"},
		{HTMLBlueCornflowerBlueColor, "HTMLBlueCornflowerBlue"},
		{HTMLBlueSteelBlueColor, "HTMLBlueSteelBlue"},
		{HTMLBlueRoyalBlueColor, "HTMLBlueRoyalBlue"},
		{HTMLBlueBlueColor, "HTMLBlueBlue"},
		{HTMLBlueMediumBlueColor, "HTMLBlueMediumBlue"},
		{HTMLBlueDarkBlueColor, "HTMLBlueDarkBlue"},
		{HTMLBlueNavyColor, "HTMLBlueNavy"},
		{HTMLBlueMidnightBlueColor, "HTMLBlueMidnightBlue"},
		{HTMLPurpleLavenderColor, "HTMLPurpleLavender"},
		{HTMLPurpleThistleColor, "HTMLPurpleThistle"},
		{HTMLPurplePlumColor, "HTMLPurplePlum"},
		{HTMLPurpleVioletColor, "HTMLPurpleViolet"},
		{HTMLPurpleOrchidColor, "HTMLPurpleOrchid"},
		{HTMLPurpleFuchsiaColor, "HTMLPurpleFuchsia"},
		{HTMLPurpleMagentaColor, "HTMLPurpleMagenta"},
		{HTMLPurpleMediumOrchidColor, "HTMLPurpleMediumOrchid"},
		{HTMLPurpleMediumPurpleColor, "HTMLPurpleMediumPurple"},
		{HTMLPurpleBlueVioletColor, "HTMLPurpleBlueViolet"},
		{HTMLPurpleDarkVioletColor, "HTMLPurpleDarkViolet"},
		{HTMLPurpleDarkOrchidColor, "HTMLPurpleDarkOrchid"},
		{HTMLPurpleDarkMagentaColor, "HTMLPurpleDarkMagenta"},
		{HTMLPurplePurpleColor, "HTMLPurplePurple"},
		{HTMLPurpleIndigoColor, "HTMLPurpleIndigo"},
		{HTMLPurpleDarkSlateBlueColor, "HTMLPurpleDarkSlateBlue"},
		{HTMLPurpleSlateBlueColor, "HTMLPurpleSlateBlue"},
		{HTMLPurpleMediumSlateBlueColor, "HTMLPurpleMediumSlateBlue"},
		{HTMLWhiteWhiteColor, "HTMLWhiteWhite"},
		{HTMLWhiteSnowColor, "HTMLWhiteSnow"},
		{HTMLWhiteHoneydewColor, "HTMLWhiteHoneydew"},
		{HTMLWhiteMintCreamColor, "HTMLWhiteMintCream"},
		{HTMLWhiteAzureColor, "HTMLWhiteAzure"},
		{HTMLWhiteAliceBlueColor, "HTMLWhiteAliceBlue"},
		{HTMLWhiteGhostWhiteColor, "HTMLWhiteGhostWhite"},
		{HTMLWhiteWhiteSmokeColor, "HTMLWhiteWhiteSmoke"},
		{HTMLWhiteSeashellColor, "HTMLWhiteSeashell"},
		{HTMLWhiteBeigeColor, "HTMLWhiteBeige"},
		{HTMLWhiteOldLaceColor, "HTMLWhiteOldLace"},
		{HTMLWhiteFloralWhiteColor, "HTMLWhiteFloralWhite"},
		{HTMLWhiteIvoryColor, "HTMLWhiteIvory"},
		{HTMLWhiteAntiqueWhiteColor, "HTMLWhiteAntiqueWhite"},
		{HTMLWhiteLinenColor, "HTMLWhiteLinen"},
		{HTMLWhiteLavenderBlushColor, "HTMLWhiteLavenderBlush"},
		{HTMLWhiteMistyRoseColor, "HTMLWhiteMistyRose"},
		{HTMLGrayGainsboroColor, "HTMLGrayGainsboro"},
		{HTMLGrayLightGrayColor, "HTMLGrayLightGray"},
		{HTMLGraySilverColor, "HTMLGraySilver"},
		{HTMLGrayDarkGrayColor, "HTMLGrayDarkGray"},
		{HTMLGrayGrayColor, "HTMLGrayGray"},
		{HTMLGrayDimGrayColor, "HTMLGrayDimGray"},
		{HTMLGrayLightSlateGrayColor, "HTMLGrayLightSlateGray"},
		{HTMLGraySlateGrayColor, "HTMLGraySlateGray"},
		{HTMLGrayDarkSlateGrayColor, "HTMLGrayDarkSlateGray"},
		{HTMLGrayBlackColor, "HTMLGrayBlack"},
		{NoneColor, "none"}};
	css_like_styles += ";color:";

	auto color = color_map.find(_color);
	if (color != color_map.end()) { css_like_styles += color.value(); }

	qde.setAttribute("style", css_like_styles);
	qde.setAttribute("antialias", _antialiased ? "true" : "false");
}


/**
	@brief CustomElementGraphicPart::stylesFromXml
	Read the style used by this, from a xml element.
	@param qde : QDomElement used to read the style
*/
void CustomElementGraphicPart::stylesFromXml(const QDomElement &qde)
{
	resetStyles();

		//Get the list of pair style/value
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)	// ### Qt 6: remove
	QStringList styles = qde.attribute("style").split(";", QString::SkipEmptyParts);
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 5.14 or later")
#endif
	QStringList styles = qde.attribute("style").split(";", Qt::SkipEmptyParts);
#endif

		//Check each pair of style
	QRegularExpression rx("^\\s*([a-z-]+)\\s*:\\s*([a-zA-Z-]+)\\s*$");
	for (auto style : styles)
	{
		auto rx_match = rx.match(style);
		if (!rx_match.hasMatch()) {
			continue;
		}

		auto style_name = rx_match.captured(1);
		auto style_value = rx_match.captured(2);

		if (style_name == "line-style")
		{
			if      (style_value == "dashed")     _linestyle = DashedStyle;
			else if (style_value == "dotted")     _linestyle = DottedStyle;
			else if (style_value == "dashdotted") _linestyle = DashdottedStyle;
			else if (style_value == "normal")     _linestyle = NormalStyle;
		}
		else if (style_name == "line-weight")
		{
			if      (style_value == "none")   _lineweight = NoneWeight;
			else if (style_value == "thin")   _lineweight = ThinWeight;
			else if (style_value == "normal") _lineweight = NormalWeight;
			else if (style_value == "hight")  _lineweight = UltraWeight;
			else if (style_value == "eleve")  _lineweight = BigWeight;
		}
		else if (style_name == "filling")
		{
			static const QMap<QString, Filling> filling_map = {
				{"white", WhiteFilling},
				{"black", BlackFilling},
				{"red", RedFilling},
				{"green", GreenFilling},
				{"blue", BlueFilling},
				{"gray", GrayFilling},
				{"brun", BrunFilling},
				{"yellow", YellowFilling},
				{"cyan", CyanFilling},
				{"magenta", MagentaFilling},
				{"lightgray", LightgrayFilling},
				{"orange", OrangeFilling},
				{"purple", PurpleFilling},
				{"HTMLPinkPink", HTMLPinkPinkFilling},
				{"HTMLPinkLightPink", HTMLPinkLightPinkFilling},
				{"HTMLPinkHotPink", HTMLPinkHotPinkFilling},
				{"HTMLPinkDeepPink", HTMLPinkDeepPinkFilling},
				{"HTMLPinkPaleVioletRed", HTMLPinkPaleVioletRedFilling},
				{"HTMLPinkMediumVioletRed", HTMLPinkMediumVioletRedFilling},
				{"HTMLRedLightSalmon", HTMLRedLightSalmonFilling},
				{"HTMLRedSalmon", HTMLRedSalmonFilling},
				{"HTMLRedDarkSalmon", HTMLRedDarkSalmonFilling},
				{"HTMLRedLightCoral", HTMLRedLightCoralFilling},
				{"HTMLRedIndianRed", HTMLRedIndianRedFilling},
				{"HTMLRedCrimson", HTMLRedCrimsonFilling},
				{"HTMLRedFirebrick", HTMLRedFirebrickFilling},
				{"HTMLRedDarkRed", HTMLRedDarkRedFilling},
				{"HTMLRedRed", HTMLRedRedFilling},
				{"HTMLOrangeOrangeRed", HTMLOrangeOrangeRedFilling},
				{"HTMLOrangeTomato", HTMLOrangeTomatoFilling},
				{"HTMLOrangeCoral", HTMLOrangeCoralFilling},
				{"HTMLOrangeDarkOrange", HTMLOrangeDarkOrangeFilling},
				{"HTMLOrangeOrange", HTMLOrangeOrangeFilling},
				{"HTMLYellowYellow", HTMLYellowYellowFilling},
				{"HTMLYellowLightYellow", HTMLYellowLightYellowFilling},
				{"HTMLYellowLemonChiffon", HTMLYellowLemonChiffonFilling},
				{"HTMLYellowLightGoldenrodYellow", HTMLYellowLightGoldenrodYellowFilling},
				{"HTMLYellowPapayaWhip", HTMLYellowPapayaWhipFilling},
				{"HTMLYellowMoccasin", HTMLYellowMoccasinFilling},
				{"HTMLYellowPeachPuff", HTMLYellowPeachPuffFilling},
				{"HTMLYellowPaleGoldenrod", HTMLYellowPaleGoldenrodFilling},
				{"HTMLYellowKhaki", HTMLYellowKhakiFilling},
				{"HTMLYellowDarkKhaki", HTMLYellowDarkKhakiFilling},
				{"HTMLYellowGold", HTMLYellowGoldFilling},
				{"HTMLBrownCornsilk", HTMLBrownCornsilkFilling},
				{"HTMLBrownBlanchedAlmond", HTMLBrownBlanchedAlmondFilling},
				{"HTMLBrownBisque", HTMLBrownBisqueFilling},
				{"HTMLBrownNavajoWhite", HTMLBrownNavajoWhiteFilling},
				{"HTMLBrownWheat", HTMLBrownWheatFilling},
				{"HTMLBrownBurlywood", HTMLBrownBurlywoodFilling},
				{"HTMLBrownTan", HTMLBrownTanFilling},
				{"HTMLBrownRosyBrown", HTMLBrownRosyBrownFilling},
				{"HTMLBrownSandyBrown", HTMLBrownSandyBrownFilling},
				{"HTMLBrownGoldenrod", HTMLBrownGoldenrodFilling},
				{"HTMLBrownDarkGoldenrod", HTMLBrownDarkGoldenrodFilling},
				{"HTMLBrownPeru", HTMLBrownPeruFilling},
				{"HTMLBrownChocolate", HTMLBrownChocolateFilling},
				{"HTMLBrownSaddleBrown", HTMLBrownSaddleBrownFilling},
				{"HTMLBrownSienna", HTMLBrownSiennaFilling},
				{"HTMLBrownBrown", HTMLBrownBrownFilling},
				{"HTMLBrownMaroon", HTMLBrownMaroonFilling},
				{"HTMLGreenDarkOliveGreen", HTMLGreenDarkOliveGreenFilling},
				{"HTMLGreenOlive", HTMLGreenOliveFilling},
				{"HTMLGreenOliveDrab", HTMLGreenOliveDrabFilling},
				{"HTMLGreenYellowGreen", HTMLGreenYellowGreenFilling},
				{"HTMLGreenLimeGreen", HTMLGreenLimeGreenFilling},
				{"HTMLGreenLime", HTMLGreenLimeFilling},
				{"HTMLGreenLawnGreen", HTMLGreenLawnGreenFilling},
				{"HTMLGreenChartreuse", HTMLGreenChartreuseFilling},
				{"HTMLGreenGreenYellow", HTMLGreenGreenYellowFilling},
				{"HTMLGreenSpringGreen", HTMLGreenSpringGreenFilling},
				{"HTMLGreenMediumSpringGreen", HTMLGreenMediumSpringGreenFilling},
				{"HTMLGreenLightGreen", HTMLGreenLightGreenFilling},
				{"HTMLGreenPaleGreen", HTMLGreenPaleGreenFilling},
				{"HTMLGreenDarkSeaGreen", HTMLGreenDarkSeaGreenFilling},
				{"HTMLGreenMediumAquamarine", HTMLGreenMediumAquamarineFilling},
				{"HTMLGreenMediumSeaGreen", HTMLGreenMediumSeaGreenFilling},
				{"HTMLGreenSeaGreen", HTMLGreenSeaGreenFilling},
				{"HTMLGreenForestGreen", HTMLGreenForestGreenFilling},
				{"HTMLGreenGreen", HTMLGreenGreenFilling},
				{"HTMLGreenDarkGreen", HTMLGreenDarkGreenFilling},
				{"HTMLCyanAqua", HTMLCyanAquaFilling},
				{"HTMLCyanCyan", HTMLCyanCyanFilling},
				{"HTMLCyanLightCyan", HTMLCyanLightCyanFilling},
				{"HTMLCyanPaleTurquoise", HTMLCyanPaleTurquoiseFilling},
				{"HTMLCyanAquamarine", HTMLCyanAquamarineFilling},
				{"HTMLCyanTurquoise", HTMLCyanTurquoiseFilling},
				{"HTMLCyanMediumTurquoise", HTMLCyanMediumTurquoiseFilling},
				{"HTMLCyanDarkTurquoise", HTMLCyanDarkTurquoiseFilling},
				{"HTMLCyanLightSeaGreen", HTMLCyanLightSeaGreenFilling},
				{"HTMLCyanCadetBlue", HTMLCyanCadetBlueFilling},
				{"HTMLCyanDarkCyan", HTMLCyanDarkCyanFilling},
				{"HTMLCyanTeal", HTMLCyanTealFilling},
				{"HTMLBlueLightSteelBlue", HTMLBlueLightSteelBlueFilling},
				{"HTMLBluePowderBlue", HTMLBluePowderBlueFilling},
				{"HTMLBlueLightBlue", HTMLBlueLightBlueFilling},
				{"HTMLBlueSkyBlue", HTMLBlueSkyBlueFilling},
				{"HTMLBlueLightSkyBlue", HTMLBlueLightSkyBlueFilling},
				{"HTMLBlueDeepSkyBlue", HTMLBlueDeepSkyBlueFilling},
				{"HTMLBlueDodgerBlue", HTMLBlueDodgerBlueFilling},
				{"HTMLBlueCornflowerBlue", HTMLBlueCornflowerBlueFilling},
				{"HTMLBlueSteelBlue", HTMLBlueSteelBlueFilling},
				{"HTMLBlueRoyalBlue", HTMLBlueRoyalBlueFilling},
				{"HTMLBlueBlue", HTMLBlueBlueFilling},
				{"HTMLBlueMediumBlue", HTMLBlueMediumBlueFilling},
				{"HTMLBlueDarkBlue", HTMLBlueDarkBlueFilling},
				{"HTMLBlueNavy", HTMLBlueNavyFilling},
				{"HTMLBlueMidnightBlue", HTMLBlueMidnightBlueFilling},
				{"HTMLPurpleLavender", HTMLPurpleLavenderFilling},
				{"HTMLPurpleThistle", HTMLPurpleThistleFilling},
				{"HTMLPurplePlum", HTMLPurplePlumFilling},
				{"HTMLPurpleViolet", HTMLPurpleVioletFilling},
				{"HTMLPurpleOrchid", HTMLPurpleOrchidFilling},
				{"HTMLPurpleFuchsia", HTMLPurpleFuchsiaFilling},
				{"HTMLPurpleMagenta", HTMLPurpleMagentaFilling},
				{"HTMLPurpleMediumOrchid", HTMLPurpleMediumOrchidFilling},
				{"HTMLPurpleMediumPurple", HTMLPurpleMediumPurpleFilling},
				{"HTMLPurpleBlueViolet", HTMLPurpleBlueVioletFilling},
				{"HTMLPurpleDarkViolet", HTMLPurpleDarkVioletFilling},
				{"HTMLPurpleDarkOrchid", HTMLPurpleDarkOrchidFilling},
				{"HTMLPurpleDarkMagenta", HTMLPurpleDarkMagentaFilling},
				{"HTMLPurplePurple", HTMLPurplePurpleFilling},
				{"HTMLPurpleIndigo", HTMLPurpleIndigoFilling},
				{"HTMLPurpleDarkSlateBlue", HTMLPurpleDarkSlateBlueFilling},
				{"HTMLPurpleSlateBlue", HTMLPurpleSlateBlueFilling},
				{"HTMLPurpleMediumSlateBlue", HTMLPurpleMediumSlateBlueFilling},
				{"HTMLWhiteWhite", HTMLWhiteWhiteFilling},
				{"HTMLWhiteSnow", HTMLWhiteSnowFilling},
				{"HTMLWhiteHoneydew", HTMLWhiteHoneydewFilling},
				{"HTMLWhiteMintCream", HTMLWhiteMintCreamFilling},
				{"HTMLWhiteAzure", HTMLWhiteAzureFilling},
				{"HTMLWhiteAliceBlue", HTMLWhiteAliceBlueFilling},
				{"HTMLWhiteGhostWhite", HTMLWhiteGhostWhiteFilling},
				{"HTMLWhiteWhiteSmoke", HTMLWhiteWhiteSmokeFilling},
				{"HTMLWhiteSeashell", HTMLWhiteSeashellFilling},
				{"HTMLWhiteBeige", HTMLWhiteBeigeFilling},
				{"HTMLWhiteOldLace", HTMLWhiteOldLaceFilling},
				{"HTMLWhiteFloralWhite", HTMLWhiteFloralWhiteFilling},
				{"HTMLWhiteIvory", HTMLWhiteIvoryFilling},
				{"HTMLWhiteAntiqueWhite", HTMLWhiteAntiqueWhiteFilling},
				{"HTMLWhiteLinen", HTMLWhiteLinenFilling},
				{"HTMLWhiteLavenderBlush", HTMLWhiteLavenderBlushFilling},
				{"HTMLWhiteMistyRose", HTMLWhiteMistyRoseFilling},
				{"HTMLGrayGainsboro", HTMLGrayGainsboroFilling},
				{"HTMLGrayLightGray", HTMLGrayLightGrayFilling},
				{"HTMLGraySilver", HTMLGraySilverFilling},
				{"HTMLGrayDarkGray", HTMLGrayDarkGrayFilling},
				{"HTMLGrayGray", HTMLGrayGrayFilling},
				{"HTMLGrayDimGray", HTMLGrayDimGrayFilling},
				{"HTMLGrayLightSlateGray", HTMLGrayLightSlateGrayFilling},
				{"HTMLGraySlateGray", HTMLGraySlateGrayFilling},
				{"HTMLGrayDarkSlateGray", HTMLGrayDarkSlateGrayFilling},
				{"HTMLGrayBlack", HTMLGrayBlackFilling},
				{"hor", HorFilling},
				{"ver", VerFilling},
				{"bdiag", BdiagFilling},
				{"fdiag", FdiagFilling},
				{"none", NoneFilling}};

			auto filling = filling_map.find(style_value);
			if (filling != filling_map.end()) { _filling = filling.value(); }
		}
		else if (style_name == "color")
		{
			static const QMap<QString, Color> color_map = {
				{"black", BlackColor},
				{"white", WhiteColor},
				{"green", GreenColor},
				{"red", RedColor},
				{"blue", BlueColor},
				{"gray", GrayColor},
				{"brun", BrunColor},
				{"yellow", YellowColor},
				{"cyan", CyanColor},
				{"magenta", MagentaColor},
				{"lightgray", LightgrayColor},
				{"orange", OrangeColor},
				{"purple", PurpleColor},
				{"HTMLPinkPink", HTMLPinkPinkColor},
				{"HTMLPinkLightPink", HTMLPinkLightPinkColor},
				{"HTMLPinkHotPink", HTMLPinkHotPinkColor},
				{"HTMLPinkDeepPink", HTMLPinkDeepPinkColor},
				{"HTMLPinkPaleVioletRed", HTMLPinkPaleVioletRedColor},
				{"HTMLPinkMediumVioletRed", HTMLPinkMediumVioletRedColor},
				{"HTMLRedLightSalmon", HTMLRedLightSalmonColor},
				{"HTMLRedSalmon", HTMLRedSalmonColor},
				{"HTMLRedDarkSalmon", HTMLRedDarkSalmonColor},
				{"HTMLRedLightCoral", HTMLRedLightCoralColor},
				{"HTMLRedIndianRed", HTMLRedIndianRedColor},
				{"HTMLRedCrimson", HTMLRedCrimsonColor},
				{"HTMLRedFirebrick", HTMLRedFirebrickColor},
				{"HTMLRedDarkRed", HTMLRedDarkRedColor},
				{"HTMLRedRed", HTMLRedRedColor},
				{"HTMLOrangeOrangeRed", HTMLOrangeOrangeRedColor},
				{"HTMLOrangeTomato", HTMLOrangeTomatoColor},
				{"HTMLOrangeCoral", HTMLOrangeCoralColor},
				{"HTMLOrangeDarkOrange", HTMLOrangeDarkOrangeColor},
				{"HTMLOrangeOrange", HTMLOrangeOrangeColor},
				{"HTMLYellowYellow", HTMLYellowYellowColor},
				{"HTMLYellowLightYellow", HTMLYellowLightYellowColor},
				{"HTMLYellowLemonChiffon", HTMLYellowLemonChiffonColor},
				{"HTMLYellowLightGoldenrodYellow", HTMLYellowLightGoldenrodYellowColor},
				{"HTMLYellowPapayaWhip", HTMLYellowPapayaWhipColor},
				{"HTMLYellowMoccasin", HTMLYellowMoccasinColor},
				{"HTMLYellowPeachPuff", HTMLYellowPeachPuffColor},
				{"HTMLYellowPaleGoldenrod", HTMLYellowPaleGoldenrodColor},
				{"HTMLYellowKhaki", HTMLYellowKhakiColor},
				{"HTMLYellowDarkKhaki", HTMLYellowDarkKhakiColor},
				{"HTMLYellowGold", HTMLYellowGoldColor},
				{"HTMLBrownCornsilk", HTMLBrownCornsilkColor},
				{"HTMLBrownBlanchedAlmond", HTMLBrownBlanchedAlmondColor},
				{"HTMLBrownBisque", HTMLBrownBisqueColor},
				{"HTMLBrownNavajoWhite", HTMLBrownNavajoWhiteColor},
				{"HTMLBrownWheat", HTMLBrownWheatColor},
				{"HTMLBrownBurlywood", HTMLBrownBurlywoodColor},
				{"HTMLBrownTan", HTMLBrownTanColor},
				{"HTMLBrownRosyBrown", HTMLBrownRosyBrownColor},
				{"HTMLBrownSandyBrown", HTMLBrownSandyBrownColor},
				{"HTMLBrownGoldenrod", HTMLBrownGoldenrodColor},
				{"HTMLBrownDarkGoldenrod", HTMLBrownDarkGoldenrodColor},
				{"HTMLBrownPeru", HTMLBrownPeruColor},
				{"HTMLBrownChocolate", HTMLBrownChocolateColor},
				{"HTMLBrownSaddleBrown", HTMLBrownSaddleBrownColor},
				{"HTMLBrownSienna", HTMLBrownSiennaColor},
				{"HTMLBrownBrown", HTMLBrownBrownColor},
				{"HTMLBrownMaroon", HTMLBrownMaroonColor},
				{"HTMLGreenDarkOliveGreen", HTMLGreenDarkOliveGreenColor},
				{"HTMLGreenOlive", HTMLGreenOliveColor},
				{"HTMLGreenOliveDrab", HTMLGreenOliveDrabColor},
				{"HTMLGreenYellowGreen", HTMLGreenYellowGreenColor},
				{"HTMLGreenLimeGreen", HTMLGreenLimeGreenColor},
				{"HTMLGreenLime", HTMLGreenLimeColor},
				{"HTMLGreenLawnGreen", HTMLGreenLawnGreenColor},
				{"HTMLGreenChartreuse", HTMLGreenChartreuseColor},
				{"HTMLGreenGreenYellow", HTMLGreenGreenYellowColor},
				{"HTMLGreenSpringGreen", HTMLGreenSpringGreenColor},
				{"HTMLGreenMediumSpringGreen", HTMLGreenMediumSpringGreenColor},
				{"HTMLGreenLightGreen", HTMLGreenLightGreenColor},
				{"HTMLGreenPaleGreen", HTMLGreenPaleGreenColor},
				{"HTMLGreenDarkSeaGreen", HTMLGreenDarkSeaGreenColor},
				{"HTMLGreenMediumAquamarine", HTMLGreenMediumAquamarineColor},
				{"HTMLGreenMediumSeaGreen", HTMLGreenMediumSeaGreenColor},
				{"HTMLGreenSeaGreen", HTMLGreenSeaGreenColor},
				{"HTMLGreenForestGreen", HTMLGreenForestGreenColor},
				{"HTMLGreenGreen", HTMLGreenGreenColor},
				{"HTMLGreenDarkGreen", HTMLGreenDarkGreenColor},
				{"HTMLCyanAqua", HTMLCyanAquaColor},
				{"HTMLCyanCyan", HTMLCyanCyanColor},
				{"HTMLCyanLightCyan", HTMLCyanLightCyanColor},
				{"HTMLCyanPaleTurquoise", HTMLCyanPaleTurquoiseColor},
				{"HTMLCyanAquamarine", HTMLCyanAquamarineColor},
				{"HTMLCyanTurquoise", HTMLCyanTurquoiseColor},
				{"HTMLCyanMediumTurquoise", HTMLCyanMediumTurquoiseColor},
				{"HTMLCyanDarkTurquoise", HTMLCyanDarkTurquoiseColor},
				{"HTMLCyanLightSeaGreen", HTMLCyanLightSeaGreenColor},
				{"HTMLCyanCadetBlue", HTMLCyanCadetBlueColor},
				{"HTMLCyanDarkCyan", HTMLCyanDarkCyanColor},
				{"HTMLCyanTeal", HTMLCyanTealColor},
				{"HTMLBlueLightSteelBlue", HTMLBlueLightSteelBlueColor},
				{"HTMLBluePowderBlue", HTMLBluePowderBlueColor},
				{"HTMLBlueLightBlue", HTMLBlueLightBlueColor},
				{"HTMLBlueSkyBlue", HTMLBlueSkyBlueColor},
				{"HTMLBlueLightSkyBlue", HTMLBlueLightSkyBlueColor},
				{"HTMLBlueDeepSkyBlue", HTMLBlueDeepSkyBlueColor},
				{"HTMLBlueDodgerBlue", HTMLBlueDodgerBlueColor},
				{"HTMLBlueCornflowerBlue", HTMLBlueCornflowerBlueColor},
				{"HTMLBlueSteelBlue", HTMLBlueSteelBlueColor},
				{"HTMLBlueRoyalBlue", HTMLBlueRoyalBlueColor},
				{"HTMLBlueBlue", HTMLBlueBlueColor},
				{"HTMLBlueMediumBlue", HTMLBlueMediumBlueColor},
				{"HTMLBlueDarkBlue", HTMLBlueDarkBlueColor},
				{"HTMLBlueNavy", HTMLBlueNavyColor},
				{"HTMLBlueMidnightBlue", HTMLBlueMidnightBlueColor},
				{"HTMLPurpleLavender", HTMLPurpleLavenderColor},
				{"HTMLPurpleThistle", HTMLPurpleThistleColor},
				{"HTMLPurplePlum", HTMLPurplePlumColor},
				{"HTMLPurpleViolet", HTMLPurpleVioletColor},
				{"HTMLPurpleOrchid", HTMLPurpleOrchidColor},
				{"HTMLPurpleFuchsia", HTMLPurpleFuchsiaColor},
				{"HTMLPurpleMagenta", HTMLPurpleMagentaColor},
				{"HTMLPurpleMediumOrchid", HTMLPurpleMediumOrchidColor},
				{"HTMLPurpleMediumPurple", HTMLPurpleMediumPurpleColor},
				{"HTMLPurpleBlueViolet", HTMLPurpleBlueVioletColor},
				{"HTMLPurpleDarkViolet", HTMLPurpleDarkVioletColor},
				{"HTMLPurpleDarkOrchid", HTMLPurpleDarkOrchidColor},
				{"HTMLPurpleDarkMagenta", HTMLPurpleDarkMagentaColor},
				{"HTMLPurplePurple", HTMLPurplePurpleColor},
				{"HTMLPurpleIndigo", HTMLPurpleIndigoColor},
				{"HTMLPurpleDarkSlateBlue", HTMLPurpleDarkSlateBlueColor},
				{"HTMLPurpleSlateBlue", HTMLPurpleSlateBlueColor},
				{"HTMLPurpleMediumSlateBlue", HTMLPurpleMediumSlateBlueColor},
				{"HTMLWhiteWhite", HTMLWhiteWhiteColor},
				{"HTMLWhiteSnow", HTMLWhiteSnowColor},
				{"HTMLWhiteHoneydew", HTMLWhiteHoneydewColor},
				{"HTMLWhiteMintCream", HTMLWhiteMintCreamColor},
				{"HTMLWhiteAzure", HTMLWhiteAzureColor},
				{"HTMLWhiteAliceBlue", HTMLWhiteAliceBlueColor},
				{"HTMLWhiteGhostWhite", HTMLWhiteGhostWhiteColor},
				{"HTMLWhiteWhiteSmoke", HTMLWhiteWhiteSmokeColor},
				{"HTMLWhiteSeashell", HTMLWhiteSeashellColor},
				{"HTMLWhiteBeige", HTMLWhiteBeigeColor},
				{"HTMLWhiteOldLace", HTMLWhiteOldLaceColor},
				{"HTMLWhiteFloralWhite", HTMLWhiteFloralWhiteColor},
				{"HTMLWhiteIvory", HTMLWhiteIvoryColor},
				{"HTMLWhiteAntiqueWhite", HTMLWhiteAntiqueWhiteColor},
				{"HTMLWhiteLinen", HTMLWhiteLinenColor},
				{"HTMLWhiteLavenderBlush", HTMLWhiteLavenderBlushColor},
				{"HTMLWhiteMistyRose", HTMLWhiteMistyRoseColor},
				{"HTMLGrayGainsboro", HTMLGrayGainsboroColor},
				{"HTMLGrayLightGray", HTMLGrayLightGrayColor},
				{"HTMLGraySilver", HTMLGraySilverColor},
				{"HTMLGrayDarkGray", HTMLGrayDarkGrayColor},
				{"HTMLGrayGray", HTMLGrayGrayColor},
				{"HTMLGrayDimGray", HTMLGrayDimGrayColor},
				{"HTMLGrayLightSlateGray", HTMLGrayLightSlateGrayColor},
				{"HTMLGraySlateGray", HTMLGraySlateGrayColor},
				{"HTMLGrayDarkSlateGray", HTMLGrayDarkSlateGrayColor},
				{"HTMLGrayBlack", HTMLGrayBlackColor},
				{"none", NoneColor}};

			auto color = color_map.find(style_value);
			if (color != color_map.end()) { _color = color.value(); }
		}
	}
		//Get antialiasing
	_antialiased = qde.attribute("antialias") == "true";
}


/**
	@brief CustomElementGraphicPart::resetStyles
	Reset the current style to default,
	same style of default constructor
*/
void CustomElementGraphicPart::resetStyles()
{
	_linestyle = NormalStyle;
	_lineweight = NormalWeight;
	_filling = NoneFilling;
	_color = BlackColor;
	_antialiased = false;
}

/**
	@brief CustomElementGraphicPart::applyStylesToQPainter
	Apply the current style to the QPainter
	@param painter
*/
void CustomElementGraphicPart::applyStylesToQPainter(QPainter &painter) const
{
		//Get the pen and brush
	QPen pen = painter.pen();
	QBrush brush = painter.brush();

		//Apply pen style
	if      (_linestyle == DashedStyle)     pen.setStyle(Qt::DashLine);
	else if (_linestyle == DashdottedStyle) pen.setStyle(Qt::DashDotLine);
	else if (_linestyle == DottedStyle)     pen.setStyle(Qt::DotLine);
	else if (_linestyle == NormalStyle)     pen.setStyle(Qt::SolidLine);

		//Apply pen width
	if      (_lineweight == NoneWeight)   pen.setColor(QColor(0, 0, 0, 0));
	else if (_lineweight == ThinWeight)   pen.setWidth(0);
	else if (_lineweight == NormalWeight) pen.setWidthF(1.0);
	else if (_lineweight == UltraWeight)  pen.setWidthF(2.0);
	else if (_lineweight == BigWeight)    pen.setWidthF(5.0);

		//Apply brush color
	if (_filling == NoneFilling) brush.setStyle(Qt::NoBrush);
	else if (_filling == HorFilling) brush.setStyle(Qt::HorPattern);
	else if (_filling == VerFilling) brush.setStyle(Qt::VerPattern);
	else if (_filling == BdiagFilling) brush.setStyle(Qt::BDiagPattern);
	else if (_filling == FdiagFilling) brush.setStyle(Qt::FDiagPattern);
	else
	{
		static const QMap<Filling, QColor>
			filling_style_map = {
				{BlackFilling, Qt::black},
				{WhiteFilling, Qt::white},
				{GreenFilling, Qt::green},
				{RedFilling, Qt::red},
				{BlueFilling, Qt::blue},
				{GrayFilling, Qt::gray},
				{BrunFilling, QColor(97, 44, 0)},
				{YellowFilling, Qt::yellow},
				{CyanFilling, Qt::cyan},
				{MagentaFilling, Qt::magenta},
				{LightgrayFilling, Qt::lightGray},
				{OrangeFilling, QColor(255, 128, 0)},
				{PurpleFilling, QColor(136, 28, 168)},
				{HTMLPinkPinkFilling, QColor(255, 192, 203)},
				{HTMLPinkLightPinkFilling, QColor(255, 182, 193)},
				{HTMLPinkHotPinkFilling, QColor(255, 105, 180)},
				{HTMLPinkDeepPinkFilling, QColor(255, 20, 147)},
				{HTMLPinkPaleVioletRedFilling, QColor(219, 112, 147)},
				{HTMLPinkMediumVioletRedFilling, QColor(199, 21, 133)},
				{HTMLRedLightSalmonFilling, QColor(255, 160, 122)},
				{HTMLRedSalmonFilling, QColor(250, 128, 114)},
				{HTMLRedDarkSalmonFilling, QColor(233, 150, 122)},
				{HTMLRedLightCoralFilling, QColor(240, 128, 128)},
				{HTMLRedIndianRedFilling, QColor(205, 92, 92)},
				{HTMLRedCrimsonFilling, QColor(220, 20, 60)},
				{HTMLRedFirebrickFilling, QColor(178, 34, 34)},
				{HTMLRedDarkRedFilling, QColor(139, 0, 0)},
				{HTMLRedRedFilling, QColor(255, 0, 0)},
				{HTMLOrangeOrangeRedFilling, QColor(255, 69, 0)},
				{HTMLOrangeTomatoFilling, QColor(255, 99, 71)},
				{HTMLOrangeCoralFilling, QColor(255, 127, 80)},
				{HTMLOrangeDarkOrangeFilling, QColor(255, 140, 0)},
				{HTMLOrangeOrangeFilling, QColor(255, 165, 0)},
				{HTMLYellowYellowFilling, QColor(255, 255, 0)},
				{HTMLYellowLightYellowFilling, QColor(255, 255, 224)},
				{HTMLYellowLemonChiffonFilling, QColor(255, 250, 205)},
				{HTMLYellowLightGoldenrodYellowFilling, QColor(250, 250, 210)},
				{HTMLYellowPapayaWhipFilling, QColor(255, 239, 213)},
				{HTMLYellowMoccasinFilling, QColor(255, 228, 181)},
				{HTMLYellowPeachPuffFilling, QColor(255, 218, 185)},
				{HTMLYellowPaleGoldenrodFilling, QColor(238, 232, 170)},
				{HTMLYellowKhakiFilling, QColor(240, 230, 140)},
				{HTMLYellowDarkKhakiFilling, QColor(189, 183, 107)},
				{HTMLYellowGoldFilling, QColor(255, 215, 0)},
				{HTMLBrownCornsilkFilling, QColor(255, 248, 220)},
				{HTMLBrownBlanchedAlmondFilling, QColor(255, 235, 205)},
				{HTMLBrownBisqueFilling, QColor(255, 228, 196)},
				{HTMLBrownNavajoWhiteFilling, QColor(255, 222, 173)},
				{HTMLBrownWheatFilling, QColor(245, 222, 179)},
				{HTMLBrownBurlywoodFilling, QColor(222, 184, 135)},
				{HTMLBrownTanFilling, QColor(210, 180, 140)},
				{HTMLBrownRosyBrownFilling, QColor(188, 143, 143)},
				{HTMLBrownSandyBrownFilling, QColor(244, 164, 96)},
				{HTMLBrownGoldenrodFilling, QColor(218, 165, 32)},
				{HTMLBrownDarkGoldenrodFilling, QColor(184, 134, 11)},
				{HTMLBrownPeruFilling, QColor(205, 133, 63)},
				{HTMLBrownChocolateFilling, QColor(210, 105, 30)},
				{HTMLBrownSaddleBrownFilling, QColor(139, 69, 19)},
				{HTMLBrownSiennaFilling, QColor(160, 82, 45)},
				{HTMLBrownBrownFilling, QColor(165, 42, 42)},
				{HTMLBrownMaroonFilling, QColor(128, 0, 0)},
				{HTMLGreenDarkOliveGreenFilling, QColor(85, 107, 47)},
				{HTMLGreenOliveFilling, QColor(128, 128, 0)},
				{HTMLGreenOliveDrabFilling, QColor(107, 142, 35)},
				{HTMLGreenYellowGreenFilling, QColor(154, 205, 50)},
				{HTMLGreenLimeGreenFilling, QColor(50, 205, 50)},
				{HTMLGreenLimeFilling, QColor(0, 255, 0)},
				{HTMLGreenLawnGreenFilling, QColor(124, 252, 0)},
				{HTMLGreenChartreuseFilling, QColor(127, 255, 0)},
				{HTMLGreenGreenYellowFilling, QColor(173, 255, 47)},
				{HTMLGreenSpringGreenFilling, QColor(0, 255, 127)},
				{HTMLGreenMediumSpringGreenFilling, QColor(0, 250, 154)},
				{HTMLGreenLightGreenFilling, QColor(144, 238, 144)},
				{HTMLGreenPaleGreenFilling, QColor(152, 251, 152)},
				{HTMLGreenDarkSeaGreenFilling, QColor(143, 188, 143)},
				{HTMLGreenMediumAquamarineFilling, QColor(102, 205, 170)},
				{HTMLGreenMediumSeaGreenFilling, QColor(60, 179, 113)},
				{HTMLGreenSeaGreenFilling, QColor(46, 139, 87)},
				{HTMLGreenForestGreenFilling, QColor(34, 139, 34)},
				{HTMLGreenGreenFilling, QColor(0, 128, 0)},
				{HTMLGreenDarkGreenFilling, QColor(0, 100, 0)},
				{HTMLCyanAquaFilling, QColor(0, 255, 255)},
				{HTMLCyanCyanFilling, QColor(0, 255, 255)},
				{HTMLCyanLightCyanFilling, QColor(224, 255, 255)},
				{HTMLCyanPaleTurquoiseFilling, QColor(175, 238, 238)},
				{HTMLCyanAquamarineFilling, QColor(127, 255, 212)},
				{HTMLCyanTurquoiseFilling, QColor(64, 224, 208)},
				{HTMLCyanMediumTurquoiseFilling, QColor(72, 209, 204)},
				{HTMLCyanDarkTurquoiseFilling, QColor(0, 206, 209)},
				{HTMLCyanLightSeaGreenFilling, QColor(32, 178, 170)},
				{HTMLCyanCadetBlueFilling, QColor(95, 158, 160)},
				{HTMLCyanDarkCyanFilling, QColor(0, 139, 139)},
				{HTMLCyanTealFilling, QColor(0, 128, 128)},
				{HTMLBlueLightSteelBlueFilling, QColor(176, 196, 222)},
				{HTMLBluePowderBlueFilling, QColor(176, 224, 230)},
				{HTMLBlueLightBlueFilling, QColor(173, 216, 230)},
				{HTMLBlueSkyBlueFilling, QColor(135, 206, 235)},
				{HTMLBlueLightSkyBlueFilling, QColor(135, 206, 250)},
				{HTMLBlueDeepSkyBlueFilling, QColor(0, 191, 255)},
				{HTMLBlueDodgerBlueFilling, QColor(30, 144, 255)},
				{HTMLBlueCornflowerBlueFilling, QColor(100, 149, 237)},
				{HTMLBlueSteelBlueFilling, QColor(70, 130, 180)},
				{HTMLBlueRoyalBlueFilling, QColor(65, 105, 225)},
				{HTMLBlueBlueFilling, QColor(0, 0, 255)},
				{HTMLBlueMediumBlueFilling, QColor(0, 0, 205)},
				{HTMLBlueDarkBlueFilling, QColor(0, 0, 139)},
				{HTMLBlueNavyFilling, QColor(0, 0, 128)},
				{HTMLBlueMidnightBlueFilling, QColor(25, 25, 112)},
				{HTMLPurpleLavenderFilling, QColor(230, 230, 250)},
				{HTMLPurpleThistleFilling, QColor(216, 191, 216)},
				{HTMLPurplePlumFilling, QColor(221, 160, 221)},
				{HTMLPurpleVioletFilling, QColor(238, 130, 238)},
				{HTMLPurpleOrchidFilling, QColor(218, 112, 214)},
				{HTMLPurpleFuchsiaFilling, QColor(255, 0, 255)},
				{HTMLPurpleMagentaFilling, QColor(255, 0, 255)},
				{HTMLPurpleMediumOrchidFilling, QColor(186, 85, 211)},
				{HTMLPurpleMediumPurpleFilling, QColor(147, 112, 219)},
				{HTMLPurpleBlueVioletFilling, QColor(138, 43, 226)},
				{HTMLPurpleDarkVioletFilling, QColor(148, 0, 211)},
				{HTMLPurpleDarkOrchidFilling, QColor(153, 50, 204)},
				{HTMLPurpleDarkMagentaFilling, QColor(139, 0, 139)},
				{HTMLPurplePurpleFilling, QColor(128, 0, 128)},
				{HTMLPurpleIndigoFilling, QColor(75, 0, 130)},
				{HTMLPurpleDarkSlateBlueFilling, QColor(72, 61, 139)},
				{HTMLPurpleSlateBlueFilling, QColor(106, 90, 205)},
				{HTMLPurpleMediumSlateBlueFilling, QColor(123, 104, 238)},
				{HTMLWhiteWhiteFilling, QColor(255, 255, 255)},
				{HTMLWhiteSnowFilling, QColor(255, 250, 250)},
				{HTMLWhiteHoneydewFilling, QColor(240, 255, 240)},
				{HTMLWhiteMintCreamFilling, QColor(245, 255, 250)},
				{HTMLWhiteAzureFilling, QColor(240, 255, 255)},
				{HTMLWhiteAliceBlueFilling, QColor(240, 248, 255)},
				{HTMLWhiteGhostWhiteFilling, QColor(248, 248, 255)},
				{HTMLWhiteWhiteSmokeFilling, QColor(245, 245, 245)},
				{HTMLWhiteSeashellFilling, QColor(255, 245, 238)},
				{HTMLWhiteBeigeFilling, QColor(245, 245, 220)},
				{HTMLWhiteOldLaceFilling, QColor(253, 245, 230)},
				{HTMLWhiteFloralWhiteFilling, QColor(255, 250, 240)},
				{HTMLWhiteIvoryFilling, QColor(255, 255, 240)},
				{HTMLWhiteAntiqueWhiteFilling, QColor(250, 235, 215)},
				{HTMLWhiteLinenFilling, QColor(250, 240, 230)},
				{HTMLWhiteLavenderBlushFilling, QColor(255, 240, 245)},
				{HTMLWhiteMistyRoseFilling, QColor(255, 228, 225)},
				{HTMLGrayGainsboroFilling, QColor(220, 220, 220)},
				{HTMLGrayLightGrayFilling, QColor(211, 211, 211)},
				{HTMLGraySilverFilling, QColor(192, 192, 192)},
				{HTMLGrayDarkGrayFilling, QColor(169, 169, 169)},
				{HTMLGrayGrayFilling, QColor(128, 128, 128)},
				{HTMLGrayDimGrayFilling, QColor(105, 105, 105)},
				{HTMLGrayLightSlateGrayFilling, QColor(119, 136, 153)},
				{HTMLGraySlateGrayFilling, QColor(112, 128, 144)},
				{HTMLGrayDarkSlateGrayFilling, QColor(47, 79, 79)},
				{HTMLGrayBlackFilling, QColor(0, 0, 0)}};

		brush.setStyle(Qt::SolidPattern);
		auto color = filling_style_map.find(_filling);
		if (color != filling_style_map.end()) { brush.setColor(*color); }
	}

	// Apply pen color
	static const QMap<Color, QColor> color_map =
		{{GreenColor, Qt::green},
		 {RedColor, Qt::red},
		 {BlueColor, Qt::blue},
		 {GrayColor, Qt::gray},
		 {BrunColor, QColor(97, 44, 0)},
		 {YellowColor, Qt::yellow},
		 {CyanColor, Qt::cyan},
		 {MagentaColor, Qt::magenta},
		 {LightgrayColor, Qt::lightGray},
		 {OrangeColor, QColor(255, 128, 0)},
		 {PurpleColor, QColor(136, 28, 168)},
		 {HTMLPinkPinkColor, QColor(255, 192, 203)},
		 {HTMLPinkLightPinkColor, QColor(255, 182, 193)},
		 {HTMLPinkHotPinkColor, QColor(255, 105, 180)},
		 {HTMLPinkDeepPinkColor, QColor(255, 20, 147)},
		 {HTMLPinkPaleVioletRedColor, QColor(219, 112, 147)},
		 {HTMLPinkMediumVioletRedColor, QColor(199, 21, 133)},
		 {HTMLRedLightSalmonColor, QColor(255, 160, 122)},
		 {HTMLRedSalmonColor, QColor(250, 128, 114)},
		 {HTMLRedDarkSalmonColor, QColor(233, 150, 122)},
		 {HTMLRedLightCoralColor, QColor(240, 128, 128)},
		 {HTMLRedIndianRedColor, QColor(205, 92, 92)},
		 {HTMLRedCrimsonColor, QColor(220, 20, 60)},
		 {HTMLRedFirebrickColor, QColor(178, 34, 34)},
		 {HTMLRedDarkRedColor, QColor(139, 0, 0)},
		 {HTMLRedRedColor, QColor(255, 0, 0)},
		 {HTMLOrangeOrangeRedColor, QColor(255, 69, 0)},
		 {HTMLOrangeTomatoColor, QColor(255, 99, 71)},
		 {HTMLOrangeCoralColor, QColor(255, 127, 80)},
		 {HTMLOrangeDarkOrangeColor, QColor(255, 140, 0)},
		 {HTMLOrangeOrangeColor, QColor(255, 165, 0)},
		 {HTMLYellowYellowColor, QColor(255, 255, 0)},
		 {HTMLYellowLightYellowColor, QColor(255, 255, 224)},
		 {HTMLYellowLemonChiffonColor, QColor(255, 250, 205)},
		 {HTMLYellowLightGoldenrodYellowColor, QColor(250, 250, 210)},
		 {HTMLYellowPapayaWhipColor, QColor(255, 239, 213)},
		 {HTMLYellowMoccasinColor, QColor(255, 228, 181)},
		 {HTMLYellowPeachPuffColor, QColor(255, 218, 185)},
		 {HTMLYellowPaleGoldenrodColor, QColor(238, 232, 170)},
		 {HTMLYellowKhakiColor, QColor(240, 230, 140)},
		 {HTMLYellowDarkKhakiColor, QColor(189, 183, 107)},
		 {HTMLYellowGoldColor, QColor(255, 215, 0)},
		 {HTMLBrownCornsilkColor, QColor(255, 248, 220)},
		 {HTMLBrownBlanchedAlmondColor, QColor(255, 235, 205)},
		 {HTMLBrownBisqueColor, QColor(255, 228, 196)},
		 {HTMLBrownNavajoWhiteColor, QColor(255, 222, 173)},
		 {HTMLBrownWheatColor, QColor(245, 222, 179)},
		 {HTMLBrownBurlywoodColor, QColor(222, 184, 135)},
		 {HTMLBrownTanColor, QColor(210, 180, 140)},
		 {HTMLBrownRosyBrownColor, QColor(188, 143, 143)},
		 {HTMLBrownSandyBrownColor, QColor(244, 164, 96)},
		 {HTMLBrownGoldenrodColor, QColor(218, 165, 32)},
		 {HTMLBrownDarkGoldenrodColor, QColor(184, 134, 11)},
		 {HTMLBrownPeruColor, QColor(205, 133, 63)},
		 {HTMLBrownChocolateColor, QColor(210, 105, 30)},
		 {HTMLBrownSaddleBrownColor, QColor(139, 69, 19)},
		 {HTMLBrownSiennaColor, QColor(160, 82, 45)},
		 {HTMLBrownBrownColor, QColor(165, 42, 42)},
		 {HTMLBrownMaroonColor, QColor(128, 0, 0)},
		 {HTMLGreenDarkOliveGreenColor, QColor(85, 107, 47)},
		 {HTMLGreenOliveColor, QColor(128, 128, 0)},
		 {HTMLGreenOliveDrabColor, QColor(107, 142, 35)},
		 {HTMLGreenYellowGreenColor, QColor(154, 205, 50)},
		 {HTMLGreenLimeGreenColor, QColor(50, 205, 50)},
		 {HTMLGreenLimeColor, QColor(0, 255, 0)},
		 {HTMLGreenLawnGreenColor, QColor(124, 252, 0)},
		 {HTMLGreenChartreuseColor, QColor(127, 255, 0)},
		 {HTMLGreenGreenYellowColor, QColor(173, 255, 47)},
		 {HTMLGreenSpringGreenColor, QColor(0, 255, 127)},
		 {HTMLGreenMediumSpringGreenColor, QColor(0, 250, 154)},
		 {HTMLGreenLightGreenColor, QColor(144, 238, 144)},
		 {HTMLGreenPaleGreenColor, QColor(152, 251, 152)},
		 {HTMLGreenDarkSeaGreenColor, QColor(143, 188, 143)},
		 {HTMLGreenMediumAquamarineColor, QColor(102, 205, 170)},
		 {HTMLGreenMediumSeaGreenColor, QColor(60, 179, 113)},
		 {HTMLGreenSeaGreenColor, QColor(46, 139, 87)},
		 {HTMLGreenForestGreenColor, QColor(34, 139, 34)},
		 {HTMLGreenGreenColor, QColor(0, 128, 0)},
		 {HTMLGreenDarkGreenColor, QColor(0, 100, 0)},
		 {HTMLCyanAquaColor, QColor(0, 255, 255)},
		 {HTMLCyanCyanColor, QColor(0, 255, 255)},
		 {HTMLCyanLightCyanColor, QColor(224, 255, 255)},
		 {HTMLCyanPaleTurquoiseColor, QColor(175, 238, 238)},
		 {HTMLCyanAquamarineColor, QColor(127, 255, 212)},
		 {HTMLCyanTurquoiseColor, QColor(64, 224, 208)},
		 {HTMLCyanMediumTurquoiseColor, QColor(72, 209, 204)},
		 {HTMLCyanDarkTurquoiseColor, QColor(0, 206, 209)},
		 {HTMLCyanLightSeaGreenColor, QColor(32, 178, 170)},
		 {HTMLCyanCadetBlueColor, QColor(95, 158, 160)},
		 {HTMLCyanDarkCyanColor, QColor(0, 139, 139)},
		 {HTMLCyanTealColor, QColor(0, 128, 128)},
		 {HTMLBlueLightSteelBlueColor, QColor(176, 196, 222)},
		 {HTMLBluePowderBlueColor, QColor(176, 224, 230)},
		 {HTMLBlueLightBlueColor, QColor(173, 216, 230)},
		 {HTMLBlueSkyBlueColor, QColor(135, 206, 235)},
		 {HTMLBlueLightSkyBlueColor, QColor(135, 206, 250)},
		 {HTMLBlueDeepSkyBlueColor, QColor(0, 191, 255)},
		 {HTMLBlueDodgerBlueColor, QColor(30, 144, 255)},
		 {HTMLBlueCornflowerBlueColor, QColor(100, 149, 237)},
		 {HTMLBlueSteelBlueColor, QColor(70, 130, 180)},
		 {HTMLBlueRoyalBlueColor, QColor(65, 105, 225)},
		 {HTMLBlueBlueColor, QColor(0, 0, 255)},
		 {HTMLBlueMediumBlueColor, QColor(0, 0, 205)},
		 {HTMLBlueDarkBlueColor, QColor(0, 0, 139)},
		 {HTMLBlueNavyColor, QColor(0, 0, 128)},
		 {HTMLBlueMidnightBlueColor, QColor(25, 25, 112)},
		 {HTMLPurpleLavenderColor, QColor(230, 230, 250)},
		 {HTMLPurpleThistleColor, QColor(216, 191, 216)},
		 {HTMLPurplePlumColor, QColor(221, 160, 221)},
		 {HTMLPurpleVioletColor, QColor(238, 130, 238)},
		 {HTMLPurpleOrchidColor, QColor(218, 112, 214)},
		 {HTMLPurpleFuchsiaColor, QColor(255, 0, 255)},
		 {HTMLPurpleMagentaColor, QColor(255, 0, 255)},
		 {HTMLPurpleMediumOrchidColor, QColor(186, 85, 211)},
		 {HTMLPurpleMediumPurpleColor, QColor(147, 112, 219)},
		 {HTMLPurpleBlueVioletColor, QColor(138, 43, 226)},
		 {HTMLPurpleDarkVioletColor, QColor(148, 0, 211)},
		 {HTMLPurpleDarkOrchidColor, QColor(153, 50, 204)},
		 {HTMLPurpleDarkMagentaColor, QColor(139, 0, 139)},
		 {HTMLPurplePurpleColor, QColor(128, 0, 128)},
		 {HTMLPurpleIndigoColor, QColor(75, 0, 130)},
		 {HTMLPurpleDarkSlateBlueColor, QColor(72, 61, 139)},
		 {HTMLPurpleSlateBlueColor, QColor(106, 90, 205)},
		 {HTMLPurpleMediumSlateBlueColor, QColor(123, 104, 238)},
		 {HTMLWhiteWhiteColor, QColor(255, 255, 255)},
		 {HTMLWhiteSnowColor, QColor(255, 250, 250)},
		 {HTMLWhiteHoneydewColor, QColor(240, 255, 240)},
		 {HTMLWhiteMintCreamColor, QColor(245, 255, 250)},
		 {HTMLWhiteAzureColor, QColor(240, 255, 255)},
		 {HTMLWhiteAliceBlueColor, QColor(240, 248, 255)},
		 {HTMLWhiteGhostWhiteColor, QColor(248, 248, 255)},
		 {HTMLWhiteWhiteSmokeColor, QColor(245, 245, 245)},
		 {HTMLWhiteSeashellColor, QColor(255, 245, 238)},
		 {HTMLWhiteBeigeColor, QColor(245, 245, 220)},
		 {HTMLWhiteOldLaceColor, QColor(253, 245, 230)},
		 {HTMLWhiteFloralWhiteColor, QColor(255, 250, 240)},
		 {HTMLWhiteIvoryColor, QColor(255, 255, 240)},
		 {HTMLWhiteAntiqueWhiteColor, QColor(250, 235, 215)},
		 {HTMLWhiteLinenColor, QColor(250, 240, 230)},
		 {HTMLWhiteLavenderBlushColor, QColor(255, 240, 245)},
		 {HTMLWhiteMistyRoseColor, QColor(255, 228, 225)},
		 {HTMLGrayGainsboroColor, QColor(220, 220, 220)},
		 {HTMLGrayLightGrayColor, QColor(211, 211, 211)},
		 {HTMLGraySilverColor, QColor(192, 192, 192)},
		 {HTMLGrayDarkGrayColor, QColor(169, 169, 169)},
		 {HTMLGrayGrayColor, QColor(128, 128, 128)},
		 {HTMLGrayDimGrayColor, QColor(105, 105, 105)},
		 {HTMLGrayLightSlateGrayColor, QColor(119, 136, 153)},
		 {HTMLGraySlateGrayColor, QColor(112, 128, 144)},
		 {HTMLGrayDarkSlateGrayColor, QColor(47, 79, 79)},
		 {HTMLGrayBlackColor, QColor(0, 0, 0)}};

	if (_color == NoneColor) { pen.setBrush(Qt::transparent); }
	else if (_color == BlackColor)
	{
		pen.setBrush(QColor(0, 0, 0, pen.color().alpha()));
	}
	else if (_color == WhiteColor)
	{
		pen.setBrush(QColor(255, 255, 255, pen.color().alpha()));
	}
	else
	{
		auto style_ = color_map.find(_color);
		if (style_ != color_map.end()) { pen.setColor(*style_); }
	}

	//Apply antialiasing
	painter.setRenderHint(QPainter::Antialiasing,          _antialiased);
	painter.setRenderHint(QPainter::TextAntialiasing,      _antialiased);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, _antialiased);

	painter.setPen(pen);
	painter.setBrush(brush);
}

/**
	@brief CustomElementGraphicPart::drawShadowShape
	Draw a transparent blue shadow arround the shape of this item.
	The QPainterPathStroker used to draw shadows have a width of SHADOWS_HEIGHT
	Be carefull if penWeight of this item is to 0 the outline of strock is bigger of 0.5
	@param painter : painter to use for draw this shadows
*/
void CustomElementGraphicPart::drawShadowShape(QPainter *painter)
{
		//@FIXME if pen weight is 0, the strock outline is SHADOWS_HEIGHT/2 + 0.5
		//may be because shape have no line weight
	QPainterPathStroker strock;
	strock.setWidth(SHADOWS_HEIGHT);
	strock.setJoinStyle(Qt::RoundJoin);

	painter->save();
	QColor color(Qt::darkBlue);
	color.setAlpha(50);
	painter -> setBrush (QBrush (color));
	painter -> setPen   (Qt::NoPen);
	painter -> drawPath (strock.createStroke(shadowShape()));
	painter -> restore  ();
}

/**
	@brief CustomElementGraphicPart::hoverEnterEvent
	Reimplemented from QGraphicsObject.
	Set m_hovered to true
	@param event
*/
void CustomElementGraphicPart::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	m_hovered = true;
	QGraphicsObject::hoverEnterEvent(event);
}

/**
	@brief CustomElementGraphicPart::hoverLeaveEvent
	Reimplemented from QGraphicsObject.
	Set m_hovered to false
	@param event
*/
void CustomElementGraphicPart::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	m_hovered = false;
	QGraphicsObject::hoverLeaveEvent(event);
}

void CustomElementGraphicPart::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if(event->button() == Qt::LeftButton) {
		m_origin_pos = this->pos();
		m_first_move = true;
	}

	QGraphicsObject::mousePressEvent(event);
}

void CustomElementGraphicPart::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
		//m_first_move is used to avoid an unwanted behavior
		//when the properties dock widget is displayed :
		//1 there is no selection
		//2 the dock widget width is set to minimum
		//3 select a part, the dock widget gain new widgets used to edit
		//the current selected part and the width of the dock grow
		//so the width of the QGraphicsView is reduced and cause a mouse move event.
		//When this case occur the part is moved but they should not. This bool fix it.
	if (Q_UNLIKELY(m_first_move)) {
		m_first_move = false;
		return;
	}

	if((event->buttons() & Qt::LeftButton) && (flags() & QGraphicsItem::ItemIsMovable))
	{
		QPointF pos = event->scenePos() + (m_origin_pos - event->buttonDownScenePos(Qt::LeftButton));
		event->modifiers() == Qt::ControlModifier ? setPos(pos) : setPos(elementScene()->snapToGrid(pos));
	}
	else
		QGraphicsObject::mouseMoveEvent(event);
}

void CustomElementGraphicPart::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if((event->button() & Qt::LeftButton) && (flags() & QGraphicsItem::ItemIsMovable) && m_origin_pos != pos())
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(this, "pos", QVariant(m_origin_pos), QVariant(pos()));
		undo->setText(tr("Déplacer une primitive"));
		undo->enableAnimation();
		elementScene()->undoStack().push(undo);
	}

	QGraphicsObject::mouseReleaseEvent(event);
}
