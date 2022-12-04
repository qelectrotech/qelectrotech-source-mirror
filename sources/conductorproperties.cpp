/*
	Copyright 2006-2021 The QElectroTech Team
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
#include "conductorproperties.h"
#include <QPainter>
#include <QMetaEnum>
#include <QRegularExpression>
#include <QtDebug>
/**
	Constructeur par defaut
*/
SingleLineProperties::SingleLineProperties() :
	hasGround(true),
	hasNeutral(true),
	is_pen(false),
	phases(1)
{
}

/// Destructeur
SingleLineProperties::~SingleLineProperties()
{
}

/**
	Definit le nombre de phases (0, 1, 2, ou 3)
	@param n Nombre de phases
*/
void SingleLineProperties::setPhasesCount(int n) {
	phases = qBound(0, n, 3);
}

/// @return le nombre de phases (0, 1, 2, ou 3)
unsigned short int SingleLineProperties::phasesCount()
{
	return(phases);
}

/**
	@return true if the singleline conductor should be drawn using the PEN
	(Protective Earth Neutral) representation and if it features the ground and
	the neutral.
*/
bool SingleLineProperties::isPen() const
{
	return(hasNeutral && hasGround && is_pen);
}

/**
	Dessine les symboles propres a un conducteur unifilaire
	@param painter QPainter a utiliser pour dessiner les symboles
	@param direction direction du segment sur lequel les symboles apparaitront
	@param rect rectangle englobant le dessin ; utilise pour specifier a la fois la position et la taille du dessin
*/
void SingleLineProperties::draw(QPainter *painter,
				QET::ConductorSegmentType direction,
				const QRectF &rect) {
	// s'il n'y a rien a dessiner, on retourne immediatement
	if (!hasNeutral && !hasGround && !phases) return;

	// prepare le QPainter
	painter -> save();
	QPen pen(painter -> pen());
	pen.setCapStyle(Qt::FlatCap);
	pen.setJoinStyle(Qt::MiterJoin);
	pen.setStyle(Qt::SolidLine);
	pen.setWidthF(1);
	painter -> setPen(pen);
	painter -> setRenderHint(QPainter::Antialiasing, true);

	uint symbols_count = (hasNeutral ? 1 : 0) + (hasGround ? 1 : 0) - (isPen() ? 1 : 0) + phases;
	qreal interleave_base = (direction == QET::Horizontal ? rect.width() : rect.height());
	qreal interleave = interleave_base / (symbols_count + 1);;
	qreal symbol_width = interleave_base / 12;

	for (uint i = 1 ; i <= symbols_count ; ++ i) {
		// dessine le tronc du symbole
		QPointF symbol_p1, symbol_p2;
		if (direction == QET::Horizontal) {
			symbol_p1 = QPointF(rect.x() + (i * interleave) + symbol_width, rect.y() + rect.height() * 0.75);
			symbol_p2 = QPointF(rect.x() + (i * interleave) - symbol_width, rect.y() + rect.height() * 0.25);
		} else {
			symbol_p2 = QPointF(rect.x() + rect.width() * 0.75, rect.y() + (i * interleave) - symbol_width);
			symbol_p1 = QPointF(rect.x() + rect.width() * 0.25, rect.y() + (i * interleave) + symbol_width);
		}
		painter -> drawLine(QLineF(symbol_p1, symbol_p2));

		// dessine le reste des symboles terre et neutre
		if (isPen()) {
			if (i == 1) {
				drawPen(painter, direction, symbol_p2, symbol_width);
			}
		} else {
			if (hasGround && i == 1) {
				drawGround(painter, direction, symbol_p2, symbol_width * 2.0);
			} else if (hasNeutral && ((i == 1 && !hasGround) || (i == 2 && hasGround))) {
				drawNeutral(painter, symbol_p2, symbol_width * 1.5);
			}
		}
	}
	painter -> restore();
}

