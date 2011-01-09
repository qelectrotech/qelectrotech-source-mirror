/*
	Copyright 2006-2010 Xavier Guerrin
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
#include "titleblocktemplate.h"
#include "qet.h"
#include "qetapp.h"

/**
	Constructor
	@param parent parent QObject
*/
TitleBlockTemplate::TitleBlockTemplate(QObject *parent) :
	QObject(parent)
{
}

/**
	Destructor
*/
TitleBlockTemplate::~TitleBlockTemplate() {
	loadLogos(QDomElement(), true);
}

/**
	@param filepath A file path to read the template from.
	@return true if the reading succeeds, false otherwise.
*/
bool TitleBlockTemplate::loadFromXmlFile(const QString &filepath) {
	// opens the file
	QFile template_file(filepath);
	if (!template_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
	    return(false);
	}
	
	// parses its content as XML
	bool xml_parsing = xml_description_.setContent(&template_file);
	if (!xml_parsing) {
		return(false);
	}
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
	qDebug() << Q_FUNC_INFO << filepath << "opened";
#endif
	return(loadFromXmlElement(xml_description_.documentElement()));
}

/**
	@param xml_element An XML document to read the template from.
	@return true if the reading succeeds, false otherwise.
*/
bool TitleBlockTemplate::loadFromXmlElement(const QDomElement &xml_element) {
	// we expect the XML element to be an <titleblocktemplate>
	if (xml_element.tagName() != "titleblocktemplate") {
		return(false);
	}
	if (!xml_element.hasAttribute("name")) {
		return(false);
	}
	name_ = xml_element.attribute("name");
	
	loadLogos(xml_element, true);
	loadGrid(xml_element);
	return(true);
}

/**
	Imports the logos from a given XML titleblock template.
	@param xml_element An XML element representing an titleblock template.
	@param reset true to delete all previously known logos before, false
	otherwise.
	@return true if the reading succeeds, false otherwise.
*/
bool TitleBlockTemplate::loadLogos(const QDomElement &xml_element, bool reset) {
	if (reset) {
		qDeleteAll(vector_logos_.begin(), vector_logos_.end());
		vector_logos_.clear();
		qDeleteAll(bitmap_logos_.begin(), bitmap_logos_.end());
		bitmap_logos_.clear();
	}
	
	// we look for //logos/logo elements
	for (QDomNode n = xml_element.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
		if (n.isElement() && n.toElement().tagName() == "logos") {
			for (QDomNode p = n.firstChild() ; !p.isNull() ; p = p.nextSibling()) {
				if (p.isElement() && p.toElement().tagName() == "logo") {
					loadLogo(p.toElement());
				}
			}
		}
	}
	
	return(true);
}

/**
	Imports the logo from a given XML logo description.
	@param xml_element An XML element representing a logo within an titleblock
	template.
	@return true if the reading succeeds, false otherwise.
*/
bool TitleBlockTemplate::loadLogo(const QDomElement &xml_element) {
	// we require a name
	if (!xml_element.hasAttribute("name")) {
		return(false);
	}
	QString logo_name    = xml_element.attribute("name");
	QString logo_type    = xml_element.attribute("type", "png");
	QString logo_storage = xml_element.attribute("storage", "base64");
	
	// Both QSvgRenderer and QPixmap read their data from a QByteArray, so
	// we convert the available data to that format.
	QByteArray logo_data;
	if (logo_storage == "xml") {
		// only svg uses xml storage
		QDomNodeList svg_nodes = xml_element.elementsByTagName("svg");
		if (svg_nodes.isEmpty()) {
			return(false);
		}
		QDomElement svg_element = svg_nodes.at(0).toElement();
		QTextStream xml_to_byte_array(&logo_data);
		svg_element.save(xml_to_byte_array, 0);
	} else if (logo_storage == "base64") {
		logo_data = QByteArray::fromBase64(xml_element.text().toAscii());
	} else {
		return(false);
	}
	
	// we can now create our image object from the byte array
	if (logo_type == "svg") {
		// SVG format is handled by the QSvgRenderer class
		QSvgRenderer *svg = new QSvgRenderer(logo_data);
		vector_logos_.insert(logo_name, svg);
		
		/*QSvgWidget *test_svgwidget = new QSvgWidget();
		test_svgwidget -> load(logo_data);
		test_svgwidget -> show();*/
	} else {
		// bitmap formats are handled by the QPixmap class
		QPixmap *logo_pixmap = new QPixmap();
		logo_pixmap -> loadFromData(logo_data);
		if (!logo_pixmap -> width() || !logo_pixmap -> height()) {
			return(false);
		}
		bitmap_logos_.insert(logo_name, logo_pixmap);
		
		/*QLabel *test_label = new QLabel();
		test_label -> setPixmap(*logo_pixmap);
		test_label -> show();*/
	}
	
	return(true);
}

