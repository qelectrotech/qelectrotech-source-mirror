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
#include "customelementgraphicpart.h"
#include "elementscene.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

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
	Write the curent style to xml element.
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


	css_like_styles += ";filling:";
	if      (_filling == NoneFilling)  css_like_styles += "none";
	else if (_filling == BlackFilling) css_like_styles += "black";
	else if (_filling == WhiteFilling) css_like_styles += "white";
	else if (_filling == GreenFilling) css_like_styles += "green";
	else if (_filling == BlueFilling)  css_like_styles += "blue";
	else if (_filling == RedFilling)   css_like_styles += "red";
	else if (_filling == GrayFilling)  css_like_styles += "gray";
	else if (_filling == BrunFilling)  css_like_styles += "brun";
	else if (_filling == YellowFilling)  css_like_styles += "yellow";
	else if (_filling == CyanFilling)  css_like_styles += "cyan";
	else if (_filling == MagentaFilling)  css_like_styles += "magenta";
	else if (_filling == LightgrayFilling)  css_like_styles += "lightgray";
	else if (_filling == OrangeFilling)  css_like_styles += "orange";
	else if (_filling == PurpleFilling)  css_like_styles += "purple";
	else if (_filling == HTMLPinkPinkFilling)  css_like_styles += "HTMLPinkPink";
	else if (_filling == HTMLPinkLightPinkFilling)  css_like_styles += "HTMLPinkLightPink";
	else if (_filling == HTMLPinkHotPinkFilling)  css_like_styles += "HTMLPinkHotPink";
	else if (_filling == HTMLPinkDeepPinkFilling)  css_like_styles += "HTMLPinkDeepPink";
	else if (_filling == HTMLPinkPaleVioletRedFilling)  css_like_styles += "HTMLPinkPaleVioletRed";
	else if (_filling == HTMLPinkMediumVioletRedFilling)  css_like_styles += "HTMLPinkMediumVioletRed";
	else if (_filling == HTMLRedLightSalmonFilling)  css_like_styles += "HTMLRedLightSalmon";
	else if (_filling == HTMLRedSalmonFilling)  css_like_styles += "HTMLRedSalmon";
	else if (_filling == HTMLRedDarkSalmonFilling)  css_like_styles += "HTMLRedDarkSalmon";
	else if (_filling == HTMLRedLightCoralFilling)  css_like_styles += "HTMLRedLightCoral";
	else if (_filling == HTMLRedIndianRedFilling)  css_like_styles += "HTMLRedIndianRed";
	else if (_filling == HTMLRedCrimsonFilling)  css_like_styles += "HTMLRedCrimson";
	else if (_filling == HTMLRedFirebrickFilling)  css_like_styles += "HTMLRedFirebrick";
	else if (_filling == HTMLRedDarkRedFilling)  css_like_styles += "HTMLRedDarkRed";
	else if (_filling == HTMLRedRedFilling)  css_like_styles += "HTMLRedRed";
	else if (_filling == HTMLOrangeOrangeRedFilling)  css_like_styles += "HTMLOrangeOrangeRed";
	else if (_filling == HTMLOrangeTomatoFilling)  css_like_styles += "HTMLOrangeTomato";
	else if (_filling == HTMLOrangeCoralFilling)  css_like_styles += "HTMLOrangeCoral";
	else if (_filling == HTMLOrangeDarkOrangeFilling)  css_like_styles += "HTMLOrangeDarkOrange";
	else if (_filling == HTMLOrangeOrangeFilling)  css_like_styles += "HTMLOrangeOrange";
	else if (_filling == HTMLYellowYellowFilling)  css_like_styles += "HTMLYellowYellow";
	else if (_filling == HTMLYellowLightYellowFilling)  css_like_styles += "HTMLYellowLightYellow";
	else if (_filling == HTMLYellowLemonChiffonFilling)  css_like_styles += "HTMLYellowLemonChiffon";
	else if (_filling == HTMLYellowLightGoldenrodYellowFilling)  css_like_styles += "HTMLYellowLightGoldenrodYellow";
	else if (_filling == HTMLYellowPapayaWhipFilling)  css_like_styles += "HTMLYellowPapayaWhip";
	else if (_filling == HTMLYellowMoccasinFilling)  css_like_styles += "HTMLYellowMoccasin";
	else if (_filling == HTMLYellowPeachPuffFilling)  css_like_styles += "HTMLYellowPeachPuff";
	else if (_filling == HTMLYellowPaleGoldenrodFilling)  css_like_styles += "HTMLYellowPaleGoldenrod";
	else if (_filling == HTMLYellowKhakiFilling)  css_like_styles += "HTMLYellowKhaki";
	else if (_filling == HTMLYellowDarkKhakiFilling)  css_like_styles += "HTMLYellowDarkKhaki";
	else if (_filling == HTMLYellowGoldFilling)  css_like_styles += "HTMLYellowGold";
	else if (_filling == HTMLBrownCornsilkFilling)  css_like_styles += "HTMLBrownCornsilk";
	else if (_filling == HTMLBrownBlanchedAlmondFilling)  css_like_styles += "HTMLBrownBlanchedAlmond";
	else if (_filling == HTMLBrownBisqueFilling)  css_like_styles += "HTMLBrownBisque";
	else if (_filling == HTMLBrownNavajoWhiteFilling)  css_like_styles += "HTMLBrownNavajoWhite";
	else if (_filling == HTMLBrownWheatFilling)  css_like_styles += "HTMLBrownWheat";
	else if (_filling == HTMLBrownBurlywoodFilling)  css_like_styles += "HTMLBrownBurlywood";
	else if (_filling == HTMLBrownTanFilling)  css_like_styles += "HTMLBrownTan";
	else if (_filling == HTMLBrownRosyBrownFilling)  css_like_styles += "HTMLBrownRosyBrown";
	else if (_filling == HTMLBrownSandyBrownFilling)  css_like_styles += "HTMLBrownSandyBrown";
	else if (_filling == HTMLBrownGoldenrodFilling)  css_like_styles += "HTMLBrownGoldenrod";
	else if (_filling == HTMLBrownDarkGoldenrodFilling)  css_like_styles += "HTMLBrownDarkGoldenrod";
	else if (_filling == HTMLBrownPeruFilling)  css_like_styles += "HTMLBrownPeru";
	else if (_filling == HTMLBrownChocolateFilling)  css_like_styles += "HTMLBrownChocolate";
	else if (_filling == HTMLBrownSaddleBrownFilling)  css_like_styles += "HTMLBrownSaddleBrown";
	else if (_filling == HTMLBrownSiennaFilling)  css_like_styles += "HTMLBrownSienna";
	else if (_filling == HTMLBrownBrownFilling)  css_like_styles += "HTMLBrownBrown";
	else if (_filling == HTMLBrownMaroonFilling)  css_like_styles += "HTMLBrownMaroon";
	else if (_filling == HTMLGreenDarkOliveGreenFilling)  css_like_styles += "HTMLGreenDarkOliveGreen";
	else if (_filling == HTMLGreenOliveFilling)  css_like_styles += "HTMLGreenOlive";
	else if (_filling == HTMLGreenOliveDrabFilling)  css_like_styles += "HTMLGreenOliveDrab";
	else if (_filling == HTMLGreenYellowGreenFilling)  css_like_styles += "HTMLGreenYellowGreen";
	else if (_filling == HTMLGreenLimeGreenFilling)  css_like_styles += "HTMLGreenLimeGreen";
	else if (_filling == HTMLGreenLimeFilling)  css_like_styles += "HTMLGreenLime";
	else if (_filling == HTMLGreenLawnGreenFilling)  css_like_styles += "HTMLGreenLawnGreen";
	else if (_filling == HTMLGreenChartreuseFilling)  css_like_styles += "HTMLGreenChartreuse";
	else if (_filling == HTMLGreenGreenYellowFilling)  css_like_styles += "HTMLGreenGreenYellow";
	else if (_filling == HTMLGreenSpringGreenFilling)  css_like_styles += "HTMLGreenSpringGreen";
	else if (_filling == HTMLGreenMediumSpringGreenFilling)  css_like_styles += "HTMLGreenMediumSpringGreen";
	else if (_filling == HTMLGreenLightGreenFilling)  css_like_styles += "HTMLGreenLightGreen";
	else if (_filling == HTMLGreenPaleGreenFilling)  css_like_styles += "HTMLGreenPaleGreen";
	else if (_filling == HTMLGreenDarkSeaGreenFilling)  css_like_styles += "HTMLGreenDarkSeaGreen";
	else if (_filling == HTMLGreenMediumAquamarineFilling)  css_like_styles += "HTMLGreenMediumAquamarine";
	else if (_filling == HTMLGreenMediumSeaGreenFilling)  css_like_styles += "HTMLGreenMediumSeaGreen";
	else if (_filling == HTMLGreenSeaGreenFilling)  css_like_styles += "HTMLGreenSeaGreen";
	else if (_filling == HTMLGreenForestGreenFilling)  css_like_styles += "HTMLGreenForestGreen";
	else if (_filling == HTMLGreenGreenFilling)  css_like_styles += "HTMLGreenGreen";
	else if (_filling == HTMLGreenDarkGreenFilling)  css_like_styles += "HTMLGreenDarkGreen";
	else if (_filling == HTMLCyanAquaFilling)  css_like_styles += "HTMLCyanAqua";
	else if (_filling == HTMLCyanCyanFilling)  css_like_styles += "HTMLCyanCyan";
	else if (_filling == HTMLCyanLightCyanFilling)  css_like_styles += "HTMLCyanLightCyan";
	else if (_filling == HTMLCyanPaleTurquoiseFilling)  css_like_styles += "HTMLCyanPaleTurquoise";
	else if (_filling == HTMLCyanAquamarineFilling)  css_like_styles += "HTMLCyanAquamarine";
	else if (_filling == HTMLCyanTurquoiseFilling)  css_like_styles += "HTMLCyanTurquoise";
	else if (_filling == HTMLCyanMediumTurquoiseFilling)  css_like_styles += "HTMLCyanMediumTurquoise";
	else if (_filling == HTMLCyanDarkTurquoiseFilling)  css_like_styles += "HTMLCyanDarkTurquoise";
	else if (_filling == HTMLCyanLightSeaGreenFilling)  css_like_styles += "HTMLCyanLightSeaGreen";
	else if (_filling == HTMLCyanCadetBlueFilling)  css_like_styles += "HTMLCyanCadetBlue";
	else if (_filling == HTMLCyanDarkCyanFilling)  css_like_styles += "HTMLCyanDarkCyan";
	else if (_filling == HTMLCyanTealFilling)  css_like_styles += "HTMLCyanTeal";
	else if (_filling == HTMLBlueLightSteelBlueFilling)  css_like_styles += "HTMLBlueLightSteelBlue";
	else if (_filling == HTMLBluePowderBlueFilling)  css_like_styles += "HTMLBluePowderBlue";
	else if (_filling == HTMLBlueLightBlueFilling)  css_like_styles += "HTMLBlueLightBlue";
	else if (_filling == HTMLBlueSkyBlueFilling)  css_like_styles += "HTMLBlueSkyBlue";
	else if (_filling == HTMLBlueLightSkyBlueFilling)  css_like_styles += "HTMLBlueLightSkyBlue";
	else if (_filling == HTMLBlueDeepSkyBlueFilling)  css_like_styles += "HTMLBlueDeepSkyBlue";
	else if (_filling == HTMLBlueDodgerBlueFilling)  css_like_styles += "HTMLBlueDodgerBlue";
	else if (_filling == HTMLBlueCornflowerBlueFilling)  css_like_styles += "HTMLBlueCornflowerBlue";
	else if (_filling == HTMLBlueSteelBlueFilling)  css_like_styles += "HTMLBlueSteelBlue";
	else if (_filling == HTMLBlueRoyalBlueFilling)  css_like_styles += "HTMLBlueRoyalBlue";
	else if (_filling == HTMLBlueBlueFilling)  css_like_styles += "HTMLBlueBlue";
	else if (_filling == HTMLBlueMediumBlueFilling)  css_like_styles += "HTMLBlueMediumBlue";
	else if (_filling == HTMLBlueDarkBlueFilling)  css_like_styles += "HTMLBlueDarkBlue";
	else if (_filling == HTMLBlueNavyFilling)  css_like_styles += "HTMLBlueNavy";
	else if (_filling == HTMLBlueMidnightBlueFilling)  css_like_styles += "HTMLBlueMidnightBlue";
	else if (_filling == HTMLPurpleLavenderFilling)  css_like_styles += "HTMLPurpleLavender";
	else if (_filling == HTMLPurpleThistleFilling)  css_like_styles += "HTMLPurpleThistle";
	else if (_filling == HTMLPurplePlumFilling)  css_like_styles += "HTMLPurplePlum";
	else if (_filling == HTMLPurpleVioletFilling)  css_like_styles += "HTMLPurpleViolet";
	else if (_filling == HTMLPurpleOrchidFilling)  css_like_styles += "HTMLPurpleOrchid";
	else if (_filling == HTMLPurpleFuchsiaFilling)  css_like_styles += "HTMLPurpleFuchsia";
	else if (_filling == HTMLPurpleMagentaFilling)  css_like_styles += "HTMLPurpleMagenta";
	else if (_filling == HTMLPurpleMediumOrchidFilling)  css_like_styles += "HTMLPurpleMediumOrchid";
	else if (_filling == HTMLPurpleMediumPurpleFilling)  css_like_styles += "HTMLPurpleMediumPurple";
	else if (_filling == HTMLPurpleBlueVioletFilling)  css_like_styles += "HTMLPurpleBlueViolet";
	else if (_filling == HTMLPurpleDarkVioletFilling)  css_like_styles += "HTMLPurpleDarkViolet";
	else if (_filling == HTMLPurpleDarkOrchidFilling)  css_like_styles += "HTMLPurpleDarkOrchid";
	else if (_filling == HTMLPurpleDarkMagentaFilling)  css_like_styles += "HTMLPurpleDarkMagenta";
	else if (_filling == HTMLPurplePurpleFilling)  css_like_styles += "HTMLPurplePurple";
	else if (_filling == HTMLPurpleIndigoFilling)  css_like_styles += "HTMLPurpleIndigo";
	else if (_filling == HTMLPurpleDarkSlateBlueFilling)  css_like_styles += "HTMLPurpleDarkSlateBlue";
	else if (_filling == HTMLPurpleSlateBlueFilling)  css_like_styles += "HTMLPurpleSlateBlue";
	else if (_filling == HTMLPurpleMediumSlateBlueFilling)  css_like_styles += "HTMLPurpleMediumSlateBlue";
	else if (_filling == HTMLWhiteWhiteFilling)  css_like_styles += "HTMLWhiteWhite";
	else if (_filling == HTMLWhiteSnowFilling)  css_like_styles += "HTMLWhiteSnow";
	else if (_filling == HTMLWhiteHoneydewFilling)  css_like_styles += "HTMLWhiteHoneydew";
	else if (_filling == HTMLWhiteMintCreamFilling)  css_like_styles += "HTMLWhiteMintCream";
	else if (_filling == HTMLWhiteAzureFilling)  css_like_styles += "HTMLWhiteAzure";
	else if (_filling == HTMLWhiteAliceBlueFilling)  css_like_styles += "HTMLWhiteAliceBlue";
	else if (_filling == HTMLWhiteGhostWhiteFilling)  css_like_styles += "HTMLWhiteGhostWhite";
	else if (_filling == HTMLWhiteWhiteSmokeFilling)  css_like_styles += "HTMLWhiteWhiteSmoke";
	else if (_filling == HTMLWhiteSeashellFilling)  css_like_styles += "HTMLWhiteSeashell";
	else if (_filling == HTMLWhiteBeigeFilling)  css_like_styles += "HTMLWhiteBeige";
	else if (_filling == HTMLWhiteOldLaceFilling)  css_like_styles += "HTMLWhiteOldLace";
	else if (_filling == HTMLWhiteFloralWhiteFilling)  css_like_styles += "HTMLWhiteFloralWhite";
	else if (_filling == HTMLWhiteIvoryFilling)  css_like_styles += "HTMLWhiteIvory";
	else if (_filling == HTMLWhiteAntiqueWhiteFilling)  css_like_styles += "HTMLWhiteAntiqueWhite";
	else if (_filling == HTMLWhiteLinenFilling)  css_like_styles += "HTMLWhiteLinen";
	else if (_filling == HTMLWhiteLavenderBlushFilling)  css_like_styles += "HTMLWhiteLavenderBlush";
	else if (_filling == HTMLWhiteMistyRoseFilling)  css_like_styles += "HTMLWhiteMistyRose";
	else if (_filling == HTMLGrayGainsboroFilling)  css_like_styles += "HTMLGrayGainsboro";
	else if (_filling == HTMLGrayLightGrayFilling)  css_like_styles += "HTMLGrayLightGray";
	else if (_filling == HTMLGraySilverFilling)  css_like_styles += "HTMLGraySilver";
	else if (_filling == HTMLGrayDarkGrayFilling)  css_like_styles += "HTMLGrayDarkGray";
	else if (_filling == HTMLGrayGrayFilling)  css_like_styles += "HTMLGrayGray";
	else if (_filling == HTMLGrayDimGrayFilling)  css_like_styles += "HTMLGrayDimGray";
	else if (_filling == HTMLGrayLightSlateGrayFilling)  css_like_styles += "HTMLGrayLightSlateGray";
	else if (_filling == HTMLGraySlateGrayFilling)  css_like_styles += "HTMLGraySlateGray";
	else if (_filling == HTMLGrayDarkSlateGrayFilling)  css_like_styles += "HTMLGrayDarkSlateGray";
	else if (_filling == HTMLGrayBlackFilling)  css_like_styles += "HTMLGrayBlack";
	else if (_filling == HorFilling) css_like_styles += "hor";
	else if (_filling == VerFilling) css_like_styles += "ver";
	else if (_filling == BdiagFilling) css_like_styles += "bdiag";
	else if (_filling == FdiagFilling) css_like_styles += "fdiag";


	css_like_styles += ";color:";
	if      (_color == WhiteColor) css_like_styles += "white";
	else if (_color == BlackColor) css_like_styles += "black";
	else if (_color == GreenColor) css_like_styles += "green";
	else if (_color == RedColor)   css_like_styles += "red";
	else if (_color == BlueColor)  css_like_styles += "blue";
	else if (_color == GrayColor)  css_like_styles += "gray";
	else if (_color == BrunColor)  css_like_styles += "brun";
	else if (_color == YellowColor)  css_like_styles += "yellow";
	else if (_color == CyanColor)  css_like_styles += "cyan";
	else if (_color == MagentaColor)  css_like_styles += "magenta";
	else if (_color == LightgrayColor)  css_like_styles += "lightgray";
	else if (_color == OrangeColor)  css_like_styles += "orange";
	else if (_color == PurpleColor)  css_like_styles += "purple";
	else if (_color == HTMLPinkPinkColor)  css_like_styles += "HTMLPinkPink";
	else if (_color == HTMLPinkLightPinkColor)  css_like_styles += "HTMLPinkLightPink";
	else if (_color == HTMLPinkHotPinkColor)  css_like_styles += "HTMLPinkHotPink";
	else if (_color == HTMLPinkDeepPinkColor)  css_like_styles += "HTMLPinkDeepPink";
	else if (_color == HTMLPinkPaleVioletRedColor)  css_like_styles += "HTMLPinkPaleVioletRed";
	else if (_color == HTMLPinkMediumVioletRedColor)  css_like_styles += "HTMLPinkMediumVioletRed";
	else if (_color == HTMLRedLightSalmonColor)  css_like_styles += "HTMLRedLightSalmon";
	else if (_color == HTMLRedSalmonColor)  css_like_styles += "HTMLRedSalmon";
	else if (_color == HTMLRedDarkSalmonColor)  css_like_styles += "HTMLRedDarkSalmon";
	else if (_color == HTMLRedLightCoralColor)  css_like_styles += "HTMLRedLightCoral";
	else if (_color == HTMLRedIndianRedColor)  css_like_styles += "HTMLRedIndianRed";
	else if (_color == HTMLRedCrimsonColor)  css_like_styles += "HTMLRedCrimson";
	else if (_color == HTMLRedFirebrickColor)  css_like_styles += "HTMLRedFirebrick";
	else if (_color == HTMLRedDarkRedColor)  css_like_styles += "HTMLRedDarkRed";
	else if (_color == HTMLRedRedColor)  css_like_styles += "HTMLRedRed";
	else if (_color == HTMLOrangeOrangeRedColor)  css_like_styles += "HTMLOrangeOrangeRed";
	else if (_color == HTMLOrangeTomatoColor)  css_like_styles += "HTMLOrangeTomato";
	else if (_color == HTMLOrangeCoralColor)  css_like_styles += "HTMLOrangeCoral";
	else if (_color == HTMLOrangeDarkOrangeColor)  css_like_styles += "HTMLOrangeDarkOrange";
	else if (_color == HTMLOrangeOrangeColor)  css_like_styles += "HTMLOrangeOrange";
	else if (_color == HTMLYellowYellowColor)  css_like_styles += "HTMLYellowYellow";
	else if (_color == HTMLYellowLightYellowColor)  css_like_styles += "HTMLYellowLightYellow";
	else if (_color == HTMLYellowLemonChiffonColor)  css_like_styles += "HTMLYellowLemonChiffon";
	else if (_color == HTMLYellowLightGoldenrodYellowColor)  css_like_styles += "HTMLYellowLightGoldenrodYellow";
	else if (_color == HTMLYellowPapayaWhipColor)  css_like_styles += "HTMLYellowPapayaWhip";
	else if (_color == HTMLYellowMoccasinColor)  css_like_styles += "HTMLYellowMoccasin";
	else if (_color == HTMLYellowPeachPuffColor)  css_like_styles += "HTMLYellowPeachPuff";
	else if (_color == HTMLYellowPaleGoldenrodColor)  css_like_styles += "HTMLYellowPaleGoldenrod";
	else if (_color == HTMLYellowKhakiColor)  css_like_styles += "HTMLYellowKhaki";
	else if (_color == HTMLYellowDarkKhakiColor)  css_like_styles += "HTMLYellowDarkKhaki";
	else if (_color == HTMLYellowGoldColor)  css_like_styles += "HTMLYellowGold";
	else if (_color == HTMLBrownCornsilkColor)  css_like_styles += "HTMLBrownCornsilk";
	else if (_color == HTMLBrownBlanchedAlmondColor)  css_like_styles += "HTMLBrownBlanchedAlmond";
	else if (_color == HTMLBrownBisqueColor)  css_like_styles += "HTMLBrownBisque";
	else if (_color == HTMLBrownNavajoWhiteColor)  css_like_styles += "HTMLBrownNavajoWhite";
	else if (_color == HTMLBrownWheatColor)  css_like_styles += "HTMLBrownWheat";
	else if (_color == HTMLBrownBurlywoodColor)  css_like_styles += "HTMLBrownBurlywood";
	else if (_color == HTMLBrownTanColor)  css_like_styles += "HTMLBrownTan";
	else if (_color == HTMLBrownRosyBrownColor)  css_like_styles += "HTMLBrownRosyBrown";
	else if (_color == HTMLBrownSandyBrownColor)  css_like_styles += "HTMLBrownSandyBrown";
	else if (_color == HTMLBrownGoldenrodColor)  css_like_styles += "HTMLBrownGoldenrod";
	else if (_color == HTMLBrownDarkGoldenrodColor)  css_like_styles += "HTMLBrownDarkGoldenrod";
	else if (_color == HTMLBrownPeruColor)  css_like_styles += "HTMLBrownPeru";
	else if (_color == HTMLBrownChocolateColor)  css_like_styles += "HTMLBrownChocolate";
	else if (_color == HTMLBrownSaddleBrownColor)  css_like_styles += "HTMLBrownSaddleBrown";
	else if (_color == HTMLBrownSiennaColor)  css_like_styles += "HTMLBrownSienna";
	else if (_color == HTMLBrownBrownColor)  css_like_styles += "HTMLBrownBrown";
	else if (_color == HTMLBrownMaroonColor)  css_like_styles += "HTMLBrownMaroon";
	else if (_color == HTMLGreenDarkOliveGreenColor)  css_like_styles += "HTMLGreenDarkOliveGreen";
	else if (_color == HTMLGreenOliveColor)  css_like_styles += "HTMLGreenOlive";
	else if (_color == HTMLGreenOliveDrabColor)  css_like_styles += "HTMLGreenOliveDrab";
	else if (_color == HTMLGreenYellowGreenColor)  css_like_styles += "HTMLGreenYellowGreen";
	else if (_color == HTMLGreenLimeGreenColor)  css_like_styles += "HTMLGreenLimeGreen";
	else if (_color == HTMLGreenLimeColor)  css_like_styles += "HTMLGreenLime";
	else if (_color == HTMLGreenLawnGreenColor)  css_like_styles += "HTMLGreenLawnGreen";
	else if (_color == HTMLGreenChartreuseColor)  css_like_styles += "HTMLGreenChartreuse";
	else if (_color == HTMLGreenGreenYellowColor)  css_like_styles += "HTMLGreenGreenYellow";
	else if (_color == HTMLGreenSpringGreenColor)  css_like_styles += "HTMLGreenSpringGreen";
	else if (_color == HTMLGreenMediumSpringGreenColor)  css_like_styles += "HTMLGreenMediumSpringGreen";
	else if (_color == HTMLGreenLightGreenColor)  css_like_styles += "HTMLGreenLightGreen";
	else if (_color == HTMLGreenPaleGreenColor)  css_like_styles += "HTMLGreenPaleGreen";
	else if (_color == HTMLGreenDarkSeaGreenColor)  css_like_styles += "HTMLGreenDarkSeaGreen";
	else if (_color == HTMLGreenMediumAquamarineColor)  css_like_styles += "HTMLGreenMediumAquamarine";
	else if (_color == HTMLGreenMediumSeaGreenColor)  css_like_styles += "HTMLGreenMediumSeaGreen";
	else if (_color == HTMLGreenSeaGreenColor)  css_like_styles += "HTMLGreenSeaGreen";
	else if (_color == HTMLGreenForestGreenColor)  css_like_styles += "HTMLGreenForestGreen";
	else if (_color == HTMLGreenGreenColor)  css_like_styles += "HTMLGreenGreen";
	else if (_color == HTMLGreenDarkGreenColor)  css_like_styles += "HTMLGreenDarkGreen";
	else if (_color == HTMLCyanAquaColor)  css_like_styles += "HTMLCyanAqua";
	else if (_color == HTMLCyanCyanColor)  css_like_styles += "HTMLCyanCyan";
	else if (_color == HTMLCyanLightCyanColor)  css_like_styles += "HTMLCyanLightCyan";
	else if (_color == HTMLCyanPaleTurquoiseColor)  css_like_styles += "HTMLCyanPaleTurquoise";
	else if (_color == HTMLCyanAquamarineColor)  css_like_styles += "HTMLCyanAquamarine";
	else if (_color == HTMLCyanTurquoiseColor)  css_like_styles += "HTMLCyanTurquoise";
	else if (_color == HTMLCyanMediumTurquoiseColor)  css_like_styles += "HTMLCyanMediumTurquoise";
	else if (_color == HTMLCyanDarkTurquoiseColor)  css_like_styles += "HTMLCyanDarkTurquoise";
	else if (_color == HTMLCyanLightSeaGreenColor)  css_like_styles += "HTMLCyanLightSeaGreen";
	else if (_color == HTMLCyanCadetBlueColor)  css_like_styles += "HTMLCyanCadetBlue";
	else if (_color == HTMLCyanDarkCyanColor)  css_like_styles += "HTMLCyanDarkCyan";
	else if (_color == HTMLCyanTealColor)  css_like_styles += "HTMLCyanTeal";
	else if (_color == HTMLBlueLightSteelBlueColor)  css_like_styles += "HTMLBlueLightSteelBlue";
	else if (_color == HTMLBluePowderBlueColor)  css_like_styles += "HTMLBluePowderBlue";
	else if (_color == HTMLBlueLightBlueColor)  css_like_styles += "HTMLBlueLightBlue";
	else if (_color == HTMLBlueSkyBlueColor)  css_like_styles += "HTMLBlueSkyBlue";
	else if (_color == HTMLBlueLightSkyBlueColor)  css_like_styles += "HTMLBlueLightSkyBlue";
	else if (_color == HTMLBlueDeepSkyBlueColor)  css_like_styles += "HTMLBlueDeepSkyBlue";
	else if (_color == HTMLBlueDodgerBlueColor)  css_like_styles += "HTMLBlueDodgerBlue";
	else if (_color == HTMLBlueCornflowerBlueColor)  css_like_styles += "HTMLBlueCornflowerBlue";
	else if (_color == HTMLBlueSteelBlueColor)  css_like_styles += "HTMLBlueSteelBlue";
	else if (_color == HTMLBlueRoyalBlueColor)  css_like_styles += "HTMLBlueRoyalBlue";
	else if (_color == HTMLBlueBlueColor)  css_like_styles += "HTMLBlueBlue";
	else if (_color == HTMLBlueMediumBlueColor)  css_like_styles += "HTMLBlueMediumBlue";
	else if (_color == HTMLBlueDarkBlueColor)  css_like_styles += "HTMLBlueDarkBlue";
	else if (_color == HTMLBlueNavyColor)  css_like_styles += "HTMLBlueNavy";
	else if (_color == HTMLBlueMidnightBlueColor)  css_like_styles += "HTMLBlueMidnightBlue";
	else if (_color == HTMLPurpleLavenderColor)  css_like_styles += "HTMLPurpleLavender";
	else if (_color == HTMLPurpleThistleColor)  css_like_styles += "HTMLPurpleThistle";
	else if (_color == HTMLPurplePlumColor)  css_like_styles += "HTMLPurplePlum";
	else if (_color == HTMLPurpleVioletColor)  css_like_styles += "HTMLPurpleViolet";
	else if (_color == HTMLPurpleOrchidColor)  css_like_styles += "HTMLPurpleOrchid";
	else if (_color == HTMLPurpleFuchsiaColor)  css_like_styles += "HTMLPurpleFuchsia";
	else if (_color == HTMLPurpleMagentaColor)  css_like_styles += "HTMLPurpleMagenta";
	else if (_color == HTMLPurpleMediumOrchidColor)  css_like_styles += "HTMLPurpleMediumOrchid";
	else if (_color == HTMLPurpleMediumPurpleColor)  css_like_styles += "HTMLPurpleMediumPurple";
	else if (_color == HTMLPurpleBlueVioletColor)  css_like_styles += "HTMLPurpleBlueViolet";
	else if (_color == HTMLPurpleDarkVioletColor)  css_like_styles += "HTMLPurpleDarkViolet";
	else if (_color == HTMLPurpleDarkOrchidColor)  css_like_styles += "HTMLPurpleDarkOrchid";
	else if (_color == HTMLPurpleDarkMagentaColor)  css_like_styles += "HTMLPurpleDarkMagenta";
	else if (_color == HTMLPurplePurpleColor)  css_like_styles += "HTMLPurplePurple";
	else if (_color == HTMLPurpleIndigoColor)  css_like_styles += "HTMLPurpleIndigo";
	else if (_color == HTMLPurpleDarkSlateBlueColor)  css_like_styles += "HTMLPurpleDarkSlateBlue";
	else if (_color == HTMLPurpleSlateBlueColor)  css_like_styles += "HTMLPurpleSlateBlue";
	else if (_color == HTMLPurpleMediumSlateBlueColor)  css_like_styles += "HTMLPurpleMediumSlateBlue";
	else if (_color == HTMLWhiteWhiteColor)  css_like_styles += "HTMLWhiteWhite";
	else if (_color == HTMLWhiteSnowColor)  css_like_styles += "HTMLWhiteSnow";
	else if (_color == HTMLWhiteHoneydewColor)  css_like_styles += "HTMLWhiteHoneydew";
	else if (_color == HTMLWhiteMintCreamColor)  css_like_styles += "HTMLWhiteMintCream";
	else if (_color == HTMLWhiteAzureColor)  css_like_styles += "HTMLWhiteAzure";
	else if (_color == HTMLWhiteAliceBlueColor)  css_like_styles += "HTMLWhiteAliceBlue";
	else if (_color == HTMLWhiteGhostWhiteColor)  css_like_styles += "HTMLWhiteGhostWhite";
	else if (_color == HTMLWhiteWhiteSmokeColor)  css_like_styles += "HTMLWhiteWhiteSmoke";
	else if (_color == HTMLWhiteSeashellColor)  css_like_styles += "HTMLWhiteSeashell";
	else if (_color == HTMLWhiteBeigeColor)  css_like_styles += "HTMLWhiteBeige";
	else if (_color == HTMLWhiteOldLaceColor)  css_like_styles += "HTMLWhiteOldLace";
	else if (_color == HTMLWhiteFloralWhiteColor)  css_like_styles += "HTMLWhiteFloralWhite";
	else if (_color == HTMLWhiteIvoryColor)  css_like_styles += "HTMLWhiteIvory";
	else if (_color == HTMLWhiteAntiqueWhiteColor)  css_like_styles += "HTMLWhiteAntiqueWhite";
	else if (_color == HTMLWhiteLinenColor)  css_like_styles += "HTMLWhiteLinen";
	else if (_color == HTMLWhiteLavenderBlushColor)  css_like_styles += "HTMLWhiteLavenderBlush";
	else if (_color == HTMLWhiteMistyRoseColor)  css_like_styles += "HTMLWhiteMistyRose";
	else if (_color == HTMLGrayGainsboroColor)  css_like_styles += "HTMLGrayGainsboro";
	else if (_color == HTMLGrayLightGrayColor)  css_like_styles += "HTMLGrayLightGray";
	else if (_color == HTMLGraySilverColor)  css_like_styles += "HTMLGraySilver";
	else if (_color == HTMLGrayDarkGrayColor)  css_like_styles += "HTMLGrayDarkGray";
	else if (_color == HTMLGrayGrayColor)  css_like_styles += "HTMLGrayGray";
	else if (_color == HTMLGrayDimGrayColor)  css_like_styles += "HTMLGrayDimGray";
	else if (_color == HTMLGrayLightSlateGrayColor)  css_like_styles += "HTMLGrayLightSlateGray";
	else if (_color == HTMLGraySlateGrayColor)  css_like_styles += "HTMLGraySlateGray";
	else if (_color == HTMLGrayDarkSlateGrayColor)  css_like_styles += "HTMLGrayDarkSlateGray";
	else if (_color == HTMLGrayBlackColor)  css_like_styles += "HTMLGrayBlack";
	else if (_color == NoneColor)  css_like_styles += "none";


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
			if      (style_value == "white") _filling = WhiteFilling;
			else if (style_value == "black") _filling = BlackFilling;
			else if (style_value == "red")   _filling = RedFilling;
			else if (style_value == "green") _filling = GreenFilling;
			else if (style_value == "blue")  _filling = BlueFilling;
			else if (style_value == "gray")  _filling = GrayFilling;
			else if (style_value == "brun")  _filling = BrunFilling;
			else if (style_value == "yellow")  _filling = YellowFilling;
			else if (style_value == "cyan")  _filling = CyanFilling;
			else if (style_value == "magenta")  _filling = MagentaFilling;
			else if (style_value == "lightgray")  _filling = LightgrayFilling;
			else if (style_value == "orange")  _filling = OrangeFilling;
			else if (style_value == "purple")  _filling = PurpleFilling;
			else if (style_value == "HTMLPinkPink")  _filling = HTMLPinkPinkFilling;
			else if (style_value == "HTMLPinkLightPink")  _filling = HTMLPinkLightPinkFilling;
			else if (style_value == "HTMLPinkHotPink")  _filling = HTMLPinkHotPinkFilling;
			else if (style_value == "HTMLPinkDeepPink")  _filling = HTMLPinkDeepPinkFilling;
			else if (style_value == "HTMLPinkPaleVioletRed")  _filling = HTMLPinkPaleVioletRedFilling;
			else if (style_value == "HTMLPinkMediumVioletRed")  _filling = HTMLPinkMediumVioletRedFilling;
			else if (style_value == "HTMLRedLightSalmon")  _filling = HTMLRedLightSalmonFilling;
			else if (style_value == "HTMLRedSalmon")  _filling = HTMLRedSalmonFilling;
			else if (style_value == "HTMLRedDarkSalmon")  _filling = HTMLRedDarkSalmonFilling;
			else if (style_value == "HTMLRedLightCoral")  _filling = HTMLRedLightCoralFilling;
			else if (style_value == "HTMLRedIndianRed")  _filling = HTMLRedIndianRedFilling;
			else if (style_value == "HTMLRedCrimson")  _filling = HTMLRedCrimsonFilling;
			else if (style_value == "HTMLRedFirebrick")  _filling = HTMLRedFirebrickFilling;
			else if (style_value == "HTMLRedDarkRed")  _filling = HTMLRedDarkRedFilling;
			else if (style_value == "HTMLRedRed")  _filling = HTMLRedRedFilling;
			else if (style_value == "HTMLOrangeOrangeRed")  _filling = HTMLOrangeOrangeRedFilling;
			else if (style_value == "HTMLOrangeTomato")  _filling = HTMLOrangeTomatoFilling;
			else if (style_value == "HTMLOrangeCoral")  _filling = HTMLOrangeCoralFilling;
			else if (style_value == "HTMLOrangeDarkOrange")  _filling = HTMLOrangeDarkOrangeFilling;
			else if (style_value == "HTMLOrangeOrange")  _filling = HTMLOrangeOrangeFilling;
			else if (style_value == "HTMLYellowYellow")  _filling = HTMLYellowYellowFilling;
			else if (style_value == "HTMLYellowLightYellow")  _filling = HTMLYellowLightYellowFilling;
			else if (style_value == "HTMLYellowLemonChiffon")  _filling = HTMLYellowLemonChiffonFilling;
			else if (style_value == "HTMLYellowLightGoldenrodYellow")  _filling = HTMLYellowLightGoldenrodYellowFilling;
			else if (style_value == "HTMLYellowPapayaWhip")  _filling = HTMLYellowPapayaWhipFilling;
			else if (style_value == "HTMLYellowMoccasin")  _filling = HTMLYellowMoccasinFilling;
			else if (style_value == "HTMLYellowPeachPuff")  _filling = HTMLYellowPeachPuffFilling;
			else if (style_value == "HTMLYellowPaleGoldenrod")  _filling = HTMLYellowPaleGoldenrodFilling;
			else if (style_value == "HTMLYellowKhaki")  _filling = HTMLYellowKhakiFilling;
			else if (style_value == "HTMLYellowDarkKhaki")  _filling = HTMLYellowDarkKhakiFilling;
			else if (style_value == "HTMLYellowGold")  _filling = HTMLYellowGoldFilling;
			else if (style_value == "HTMLBrownCornsilk")  _filling = HTMLBrownCornsilkFilling;
			else if (style_value == "HTMLBrownBlanchedAlmond")  _filling = HTMLBrownBlanchedAlmondFilling;
			else if (style_value == "HTMLBrownBisque")  _filling = HTMLBrownBisqueFilling;
			else if (style_value == "HTMLBrownNavajoWhite")  _filling = HTMLBrownNavajoWhiteFilling;
			else if (style_value == "HTMLBrownWheat")  _filling = HTMLBrownWheatFilling;
			else if (style_value == "HTMLBrownBurlywood")  _filling = HTMLBrownBurlywoodFilling;
			else if (style_value == "HTMLBrownTan")  _filling = HTMLBrownTanFilling;
			else if (style_value == "HTMLBrownRosyBrown")  _filling = HTMLBrownRosyBrownFilling;
			else if (style_value == "HTMLBrownSandyBrown")  _filling = HTMLBrownSandyBrownFilling;
			else if (style_value == "HTMLBrownGoldenrod")  _filling = HTMLBrownGoldenrodFilling;
			else if (style_value == "HTMLBrownDarkGoldenrod")  _filling = HTMLBrownDarkGoldenrodFilling;
			else if (style_value == "HTMLBrownPeru")  _filling = HTMLBrownPeruFilling;
			else if (style_value == "HTMLBrownChocolate")  _filling = HTMLBrownChocolateFilling;
			else if (style_value == "HTMLBrownSaddleBrown")  _filling = HTMLBrownSaddleBrownFilling;
			else if (style_value == "HTMLBrownSienna")  _filling = HTMLBrownSiennaFilling;
			else if (style_value == "HTMLBrownBrown")  _filling = HTMLBrownBrownFilling;
			else if (style_value == "HTMLBrownMaroon")  _filling = HTMLBrownMaroonFilling;
			else if (style_value == "HTMLGreenDarkOliveGreen")  _filling = HTMLGreenDarkOliveGreenFilling;
			else if (style_value == "HTMLGreenOlive")  _filling = HTMLGreenOliveFilling;
			else if (style_value == "HTMLGreenOliveDrab")  _filling = HTMLGreenOliveDrabFilling;
			else if (style_value == "HTMLGreenYellowGreen")  _filling = HTMLGreenYellowGreenFilling;
			else if (style_value == "HTMLGreenLimeGreen")  _filling = HTMLGreenLimeGreenFilling;
			else if (style_value == "HTMLGreenLime")  _filling = HTMLGreenLimeFilling;
			else if (style_value == "HTMLGreenLawnGreen")  _filling = HTMLGreenLawnGreenFilling;
			else if (style_value == "HTMLGreenChartreuse")  _filling = HTMLGreenChartreuseFilling;
			else if (style_value == "HTMLGreenGreenYellow")  _filling = HTMLGreenGreenYellowFilling;
			else if (style_value == "HTMLGreenSpringGreen")  _filling = HTMLGreenSpringGreenFilling;
			else if (style_value == "HTMLGreenMediumSpringGreen")  _filling = HTMLGreenMediumSpringGreenFilling;
			else if (style_value == "HTMLGreenLightGreen")  _filling = HTMLGreenLightGreenFilling;
			else if (style_value == "HTMLGreenPaleGreen")  _filling = HTMLGreenPaleGreenFilling;
			else if (style_value == "HTMLGreenDarkSeaGreen")  _filling = HTMLGreenDarkSeaGreenFilling;
			else if (style_value == "HTMLGreenMediumAquamarine")  _filling = HTMLGreenMediumAquamarineFilling;
			else if (style_value == "HTMLGreenMediumSeaGreen")  _filling = HTMLGreenMediumSeaGreenFilling;
			else if (style_value == "HTMLGreenSeaGreen")  _filling = HTMLGreenSeaGreenFilling;
			else if (style_value == "HTMLGreenForestGreen")  _filling = HTMLGreenForestGreenFilling;
			else if (style_value == "HTMLGreenGreen")  _filling = HTMLGreenGreenFilling;
			else if (style_value == "HTMLGreenDarkGreen")  _filling = HTMLGreenDarkGreenFilling;
			else if (style_value == "HTMLCyanAqua")  _filling = HTMLCyanAquaFilling;
			else if (style_value == "HTMLCyanCyan")  _filling = HTMLCyanCyanFilling;
			else if (style_value == "HTMLCyanLightCyan")  _filling = HTMLCyanLightCyanFilling;
			else if (style_value == "HTMLCyanPaleTurquoise")  _filling = HTMLCyanPaleTurquoiseFilling;
			else if (style_value == "HTMLCyanAquamarine")  _filling = HTMLCyanAquamarineFilling;
			else if (style_value == "HTMLCyanTurquoise")  _filling = HTMLCyanTurquoiseFilling;
			else if (style_value == "HTMLCyanMediumTurquoise")  _filling = HTMLCyanMediumTurquoiseFilling;
			else if (style_value == "HTMLCyanDarkTurquoise")  _filling = HTMLCyanDarkTurquoiseFilling;
			else if (style_value == "HTMLCyanLightSeaGreen")  _filling = HTMLCyanLightSeaGreenFilling;
			else if (style_value == "HTMLCyanCadetBlue")  _filling = HTMLCyanCadetBlueFilling;
			else if (style_value == "HTMLCyanDarkCyan")  _filling = HTMLCyanDarkCyanFilling;
			else if (style_value == "HTMLCyanTeal")  _filling = HTMLCyanTealFilling;
			else if (style_value == "HTMLBlueLightSteelBlue")  _filling = HTMLBlueLightSteelBlueFilling;
			else if (style_value == "HTMLBluePowderBlue")  _filling = HTMLBluePowderBlueFilling;
			else if (style_value == "HTMLBlueLightBlue")  _filling = HTMLBlueLightBlueFilling;
			else if (style_value == "HTMLBlueSkyBlue")  _filling = HTMLBlueSkyBlueFilling;
			else if (style_value == "HTMLBlueLightSkyBlue")  _filling = HTMLBlueLightSkyBlueFilling;
			else if (style_value == "HTMLBlueDeepSkyBlue")  _filling = HTMLBlueDeepSkyBlueFilling;
			else if (style_value == "HTMLBlueDodgerBlue")  _filling = HTMLBlueDodgerBlueFilling;
			else if (style_value == "HTMLBlueCornflowerBlue")  _filling = HTMLBlueCornflowerBlueFilling;
			else if (style_value == "HTMLBlueSteelBlue")  _filling = HTMLBlueSteelBlueFilling;
			else if (style_value == "HTMLBlueRoyalBlue")  _filling = HTMLBlueRoyalBlueFilling;
			else if (style_value == "HTMLBlueBlue")  _filling = HTMLBlueBlueFilling;
			else if (style_value == "HTMLBlueMediumBlue")  _filling = HTMLBlueMediumBlueFilling;
			else if (style_value == "HTMLBlueDarkBlue")  _filling = HTMLBlueDarkBlueFilling;
			else if (style_value == "HTMLBlueNavy")  _filling = HTMLBlueNavyFilling;
			else if (style_value == "HTMLBlueMidnightBlue")  _filling = HTMLBlueMidnightBlueFilling;
			else if (style_value == "HTMLPurpleLavender")  _filling = HTMLPurpleLavenderFilling;
			else if (style_value == "HTMLPurpleThistle")  _filling = HTMLPurpleThistleFilling;
			else if (style_value == "HTMLPurplePlum")  _filling = HTMLPurplePlumFilling;
			else if (style_value == "HTMLPurpleViolet")  _filling = HTMLPurpleVioletFilling;
			else if (style_value == "HTMLPurpleOrchid")  _filling = HTMLPurpleOrchidFilling;
			else if (style_value == "HTMLPurpleFuchsia")  _filling = HTMLPurpleFuchsiaFilling;
			else if (style_value == "HTMLPurpleMagenta")  _filling = HTMLPurpleMagentaFilling;
			else if (style_value == "HTMLPurpleMediumOrchid")  _filling = HTMLPurpleMediumOrchidFilling;
			else if (style_value == "HTMLPurpleMediumPurple")  _filling = HTMLPurpleMediumPurpleFilling;
			else if (style_value == "HTMLPurpleBlueViolet")  _filling = HTMLPurpleBlueVioletFilling;
			else if (style_value == "HTMLPurpleDarkViolet")  _filling = HTMLPurpleDarkVioletFilling;
			else if (style_value == "HTMLPurpleDarkOrchid")  _filling = HTMLPurpleDarkOrchidFilling;
			else if (style_value == "HTMLPurpleDarkMagenta")  _filling = HTMLPurpleDarkMagentaFilling;
			else if (style_value == "HTMLPurplePurple")  _filling = HTMLPurplePurpleFilling;
			else if (style_value == "HTMLPurpleIndigo")  _filling = HTMLPurpleIndigoFilling;
			else if (style_value == "HTMLPurpleDarkSlateBlue")  _filling = HTMLPurpleDarkSlateBlueFilling;
			else if (style_value == "HTMLPurpleSlateBlue")  _filling = HTMLPurpleSlateBlueFilling;
			else if (style_value == "HTMLPurpleMediumSlateBlue")  _filling = HTMLPurpleMediumSlateBlueFilling;
			else if (style_value == "HTMLWhiteWhite")  _filling = HTMLWhiteWhiteFilling;
			else if (style_value == "HTMLWhiteSnow")  _filling = HTMLWhiteSnowFilling;
			else if (style_value == "HTMLWhiteHoneydew")  _filling = HTMLWhiteHoneydewFilling;
			else if (style_value == "HTMLWhiteMintCream")  _filling = HTMLWhiteMintCreamFilling;
			else if (style_value == "HTMLWhiteAzure")  _filling = HTMLWhiteAzureFilling;
			else if (style_value == "HTMLWhiteAliceBlue")  _filling = HTMLWhiteAliceBlueFilling;
			else if (style_value == "HTMLWhiteGhostWhite")  _filling = HTMLWhiteGhostWhiteFilling;
			else if (style_value == "HTMLWhiteWhiteSmoke")  _filling = HTMLWhiteWhiteSmokeFilling;
			else if (style_value == "HTMLWhiteSeashell")  _filling = HTMLWhiteSeashellFilling;
			else if (style_value == "HTMLWhiteBeige")  _filling = HTMLWhiteBeigeFilling;
			else if (style_value == "HTMLWhiteOldLace")  _filling = HTMLWhiteOldLaceFilling;
			else if (style_value == "HTMLWhiteFloralWhite")  _filling = HTMLWhiteFloralWhiteFilling;
			else if (style_value == "HTMLWhiteIvory")  _filling = HTMLWhiteIvoryFilling;
			else if (style_value == "HTMLWhiteAntiqueWhite")  _filling = HTMLWhiteAntiqueWhiteFilling;
			else if (style_value == "HTMLWhiteLinen")  _filling = HTMLWhiteLinenFilling;
			else if (style_value == "HTMLWhiteLavenderBlush")  _filling = HTMLWhiteLavenderBlushFilling;
			else if (style_value == "HTMLWhiteMistyRose")  _filling = HTMLWhiteMistyRoseFilling;
			else if (style_value == "HTMLGrayGainsboro")  _filling = HTMLGrayGainsboroFilling;
			else if (style_value == "HTMLGrayLightGray")  _filling = HTMLGrayLightGrayFilling;
			else if (style_value == "HTMLGraySilver")  _filling = HTMLGraySilverFilling;
			else if (style_value == "HTMLGrayDarkGray")  _filling = HTMLGrayDarkGrayFilling;
			else if (style_value == "HTMLGrayGray")  _filling = HTMLGrayGrayFilling;
			else if (style_value == "HTMLGrayDimGray")  _filling = HTMLGrayDimGrayFilling;
			else if (style_value == "HTMLGrayLightSlateGray")  _filling = HTMLGrayLightSlateGrayFilling;
			else if (style_value == "HTMLGraySlateGray")  _filling = HTMLGraySlateGrayFilling;
			else if (style_value == "HTMLGrayDarkSlateGray")  _filling = HTMLGrayDarkSlateGrayFilling;
			else if (style_value == "HTMLGrayBlack")  _filling = HTMLGrayBlackFilling;
			else if (style_value == "hor")  _filling = HorFilling;
			else if (style_value == "ver")  _filling = VerFilling;
			else if (style_value == "bdiag")  _filling = BdiagFilling;
			else if (style_value == "fdiag")  _filling = FdiagFilling;
			else if (style_value == "none")  _filling = NoneFilling;
		}
		else if (style_name == "color")
		{
			if      (style_value == "black") _color = BlackColor;
			else if (style_value == "white") _color = WhiteColor;
			else if (style_value == "green") _color = GreenColor;
			else if (style_value == "red")   _color = RedColor;
			else if (style_value == "blue")  _color = BlueColor;
			else if (style_value == "gray")  _color = GrayColor;
			else if (style_value == "brun")  _color = BrunColor;
			else if (style_value == "yellow")  _color = YellowColor;
			else if (style_value == "cyan")  _color = CyanColor;
			else if (style_value == "magenta")  _color = MagentaColor;
			else if (style_value == "lightgray")  _color = LightgrayColor;
			else if (style_value == "orange")  _color = OrangeColor;
			else if (style_value == "purple")  _color = PurpleColor;
			else if (style_value == "HTMLPinkPink")  _color = HTMLPinkPinkColor;
			else if (style_value == "HTMLPinkLightPink")  _color = HTMLPinkLightPinkColor;
			else if (style_value == "HTMLPinkHotPink")  _color = HTMLPinkHotPinkColor;
			else if (style_value == "HTMLPinkDeepPink")  _color = HTMLPinkDeepPinkColor;
			else if (style_value == "HTMLPinkPaleVioletRed")  _color = HTMLPinkPaleVioletRedColor;
			else if (style_value == "HTMLPinkMediumVioletRed")  _color = HTMLPinkMediumVioletRedColor;
			else if (style_value == "HTMLRedLightSalmon")  _color = HTMLRedLightSalmonColor;
			else if (style_value == "HTMLRedSalmon")  _color = HTMLRedSalmonColor;
			else if (style_value == "HTMLRedDarkSalmon")  _color = HTMLRedDarkSalmonColor;
			else if (style_value == "HTMLRedLightCoral")  _color = HTMLRedLightCoralColor;
			else if (style_value == "HTMLRedIndianRed")  _color = HTMLRedIndianRedColor;
			else if (style_value == "HTMLRedCrimson")  _color = HTMLRedCrimsonColor;
			else if (style_value == "HTMLRedFirebrick")  _color = HTMLRedFirebrickColor;
			else if (style_value == "HTMLRedDarkRed")  _color = HTMLRedDarkRedColor;
			else if (style_value == "HTMLRedRed")  _color = HTMLRedRedColor;
			else if (style_value == "HTMLOrangeOrangeRed")  _color = HTMLOrangeOrangeRedColor;
			else if (style_value == "HTMLOrangeTomato")  _color = HTMLOrangeTomatoColor;
			else if (style_value == "HTMLOrangeCoral")  _color = HTMLOrangeCoralColor;
			else if (style_value == "HTMLOrangeDarkOrange")  _color = HTMLOrangeDarkOrangeColor;
			else if (style_value == "HTMLOrangeOrange")  _color = HTMLOrangeOrangeColor;
			else if (style_value == "HTMLYellowYellow")  _color = HTMLYellowYellowColor;
			else if (style_value == "HTMLYellowLightYellow")  _color = HTMLYellowLightYellowColor;
			else if (style_value == "HTMLYellowLemonChiffon")  _color = HTMLYellowLemonChiffonColor;
			else if (style_value == "HTMLYellowLightGoldenrodYellow")  _color = HTMLYellowLightGoldenrodYellowColor;
			else if (style_value == "HTMLYellowPapayaWhip")  _color = HTMLYellowPapayaWhipColor;
			else if (style_value == "HTMLYellowMoccasin")  _color = HTMLYellowMoccasinColor;
			else if (style_value == "HTMLYellowPeachPuff")  _color = HTMLYellowPeachPuffColor;
			else if (style_value == "HTMLYellowPaleGoldenrod")  _color = HTMLYellowPaleGoldenrodColor;
			else if (style_value == "HTMLYellowKhaki")  _color = HTMLYellowKhakiColor;
			else if (style_value == "HTMLYellowDarkKhaki")  _color = HTMLYellowDarkKhakiColor;
			else if (style_value == "HTMLYellowGold")  _color = HTMLYellowGoldColor;
			else if (style_value == "HTMLBrownCornsilk")  _color = HTMLBrownCornsilkColor;
			else if (style_value == "HTMLBrownBlanchedAlmond")  _color = HTMLBrownBlanchedAlmondColor;
			else if (style_value == "HTMLBrownBisque")  _color = HTMLBrownBisqueColor;
			else if (style_value == "HTMLBrownNavajoWhite")  _color = HTMLBrownNavajoWhiteColor;
			else if (style_value == "HTMLBrownWheat")  _color = HTMLBrownWheatColor;
			else if (style_value == "HTMLBrownBurlywood")  _color = HTMLBrownBurlywoodColor;
			else if (style_value == "HTMLBrownTan")  _color = HTMLBrownTanColor;
			else if (style_value == "HTMLBrownRosyBrown")  _color = HTMLBrownRosyBrownColor;
			else if (style_value == "HTMLBrownSandyBrown")  _color = HTMLBrownSandyBrownColor;
			else if (style_value == "HTMLBrownGoldenrod")  _color = HTMLBrownGoldenrodColor;
			else if (style_value == "HTMLBrownDarkGoldenrod")  _color = HTMLBrownDarkGoldenrodColor;
			else if (style_value == "HTMLBrownPeru")  _color = HTMLBrownPeruColor;
			else if (style_value == "HTMLBrownChocolate")  _color = HTMLBrownChocolateColor;
			else if (style_value == "HTMLBrownSaddleBrown")  _color = HTMLBrownSaddleBrownColor;
			else if (style_value == "HTMLBrownSienna")  _color = HTMLBrownSiennaColor;
			else if (style_value == "HTMLBrownBrown")  _color = HTMLBrownBrownColor;
			else if (style_value == "HTMLBrownMaroon")  _color = HTMLBrownMaroonColor;
			else if (style_value == "HTMLGreenDarkOliveGreen")  _color = HTMLGreenDarkOliveGreenColor;
			else if (style_value == "HTMLGreenOlive")  _color = HTMLGreenOliveColor;
			else if (style_value == "HTMLGreenOliveDrab")  _color = HTMLGreenOliveDrabColor;
			else if (style_value == "HTMLGreenYellowGreen")  _color = HTMLGreenYellowGreenColor;
			else if (style_value == "HTMLGreenLimeGreen")  _color = HTMLGreenLimeGreenColor;
			else if (style_value == "HTMLGreenLime")  _color = HTMLGreenLimeColor;
			else if (style_value == "HTMLGreenLawnGreen")  _color = HTMLGreenLawnGreenColor;
			else if (style_value == "HTMLGreenChartreuse")  _color = HTMLGreenChartreuseColor;
			else if (style_value == "HTMLGreenGreenYellow")  _color = HTMLGreenGreenYellowColor;
			else if (style_value == "HTMLGreenSpringGreen")  _color = HTMLGreenSpringGreenColor;
			else if (style_value == "HTMLGreenMediumSpringGreen")  _color = HTMLGreenMediumSpringGreenColor;
			else if (style_value == "HTMLGreenLightGreen")  _color = HTMLGreenLightGreenColor;
			else if (style_value == "HTMLGreenPaleGreen")  _color = HTMLGreenPaleGreenColor;
			else if (style_value == "HTMLGreenDarkSeaGreen")  _color = HTMLGreenDarkSeaGreenColor;
			else if (style_value == "HTMLGreenMediumAquamarine")  _color = HTMLGreenMediumAquamarineColor;
			else if (style_value == "HTMLGreenMediumSeaGreen")  _color = HTMLGreenMediumSeaGreenColor;
			else if (style_value == "HTMLGreenSeaGreen")  _color = HTMLGreenSeaGreenColor;
			else if (style_value == "HTMLGreenForestGreen")  _color = HTMLGreenForestGreenColor;
			else if (style_value == "HTMLGreenGreen")  _color = HTMLGreenGreenColor;
			else if (style_value == "HTMLGreenDarkGreen")  _color = HTMLGreenDarkGreenColor;
			else if (style_value == "HTMLCyanAqua")  _color = HTMLCyanAquaColor;
			else if (style_value == "HTMLCyanCyan")  _color = HTMLCyanCyanColor;
			else if (style_value == "HTMLCyanLightCyan")  _color = HTMLCyanLightCyanColor;
			else if (style_value == "HTMLCyanPaleTurquoise")  _color = HTMLCyanPaleTurquoiseColor;
			else if (style_value == "HTMLCyanAquamarine")  _color = HTMLCyanAquamarineColor;
			else if (style_value == "HTMLCyanTurquoise")  _color = HTMLCyanTurquoiseColor;
			else if (style_value == "HTMLCyanMediumTurquoise")  _color = HTMLCyanMediumTurquoiseColor;
			else if (style_value == "HTMLCyanDarkTurquoise")  _color = HTMLCyanDarkTurquoiseColor;
			else if (style_value == "HTMLCyanLightSeaGreen")  _color = HTMLCyanLightSeaGreenColor;
			else if (style_value == "HTMLCyanCadetBlue")  _color = HTMLCyanCadetBlueColor;
			else if (style_value == "HTMLCyanDarkCyan")  _color = HTMLCyanDarkCyanColor;
			else if (style_value == "HTMLCyanTeal")  _color = HTMLCyanTealColor;
			else if (style_value == "HTMLBlueLightSteelBlue")  _color = HTMLBlueLightSteelBlueColor;
			else if (style_value == "HTMLBluePowderBlue")  _color = HTMLBluePowderBlueColor;
			else if (style_value == "HTMLBlueLightBlue")  _color = HTMLBlueLightBlueColor;
			else if (style_value == "HTMLBlueSkyBlue")  _color = HTMLBlueSkyBlueColor;
			else if (style_value == "HTMLBlueLightSkyBlue")  _color = HTMLBlueLightSkyBlueColor;
			else if (style_value == "HTMLBlueDeepSkyBlue")  _color = HTMLBlueDeepSkyBlueColor;
			else if (style_value == "HTMLBlueDodgerBlue")  _color = HTMLBlueDodgerBlueColor;
			else if (style_value == "HTMLBlueCornflowerBlue")  _color = HTMLBlueCornflowerBlueColor;
			else if (style_value == "HTMLBlueSteelBlue")  _color = HTMLBlueSteelBlueColor;
			else if (style_value == "HTMLBlueRoyalBlue")  _color = HTMLBlueRoyalBlueColor;
			else if (style_value == "HTMLBlueBlue")  _color = HTMLBlueBlueColor;
			else if (style_value == "HTMLBlueMediumBlue")  _color = HTMLBlueMediumBlueColor;
			else if (style_value == "HTMLBlueDarkBlue")  _color = HTMLBlueDarkBlueColor;
			else if (style_value == "HTMLBlueNavy")  _color = HTMLBlueNavyColor;
			else if (style_value == "HTMLBlueMidnightBlue")  _color = HTMLBlueMidnightBlueColor;
			else if (style_value == "HTMLPurpleLavender")  _color = HTMLPurpleLavenderColor;
			else if (style_value == "HTMLPurpleThistle")  _color = HTMLPurpleThistleColor;
			else if (style_value == "HTMLPurplePlum")  _color = HTMLPurplePlumColor;
			else if (style_value == "HTMLPurpleViolet")  _color = HTMLPurpleVioletColor;
			else if (style_value == "HTMLPurpleOrchid")  _color = HTMLPurpleOrchidColor;
			else if (style_value == "HTMLPurpleFuchsia")  _color = HTMLPurpleFuchsiaColor;
			else if (style_value == "HTMLPurpleMagenta")  _color = HTMLPurpleMagentaColor;
			else if (style_value == "HTMLPurpleMediumOrchid")  _color = HTMLPurpleMediumOrchidColor;
			else if (style_value == "HTMLPurpleMediumPurple")  _color = HTMLPurpleMediumPurpleColor;
			else if (style_value == "HTMLPurpleBlueViolet")  _color = HTMLPurpleBlueVioletColor;
			else if (style_value == "HTMLPurpleDarkViolet")  _color = HTMLPurpleDarkVioletColor;
			else if (style_value == "HTMLPurpleDarkOrchid")  _color = HTMLPurpleDarkOrchidColor;
			else if (style_value == "HTMLPurpleDarkMagenta")  _color = HTMLPurpleDarkMagentaColor;
			else if (style_value == "HTMLPurplePurple")  _color = HTMLPurplePurpleColor;
			else if (style_value == "HTMLPurpleIndigo")  _color = HTMLPurpleIndigoColor;
			else if (style_value == "HTMLPurpleDarkSlateBlue")  _color = HTMLPurpleDarkSlateBlueColor;
			else if (style_value == "HTMLPurpleSlateBlue")  _color = HTMLPurpleSlateBlueColor;
			else if (style_value == "HTMLPurpleMediumSlateBlue")  _color = HTMLPurpleMediumSlateBlueColor;
			else if (style_value == "HTMLWhiteWhite")  _color = HTMLWhiteWhiteColor;
			else if (style_value == "HTMLWhiteSnow")  _color = HTMLWhiteSnowColor;
			else if (style_value == "HTMLWhiteHoneydew")  _color = HTMLWhiteHoneydewColor;
			else if (style_value == "HTMLWhiteMintCream")  _color = HTMLWhiteMintCreamColor;
			else if (style_value == "HTMLWhiteAzure")  _color = HTMLWhiteAzureColor;
			else if (style_value == "HTMLWhiteAliceBlue")  _color = HTMLWhiteAliceBlueColor;
			else if (style_value == "HTMLWhiteGhostWhite")  _color = HTMLWhiteGhostWhiteColor;
			else if (style_value == "HTMLWhiteWhiteSmoke")  _color = HTMLWhiteWhiteSmokeColor;
			else if (style_value == "HTMLWhiteSeashell")  _color = HTMLWhiteSeashellColor;
			else if (style_value == "HTMLWhiteBeige")  _color = HTMLWhiteBeigeColor;
			else if (style_value == "HTMLWhiteOldLace")  _color = HTMLWhiteOldLaceColor;
			else if (style_value == "HTMLWhiteFloralWhite")  _color = HTMLWhiteFloralWhiteColor;
			else if (style_value == "HTMLWhiteIvory")  _color = HTMLWhiteIvoryColor;
			else if (style_value == "HTMLWhiteAntiqueWhite")  _color = HTMLWhiteAntiqueWhiteColor;
			else if (style_value == "HTMLWhiteLinen")  _color = HTMLWhiteLinenColor;
			else if (style_value == "HTMLWhiteLavenderBlush")  _color = HTMLWhiteLavenderBlushColor;
			else if (style_value == "HTMLWhiteMistyRose")  _color = HTMLWhiteMistyRoseColor;
			else if (style_value == "HTMLGrayGainsboro")  _color = HTMLGrayGainsboroColor;
			else if (style_value == "HTMLGrayLightGray")  _color = HTMLGrayLightGrayColor;
			else if (style_value == "HTMLGraySilver")  _color = HTMLGraySilverColor;
			else if (style_value == "HTMLGrayDarkGray")  _color = HTMLGrayDarkGrayColor;
			else if (style_value == "HTMLGrayGray")  _color = HTMLGrayGrayColor;
			else if (style_value == "HTMLGrayDimGray")  _color = HTMLGrayDimGrayColor;
			else if (style_value == "HTMLGrayLightSlateGray")  _color = HTMLGrayLightSlateGrayColor;
			else if (style_value == "HTMLGraySlateGray")  _color = HTMLGraySlateGrayColor;
			else if (style_value == "HTMLGrayDarkSlateGray")  _color = HTMLGrayDarkSlateGrayColor;
			else if (style_value == "HTMLGrayBlack")  _color = HTMLGrayBlackColor;
			else if (style_value == "none")  _color = NoneColor;
		}
	}
		//Get antialiasing
	_antialiased = qde.attribute("antialias") == "true";
}