/**
	Dessine le segment correspondant au symbole de la terre sur un conducteur unifilaire
	@param painter QPainter a utiliser pour dessiner le segment
	@param direction direction du segment sur lequel le symbole apparaitra
	@param center centre du segment
	@param size taille du segment
*/
void SingleLineProperties::drawGround(QPainter *painter,
				      QET::ConductorSegmentType direction,
				      QPointF center,
				      qreal size) {
	painter -> save();

	// prepare le QPainter
	painter -> setRenderHint(QPainter::Antialiasing, false);
	QPen pen2(painter -> pen());
	pen2.setCapStyle(Qt::SquareCap);
	painter -> setPen(pen2);

	// dessine le segment representant la terre
	qreal half_size = size / 2.0;
	QPointF offset_point(
		(direction == QET::Horizontal) ? half_size : 0.0,
		(direction == QET::Horizontal) ? 0.0 : half_size
	);
	painter -> drawLine(
		QLineF(
			center + offset_point,
			center - offset_point
		)
	);

	painter -> restore();
}

/**
	Dessine le cercle correspondant au symbole du neutre sur un conducteur unifilaire
	@param painter QPainter a utiliser pour dessiner le segment
	@param center centre du cercle
	@param size diametre du cercle
*/
void SingleLineProperties::drawNeutral(
		QPainter *painter,
		QPointF center,
		qreal size)
{
	painter -> save();

	// prepare le QPainter
	if (painter -> brush() == Qt::NoBrush) painter -> setBrush(Qt::black);
	painter -> setPen(Qt::NoPen);

	// desine le cercle representant le neutre
	painter -> drawEllipse(
		QRectF(
			center - QPointF(size / 2.0, size / 2.0),
			QSizeF(size, size)
		)
	);

	painter -> restore();
}

/**
	@brief SingleLineProperties::drawPen
	Draw the PEN (Protective Earth Neutral) symbol using
	\a painter at position \a center, using a size hint of \a size.
	@param painter
	@param direction :
	Indicate the direction of the underlying conductor segment
	@param center
	@param size
*/
void SingleLineProperties::drawPen(QPainter *painter,
				   QET::ConductorSegmentType direction,
				   QPointF center,
				   qreal size) {
	painter -> save();

	//painter -> setBrush(Qt::white);
	// desine le cercle representant le neutre
	//painter -> drawEllipse(
	//	QRectF(
	//		center - QPointF(size * 1.5 / 2.0, size * 1.5 / 2.0),
	//		QSizeF(size * 1.5, size * 1.5)
	//	)
	//);
	drawNeutral(painter, center, size * 1.5);

	int offset = (size * 1.5 / 2.0);
	QPointF pos = center + (direction == QET::Horizontal ? QPointF(0.0, -offset - 0.5) : QPointF(offset + 0.5, 0.0));
	drawGround(painter, direction, pos, 2.0 * size);
	painter -> restore();
}

/**
	Exporte les parametres du conducteur unifilaire sous formes d'attributs XML
	ajoutes a l'element e.
	@param e Element XML auquel seront ajoutes des attributs
*/
void SingleLineProperties::toXml(QDomElement &e) const
{
	e.setAttribute("ground",  hasGround  ? "true" : "false");
	e.setAttribute("neutral", hasNeutral ? "true" : "false");
	e.setAttribute("phase",   phases);
	if (isPen()) e.setAttribute("pen", "true");
}

/**
	Importe les parametres du conducteur unifilaire a partir des attributs XML
	de l'element e
	@param e Element XML dont les attributs seront lus
*/
void SingleLineProperties::fromXml(QDomElement &e) {
	hasGround  = e.attribute("ground")  == "true";
	hasNeutral = e.attribute("neutral") == "true";
	setPhasesCount(e.attribute("phase").toInt());
	is_pen = (hasGround && hasNeutral && e.attribute("pen", "false") == "true");
}

/**
	Constructeur : par defaut, les proprietes font un conducteur
	multifilaire noir dont le texte est "_"
*/
ConductorProperties::ConductorProperties() :
	type(Multi),
	color(Qt::black),
	text_color(Qt::black),
	text("_"),
	text_size(9),
	cond_size(1),
	verti_rotate_text(270),
	horiz_rotate_text(0),
	m_show_text(true),
	m_one_text_per_folio(false),
	style(Qt::SolidLine)
{}