/**
	Imports the grid from a given XML titleblock template.
	@param xml_element An XML element representing an titleblock template.
	@return true if the reading succeeds, false otherwise.
*/
bool TitleBlockTemplate::loadGrid(const QDomElement &xml_element) {
	// we parse the first available "grid" XML element
	QDomElement grid_element;
	for (QDomNode n = xml_element.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
		if (n.isElement() && n.toElement().tagName() == "grid") {
			grid_element = n.toElement();
			break;
		}
	}
	
	if (!grid_element.hasAttribute("rows") || !grid_element.hasAttribute("cols")) {
		return(false);
	}
	
	parseRows(grid_element.attribute("rows"));
	parseColumns(grid_element.attribute("cols"));
	loadCells(grid_element);
	return(true);
}

/**
	Parses the rows heights
	@param rows_string A string describing the rows heights of the titleblock
*/
void TitleBlockTemplate::parseRows(const QString &rows_string) {
	rows_heights_.clear();
	// parse the rows attribute: we expect a serie of absolute heights
	QRegExp row_size_format("^([0-9]+)(?:px)?$", Qt::CaseInsensitive);
	bool conv_ok;
	
	QStringList rows_descriptions = rows_string.split(QChar(';'), QString::SkipEmptyParts);
	foreach (QString rows_description, rows_descriptions) {
		if (row_size_format.exactMatch(rows_description)) {
			int row_size = row_size_format.capturedTexts().at(1).toInt(&conv_ok);
			if (conv_ok) rows_heights_ << row_size;
		}
	}
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
	qDebug() << Q_FUNC_INFO << "Rows heights:" << rows_heights_;
#endif
}

/**
	Parses the columns widths
	@param cols_string A string describing the columns widths of the titleblock
*/
void TitleBlockTemplate::parseColumns(const QString &cols_string) {
	columns_width_.clear();
	// parse the cols attribute: we expect a serie of absolute or relative widths
	QRegExp abs_col_size_format("^([0-9]+)(?:px)?$", Qt::CaseInsensitive);
	QRegExp rel_col_size_format("^([rt])([0-9]+)%$", Qt::CaseInsensitive);
	bool conv_ok;
	
	QStringList cols_descriptions = cols_string.split(QChar(';'), QString::SkipEmptyParts);
	foreach (QString cols_description, cols_descriptions) {
		if (abs_col_size_format.exactMatch(cols_description)) {
			int col_size = abs_col_size_format.capturedTexts().at(1).toInt(&conv_ok);
			if (conv_ok) columns_width_ << TitleBlockColDimension(col_size, QET::Absolute);
		} else if (rel_col_size_format.exactMatch(cols_description)) {
			int col_size = rel_col_size_format.capturedTexts().at(2).toInt(&conv_ok);
			QET::TitleBlockColumnLength col_type = rel_col_size_format.capturedTexts().at(1) == "t" ? QET::RelativeToTotalLength : QET::RelativeToRemainingLength;
			if (conv_ok) columns_width_ << TitleBlockColDimension(col_size, col_type );
		}
	}
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
	foreach (TitleBlockColDimension icd, columns_width_) {
		qDebug() << Q_FUNC_INFO << QString("%1 [%2]").arg(icd.value).arg(QET::titleBlockColumnLengthToString(icd.type));
	}
#endif
}

