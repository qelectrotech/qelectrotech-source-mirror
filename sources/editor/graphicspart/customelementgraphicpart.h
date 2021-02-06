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
#ifndef CUSTOM_ELEMENT_GRAPHIC_PART_H
#define CUSTOM_ELEMENT_GRAPHIC_PART_H

#include <QGraphicsObject>
#include "customelementpart.h"

class QETElementEditor;
class QPainter;


/**
	@brief The CustomElementGraphicPart class
	This class is the base for all home-made primitive like line,
	rectangle, ellipse etc....
	It provides methods and enums to manage style attributes available
	for primitive (color, pen style, etc...)
*/
class CustomElementGraphicPart : public QGraphicsObject, public CustomElementPart
{
		#define SHADOWS_HEIGHT 4.0

		Q_OBJECT

		Q_PROPERTY(LineStyle line_style   READ lineStyle   WRITE setLineStyle)
		Q_PROPERTY(LineWeight line_weight READ lineWeight  WRITE setLineWeight)
		Q_PROPERTY(Filling filling        READ filling     WRITE setFilling)
		Q_PROPERTY(Color color            READ color       WRITE setColor)
		Q_PROPERTY(bool antialias         READ antialiased WRITE setAntialiased)

	public:
			//Line style
		enum LineStyle {NormalStyle,
				DashedStyle,
				DottedStyle,
				DashdottedStyle};
		Q_ENUM (LineStyle)

			//Line weight : invisible, 0px, 1px, 2px, 5px
		enum LineWeight {NoneWeight,
				 ThinWeight,
				 NormalWeight,
				 UltraWeight,
				 BigWeight};
		Q_ENUM (LineWeight)