/**
	Destructeur
*/
ConductorProperties::~ConductorProperties()
{
}


/**
	@brief ConductorProperties::toXml
	Export conductor properties to the XML element 'e'
	@param e the xml element
*/
void ConductorProperties::toXml(QDomElement &e) const
{
	e.setAttribute("type", typeToString(type));

	if (color != QColor(Qt::black))
		e.setAttribute("color", color.name());

	e.setAttribute("bicolor", m_bicolor? "true" : "false");
	e.setAttribute("color2", m_color_2.name());
	e.setAttribute("dash-size", QString::number(m_dash_size));

	if (type == Single)
		singleLineProperties.toXml(e);

	e.setAttribute("num", text);
	e.setAttribute("text_color", text_color.name());
	e.setAttribute("formula", m_formula);
	e.setAttribute("cable", m_cable);
	e.setAttribute("bus", m_bus);
	e.setAttribute("function", m_function);
	e.setAttribute("tension_protocol", m_tension_protocol);
	e.setAttribute("conductor_color", m_wire_color);
	e.setAttribute("conductor_section", m_wire_section);
	e.setAttribute("numsize", QString::number(text_size));
	e.setAttribute("condsize", QString::number(cond_size));
	e.setAttribute("displaytext", m_show_text);
	e.setAttribute("onetextperfolio", m_one_text_per_folio);
	e.setAttribute("vertirotatetext", QString::number(verti_rotate_text));
	e.setAttribute("horizrotatetext", QString::number(horiz_rotate_text));

	QMetaEnum me = QMetaEnum::fromType<Qt::Alignment>();
	e.setAttribute("horizontal-alignment", me.valueToKey(m_horizontal_alignment));
	e.setAttribute("vertical-alignment", me.valueToKey(m_vertical_alignment));

	QString conductor_style = writeStyle();
	if (!conductor_style.isEmpty())
		e.setAttribute("style", conductor_style);
}


/**
	@brief ConductorProperties::fromXml
	Import conductor properties from the attribute of the xml element 'e'
	@param e the xml document
*/
void ConductorProperties::fromXml(QDomElement &e)
{
		// get conductor color
	QColor xml_color= QColor(e.attribute("color"));
	color = (xml_color.isValid()? xml_color : QColor(Qt::black));

	QString bicolor_str = e.attribute("bicolor", "false");
	m_bicolor = bicolor_str == "true"? true : false;

	QColor xml_color_2 = QColor(e.attribute("color2"));
	m_color_2 = xml_color_2.isValid()? xml_color_2 : QColor(Qt::black);

	m_dash_size = e.attribute("dash-size", QString::number(1)).toInt();

		// read style of conductor
	readStyle(e.attribute("style"));

	if (e.attribute("type") == typeToString(Single))
	{
			// get specific properties for single conductor
		singleLineProperties.fromXml(e);
		type = Single;
	}
	else
		type = Multi;

	text                 = e.attribute("num");
	// get text color
	QColor xml_text_color= QColor(e.attribute("text_color"));
	text_color = (xml_text_color.isValid()? xml_text_color : QColor(Qt::black));
	m_formula            = e.attribute("formula");
	m_cable              = e.attribute("cable");
	m_bus                = e.attribute("bus");
	m_function           = e.attribute("function");
	m_tension_protocol   = e.attribute("tension_protocol");
	m_wire_color         = e.attribute("conductor_color");
	m_wire_section       = e.attribute("conductor_section");
	text_size            = e.attribute("numsize", QString::number(9)).toInt();
	cond_size            = e.attribute("condsize", QString::number(1)).toDouble();
	m_show_text          = e.attribute("displaytext", QString::number(1)).toInt();
	m_one_text_per_folio = e.attribute("onetextperfolio", QString::number(0)).toInt();
	verti_rotate_text    = e.attribute("vertirotatetext").toDouble();
	horiz_rotate_text    = e.attribute("horizrotatetext").toDouble();

	QMetaEnum me = QMetaEnum::fromType<Qt::Alignment>();
	m_horizontal_alignment = Qt::Alignment(
				me.keyToValue(
					e.attribute(
						"horizontal-alignment",
						"AlignBottom"
						).toStdString().data()));
	m_vertical_alignment = Qt::Alignment(
				me.keyToValue(
					e.attribute(
						"vertical-alignment",
						"AlignRight"
						).toStdString().data()));
}