/**
	Analyze an XML element, looking for grid cells. The grid cells are checked
	and stored in this object.
	@param xml_element XML element to analyze
*/
bool TitleBlockTemplate::loadCells(const QDomElement &xml_element) {
	initCells();
	// we are interested by the "logo" and "field" elements
	QDomElement grid_element;
	for (QDomNode n = xml_element.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
		if (!n.isElement()) continue;
		QDomElement cell_element = n.toElement();
		if (cell_element.tagName() == "field" || cell_element.tagName() == "logo") {
			TitleBlockCell *loaded_cell;
			if (!checkCell(cell_element, &loaded_cell)) continue;
			
			if (cell_element.tagName() == "logo") {
				if (cell_element.hasAttribute("resource") && !cell_element.attribute("resource").isEmpty()) {
					loaded_cell -> logo_reference = cell_element.attribute("resource");
				}
			} else if (cell_element.tagName() == "field") {
				if (cell_element.hasAttribute("name") && !cell_element.attribute("name").isEmpty()) {
					loaded_cell -> value_name = cell_element.attribute("name");
				}
				if (cell_element.hasAttribute("value") && !cell_element.attribute("value").isEmpty()) {
					loaded_cell -> value = cell_element.attribute("value");
				}
				if (cell_element.hasAttribute("label") && !cell_element.attribute("label").isEmpty()) {
					loaded_cell -> label = cell_element.attribute("label");
				}
				if (cell_element.hasAttribute("displaylabel") && cell_element.attribute("displaylabel").compare("false", Qt::CaseInsensitive) == 0) {
					loaded_cell -> display_label = false;
				}
				int fontsize;
				if (QET::attributeIsAnInteger(cell_element, "fontsize", &fontsize)) {
					loaded_cell -> font_size = fontsize;
				} else {
					loaded_cell -> font_size = -1;
				}
				
				// horiwontal and vertical alignments
				loaded_cell -> alignment = 0;
				
				QString halignment = cell_element.attribute("align", "left");
				if (halignment == "right") loaded_cell -> alignment |= Qt::AlignRight;
				else if (halignment == "center") loaded_cell -> alignment |= Qt::AlignHCenter;
				else loaded_cell -> alignment |= Qt::AlignLeft;
				
				QString valignment = cell_element.attribute("valign", "center");
				if (halignment == "bottom") loaded_cell -> alignment |= Qt::AlignBottom;
				else if (halignment == "top") loaded_cell -> alignment |= Qt::AlignTop;
				else loaded_cell -> alignment |= Qt::AlignVCenter;
			}
		}
	}
	
	return(true);
}

/**
	@param xml_element XML element representing a cell, i.e. either an titleblock
	logo or an titleblock field.
	@param titleblock_cell_ptr Pointer to an TitleBlockCell object pointer - if non-zero and if
	this method returns true, will be filled with the created TitleBlockCell
	@return TRUE if the cell appears to be ok, FALSE otherwise
*/
bool TitleBlockTemplate::checkCell(const QDomElement &xml_element, TitleBlockCell **titleblock_cell_ptr) {
	int col_count = columns_width_.count(), row_count = rows_heights_.count();
	
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
	qDebug() << Q_FUNC_INFO << "begin" << row_count << col_count;
#endif
	
	int row_num, col_num, row_span, col_span;
	bool has_row_span = false;
	bool has_col_span = false;
	row_num = col_num = -1;
	row_span = col_span = 0;
	
	// parse the row and col attributes
	if (!QET::attributeIsAnInteger(xml_element, "row", &row_num) || row_num < 0 || row_num >= row_count) {
		return(false);
	}
	if (!QET::attributeIsAnInteger(xml_element, "col", &col_num) || col_num < 0 || col_num >= col_count) {
		return(false);
	}
	
	// check whether the target cell can be used or not
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
	qDebug() << Q_FUNC_INFO << "cell access" << col_num << row_num;
#endif
	TitleBlockCell *cell_ptr = &(cells_[col_num][row_num]);
	if (!cell_ptr -> is_null || cell_ptr -> spanner_cell) {
		return(false);
	}
	
	// parse the rowspan and colspan attributes
	if (QET::attributeIsAnInteger(xml_element, "rowspan", &row_span) && row_span > 0) {
		if (row_num + row_span >= row_count) row_span = row_count - 1 - row_num;
		has_row_span = true;
	}
	
	if (QET::attributeIsAnInteger(xml_element, "colspan", &col_span) && col_span > 0) {
		if (col_num + col_span >= col_count) col_span = col_count - 1 - col_num;
		has_col_span = true;
	}
	
	// check if we can span on the required area
	if (has_row_span || has_col_span) {
		for (int i = col_num ; i <= col_num + col_span ; ++ i) {
			for (int j = row_num ; j <= row_num + row_span ; ++ j) {
				if (i == col_num && j == row_num) continue;
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
				qDebug() << Q_FUNC_INFO << "span check" << i << j;
#endif
				TitleBlockCell *current_cell = &(cells_[i][j]);
				if (!current_cell -> is_null || current_cell -> spanner_cell) {
					return(false);
				}
			}
		}
	}
	
	// at this point, the cell is ok - we fill the adequate cells in the matrix
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
	qDebug() << Q_FUNC_INFO << "cell writing";
#endif
	cell_ptr -> num_row = row_num;
	cell_ptr -> num_col = col_num;
	if (has_row_span) cell_ptr -> row_span = row_span;
	if (has_col_span) cell_ptr -> col_span = col_span;
	cell_ptr -> is_null = false;
	if (titleblock_cell_ptr) *titleblock_cell_ptr = cell_ptr;
	
	if (has_row_span || has_col_span) {
		for (int i = col_num ; i <= col_num + col_span ; ++ i) {
			for (int j = row_num ; j <= row_num + row_span ; ++ j) {
				if (i == col_num && j == row_num) continue;
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
				qDebug() << Q_FUNC_INFO << "span cells writing" << i << j;
#endif
				TitleBlockCell *current_cell = &(cells_[i][j]);
				current_cell -> num_row = j;
				current_cell -> num_col = i;
				current_cell -> is_null = false;
				current_cell -> spanner_cell = cell_ptr;
			}
		}
	}
	
	return(true);
}