		//Filling color of the part : NoneFilling -> No filling (i.e. transparent)
		enum Filling { NoneFilling, BlackFilling, WhiteFilling,
			       GreenFilling, RedFilling, BlueFilling,
			       GrayFilling, BrunFilling, YellowFilling,
			       CyanFilling, MagentaFilling, LightgrayFilling,
			       OrangeFilling, PurpleFilling,
			       HTMLPinkPinkFilling, HTMLPinkLightPinkFilling,
			       HTMLPinkHotPinkFilling, HTMLPinkDeepPinkFilling,
			       HTMLPinkPaleVioletRedFilling,
			       HTMLPinkMediumVioletRedFilling,
			       HTMLRedLightSalmonFilling, HTMLRedSalmonFilling,
			       HTMLRedDarkSalmonFilling,
			       HTMLRedLightCoralFilling,
			       HTMLRedIndianRedFilling, HTMLRedCrimsonFilling,
			       HTMLRedFirebrickFilling, HTMLRedDarkRedFilling,
			       HTMLRedRedFilling, HTMLOrangeOrangeRedFilling,
			       HTMLOrangeTomatoFilling, HTMLOrangeCoralFilling,
			       HTMLOrangeDarkOrangeFilling,
			       HTMLOrangeOrangeFilling, HTMLYellowYellowFilling,
			       HTMLYellowLightYellowFilling,
			       HTMLYellowLemonChiffonFilling,
			       HTMLYellowLightGoldenrodYellowFilling,
			       HTMLYellowPapayaWhipFilling,
			       HTMLYellowMoccasinFilling,
			       HTMLYellowPeachPuffFilling,
			       HTMLYellowPaleGoldenrodFilling,
			       HTMLYellowKhakiFilling,
			       HTMLYellowDarkKhakiFilling,
			       HTMLYellowGoldFilling, HTMLBrownCornsilkFilling,
			       HTMLBrownBlanchedAlmondFilling,
			       HTMLBrownBisqueFilling,
			       HTMLBrownNavajoWhiteFilling,
			       HTMLBrownWheatFilling, HTMLBrownBurlywoodFilling,
			       HTMLBrownTanFilling, HTMLBrownRosyBrownFilling,
			       HTMLBrownSandyBrownFilling,
			       HTMLBrownGoldenrodFilling,
			       HTMLBrownDarkGoldenrodFilling,
			       HTMLBrownPeruFilling, HTMLBrownChocolateFilling,
			       HTMLBrownSaddleBrownFilling,
			       HTMLBrownSiennaFilling, HTMLBrownBrownFilling,
			       HTMLBrownMaroonFilling,
			       HTMLGreenDarkOliveGreenFilling,
			       HTMLGreenOliveFilling, HTMLGreenOliveDrabFilling,
			       HTMLGreenYellowGreenFilling,
			       HTMLGreenLimeGreenFilling, HTMLGreenLimeFilling,
			       HTMLGreenLawnGreenFilling,
			       HTMLGreenChartreuseFilling,
			       HTMLGreenGreenYellowFilling,
			       HTMLGreenSpringGreenFilling,
			       HTMLGreenMediumSpringGreenFilling,
			       HTMLGreenLightGreenFilling,
			       HTMLGreenPaleGreenFilling,
			       HTMLGreenDarkSeaGreenFilling,
			       HTMLGreenMediumAquamarineFilling,
			       HTMLGreenMediumSeaGreenFilling,
			       HTMLGreenSeaGreenFilling,
			       HTMLGreenForestGreenFilling,
			       HTMLGreenGreenFilling, HTMLGreenDarkGreenFilling,
			       HTMLCyanAquaFilling, HTMLCyanCyanFilling,
			       HTMLCyanLightCyanFilling,
			       HTMLCyanPaleTurquoiseFilling,
			       HTMLCyanAquamarineFilling,
			       HTMLCyanTurquoiseFilling,
			       HTMLCyanMediumTurquoiseFilling,
			       HTMLCyanDarkTurquoiseFilling,
			       HTMLCyanLightSeaGreenFilling,
			       HTMLCyanCadetBlueFilling,
			       HTMLCyanDarkCyanFilling, HTMLCyanTealFilling,
			       HTMLBlueLightSteelBlueFilling,
			       HTMLBluePowderBlueFilling,
			       HTMLBlueLightBlueFilling, HTMLBlueSkyBlueFilling,
			       HTMLBlueLightSkyBlueFilling,
			       HTMLBlueDeepSkyBlueFilling,
			       HTMLBlueDodgerBlueFilling,
			       HTMLBlueCornflowerBlueFilling,
			       HTMLBlueSteelBlueFilling,
			       HTMLBlueRoyalBlueFilling, HTMLBlueBlueFilling,
			       HTMLBlueMediumBlueFilling,
			       HTMLBlueDarkBlueFilling, HTMLBlueNavyFilling,
			       HTMLBlueMidnightBlueFilling,
			       HTMLPurpleLavenderFilling,
			       HTMLPurpleThistleFilling, HTMLPurplePlumFilling,
			       HTMLPurpleVioletFilling, HTMLPurpleOrchidFilling,
			       HTMLPurpleFuchsiaFilling,
			       HTMLPurpleMagentaFilling,
			       HTMLPurpleMediumOrchidFilling,
			       HTMLPurpleMediumPurpleFilling,
			       HTMLPurpleBlueVioletFilling,
			       HTMLPurpleDarkVioletFilling,
			       HTMLPurpleDarkOrchidFilling,
			       HTMLPurpleDarkMagentaFilling,
			       HTMLPurplePurpleFilling, HTMLPurpleIndigoFilling,
			       HTMLPurpleDarkSlateBlueFilling,
			       HTMLPurpleSlateBlueFilling,
			       HTMLPurpleMediumSlateBlueFilling,
			       HTMLWhiteWhiteFilling, HTMLWhiteSnowFilling,
			       HTMLWhiteHoneydewFilling,
			       HTMLWhiteMintCreamFilling, HTMLWhiteAzureFilling,
			       HTMLWhiteAliceBlueFilling,
			       HTMLWhiteGhostWhiteFilling,
			       HTMLWhiteWhiteSmokeFilling,
			       HTMLWhiteSeashellFilling, HTMLWhiteBeigeFilling,
			       HTMLWhiteOldLaceFilling,
			       HTMLWhiteFloralWhiteFilling,
			       HTMLWhiteIvoryFilling,
			       HTMLWhiteAntiqueWhiteFilling,
			       HTMLWhiteLinenFilling,
			       HTMLWhiteLavenderBlushFilling,
			       HTMLWhiteMistyRoseFilling,
			       HTMLGrayGainsboroFilling,
			       HTMLGrayLightGrayFilling, HTMLGraySilverFilling,
			       HTMLGrayDarkGrayFilling, HTMLGrayGrayFilling,
			       HTMLGrayDimGrayFilling,
			       HTMLGrayLightSlateGrayFilling,
			       HTMLGraySlateGrayFilling,
			       HTMLGrayDarkSlateGrayFilling,
			       HTMLGrayBlackFilling, HorFilling, VerFilling,
			       BdiagFilling, FdiagFilling};
		Q_ENUM (Filling)