/**
	@brief CustomElementGraphicPart::resetStyles
	Reset the curent style to default,
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
		brush.setStyle(Qt::SolidPattern);
		if (_filling == BlackFilling)      brush.setColor(Qt::black);
		else if (_filling == WhiteFilling) brush.setColor(Qt::white);
		else if (_filling == GreenFilling) brush.setColor(Qt::green);
		else if (_filling == RedFilling)   brush.setColor(Qt::red);
		else if (_filling == BlueFilling)  brush.setColor(Qt::blue);
		else if (_filling == GrayFilling)  brush.setColor(Qt::gray);
		else if (_filling == BrunFilling)  brush.setColor(QColor(97, 44, 0));
		else if (_filling == YellowFilling)  brush.setColor(Qt::yellow);
		else if (_filling == CyanFilling)  brush.setColor(Qt::cyan);
		else if (_filling == MagentaFilling)  brush.setColor(Qt::magenta);
		else if (_filling == LightgrayFilling)  brush.setColor(Qt::lightGray);
		else if (_filling == OrangeFilling)  brush.setColor(QColor(255, 128, 0));
		else if (_filling == PurpleFilling)  brush.setColor(QColor(136, 28, 168));
		else if (_filling == HTMLPinkPinkFilling)  brush.setColor(QColor(255, 192, 203));
		else if (_filling == HTMLPinkLightPinkFilling)  brush.setColor(QColor(255, 182, 193));
		else if (_filling == HTMLPinkHotPinkFilling)  brush.setColor(QColor(255, 105, 180));
		else if (_filling == HTMLPinkDeepPinkFilling)  brush.setColor(QColor(255, 20, 147));
		else if (_filling == HTMLPinkPaleVioletRedFilling)  brush.setColor(QColor(219, 112, 147));
		else if (_filling == HTMLPinkMediumVioletRedFilling)  brush.setColor(QColor(199, 21, 133));
		else if (_filling == HTMLRedLightSalmonFilling)  brush.setColor(QColor(255, 160, 122));
		else if (_filling == HTMLRedSalmonFilling)  brush.setColor(QColor(250, 128, 114));
		else if (_filling == HTMLRedDarkSalmonFilling)  brush.setColor(QColor(233, 150, 122));
		else if (_filling == HTMLRedLightCoralFilling)  brush.setColor(QColor(240, 128, 128));
		else if (_filling == HTMLRedIndianRedFilling)  brush.setColor(QColor(205, 92, 92));
		else if (_filling == HTMLRedCrimsonFilling)  brush.setColor(QColor(220, 20, 60));
		else if (_filling == HTMLRedFirebrickFilling)  brush.setColor(QColor(178, 34, 34));
		else if (_filling == HTMLRedDarkRedFilling)  brush.setColor(QColor(139, 0, 0));
		else if (_filling == HTMLRedRedFilling)  brush.setColor(QColor(255, 0, 0));
		else if (_filling == HTMLOrangeOrangeRedFilling)  brush.setColor(QColor(255, 69, 0));
		else if (_filling == HTMLOrangeTomatoFilling)  brush.setColor(QColor(255, 99, 71));
		else if (_filling == HTMLOrangeCoralFilling)  brush.setColor(QColor(255, 127, 80));
		else if (_filling == HTMLOrangeDarkOrangeFilling)  brush.setColor(QColor(255, 140, 0));
		else if (_filling == HTMLOrangeOrangeFilling)  brush.setColor(QColor(255, 165, 0));
		else if (_filling == HTMLYellowYellowFilling)  brush.setColor(QColor(255, 255, 0));
		else if (_filling == HTMLYellowLightYellowFilling)  brush.setColor(QColor(255, 255, 224));
		else if (_filling == HTMLYellowLemonChiffonFilling)  brush.setColor(QColor(255, 250, 205));
		else if (_filling == HTMLYellowLightGoldenrodYellowFilling)  brush.setColor(QColor(250, 250, 210));
		else if (_filling == HTMLYellowPapayaWhipFilling)  brush.setColor(QColor(255, 239, 213));
		else if (_filling == HTMLYellowMoccasinFilling)  brush.setColor(QColor(255, 228, 181));
		else if (_filling == HTMLYellowPeachPuffFilling)  brush.setColor(QColor(255, 218, 185));
		else if (_filling == HTMLYellowPaleGoldenrodFilling)  brush.setColor(QColor(238, 232, 170));
		else if (_filling == HTMLYellowKhakiFilling)  brush.setColor(QColor(240, 230, 140));
		else if (_filling == HTMLYellowDarkKhakiFilling)  brush.setColor(QColor(189, 183, 107));
		else if (_filling == HTMLYellowGoldFilling)  brush.setColor(QColor(255, 215, 0));
		else if (_filling == HTMLBrownCornsilkFilling)  brush.setColor(QColor(255, 248, 220));
		else if (_filling == HTMLBrownBlanchedAlmondFilling)  brush.setColor(QColor(255, 235, 205));
		else if (_filling == HTMLBrownBisqueFilling)  brush.setColor(QColor(255, 228, 196));
		else if (_filling == HTMLBrownNavajoWhiteFilling)  brush.setColor(QColor(255, 222, 173));
		else if (_filling == HTMLBrownWheatFilling)  brush.setColor(QColor(245, 222, 179));
		else if (_filling == HTMLBrownBurlywoodFilling)  brush.setColor(QColor(222, 184, 135));
		else if (_filling == HTMLBrownTanFilling)  brush.setColor(QColor(210, 180, 140));
		else if (_filling == HTMLBrownRosyBrownFilling)  brush.setColor(QColor(188, 143, 143));
		else if (_filling == HTMLBrownSandyBrownFilling)  brush.setColor(QColor(244, 164, 96));
		else if (_filling == HTMLBrownGoldenrodFilling)  brush.setColor(QColor(218, 165, 32));
		else if (_filling == HTMLBrownDarkGoldenrodFilling)  brush.setColor(QColor(184, 134, 11));
		else if (_filling == HTMLBrownPeruFilling)  brush.setColor(QColor(205, 133, 63));
		else if (_filling == HTMLBrownChocolateFilling)  brush.setColor(QColor(210, 105, 30));
		else if (_filling == HTMLBrownSaddleBrownFilling)  brush.setColor(QColor(139, 69, 19));
		else if (_filling == HTMLBrownSiennaFilling)  brush.setColor(QColor(160, 82, 45));
		else if (_filling == HTMLBrownBrownFilling)  brush.setColor(QColor(165, 42, 42));
		else if (_filling == HTMLBrownMaroonFilling)  brush.setColor(QColor(128, 0, 0));
		else if (_filling == HTMLGreenDarkOliveGreenFilling)  brush.setColor(QColor(85, 107, 47));
		else if (_filling == HTMLGreenOliveFilling)  brush.setColor(QColor(128, 128, 0));
		else if (_filling == HTMLGreenOliveDrabFilling)  brush.setColor(QColor(107, 142, 35));
		else if (_filling == HTMLGreenYellowGreenFilling)  brush.setColor(QColor(154, 205, 50));
		else if (_filling == HTMLGreenLimeGreenFilling)  brush.setColor(QColor(50, 205, 50));
		else if (_filling == HTMLGreenLimeFilling)  brush.setColor(QColor(0, 255, 0));
		else if (_filling == HTMLGreenLawnGreenFilling)  brush.setColor(QColor(124, 252, 0));
		else if (_filling == HTMLGreenChartreuseFilling)  brush.setColor(QColor(127, 255, 0));
		else if (_filling == HTMLGreenGreenYellowFilling)  brush.setColor(QColor(173, 255, 47));
		else if (_filling == HTMLGreenSpringGreenFilling)  brush.setColor(QColor(0, 255, 127));
		else if (_filling == HTMLGreenMediumSpringGreenFilling)  brush.setColor(QColor(0, 250, 154));
		else if (_filling == HTMLGreenLightGreenFilling)  brush.setColor(QColor(144, 238, 144));
		else if (_filling == HTMLGreenPaleGreenFilling)  brush.setColor(QColor(152, 251, 152));
		else if (_filling == HTMLGreenDarkSeaGreenFilling)  brush.setColor(QColor(143, 188, 143));
		else if (_filling == HTMLGreenMediumAquamarineFilling)  brush.setColor(QColor(102, 205, 170));
		else if (_filling == HTMLGreenMediumSeaGreenFilling)  brush.setColor(QColor(60, 179, 113));
		else if (_filling == HTMLGreenSeaGreenFilling)  brush.setColor(QColor(46, 139, 87));
		else if (_filling == HTMLGreenForestGreenFilling)  brush.setColor(QColor(34, 139, 34));
		else if (_filling == HTMLGreenGreenFilling)  brush.setColor(QColor(0, 128, 0));
		else if (_filling == HTMLGreenDarkGreenFilling)  brush.setColor(QColor(0, 100, 0));
		else if (_filling == HTMLCyanAquaFilling)  brush.setColor(QColor(0, 255, 255));
		else if (_filling == HTMLCyanCyanFilling)  brush.setColor(QColor(0, 255, 255));
		else if (_filling == HTMLCyanLightCyanFilling)  brush.setColor(QColor(224, 255, 255));
		else if (_filling == HTMLCyanPaleTurquoiseFilling)  brush.setColor(QColor(175, 238, 238));
		else if (_filling == HTMLCyanAquamarineFilling)  brush.setColor(QColor(127, 255, 212));
		else if (_filling == HTMLCyanTurquoiseFilling)  brush.setColor(QColor(64, 224, 208));
		else if (_filling == HTMLCyanMediumTurquoiseFilling)  brush.setColor(QColor(72, 209, 204));
		else if (_filling == HTMLCyanDarkTurquoiseFilling)  brush.setColor(QColor(0, 206, 209));
		else if (_filling == HTMLCyanLightSeaGreenFilling)  brush.setColor(QColor(32, 178, 170));
		else if (_filling == HTMLCyanCadetBlueFilling)  brush.setColor(QColor(95, 158, 160));
		else if (_filling == HTMLCyanDarkCyanFilling)  brush.setColor(QColor(0, 139, 139));
		else if (_filling == HTMLCyanTealFilling)  brush.setColor(QColor(0, 128, 128));
		else if (_filling == HTMLBlueLightSteelBlueFilling)  brush.setColor(QColor(176, 196, 222));
		else if (_filling == HTMLBluePowderBlueFilling)  brush.setColor(QColor(176, 224, 230));
		else if (_filling == HTMLBlueLightBlueFilling)  brush.setColor(QColor(173, 216, 230));
		else if (_filling == HTMLBlueSkyBlueFilling)  brush.setColor(QColor(135, 206, 235));
		else if (_filling == HTMLBlueLightSkyBlueFilling)  brush.setColor(QColor(135, 206, 250));
		else if (_filling == HTMLBlueDeepSkyBlueFilling)  brush.setColor(QColor(0, 191, 255));
		else if (_filling == HTMLBlueDodgerBlueFilling)  brush.setColor(QColor(30, 144, 255));
		else if (_filling == HTMLBlueCornflowerBlueFilling)  brush.setColor(QColor(100, 149, 237));
		else if (_filling == HTMLBlueSteelBlueFilling)  brush.setColor(QColor(70, 130, 180));
		else if (_filling == HTMLBlueRoyalBlueFilling)  brush.setColor(QColor(65, 105, 225));
		else if (_filling == HTMLBlueBlueFilling)  brush.setColor(QColor(0, 0, 255));
		else if (_filling == HTMLBlueMediumBlueFilling)  brush.setColor(QColor(0, 0, 205));
		else if (_filling == HTMLBlueDarkBlueFilling)  brush.setColor(QColor(0, 0, 139));
		else if (_filling == HTMLBlueNavyFilling)  brush.setColor(QColor(0, 0, 128));
		else if (_filling == HTMLBlueMidnightBlueFilling)  brush.setColor(QColor(25, 25, 112));
		else if (_filling == HTMLPurpleLavenderFilling)  brush.setColor(QColor(230, 230, 250));
		else if (_filling == HTMLPurpleThistleFilling)  brush.setColor(QColor(216, 191, 216));
		else if (_filling == HTMLPurplePlumFilling)  brush.setColor(QColor(221, 160, 221));
		else if (_filling == HTMLPurpleVioletFilling)  brush.setColor(QColor(238, 130, 238));
		else if (_filling == HTMLPurpleOrchidFilling)  brush.setColor(QColor(218, 112, 214));
		else if (_filling == HTMLPurpleFuchsiaFilling)  brush.setColor(QColor(255, 0, 255));
		else if (_filling == HTMLPurpleMagentaFilling)  brush.setColor(QColor(255, 0, 255));
		else if (_filling == HTMLPurpleMediumOrchidFilling)  brush.setColor(QColor(186, 85, 211));
		else if (_filling == HTMLPurpleMediumPurpleFilling)  brush.setColor(QColor(147, 112, 219));
		else if (_filling == HTMLPurpleBlueVioletFilling)  brush.setColor(QColor(138, 43, 226));
		else if (_filling == HTMLPurpleDarkVioletFilling)  brush.setColor(QColor(148, 0, 211));
		else if (_filling == HTMLPurpleDarkOrchidFilling)  brush.setColor(QColor(153, 50, 204));
		else if (_filling == HTMLPurpleDarkMagentaFilling)  brush.setColor(QColor(139, 0, 139));
		else if (_filling == HTMLPurplePurpleFilling)  brush.setColor(QColor(128, 0, 128));
		else if (_filling == HTMLPurpleIndigoFilling)  brush.setColor(QColor(75, 0, 130));
		else if (_filling == HTMLPurpleDarkSlateBlueFilling)  brush.setColor(QColor(72, 61, 139));
		else if (_filling == HTMLPurpleSlateBlueFilling)  brush.setColor(QColor(106, 90, 205));
		else if (_filling == HTMLPurpleMediumSlateBlueFilling)  brush.setColor(QColor(123, 104, 238));
		else if (_filling == HTMLWhiteWhiteFilling)  brush.setColor(QColor(255, 255, 255));
		else if (_filling == HTMLWhiteSnowFilling)  brush.setColor(QColor(255, 250, 250));
		else if (_filling == HTMLWhiteHoneydewFilling)  brush.setColor(QColor(240, 255, 240));
		else if (_filling == HTMLWhiteMintCreamFilling)  brush.setColor(QColor(245, 255, 250));
		else if (_filling == HTMLWhiteAzureFilling)  brush.setColor(QColor(240, 255, 255));
		else if (_filling == HTMLWhiteAliceBlueFilling)  brush.setColor(QColor(240, 248, 255));
		else if (_filling == HTMLWhiteGhostWhiteFilling)  brush.setColor(QColor(248, 248, 255));
		else if (_filling == HTMLWhiteWhiteSmokeFilling)  brush.setColor(QColor(245, 245, 245));
		else if (_filling == HTMLWhiteSeashellFilling)  brush.setColor(QColor(255, 245, 238));
		else if (_filling == HTMLWhiteBeigeFilling)  brush.setColor(QColor(245, 245, 220));
		else if (_filling == HTMLWhiteOldLaceFilling)  brush.setColor(QColor(253, 245, 230));
		else if (_filling == HTMLWhiteFloralWhiteFilling)  brush.setColor(QColor(255, 250, 240));
		else if (_filling == HTMLWhiteIvoryFilling)  brush.setColor(QColor(255, 255, 240));
		else if (_filling == HTMLWhiteAntiqueWhiteFilling)  brush.setColor(QColor(250, 235, 215));
		else if (_filling == HTMLWhiteLinenFilling)  brush.setColor(QColor(250, 240, 230));
		else if (_filling == HTMLWhiteLavenderBlushFilling)  brush.setColor(QColor(255, 240, 245));
		else if (_filling == HTMLWhiteMistyRoseFilling)  brush.setColor(QColor(255, 228, 225));
		else if (_filling == HTMLGrayGainsboroFilling)  brush.setColor(QColor(220, 220, 220));
		else if (_filling == HTMLGrayLightGrayFilling)  brush.setColor(QColor(211, 211, 211));
		else if (_filling == HTMLGraySilverFilling)  brush.setColor(QColor(192, 192, 192));
		else if (_filling == HTMLGrayDarkGrayFilling)  brush.setColor(QColor(169, 169, 169));
		else if (_filling == HTMLGrayGrayFilling)  brush.setColor(QColor(128, 128, 128));
		else if (_filling == HTMLGrayDimGrayFilling)  brush.setColor(QColor(105, 105, 105));
		else if (_filling == HTMLGrayLightSlateGrayFilling)  brush.setColor(QColor(119, 136, 153));
		else if (_filling == HTMLGraySlateGrayFilling)  brush.setColor(QColor(112, 128, 144));
		else if (_filling == HTMLGrayDarkSlateGrayFilling)  brush.setColor(QColor(47, 79, 79));
		else if (_filling == HTMLGrayBlackFilling)  brush.setColor(QColor(0, 0, 0));
	}

		//Apply pen color
	if      (_color == WhiteColor) pen.setColor(QColor(255, 255, 255, pen.color().alpha()));
	else if (_color == BlackColor) pen.setColor(QColor(  0,   0,   0, pen.color().alpha()));
	else if (_color == GreenColor) pen.setColor(QColor(Qt::green));
	else if (_color == RedColor)   pen.setColor(QColor(Qt::red));
	else if (_color == BlueColor)  pen.setColor(QColor(Qt::blue));
	else if (_color == GrayColor)  pen.setColor(QColor(Qt::gray));
	else if (_color == BrunColor)  pen.setColor(QColor(97, 44, 0));
	else if (_color == YellowColor)  pen.setColor(QColor(Qt::yellow));
	else if (_color == CyanColor)  pen.setColor(Qt::cyan);
	else if (_color == MagentaColor)  pen.setColor(Qt::magenta);
	else if (_color == LightgrayColor)  pen.setColor(Qt::lightGray);
	else if (_color == OrangeColor)  pen.setColor(QColor(255, 128, 0));
	else if (_color == PurpleColor)  pen.setColor(QColor(136, 28, 168));
	else if (_color == HTMLPinkPinkColor)  pen.setColor(QColor(255, 192, 203));
	else if (_color == HTMLPinkLightPinkColor)  pen.setColor(QColor(255, 182, 193));
	else if (_color == HTMLPinkHotPinkColor)  pen.setColor(QColor(255, 105, 180));
	else if (_color == HTMLPinkDeepPinkColor)  pen.setColor(QColor(255, 20, 147));
	else if (_color == HTMLPinkPaleVioletRedColor)  pen.setColor(QColor(219, 112, 147));
	else if (_color == HTMLPinkMediumVioletRedColor)  pen.setColor(QColor(199, 21, 133));
	else if (_color == HTMLRedLightSalmonColor)  pen.setColor(QColor(255, 160, 122));
	else if (_color == HTMLRedSalmonColor)  pen.setColor(QColor(250, 128, 114));
	else if (_color == HTMLRedDarkSalmonColor)  pen.setColor(QColor(233, 150, 122));
	else if (_color == HTMLRedLightCoralColor)  pen.setColor(QColor(240, 128, 128));
	else if (_color == HTMLRedIndianRedColor)  pen.setColor(QColor(205, 92, 92));
	else if (_color == HTMLRedCrimsonColor)  pen.setColor(QColor(220, 20, 60));
	else if (_color == HTMLRedFirebrickColor)  pen.setColor(QColor(178, 34, 34));
	else if (_color == HTMLRedDarkRedColor)  pen.setColor(QColor(139, 0, 0));
	else if (_color == HTMLRedRedColor)  pen.setColor(QColor(255, 0, 0));
	else if (_color == HTMLOrangeOrangeRedColor)  pen.setColor(QColor(255, 69, 0));
	else if (_color == HTMLOrangeTomatoColor)  pen.setColor(QColor(255, 99, 71));
	else if (_color == HTMLOrangeCoralColor)  pen.setColor(QColor(255, 127, 80));
	else if (_color == HTMLOrangeDarkOrangeColor)  pen.setColor(QColor(255, 140, 0));
	else if (_color == HTMLOrangeOrangeColor)  pen.setColor(QColor(255, 165, 0));
	else if (_color == HTMLYellowYellowColor)  pen.setColor(QColor(255, 255, 0));
	else if (_color == HTMLYellowLightYellowColor)  pen.setColor(QColor(255, 255, 224));
	else if (_color == HTMLYellowLemonChiffonColor)  pen.setColor(QColor(255, 250, 205));
	else if (_color == HTMLYellowLightGoldenrodYellowColor)  pen.setColor(QColor(250, 250, 210));
	else if (_color == HTMLYellowPapayaWhipColor)  pen.setColor(QColor(255, 239, 213));
	else if (_color == HTMLYellowMoccasinColor)  pen.setColor(QColor(255, 228, 181));
	else if (_color == HTMLYellowPeachPuffColor)  pen.setColor(QColor(255, 218, 185));
	else if (_color == HTMLYellowPaleGoldenrodColor)  pen.setColor(QColor(238, 232, 170));
	else if (_color == HTMLYellowKhakiColor)  pen.setColor(QColor(240, 230, 140));
	else if (_color == HTMLYellowDarkKhakiColor)  pen.setColor(QColor(189, 183, 107));
	else if (_color == HTMLYellowGoldColor)  pen.setColor(QColor(255, 215, 0));
	else if (_color == HTMLBrownCornsilkColor)  pen.setColor(QColor(255, 248, 220));
	else if (_color == HTMLBrownBlanchedAlmondColor)  pen.setColor(QColor(255, 235, 205));
	else if (_color == HTMLBrownBisqueColor)  pen.setColor(QColor(255, 228, 196));
	else if (_color == HTMLBrownNavajoWhiteColor)  pen.setColor(QColor(255, 222, 173));
	else if (_color == HTMLBrownWheatColor)  pen.setColor(QColor(245, 222, 179));
	else if (_color == HTMLBrownBurlywoodColor)  pen.setColor(QColor(222, 184, 135));
	else if (_color == HTMLBrownTanColor)  pen.setColor(QColor(210, 180, 140));
	else if (_color == HTMLBrownRosyBrownColor)  pen.setColor(QColor(188, 143, 143));
	else if (_color == HTMLBrownSandyBrownColor)  pen.setColor(QColor(244, 164, 96));
	else if (_color == HTMLBrownGoldenrodColor)  pen.setColor(QColor(218, 165, 32));
	else if (_color == HTMLBrownDarkGoldenrodColor)  pen.setColor(QColor(184, 134, 11));
	else if (_color == HTMLBrownPeruColor)  pen.setColor(QColor(205, 133, 63));
	else if (_color == HTMLBrownChocolateColor)  pen.setColor(QColor(210, 105, 30));
	else if (_color == HTMLBrownSaddleBrownColor)  pen.setColor(QColor(139, 69, 19));
	else if (_color == HTMLBrownSiennaColor)  pen.setColor(QColor(160, 82, 45));
	else if (_color == HTMLBrownBrownColor)  pen.setColor(QColor(165, 42, 42));
	else if (_color == HTMLBrownMaroonColor)  pen.setColor(QColor(128, 0, 0));
	else if (_color == HTMLGreenDarkOliveGreenColor)  pen.setColor(QColor(85, 107, 47));
	else if (_color == HTMLGreenOliveColor)  pen.setColor(QColor(128, 128, 0));
	else if (_color == HTMLGreenOliveDrabColor)  pen.setColor(QColor(107, 142, 35));
	else if (_color == HTMLGreenYellowGreenColor)  pen.setColor(QColor(154, 205, 50));
	else if (_color == HTMLGreenLimeGreenColor)  pen.setColor(QColor(50, 205, 50));
	else if (_color == HTMLGreenLimeColor)  pen.setColor(QColor(0, 255, 0));
	else if (_color == HTMLGreenLawnGreenColor)  pen.setColor(QColor(124, 252, 0));
	else if (_color == HTMLGreenChartreuseColor)  pen.setColor(QColor(127, 255, 0));
	else if (_color == HTMLGreenGreenYellowColor)  pen.setColor(QColor(173, 255, 47));
	else if (_color == HTMLGreenSpringGreenColor)  pen.setColor(QColor(0, 255, 127));
	else if (_color == HTMLGreenMediumSpringGreenColor)  pen.setColor(QColor(0, 250, 154));
	else if (_color == HTMLGreenLightGreenColor)  pen.setColor(QColor(144, 238, 144));
	else if (_color == HTMLGreenPaleGreenColor)  pen.setColor(QColor(152, 251, 152));
	else if (_color == HTMLGreenDarkSeaGreenColor)  pen.setColor(QColor(143, 188, 143));
	else if (_color == HTMLGreenMediumAquamarineColor)  pen.setColor(QColor(102, 205, 170));
	else if (_color == HTMLGreenMediumSeaGreenColor)  pen.setColor(QColor(60, 179, 113));
	else if (_color == HTMLGreenSeaGreenColor)  pen.setColor(QColor(46, 139, 87));
	else if (_color == HTMLGreenForestGreenColor)  pen.setColor(QColor(34, 139, 34));
	else if (_color == HTMLGreenGreenColor)  pen.setColor(QColor(0, 128, 0));
	else if (_color == HTMLGreenDarkGreenColor)  pen.setColor(QColor(0, 100, 0));
	else if (_color == HTMLCyanAquaColor)  pen.setColor(QColor(0, 255, 255));
	else if (_color == HTMLCyanCyanColor)  pen.setColor(QColor(0, 255, 255));
	else if (_color == HTMLCyanLightCyanColor)  pen.setColor(QColor(224, 255, 255));
	else if (_color == HTMLCyanPaleTurquoiseColor)  pen.setColor(QColor(175, 238, 238));
	else if (_color == HTMLCyanAquamarineColor)  pen.setColor(QColor(127, 255, 212));
	else if (_color == HTMLCyanTurquoiseColor)  pen.setColor(QColor(64, 224, 208));
	else if (_color == HTMLCyanMediumTurquoiseColor)  pen.setColor(QColor(72, 209, 204));
	else if (_color == HTMLCyanDarkTurquoiseColor)  pen.setColor(QColor(0, 206, 209));
	else if (_color == HTMLCyanLightSeaGreenColor)  pen.setColor(QColor(32, 178, 170));
	else if (_color == HTMLCyanCadetBlueColor)  pen.setColor(QColor(95, 158, 160));
	else if (_color == HTMLCyanDarkCyanColor)  pen.setColor(QColor(0, 139, 139));
	else if (_color == HTMLCyanTealColor)  pen.setColor(QColor(0, 128, 128));
	else if (_color == HTMLBlueLightSteelBlueColor)  pen.setColor(QColor(176, 196, 222));
	else if (_color == HTMLBluePowderBlueColor)  pen.setColor(QColor(176, 224, 230));
	else if (_color == HTMLBlueLightBlueColor)  pen.setColor(QColor(173, 216, 230));
	else if (_color == HTMLBlueSkyBlueColor)  pen.setColor(QColor(135, 206, 235));
	else if (_color == HTMLBlueLightSkyBlueColor)  pen.setColor(QColor(135, 206, 250));
	else if (_color == HTMLBlueDeepSkyBlueColor)  pen.setColor(QColor(0, 191, 255));
	else if (_color == HTMLBlueDodgerBlueColor)  pen.setColor(QColor(30, 144, 255));
	else if (_color == HTMLBlueCornflowerBlueColor)  pen.setColor(QColor(100, 149, 237));
	else if (_color == HTMLBlueSteelBlueColor)  pen.setColor(QColor(70, 130, 180));
	else if (_color == HTMLBlueRoyalBlueColor)  pen.setColor(QColor(65, 105, 225));
	else if (_color == HTMLBlueBlueColor)  pen.setColor(QColor(0, 0, 255));
	else if (_color == HTMLBlueMediumBlueColor)  pen.setColor(QColor(0, 0, 205));
	else if (_color == HTMLBlueDarkBlueColor)  pen.setColor(QColor(0, 0, 139));
	else if (_color == HTMLBlueNavyColor)  pen.setColor(QColor(0, 0, 128));
	else if (_color == HTMLBlueMidnightBlueColor)  pen.setColor(QColor(25, 25, 112));
	else if (_color == HTMLPurpleLavenderColor)  pen.setColor(QColor(230, 230, 250));
	else if (_color == HTMLPurpleThistleColor)  pen.setColor(QColor(216, 191, 216));
	else if (_color == HTMLPurplePlumColor)  pen.setColor(QColor(221, 160, 221));
	else if (_color == HTMLPurpleVioletColor)  pen.setColor(QColor(238, 130, 238));
	else if (_color == HTMLPurpleOrchidColor)  pen.setColor(QColor(218, 112, 214));
	else if (_color == HTMLPurpleFuchsiaColor)  pen.setColor(QColor(255, 0, 255));
	else if (_color == HTMLPurpleMagentaColor)  pen.setColor(QColor(255, 0, 255));
	else if (_color == HTMLPurpleMediumOrchidColor)  pen.setColor(QColor(186, 85, 211));
	else if (_color == HTMLPurpleMediumPurpleColor)  pen.setColor(QColor(147, 112, 219));
	else if (_color == HTMLPurpleBlueVioletColor)  pen.setColor(QColor(138, 43, 226));
	else if (_color == HTMLPurpleDarkVioletColor)  pen.setColor(QColor(148, 0, 211));
	else if (_color == HTMLPurpleDarkOrchidColor)  pen.setColor(QColor(153, 50, 204));
	else if (_color == HTMLPurpleDarkMagentaColor)  pen.setColor(QColor(139, 0, 139));
	else if (_color == HTMLPurplePurpleColor)  pen.setColor(QColor(128, 0, 128));
	else if (_color == HTMLPurpleIndigoColor)  pen.setColor(QColor(75, 0, 130));
	else if (_color == HTMLPurpleDarkSlateBlueColor)  pen.setColor(QColor(72, 61, 139));
	else if (_color == HTMLPurpleSlateBlueColor)  pen.setColor(QColor(106, 90, 205));
	else if (_color == HTMLPurpleMediumSlateBlueColor)  pen.setColor(QColor(123, 104, 238));
	else if (_color == HTMLWhiteWhiteColor)  pen.setColor(QColor(255, 255, 255));
	else if (_color == HTMLWhiteSnowColor)  pen.setColor(QColor(255, 250, 250));
	else if (_color == HTMLWhiteHoneydewColor)  pen.setColor(QColor(240, 255, 240));
	else if (_color == HTMLWhiteMintCreamColor)  pen.setColor(QColor(245, 255, 250));
	else if (_color == HTMLWhiteAzureColor)  pen.setColor(QColor(240, 255, 255));
	else if (_color == HTMLWhiteAliceBlueColor)  pen.setColor(QColor(240, 248, 255));
	else if (_color == HTMLWhiteGhostWhiteColor)  pen.setColor(QColor(248, 248, 255));
	else if (_color == HTMLWhiteWhiteSmokeColor)  pen.setColor(QColor(245, 245, 245));
	else if (_color == HTMLWhiteSeashellColor)  pen.setColor(QColor(255, 245, 238));
	else if (_color == HTMLWhiteBeigeColor)  pen.setColor(QColor(245, 245, 220));
	else if (_color == HTMLWhiteOldLaceColor)  pen.setColor(QColor(253, 245, 230));
	else if (_color == HTMLWhiteFloralWhiteColor)  pen.setColor(QColor(255, 250, 240));
	else if (_color == HTMLWhiteIvoryColor)  pen.setColor(QColor(255, 255, 240));
	else if (_color == HTMLWhiteAntiqueWhiteColor)  pen.setColor(QColor(250, 235, 215));
	else if (_color == HTMLWhiteLinenColor)  pen.setColor(QColor(250, 240, 230));
	else if (_color == HTMLWhiteLavenderBlushColor)  pen.setColor(QColor(255, 240, 245));
	else if (_color == HTMLWhiteMistyRoseColor)  pen.setColor(QColor(255, 228, 225));
	else if (_color == HTMLGrayGainsboroColor)  pen.setColor(QColor(220, 220, 220));
	else if (_color == HTMLGrayLightGrayColor)  pen.setColor(QColor(211, 211, 211));
	else if (_color == HTMLGraySilverColor)  pen.setColor(QColor(192, 192, 192));
	else if (_color == HTMLGrayDarkGrayColor)  pen.setColor(QColor(169, 169, 169));
	else if (_color == HTMLGrayGrayColor)  pen.setColor(QColor(128, 128, 128));
	else if (_color == HTMLGrayDimGrayColor)  pen.setColor(QColor(105, 105, 105));
	else if (_color == HTMLGrayLightSlateGrayColor)  pen.setColor(QColor(119, 136, 153));
	else if (_color == HTMLGraySlateGrayColor)  pen.setColor(QColor(112, 128, 144));
	else if (_color == HTMLGrayDarkSlateGrayColor)  pen.setColor(QColor(47, 79, 79));
	else if (_color == HTMLGrayBlackColor)  pen.setColor(QColor(0, 0, 0));
	else if (_color == NoneColor)  pen.setBrush(Qt::transparent);

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
	@brief CustomElementGraphicPart::itemChange
	Reimplemented from QGraphicsObject.
	If the item position change call updateCurrentPartEditor()
	the change is always send to QGraphicsObject
	@param change
	@param value
	@return the returned value of QGraphicsObject::itemChange
*/
QVariant CustomElementGraphicPart::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (scene())
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemPositionHasChanged)
			updateCurrentPartEditor();

	return(QGraphicsObject::itemChange(change, value));
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
	if(event->button() == Qt::LeftButton)
		m_origin_pos = this->pos();

	QGraphicsObject::mousePressEvent(event);
}

void CustomElementGraphicPart::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
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