/**
	@param settings Parametres a ecrire
	@param prefix prefixe a ajouter devant les noms des parametres
*/
void ConductorProperties::toSettings(QSettings &settings, const QString &prefix) const
{
	settings.setValue(prefix + "color", color.name());
	settings.setValue(prefix + "bicolor", m_bicolor);
	settings.setValue(prefix + "color2", m_color_2.name());
	settings.setValue(prefix + "dash-size", m_dash_size);
	settings.setValue(prefix + "style", writeStyle());
	settings.setValue(prefix + "type", typeToString(type));
	settings.setValue(prefix + "text", text);
	settings.setValue(prefix + "text_color", text_color.name());
	settings.setValue(prefix + "formula", m_formula);
	settings.setValue(prefix + "cable", m_cable);
	settings.setValue(prefix + "bus", m_bus);
	settings.setValue(prefix + "function", m_function);
	settings.setValue(prefix + "tension_protocol", m_tension_protocol);
	settings.setValue(prefix + "conductor_color", m_wire_color);
	settings.setValue(prefix + "conductor_section", m_wire_section);
	settings.setValue(prefix + "textsize", QString::number(text_size));
	settings.setValue(prefix + "size", QString::number(cond_size));
	settings.setValue(prefix + "displaytext", m_show_text);
	settings.setValue(prefix + "onetextperfolio", m_one_text_per_folio);
	settings.setValue(prefix + "vertirotatetext", QString::number(verti_rotate_text));
	settings.setValue(prefix + "horizrotatetext", QString::number(horiz_rotate_text));

	QMetaEnum me = QMetaEnum::fromType<Qt::Alignment>();
	settings.setValue(prefix + "horizontal-alignment", me.valueToKey(m_horizontal_alignment));
	settings.setValue(prefix + "vertical-alignment", me.valueToKey(m_vertical_alignment));

	singleLineProperties.toSettings(settings, prefix);
}

/**
	@param settings Parametres a lire
	@param prefix prefixe a ajouter devant les noms des parametres
*/
void ConductorProperties::fromSettings(QSettings &settings, const QString &prefix)
{
	QColor settings_color = QColor(settings.value(prefix + "color").toString());
	color = (settings_color.isValid()? settings_color : QColor(Qt::black));

	QColor settings_color_2 = QColor(settings.value(prefix + "color2").toString());
	m_color_2 = (settings_color_2.isValid()? settings_color_2 : QColor(Qt::black));

	m_bicolor   = settings.value(prefix + "bicolor", false).toBool();
	m_dash_size = settings.value(prefix + "dash-size", 1).toInt();

	QString setting_type = settings.value(prefix + "type", typeToString(Multi)).toString();
	type = (setting_type == typeToString(Single)? Single : Multi);

	singleLineProperties.fromSettings(settings, prefix);

	text                 = settings.value(prefix + "text", "_").toString();
	QColor settings_text_color = QColor(settings.value(prefix + "text_color").toString());
	text_color = (settings_text_color.isValid()? settings_text_color : QColor(Qt::black));
	m_formula            = settings.value(prefix + "formula", "").toString();
	m_cable              = settings.value(prefix + "cable", "").toString();
	m_bus                = settings.value(prefix + "bus", "").toString();
	m_function           = settings.value(prefix + "function", "").toString();
	m_tension_protocol   = settings.value(prefix + "tension_protocol", "").toString();
	m_wire_color         = settings.value(prefix + "conductor_color", "").toString();
	m_wire_section       = settings.value(prefix + "conductor_section", "").toString();
	text_size            = settings.value(prefix + "textsize", "7").toInt();
	cond_size            = settings.value(prefix + "size", "1").toInt();
	m_show_text          = settings.value(prefix + "displaytext", true).toBool();
	m_one_text_per_folio = settings.value(prefix + "onetextperfolio", false).toBool();
	verti_rotate_text    = settings.value((prefix + "vertirotatetext"), "270").toDouble();
	horiz_rotate_text    = settings.value((prefix + "horizrotatetext"), "0").toDouble();

	QMetaEnum me = QMetaEnum::fromType<Qt::Alignment>();
	m_horizontal_alignment = Qt::Alignment(me.keyToValue(settings.value(prefix + "horizontal-alignment", "AlignBottom").toString().toStdString().data()));
	m_vertical_alignment = Qt::Alignment(me.keyToValue(settings.value(prefix + "vertical-alignment", "AlignRight").toString().toStdString().data()));

	readStyle(settings.value(prefix + "style").toString());
}