			//Line color
		enum Color {
			BlackColor, WhiteColor, GreenColor, RedColor, BlueColor,
			GrayColor, BrunColor, YellowColor, CyanColor,
			MagentaColor, LightgrayColor, OrangeColor, PurpleColor,
			HTMLPinkPinkColor, HTMLPinkLightPinkColor,
			HTMLPinkHotPinkColor, HTMLPinkDeepPinkColor,
			HTMLPinkPaleVioletRedColor,
			HTMLPinkMediumVioletRedColor, HTMLRedLightSalmonColor,
			HTMLRedSalmonColor, HTMLRedDarkSalmonColor,
			HTMLRedLightCoralColor, HTMLRedIndianRedColor,
			HTMLRedCrimsonColor, HTMLRedFirebrickColor,
			HTMLRedDarkRedColor, HTMLRedRedColor,
			HTMLOrangeOrangeRedColor, HTMLOrangeTomatoColor,
			HTMLOrangeCoralColor, HTMLOrangeDarkOrangeColor,
			HTMLOrangeOrangeColor, HTMLYellowYellowColor,
			HTMLYellowLightYellowColor, HTMLYellowLemonChiffonColor,
			HTMLYellowLightGoldenrodYellowColor,
			HTMLYellowPapayaWhipColor, HTMLYellowMoccasinColor,
			HTMLYellowPeachPuffColor, HTMLYellowPaleGoldenrodColor,
			HTMLYellowKhakiColor, HTMLYellowDarkKhakiColor,
			HTMLYellowGoldColor, HTMLBrownCornsilkColor,
			HTMLBrownBlanchedAlmondColor, HTMLBrownBisqueColor,
			HTMLBrownNavajoWhiteColor, HTMLBrownWheatColor,
			HTMLBrownBurlywoodColor, HTMLBrownTanColor,
			HTMLBrownRosyBrownColor, HTMLBrownSandyBrownColor,
			HTMLBrownGoldenrodColor, HTMLBrownDarkGoldenrodColor,
			HTMLBrownPeruColor, HTMLBrownChocolateColor,
			HTMLBrownSaddleBrownColor, HTMLBrownSiennaColor,
			HTMLBrownBrownColor, HTMLBrownMaroonColor,
			HTMLGreenDarkOliveGreenColor, HTMLGreenOliveColor,
			HTMLGreenOliveDrabColor, HTMLGreenYellowGreenColor,
			HTMLGreenLimeGreenColor, HTMLGreenLimeColor,
			HTMLGreenLawnGreenColor, HTMLGreenChartreuseColor,
			HTMLGreenGreenYellowColor, HTMLGreenSpringGreenColor,
			HTMLGreenMediumSpringGreenColor,
			HTMLGreenLightGreenColor, HTMLGreenPaleGreenColor,
			HTMLGreenDarkSeaGreenColor,
			HTMLGreenMediumAquamarineColor,
			HTMLGreenMediumSeaGreenColor, HTMLGreenSeaGreenColor,
			HTMLGreenForestGreenColor, HTMLGreenGreenColor,
			HTMLGreenDarkGreenColor, HTMLCyanAquaColor,
			HTMLCyanCyanColor, HTMLCyanLightCyanColor,
			HTMLCyanPaleTurquoiseColor, HTMLCyanAquamarineColor,
			HTMLCyanTurquoiseColor, HTMLCyanMediumTurquoiseColor,
			HTMLCyanDarkTurquoiseColor, HTMLCyanLightSeaGreenColor,
			HTMLCyanCadetBlueColor, HTMLCyanDarkCyanColor,
			HTMLCyanTealColor, HTMLBlueLightSteelBlueColor,
			HTMLBluePowderBlueColor, HTMLBlueLightBlueColor,
			HTMLBlueSkyBlueColor, HTMLBlueLightSkyBlueColor,
			HTMLBlueDeepSkyBlueColor, HTMLBlueDodgerBlueColor,
			HTMLBlueCornflowerBlueColor, HTMLBlueSteelBlueColor,
			HTMLBlueRoyalBlueColor, HTMLBlueBlueColor,
			HTMLBlueMediumBlueColor, HTMLBlueDarkBlueColor,
			HTMLBlueNavyColor, HTMLBlueMidnightBlueColor,
			HTMLPurpleLavenderColor, HTMLPurpleThistleColor,
			HTMLPurplePlumColor, HTMLPurpleVioletColor,
			HTMLPurpleOrchidColor, HTMLPurpleFuchsiaColor,
			HTMLPurpleMagentaColor, HTMLPurpleMediumOrchidColor,
			HTMLPurpleMediumPurpleColor, HTMLPurpleBlueVioletColor,
			HTMLPurpleDarkVioletColor, HTMLPurpleDarkOrchidColor,
			HTMLPurpleDarkMagentaColor, HTMLPurplePurpleColor,
			HTMLPurpleIndigoColor, HTMLPurpleDarkSlateBlueColor,
			HTMLPurpleSlateBlueColor,
			HTMLPurpleMediumSlateBlueColor,
			HTMLWhiteWhiteColor, HTMLWhiteSnowColor,
			HTMLWhiteHoneydewColor, HTMLWhiteMintCreamColor,
			HTMLWhiteAzureColor, HTMLWhiteAliceBlueColor,
			HTMLWhiteGhostWhiteColor, HTMLWhiteWhiteSmokeColor,
			HTMLWhiteSeashellColor, HTMLWhiteBeigeColor,
			HTMLWhiteOldLaceColor, HTMLWhiteFloralWhiteColor,
			HTMLWhiteIvoryColor, HTMLWhiteAntiqueWhiteColor,
			HTMLWhiteLinenColor, HTMLWhiteLavenderBlushColor,
			HTMLWhiteMistyRoseColor, HTMLGrayGainsboroColor,
			HTMLGrayLightGrayColor, HTMLGraySilverColor,
			HTMLGrayDarkGrayColor, HTMLGrayGrayColor,
			HTMLGrayDimGrayColor, HTMLGrayLightSlateGrayColor,
			HTMLGraySlateGrayColor, HTMLGrayDarkSlateGrayColor,
			HTMLGrayBlackColor, NoneColor};
		Q_ENUM (Color)