/**
	Initializes the internal cells grid with the row and column counts.
	Note that this method does nothing if one of the internal lists
	columns_width_ and rows_heights_ is empty.
*/
void TitleBlockTemplate::initCells() {
	if (columns_width_.count() < 1 || rows_heights_.count() < 1) return;
	
	cells_.resize(columns_width_.count());
	int row_count = rows_heights_.count();
	for (int i = 0 ; i < columns_width_.count() ; ++ i) {
		cells_[i].resize(row_count);
		// ensure every cell is a null cell
		for (int j = 0 ; j < row_count ; ++ j) {
			cells_[i][j] = TitleBlockCell();
		}
	}
	
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
	qDebug() << Q_FUNC_INFO << toString();
#endif
}

/**
	@return A string representing the titleblock template
	@see TitleBlockCell::toString()
*/
QString TitleBlockTemplate::toString() const {
	QString str = "\n";
	for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
		for (int i = 0 ; i < columns_width_.count() ; ++ i) {
			str += cells_[i][j].toString() + "    ";
		}
		str += "\n";
	}
	return(str);
}

/**
	@return the name of this template
*/
QString TitleBlockTemplate::name() const {
	return(name_);
}

/**
	@param total_width The total width of the titleblock to render
	@return the list of the columns widths for this rendering
*/
QList<int> TitleBlockTemplate::columnsWidth(int total_width) const {
	if (total_width < 0) return(QList<int>());
	
	// we first iter to determine the absolute and total-width-related widths
	QVector<int> final_widths(columns_width_.count());
	int abs_widths_sum = 0;
	
	for (int i = 0 ; i < columns_width_.count() ; ++ i) {
		TitleBlockColDimension icd = columns_width_.at(i);
		if (icd.type == QET::Absolute) {
			abs_widths_sum += icd.value;
			final_widths[i] = icd.value;
		} else if (icd.type == QET::RelativeToTotalLength) {
			int abs_value = int(total_width * icd.value / 100);
			abs_widths_sum += abs_value;
			final_widths[i] = abs_value;
		}
	}
	
	// we can now deduce the remaining width
	int remaining_width = total_width - abs_widths_sum;
	
	// we do a second iteration to build the final widths list
	for (int i = 0 ; i < columns_width_.count() ; ++ i) {
		TitleBlockColDimension icd = columns_width_.at(i);
		if (icd.type == QET::RelativeToRemainingLength) {
			final_widths[i] = int(remaining_width * icd.value / 100);
		}
	}
	return(final_widths.toList());
}

int TitleBlockTemplate::height() const {
	int height = 0;
	foreach(int row_height, rows_heights_) {
		height += row_height;
	}
	return(height);
}