/**
	@param t type du conducteur
*/
QString ConductorProperties::typeToString(ConductorType t)
{
	switch(t)
	{
		case Single: return("single");
		case Multi:  return("multi");
	}
	return QString();
}

/**
	@brief ConductorProperties::applyForEqualAttributes
	Test each attribute of properties in the list separatly.
	For each attributes, if is equal, the attribute is apply to this.
	@param list
*/
void ConductorProperties::applyForEqualAttributes(QList<ConductorProperties> list)
{
	const QList<ConductorProperties> clist = std::move(list);

	if (clist.isEmpty())
		return;

	if (clist.size() == 1)
	{
		ConductorProperties cp = clist.first();
		color                = cp.color;
		m_bicolor            = cp.m_bicolor;
		m_color_2            = cp.m_color_2;
		m_dash_size          = cp.m_dash_size;
		text                 = cp.text;
		text_color           = cp.text_color;
		m_formula            = cp.m_formula;
		m_cable              = cp.m_cable;
		m_bus                = cp.m_bus;
		m_function           = cp.m_function;
		m_tension_protocol   = cp.m_tension_protocol;
		m_wire_color         = cp.m_wire_color;
		m_wire_section       = cp.m_wire_section;
		text_size            = cp.text_size;
		cond_size            = cp.cond_size;
		m_show_text          = cp.m_show_text;
		m_one_text_per_folio = cp.m_one_text_per_folio;
		verti_rotate_text    = cp.verti_rotate_text;
		horiz_rotate_text    = cp.horiz_rotate_text;
		m_vertical_alignment = cp.m_vertical_alignment;
		m_horizontal_alignment = cp.m_horizontal_alignment;

		return;
	}

	bool equal = true;
	QColor c_value;
	bool b_value;
	QString s_value;
	int i_value;
	double d_value;
	Qt::Alignment align_value;

		//Color
	c_value = clist.first().color;
	for(ConductorProperties cp : clist)
	{
		if (cp.color != c_value)
			equal = false;
	}
	if (equal)
		color = c_value;
	equal = true;

		//bicolor
	b_value = clist.first().m_bicolor;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_bicolor != b_value)
			equal = false;
	}
	if (equal)
		m_bicolor = b_value;
	equal = true;

		//second color
	c_value = clist.first().m_color_2;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_color_2 != c_value)
			equal = false;
	}
	if (equal)
		m_color_2 = c_value;
	equal = true;

		//Dash size
	i_value = clist.first().m_dash_size;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_dash_size != i_value)
			equal = false;
	}
	if (equal)
		m_dash_size = i_value;
	equal = true;

		//text
	s_value = clist.first().text;
	for(ConductorProperties cp : clist)
	{
		if (cp.text != s_value)
			equal = false;
	}
	if (equal)
		text = s_value;
	equal = true;

		//text color
	c_value = clist.first().text_color;
	for(ConductorProperties cp : clist)
	{
		if (cp.text_color != c_value)
			equal = false;
	}
	if (equal)
		text_color = c_value;
	equal = true;

		//formula
	s_value = clist.first().m_formula;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_formula != s_value)
			equal = false;
	}
	if (equal)
		m_formula = s_value;
	equal = true;

	//cable
	s_value = clist.first().m_cable;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_cable != s_value)
			equal = false;
	}
	if (equal)
		m_cable = s_value;
	equal = true;

	//bus
	s_value = clist.first().m_bus;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_bus != s_value)
			equal = false;
	}
	if (equal)
		m_bus = s_value;
	equal = true;

	//function
	s_value = clist.first().m_function;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_function != s_value)
			equal = false;
	}
	if (equal)
		m_function = s_value;
	equal = true;

		//Tension protocol
	s_value = clist.first().m_tension_protocol;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_tension_protocol != s_value)
			equal = false;
	}
	if (equal)
		m_tension_protocol = s_value;
	equal = true;

		//conductor_color
	s_value = clist.first().m_wire_color;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_wire_color != s_value)
			equal = false;
	}
	if (equal)
		m_wire_color = s_value;
	equal = true;

		//conductor_section
	s_value = clist.first().m_wire_section;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_wire_section != s_value)
			equal = false;
	}
	if (equal)
		m_wire_section = s_value;
	equal = true;


		//text size
	i_value = clist.first().text_size;
	for(ConductorProperties cp : clist)
	{
		if (cp.text_size != i_value)
			equal = false;
	}
	if (equal)
		text_size = i_value;
	equal = true;

		//conductor size
	d_value = clist.first().cond_size;
	for(ConductorProperties cp : clist)
	{
		if (cp.cond_size != d_value)
			equal = false;
	}
	if (equal)
		cond_size = d_value;
	equal = true;

		//show text
	b_value = clist.first().m_show_text;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_show_text != b_value)
			equal = false;
	}
	if (equal)
		m_show_text = b_value;
	equal = true;

		//One text per folio
	b_value = clist.first().m_one_text_per_folio;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_one_text_per_folio != b_value)
			equal = false;
	}
	if (equal)
		m_one_text_per_folio = b_value;
	equal = true;

		//Text rotation for vertical conducor
	d_value = clist.first().verti_rotate_text;
	for(ConductorProperties cp : clist)
	{
		if (cp.verti_rotate_text != d_value)
			equal = false;
	}
	if (equal)
		verti_rotate_text = d_value;
	equal = true;

		//Text rotation for horizontal conducor
	d_value = clist.first().horiz_rotate_text;
	for(ConductorProperties cp : clist)
	{
		if (cp.horiz_rotate_text != d_value)
			equal = false;
	}
	if (equal)
		horiz_rotate_text = d_value;
	equal = true;

		//Text alignment for horizontal conducor
	align_value = clist.first().m_horizontal_alignment;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_horizontal_alignment != align_value)
			equal = false;
	}
	if (equal)
		m_horizontal_alignment = align_value;
	equal = true;

		//Text alignment for vertical conducor
	align_value = clist.first().m_vertical_alignment;
	for(ConductorProperties cp : clist)
	{
		if (cp.m_vertical_alignment != align_value)
			equal = false;
	}
	if (equal)
		m_vertical_alignment = align_value;
	equal = true;
}