		// constructors, destructor
	public:

		CustomElementGraphicPart(QETElementEditor *editor,
					 QGraphicsItem *parent = nullptr);
		~CustomElementGraphicPart() override;

		static void drawCross (const QPointF &center,
				       QPainter *painter);

			//Getter and setter
		LineStyle lineStyle    () const {return _linestyle;}
		void      setLineStyle (const LineStyle ls);

		LineWeight lineWeight    () const {return _lineweight;}
		void       setLineWeight (const LineWeight lw);
		qreal      penWeight     () const;

		Filling filling   () const {return _filling;}
		void    setFilling(const Filling f);

		Color color   () const {return _color;}
		void  setColor(const Color c);

		bool antialiased   () const {return _antialiased;}
		void setAntialiased(const bool b);
			//End of getter and setter


			//Rediriged to QObject Q_PROPERTY system
		void setProperty (const char *name,
				  const QVariant &value) override {
			QObject::setProperty(name, value);}
		QVariant property (const char *name) const override {
			return QObject::property(name);}

		virtual QPainterPath shadowShape ()const = 0;
		virtual void setHandlerColor(QPointF /*pos*/,
					     const QColor &/*color*/) {}
		virtual void resetAllHandlerColor() {}

	protected:
		void stylesToXml  (QDomElement &) const;
		void stylesFromXml(const QDomElement &);
		void resetStyles  ();
		void applyStylesToQPainter(QPainter &) const;
		void drawShadowShape (QPainter *painter);

		QVariant itemChange(GraphicsItemChange change,
				    const QVariant &value) override;
		void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(
				QGraphicsSceneMouseEvent *event) override;

		// attributes
		bool m_hovered;
	private:
		LineStyle _linestyle;
		LineWeight _lineweight;
		Filling _filling ;
		Color _color;
		bool _antialiased;
		QPointF m_origin_pos;
};

typedef CustomElementGraphicPart CEGP;
#endif
