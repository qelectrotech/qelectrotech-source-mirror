#include "titleblockcell.h"
#include "titleblocktemplate.h"
/**
	Constructor
*/
TitleBlockCell::TitleBlockCell() {
	cell_type = TitleBlockCell::EmptyCell;
	num_row = num_col = -1;
	row_span = col_span = 0;
	applied_row_span = applied_col_span = 0;
	span_state = TitleBlockCell::Enabled;
	spanner_cell = 0;
	display_label = true;
	alignment = Qt::AlignCenter | Qt::AlignVCenter;
	font_size = 9;
	hadjust = false;
	logo_reference = QString("");
}

/**
	Destructor
*/
TitleBlockCell::~TitleBlockCell() {
}

/**
	@return A string representing the titleblock cell
*/
QString TitleBlockCell::toString() const {
	if (cell_type == TitleBlockCell::EmptyCell) return("TitleBlockCell{null}");
	QString span_desc = (row_span > 0 || col_span > 0) ? QString("+%1,%2").arg(row_span).arg(col_span) : QET::pointerString(spanner_cell);
	QString base_desc = QString("TitleBlockCell{ [%1, %2] %3 }").arg(num_row).arg(num_col).arg(span_desc);
	return(base_desc);
}

/**
	@return the type of this cell
*/
TitleBlockCell::TemplateCellType TitleBlockCell::type() const {
	return(cell_type);
}

/**
	@return the horizontal alignment of this cell
*/
int TitleBlockCell::horizontalAlign() const {
	return(alignment & Qt::AlignHorizontal_Mask);
}

/**
	@return the vertical alignment of this cell
*/
int TitleBlockCell::verticalAlign() const {
	return(alignment & Qt::AlignVertical_Mask);
}

/**
	Set the new value \a attr_value to the attribute named \a attribute.
	@param attribute Name of the cell attribute which value is to be changed
	@param attr_value New value of the changed attribute
*/
void TitleBlockCell::setAttribute(const QString &attribute, const QVariant &attr_value) {
	if (attribute == "type") {
		int new_type = attr_value.toInt();
		if (new_type <= TitleBlockCell::LogoCell) {
			cell_type = static_cast<TitleBlockCell::TemplateCellType>(new_type);
		}
	} else if (attribute == "name") {
		value_name = attr_value.toString();
	} else if (attribute == "logo") {
		logo_reference = attr_value.toString();
	} else if (attribute == "label") {
		label = qvariant_cast<NamesList>(attr_value);
	} else if (attribute == "displaylabel") {
		display_label = attr_value.toBool();
	} else if (attribute == "value") {
		value = qvariant_cast<NamesList>(attr_value);
	} else if (attribute == "alignment") {
		alignment = attr_value.toInt();
	} else if (attribute == "fontsize") {
		font_size = attr_value.toInt();
	} else if (attribute == "horizontal_adjust") {
		hadjust = attr_value.toBool();
	}
}

/**
	@param attribute Name of the cell attribute which value is wanted
	@return the value of the required attribute
*/
QVariant TitleBlockCell::attribute(const QString &attribute) {
	if (attribute == "type") {
		return(type());
	} else if (attribute == "name") {
		return(value_name);
	} else if (attribute == "logo") {
		return(logo_reference);
	} else if (attribute == "label") {
		return(qVariantFromValue(label));
	} else if (attribute == "displaylabel") {
		return(display_label);
	} else if (attribute == "value") {
		return(qVariantFromValue(value));
	} else if (attribute == "alignment") {
		return(alignment);
	} else if (attribute == "fontsize") {
		return(TitleBlockTemplate::fontForCell(*this).pointSizeF());
	} else if (attribute == "horizontal_adjust") {
		return(hadjust);
	}
	return(QVariant());
}

/**
	@param attribute Name of the cell attribute which we want the human, translated name
	@return the human, translated name for this attribute.
*/
QString TitleBlockCell::attributeName(const QString &attribute) {
	if (attribute == "type") {
		return(QObject::tr("type", "title block cell property human name"));
	} else if (attribute == "name") {
		return(QObject::tr("nom", "title block cell property human name"));
	} else if (attribute == "logo") {
		return(QObject::tr("logo", "title block cell property human name"));
	} else if (attribute == "label") {
		return(QObject::tr("label", "title block cell property human name"));
	} else if (attribute == "displaylabel") {
		return(QObject::tr("affichage du label", "title block cell property human name"));
	} else if (attribute == "value") {
		return(QObject::tr("valeur affichée", "title block cell property human name"));
	} else if (attribute == "alignment") {
		return(QObject::tr("alignement du texte", "title block cell property human name"));
	} else if (attribute == "fontsize") {
		return(QObject::tr("taille du texte", "title block cell property human name"));
	} else if (attribute == "horizontal_adjust") {
		return(QObject::tr("ajustement horizontal", "title block cell property human name"));
	}
	return(QString());
}