/**
	@brief ConductorProperties::defaultProperties
	@return the default properties stored in the setting file
*/
ConductorProperties ConductorProperties::defaultProperties()
{
	QSettings settings;

	ConductorProperties def;
	def.fromSettings(settings, "diagrameditor/defaultconductor");

	return(def);
}

/**
	@brief ConductorProperties::operator ==
	@param other
	@return true if other == this
*/
bool ConductorProperties::operator==(const ConductorProperties &other) const
{
	return(
		other.type == type &&\
		other.color == color &&\
		other.m_bicolor == m_bicolor &&\
		other.m_color_2 == m_color_2 &&\
		other.m_dash_size == m_dash_size &&\
		other.style == style &&\
		other.text == text &&\
		other.text_color == text_color &&\
		other.m_formula == m_formula &&\
		other.m_cable == m_cable &&\
		other.m_bus == m_bus &&\
		other.m_function == m_function &&\
		other.m_tension_protocol == m_tension_protocol &&\
		other.m_wire_color == m_wire_color && \
		other.m_wire_section == m_wire_section && \
		other.m_show_text == m_show_text &&\
		other.text_size == text_size &&\
		other.cond_size == cond_size &&\
		other.verti_rotate_text == verti_rotate_text &&\
		other.horiz_rotate_text == horiz_rotate_text &&\
		other.singleLineProperties == singleLineProperties &&\
		other.m_one_text_per_folio == m_one_text_per_folio &&\
		other.m_horizontal_alignment == m_horizontal_alignment &&\
		other.m_vertical_alignment == m_vertical_alignment
	);
}

