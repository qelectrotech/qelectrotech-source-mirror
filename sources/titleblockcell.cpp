#include "titleblockcell.h"
#include "titleblocktemplate.h"
/**
	Constructor
*/
TitleBlockCell::TitleBlockCell() {
	cell_type = TitleBlockCell::EmptyCell;
	num_row = num_col = -1;
	row_span = col_span = 0;
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
		return(QObject::tr("type", "cell property human name"));
	} else if (attribute == "name") {
		return(QObject::tr("nom", "cell property human name"));
	} else if (attribute == "logo") {
		return(QObject::tr("logo", "cell property human name"));
	} else if (attribute == "label") {
		return(QObject::tr("label", "cell property human name"));
	} else if (attribute == "displaylabel") {
		return(QObject::tr("affichage du label", "cell property human name"));
	} else if (attribute == "value") {
		return(QObject::tr("valeur affichée", "cell property human name"));
	} else if (attribute == "alignment") {
		return(QObject::tr("alignement du texte", "cell property human name"));
	} else if (attribute == "fontsize") {
		return(QObject::tr("taille du texte", "cell property human name"));
	} else if (attribute == "horizontal_adjust") {
		return(QObject::tr("ajustement horizontal", "cell property human name"));
	}
	return(QString());
}

/**
	@return true if this cell spans over other cells, false otherwise.
*/
bool TitleBlockCell::spans() const {
	return(row_span || col_span);
}