/**
	@return true if this cell spans over other cells, false otherwise.
*/
bool TitleBlockCell::spans() const {
	return(row_span || col_span);
}

/**
	Copy the content of another cell.
	@param other_cell Another cell
*/
void TitleBlockCell::loadContentFromCell(const TitleBlockCell &other_cell) {
	value_name = other_cell.value_name;
	cell_type = other_cell.cell_type;
	logo_reference = other_cell.logo_reference;
	value = other_cell.value;
	label = other_cell.label;
	display_label = other_cell.display_label;
	font_size = other_cell.font_size;
	alignment = other_cell.alignment;
	hadjust = other_cell.hadjust;
}

/**
	@param cell_element XML element from which cell content will be read
*/
void TitleBlockCell::loadContentFromXml(const QDomElement &cell_element) {
	// common properties
	if (cell_element.hasAttribute("name") && !cell_element.attribute("name").isEmpty()) {
		value_name = cell_element.attribute("name");
	}
	
	// specific properties
	if (cell_element.tagName() == "logo") {
		if (cell_element.hasAttribute("resource") && !cell_element.attribute("resource").isEmpty()) {
			cell_type = TitleBlockCell::LogoCell;
			logo_reference = cell_element.attribute("resource");
		}
	} else if (cell_element.tagName() == "field") {
		cell_type = TitleBlockCell::TextCell;
		
		QHash<QString, QString> names_options;
		names_options["TagName"] = "translation";
		
		names_options["ParentTagName"] = "value";
		NamesList value_nameslist;
		value_nameslist.fromXml(cell_element, names_options);
		if (!value_nameslist.name().isEmpty()) {
			value = value_nameslist;
		}
		
		names_options["ParentTagName"] = "label";
		NamesList label_nameslist;
		label_nameslist.fromXml(cell_element, names_options);
		if (!label_nameslist.name().isEmpty()) {
			label = label_nameslist;
		}
		
		if (cell_element.hasAttribute("displaylabel")) {
			if (cell_element.attribute("displaylabel").compare("false", Qt::CaseInsensitive) == 0) {
				display_label = false;
			}
		}
		int fontsize;
		if (QET::attributeIsAnInteger(cell_element, "fontsize", &fontsize)) {
			font_size = fontsize;
		} else {
			font_size = -1;
		}
		
		// horizontal and vertical alignments
		alignment = 0;
		
		QString halignment = cell_element.attribute("align", "left");
		if (halignment == "right") alignment |= Qt::AlignRight;
		else if (halignment == "center") alignment |= Qt::AlignHCenter;
		else alignment |= Qt::AlignLeft;
		
		QString valignment = cell_element.attribute("valign", "center");
		if (valignment == "bottom") alignment |= Qt::AlignBottom;
		else if (valignment == "top") alignment |= Qt::AlignTop;
		else alignment |= Qt::AlignVCenter;
		
		// horizontal text adjustment
		hadjust = cell_element.attribute("hadjust", "true") == "true";
	}
}

/**
	@param xml_element XML element to which cell content will be exported
*/
void TitleBlockCell::saveContentToXml(QDomElement &cell_elmt) {
	cell_elmt.setAttribute("name", value_name);
	
	if (type() == TitleBlockCell::EmptyCell) {
		cell_elmt.setTagName("empty");
	} else if (type() == TitleBlockCell::LogoCell) {
		cell_elmt.setTagName("logo");
		cell_elmt.setAttribute("resource", logo_reference);
	} else {
		cell_elmt.setTagName("field");
		
		QDomDocument parent_document = cell_elmt.ownerDocument();
		
		QHash<QString, QString> names_options;
		names_options["TagName"] = "translation";
		names_options["ParentTagName"] = "value";
		cell_elmt.appendChild(value.toXml(parent_document, names_options));
		names_options["ParentTagName"] = "label";
		cell_elmt.appendChild(label.toXml(parent_document, names_options));
		
		cell_elmt.setAttribute("displaylabel", display_label ? "true" : "false");
		if (font_size != -1) {
			cell_elmt.setAttribute("fontsize", font_size);
		}
		
		if (alignment & Qt::AlignRight) {
			cell_elmt.setAttribute("align", "right");
		} else if (alignment & Qt::AlignHCenter) {
			cell_elmt.setAttribute("align", "center");
		} else {
			cell_elmt.setAttribute("align", "left");
		}
		
		if (alignment & Qt::AlignBottom) {
			cell_elmt.setAttribute("valign", "bottom");
		} else if (alignment & Qt::AlignTop) {
			cell_elmt.setAttribute("valign", "top");
		} else {
			cell_elmt.setAttribute("valign", "center");
		}
		
		if (hadjust) cell_elmt.setAttribute("hadjust", "true");
	}
}
