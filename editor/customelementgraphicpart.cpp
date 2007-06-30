#include "customelementgraphicpart.h"

/**
	Ecrit les attributs de style dans un element XML
	@param qde L'element XML a modifier
	
*/
void CustomElementGraphicPart::stylesToXml(QDomElement &qde) const {
	QString css_like_styles;
	
	css_like_styles += "line-style:";
	if      (_linestyle == DashedStyle) css_like_styles += "dashed";
	else if (_linestyle == NormalStyle) css_like_styles += "normal";
	
	css_like_styles += ";line-weight:";
	if      (_lineweight == NoneWeight)   css_like_styles += "none";
	else if (_lineweight == ThinWeight)   css_like_styles += "thin";
	else if (_lineweight == NormalWeight) css_like_styles += "normal";
	
	css_like_styles += ";filling:";
	if      (_filling == NoneFilling)  css_like_styles += "none";
	else if (_filling == BlackFilling) css_like_styles += "black";
	else if (_filling == WhiteFilling) css_like_styles += "white";
	
	css_like_styles += ";color:";
	if      (_color == WhiteColor) css_like_styles += "white";
	else if (_color == BlackColor) css_like_styles += "black";
	
	qde.setAttribute("style", css_like_styles);
	qde.setAttribute("antialias", _antialiased ? "true" : "false");
}

/**
	Lit les attributs de style depuis un element XML
	@param qde L'element XML a analyser
*/
void CustomElementGraphicPart::stylesFromXml(const QDomElement &qde) {
	resetStyles();
	
	// recupere la liste des couples style / valeur
	QStringList styles = qde.attribute("style").split(";", QString::SkipEmptyParts);
	
	// analyse chaque couple
	QRegExp rx("^\\s*([a-z-]+)\\s*:\\s*([a-z-]+)\\s*$");
	foreach (QString style, styles) {
		if (!rx.exactMatch(style)) continue;
		QString style_name = rx.cap(1);
		QString style_value = rx.cap(2);
		if (style_name == "line-style") {
			if      (style_value == "dashed") _linestyle = DashedStyle;
			else if (style_value == "normal") _linestyle = NormalStyle;
			// il n'y a pas de else car les valeurs non conformes sont ignorees (idem par la suite)
		} else if (style_name == "line-weight") {
			if      (style_value == "thin")   _lineweight = ThinWeight;
			else if (style_value == "normal") _lineweight = NormalWeight;
			else if (style_value == "none")   _lineweight = NoneWeight;
		} else if (style_name == "filling") {
			if      (style_value == "white") _filling = WhiteFilling;
			else if (style_value == "black") _filling = BlackFilling;
			else if (style_value == "none")  _filling = NoneFilling;
		} else if (style_name == "color") {
			if      (style_value == "black") _color = BlackColor;
			else if (style_value == "white") _color = WhiteColor;
		}
	}
	
	// recupere l'antialiasing
	_antialiased = qde.attribute("antialias") == "true";
	
	// met a jour l'editeur de style
	style_editor -> updateForm();
}

/**
	Remet les styles par defaut
*/
void CustomElementGraphicPart::resetStyles() {
	_linestyle = NormalStyle;
	_lineweight = NormalWeight;
	_filling = NoneFilling;
	_color = BlackColor;
	_antialiased = false;
}

/**
	Applique les styles a un Qpainter
	@param painter QPainter a modifier
*/
void CustomElementGraphicPart::applyStylesToQPainter(QPainter &painter) const {
	// recupere le QPen et la QBrush du QPainter
	QPen pen = painter.pen();
	QBrush brush = painter.brush();
	
	// applique le style de trait
	if      (_linestyle == DashedStyle) pen.setStyle(Qt::DashLine);
	else if (_linestyle == NormalStyle) pen.setStyle(Qt::SolidLine);
	
	// applique l'epaisseur de trait
	if      (_lineweight == NoneWeight)   pen.setColor(QColor(0, 0, 0, 0));
	else if (_lineweight == ThinWeight)   pen.setWidth(0);
	else if (_lineweight == NormalWeight) pen.setWidthF(1.0);
	
	// applique le remplissage
	if (_filling == NoneFilling) {
		brush.setStyle(Qt::NoBrush);
	} else if (_filling == BlackFilling) {
		brush.setStyle(Qt::SolidPattern);
		brush.setColor(Qt::black);
	} else if (_filling == WhiteFilling) {
		brush.setStyle(Qt::SolidPattern);
		brush.setColor(Qt::white);
	}
	
	// applique la couleur de trait
	if      (_color == WhiteColor) pen.setColor(QColor(255, 255, 255, pen.color().alpha()));
	else if (_color == BlackColor) pen.setColor(QColor(  0,   0,   0, pen.color().alpha()));
	
	
	// applique l'antialiasing
	painter.setRenderHint(QPainter::Antialiasing,          _antialiased);
	painter.setRenderHint(QPainter::TextAntialiasing,      _antialiased);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, _antialiased);
	
	painter.setPen(pen);
	painter.setBrush(brush);
}

/**
	@return Le widget permettant d'editer les styles
*/
QWidget *CustomElementGraphicPart::elementInformations() {
	return(style_editor);
}