/**
	Render the titleblock.
	@param painter Painter to use to render the titleblock
	@param diagram_context Diagram context to use to generate the titleblock strings
	@param titleblock_width Width of the titleblock to render
*/
void TitleBlockTemplate::render(QPainter &painter, const DiagramContext &diagram_context, int titleblock_width) const {
	QList<int> widths = columnsWidth(titleblock_width);
	int titleblock_height = height();
	
	// prepare the QPainter
	painter.setPen(Qt::black);
	painter.setBrush(Qt::white);
	
	// draw the titleblock border
	painter.drawRect(QRect(0, 0, titleblock_width, titleblock_height));
	
	// run through each inidividual cell
	for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
		for (int i = 0 ; i < columns_width_.count() ; ++ i) {
			if (cells_[i][j].spanner_cell || cells_[i][j].is_null) continue;
			
			// calculate the border rect of the current cell
			int x = lengthRange(0, cells_[i][j].num_col, widths); 
			int y = lengthRange(0, cells_[i][j].num_row, rows_heights_);
			int w = lengthRange(cells_[i][j].num_col, cells_[i][j].num_col + 1 + cells_[i][j].col_span, widths);
			int h = lengthRange(cells_[i][j].num_row, cells_[i][j].num_row + 1 + cells_[i][j].row_span, rows_heights_);
			QRect cell_rect(x, y, w, h);
			
			// draw the border rect of the current cell
			painter.drawRect(cell_rect);
			
			// render the inner content of the current cell
			if (!cells_[i][j].logo_reference.isEmpty()) {
				// the current cell appear to be a logo - we first look for the
				// logo reference in our vector logos list, since they offer a
				// potentially better (or, at least, not resolution-limited) rendering
				if (vector_logos_.contains(cells_[i][j].logo_reference)) {
					vector_logos_[cells_[i][j].logo_reference] -> render(&painter, cell_rect);
				} else if (bitmap_logos_.contains(cells_[i][j].logo_reference)) {
					painter.drawPixmap(cell_rect, *(bitmap_logos_[cells_[i][j].logo_reference]));
				}
			} else {
				painter.setFont(cells_[i][j].font_size == -1 ? QETApp::diagramTextsFont() : QETApp::diagramTextsFont(cells_[i][j].font_size));
				painter.drawText(cell_rect, cells_[i][j].alignment, finalTextForCell(cells_[i][j], diagram_context));
			}
			
			// draw again the border rect of the current cell, without the brush this time
			painter.setBrush(Qt::NoBrush);
			painter.drawRect(cell_rect);
		}
	}
}

/**
	@param cell A cell from this template
	@param diagram_context Diagram context to use to generate the final text for the given cell
	@return the final text that has to be drawn in the given cell
*/
QString TitleBlockTemplate::finalTextForCell(const TitleBlockCell &cell, const DiagramContext &diagram_context) const {
	QString cell_text = cell.value;
	
	foreach (QString key, diagram_context.keys()) {
		cell_text.replace("%{" + key + "}", diagram_context[key].toString());
		cell_text.replace("%" + key,        diagram_context[key].toString());
	}
	if (cell.display_label && !cell.label.isEmpty()) {
		cell_text = QString(tr(" %1 : %2", "titleblock content - please let the blank space at the beginning")).arg(cell.label).arg(cell_text);
	} else {
		cell_text = QString(tr(" %1")).arg(cell_text);
	}
	return(cell_text);
}

/**
	@return the width between two borders
	@param start start border number
	@param end end border number
*/
int TitleBlockTemplate::lengthRange(int start, int end, const QList<int> &lengths_list) const {
	if (start > end || start >= lengths_list.count() || end > lengths_list.count()) {
		qDebug() << Q_FUNC_INFO << "wont use" << start << "and" << end;
		return(0);
	}
	
	int length = 0;
	for (int i = start ; i < end ; ++i) {
		length += lengths_list[i];
	}
	return(length);
}



/**
	Constructor
*/
TitleBlockCell::TitleBlockCell() {
	num_row = num_col = -1;
	row_span = col_span = 0;
	display_label = is_null = true;
	spanner_cell = 0;
}

/**
	@return A string representing the titleblock cell
*/
QString TitleBlockCell::toString() const {
	if (is_null) return("TitleBlockCell{null}");
	QString span_desc = (row_span > 0 || col_span > 0) ? QString("+%3,%4").arg(row_span).arg(col_span) : QET::pointerString(spanner_cell);
	QString base_desc = QString("TitleBlockCell{ [%1, %2] %3 }").arg(num_row).arg(num_col).arg(span_desc);
	return(base_desc);
}