/**
	@param other l'autre ensemble de proprietes avec lequel il faut effectuer la comparaison
	@return true si les deux ensembles de proprietes sont differents, false sinon
*/
bool ConductorProperties::operator!=(const ConductorProperties &other) const{
	return(!(*this == other));
}

/**
	Applique les styles passes en parametre dans cet objet
	@param style_string Chaine decrivant le style du conducteur
*/
void ConductorProperties::readStyle(const QString &style_string) {
	style = Qt::SolidLine; // style par defaut

	if (style_string.isEmpty()) return;

	// recupere la liste des couples style / valeur
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)	// ### Qt 6: remove
	QStringList styles = style_string.split(";", QString::SkipEmptyParts);
#else
#if TODO_LIST
#pragma message("@TODO remove code QString::SkipEmptyParts for QT 5.14 or later")
#endif
	QStringList styles = style_string.split(";", Qt::SkipEmptyParts);
#endif

	QRegularExpression Rx("^(?<name>[a-z-]+): (?<value>[a-z-]+)$");
	if (!Rx.isValid())
	{
		qWarning() <<QObject::tr("this is an error in the code")
			  << Rx.errorString()
			  << Rx.patternErrorOffset();
		return;
	}
	foreach (QString style_str, styles)
	{
		QRegularExpressionMatch match = Rx.match(style_str);
		if (!match.hasMatch())
		{
			qDebug()<<"no Match"
			       <<style_str;
		} else {
			QString style_name = match.captured("name");
			QString style_value = match.captured("value");
			if (style_name == "line-style") {
				if (style_value == "dashed") style = Qt::DashLine;
				else if (style_value == "dashdotted") style = Qt::DashDotLine;
				else if (style_value == "normal") style = Qt::SolidLine;
				else {
					qWarning()<<"style_value not known"
						 <<style_value;
				}
			}
		}
	}
}

/**
	Exporte le style du conducteur sous forme d'une chaine de caracteres
	@return une chaine de caracteres decrivant le style du conducteur
*/
QString ConductorProperties::writeStyle() const
{
	if (style == Qt::DashLine) {
		return("line-style: dashed;");
	} else if (style == Qt::DashDotLine) {
		return("line-style: dashdotted");
	} else {
		return(QString());
	}
}

/**
	@param other l'autre ensemble de proprietes avec lequel il faut effectuer la comparaison
	@return true si les deux ensembles de proprietes sont identiques, false sinon
*/
int SingleLineProperties::operator==(const SingleLineProperties &other) const
{
	return(
		other.hasGround == hasGround &&\
		other.hasNeutral == hasNeutral &&\
		other.is_pen == is_pen &&\
		other.phases == phases
	);
}

/**
	@param other l'autre ensemble de proprietes avec lequel il faut effectuer la comparaison
	@return true si les deux ensembles de proprietes sont differents, false sinon
*/
int SingleLineProperties::operator!=(const SingleLineProperties &other) const
{
	return(!(other == (*this)));
}

/**
	@param settings Parametres a ecrire
	@param prefix prefix a ajouter devant les noms des parametres
*/
void SingleLineProperties::toSettings(QSettings &settings,
				      const QString &prefix) const
{
	settings.setValue(prefix + "hasGround",  hasGround);
	settings.setValue(prefix + "hasNeutral", hasNeutral);
	settings.setValue(prefix + "phases",     phases);
	settings.setValue(prefix + "pen",        is_pen);
}

/**
	@param settings Parametres a lire
	@param prefix prefix a ajouter devant les noms des parametres
*/
void SingleLineProperties::fromSettings(QSettings &settings,
					const QString &prefix) {
	hasGround  = settings.value(prefix + "hasGround",  true).toBool();
	hasNeutral = settings.value(prefix + "hasNeutral", true).toBool();
	phases     = settings.value(prefix + "phases",     1).toInt();
	is_pen     = settings.value(prefix + "pen",        false).toBool();
}
