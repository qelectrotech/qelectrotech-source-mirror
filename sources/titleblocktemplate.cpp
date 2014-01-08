/*
	Copyright 2006-2012 Xavier Guerrin
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
#include "nameslist.h"
#include "createdxf.h"
// uncomment the line below to get more debug information
//#define TITLEBLOCK_TEMPLATE_DEBUG

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
	qDeleteAll(registered_cells_);
}

/**
	Create a new cell and associate it with this template, which means that it
	will be deleted when this template is destroyed.
	@param existing_cell (optional) An existing cell that will be copied
	@return A pointer to the newly created cell
*/
TitleBlockCell *TitleBlockTemplate::createCell(const TitleBlockCell *existing_cell) {
	TitleBlockCell *new_cell = existing_cell ? new TitleBlockCell(*existing_cell) : new TitleBlockCell();
	registered_cells_ << new_cell;
	return(new_cell);
}

/**
	@param count Number of cells expected in the list
	@return a list containing count newly created (and registered) cells
	@see createCell()
*/
QList<TitleBlockCell *> TitleBlockTemplate::createCellsList(int count) {
	QList<TitleBlockCell *> new_list;
	for (int i = 0 ; i < count ; ++ i) new_list << createCell();
	return(new_list);
}

/**
	@param cell An existing cell
	@return The font that should be used to render this cell according to its properties.
*/
QFont TitleBlockTemplate::fontForCell(const TitleBlockCell &cell) {
	return(QETApp::diagramTextsFont(cell.font_size));
}

/**
	Load a titleblock template from an XML file.
	@param filepath A file path to read the template from.
	@return true if the reading succeeds, false otherwise.
*/
bool TitleBlockTemplate::loadFromXmlFile(const QString &filepath) {
	// open the file
	QFile template_file(filepath);
	if (!template_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return(false);
	}
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
	qDebug() << Q_FUNC_INFO << filepath << "opened";
#endif
	
	// parse its content as XML
	QDomDocument xml_doc;
	bool xml_parsing = xml_doc.setContent(&template_file);
	if (!xml_parsing) {
		return(false);
	}
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
	qDebug() << Q_FUNC_INFO << filepath << "opened and parsed";
#endif
	return(loadFromXmlElement(xml_doc.documentElement()));
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
	
	loadInformation(xml_element);
	loadLogos(xml_element, true);
	loadGrid(xml_element);
	
	return(true);
}

/**
	Save the title block template into an XML file.
	@param filepath The file path this title block template should be saved to.
	@return true if the operation succeeds, false otherwise
*/
bool TitleBlockTemplate::saveToXmlFile(const QString &filepath) {
	if (filepath.isEmpty()) return(false);
	
	// generate the XML document
	QDomDocument doc;
	QDomElement e = doc.createElement("root");
	bool saving = saveToXmlElement(e);
	if (!saving) return(false);
	doc.appendChild(e);
	
	return(QET::writeXmlFile(doc, filepath));
}

/**
	Save the title block template as XML.
	@param xml_element The XML element this title block template should be saved to.
	@return true if the export succeeds, false otherwise
*/
bool TitleBlockTemplate::saveToXmlElement(QDomElement &xml_element) const {
	// we are supposed to have at least a name
	if (name_.isEmpty()) return(false);
	
	xml_element.setTagName("titleblocktemplate");
	xml_element.setAttribute("name", name_);
	saveInformation(xml_element);
	saveLogos(xml_element);
	saveGrid(xml_element);
	return(true);
}

/**
	@param xml_element Parent XML element to be used when exporting \a cell
	@param cell Cell to export
*/
void TitleBlockTemplate::exportCellToXml(TitleBlockCell *cell, QDomElement &xml_element) const {
	saveCell(cell, xml_element, true);
}

/**
	@return a deep copy of the current title block template (i.e. title block
	cells are duplicated too and associated with their parent template).
*/
TitleBlockTemplate *TitleBlockTemplate::clone() const {
	TitleBlockTemplate *copy = new TitleBlockTemplate();
	copy -> name_ = name_;
	copy -> information_ = information_;
	
	// this does not really duplicates pixmaps, only the objects that hold a key to the implicitly shared pixmaps
	foreach (QString logo_key, bitmap_logos_.keys()) {
		copy -> bitmap_logos_[logo_key] = QPixmap(bitmap_logos_[logo_key]);
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
		qDebug() << Q_FUNC_INFO << "copying " << bitmap_logos_[logo_key] -> cacheKey() << "to" << copy -> bitmap_logos_[logo_key] -> cacheKey();
#endif
	}
	
	// we have to create new QSvgRenderer objects from the data (no copy constructor)
	foreach (QString logo_key, vector_logos_.keys()) {
		copy -> vector_logos_[logo_key] = new QSvgRenderer(data_logos_[logo_key]);
	}
	
	copy -> data_logos_    = data_logos_;
	copy -> storage_logos_ = storage_logos_;
	copy -> type_logos_    = type_logos_;
	copy -> rows_heights_  = rows_heights_;
	copy -> columns_width_ = columns_width_;
	
	// copy cells basically
	copy -> cells_ = cells_;
	for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
		for (int i = 0 ; i < columns_width_.count() ; ++ i) {
			copy -> cells_[i][j] = copy -> createCell(cells_[i][j]);
		}
	}
	
	// ensure the copy has no spanner_cell attribute pointing to a cell from the original object
	for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
		for (int i = 0 ; i < columns_width_.count() ; ++ i) {
			TitleBlockCell *current_cell = copy -> cells_[i][j];
			if (TitleBlockCell *original_cell = current_cell -> spanner_cell) {
				int original_cell_row = original_cell -> num_row;
				int original_cell_col = original_cell -> num_col;
				TitleBlockCell *copy_cell = copy -> cells_[original_cell_col][original_cell_row];
				current_cell -> spanner_cell = copy_cell;
			}
		}
	}
	
	return(copy);
}

/**
	Import text informations from a given XML title block template.
*/
void TitleBlockTemplate::loadInformation(const QDomElement &xml_element) {
	for (QDomNode n = xml_element.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
		if (n.isElement() && n.toElement().tagName() == "information") {
			setInformation(n.toElement().text());
		}
	}
}

/**
	Import the logos from a given XML titleblock template.
	@param xml_element An XML element representing an titleblock template.
	@param reset true to delete all previously known logos before, false
	otherwise.
	@return true if the reading succeeds, false otherwise.
*/
bool TitleBlockTemplate::loadLogos(const QDomElement &xml_element, bool reset) {
	if (reset) {
		qDeleteAll(vector_logos_.begin(), vector_logos_.end());
		vector_logos_.clear();
		
		// Note: QPixmap are only a key to access the implicitly shared pixmap
		bitmap_logos_.clear();
		
		data_logos_.clear();
		storage_logos_.clear();
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
	Import the logo from a given XML logo description.
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
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
	qDebug() << Q_FUNC_INFO << logo_name << logo_type << logo_storage;
#endif
	addLogo(logo_name, &logo_data, logo_type, logo_storage);
	
	return(true);
}

/**
	Import the grid from a given XML titleblock template.
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
	initCells();
	loadCells(grid_element);
	applyRowColNums();
	applyCellSpans();
	return(true);
}

/**
	Parse the rows heights
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
	Parse the columns widths
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
			if (conv_ok) columns_width_ << TitleBlockDimension(col_size, QET::Absolute);
		} else if (rel_col_size_format.exactMatch(cols_description)) {
			int col_size = rel_col_size_format.capturedTexts().at(2).toInt(&conv_ok);
			QET::TitleBlockColumnLength col_type = rel_col_size_format.capturedTexts().at(1) == "t" ? QET::RelativeToTotalLength : QET::RelativeToRemainingLength;
			if (conv_ok) columns_width_ << TitleBlockDimension(col_size, col_type );
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
	@return systematically true
*/
bool TitleBlockTemplate::loadCells(const QDomElement &xml_element) {
	// we are interested by the "logo" and "field" elements
	QDomElement grid_element;
	for (QDomNode n = xml_element.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
		if (!n.isElement()) continue;
		QDomElement cell_element = n.toElement();
		if (cell_element.tagName() == "field" || cell_element.tagName() == "logo") {
			loadCell(cell_element);
		}
	}
	return(true);
}

/**
	Load a cell into this template.
	@param cell_element XML element describing a cell within a title block template
*/
void TitleBlockTemplate::loadCell(const QDomElement &cell_element) {
	TitleBlockCell *loaded_cell;
	if (!checkCell(cell_element, &loaded_cell)) return;
	loaded_cell -> loadContentFromXml(cell_element);
}

/**
	Export this template's extra information.
	@param xml_element XML element under which extra informations will be attached
*/
void TitleBlockTemplate::saveInformation(QDomElement &xml_element) const {
	QDomNode information_text_node = xml_element.ownerDocument().createTextNode(information());
	
	QDomElement information_element = xml_element.ownerDocument().createElement("information");
	information_element.appendChild(information_text_node);
	xml_element.appendChild(information_element);
}

/**
	Export this template's logos as XML
	@param xml_element XML Element under which the \<logos\> element will be attached
*/
void TitleBlockTemplate::saveLogos(QDomElement &xml_element) const {
	QDomElement logos_element = xml_element.ownerDocument().createElement("logos");
	foreach(QString logo_name, type_logos_.keys()) {
		QDomElement logo_element = xml_element.ownerDocument().createElement("logo");
		saveLogo(logo_name, logo_element);
		logos_element.appendChild(logo_element);
	}
	xml_element.appendChild(logos_element);
}

/**
	Export a specific logo as XML
	@param logo_name Name of the logo to be exported
	@param xml_element XML element in which the logo will be exported
*/
void TitleBlockTemplate::saveLogo(const QString &logo_name, QDomElement &xml_element) const {
	if (!type_logos_.contains(logo_name)) return;
	
	xml_element.setAttribute("name", logo_name);
	xml_element.setAttribute("type", type_logos_[logo_name]);
	xml_element.setAttribute("storage", storage_logos_[logo_name]);
	
	if (storage_logos_[logo_name] == "xml" && type_logos_[logo_name] == "svg") {
		QDomDocument svg_logo;
		svg_logo.setContent(data_logos_[logo_name]);
		QDomNode svg_logo_element = xml_element.ownerDocument().importNode(svg_logo.documentElement(), true);
		xml_element.appendChild(svg_logo_element.toElement());
	} else if (storage_logos_[logo_name] == "base64") {
		QDomText base64_logo = xml_element.ownerDocument().createTextNode(data_logos_[logo_name].toBase64());
		xml_element.appendChild(base64_logo);
	}
}

/**
	Export this template's cells grid as XML
	@param xml_element XML element under which the \<grid\> element will be attached
*/
void TitleBlockTemplate::saveGrid(QDomElement &xml_element) const {
	QDomElement grid_element = xml_element.ownerDocument().createElement("grid");
	
	QString rows_attr, cols_attr;
	foreach(int row_height, rows_heights_) rows_attr += QString("%1;").arg(row_height);
	foreach(TitleBlockDimension col_width, columns_width_) cols_attr += col_width.toShortString();
	grid_element.setAttribute("rows", rows_attr);
	grid_element.setAttribute("cols", cols_attr);
	
	saveCells(grid_element);
	
	xml_element.appendChild(grid_element);
}

/**
	Export this template's cells as XML (without the grid-related information, usch as rows and cols)
	@param xml_element XML element under which the \<cell\> elements will be attached
*/
void TitleBlockTemplate::saveCells(QDomElement &xml_element) const {
	for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
		for (int i = 0 ; i < columns_width_.count() ; ++ i) {
			if (cells_[i][j] -> cell_type != TitleBlockCell::EmptyCell) {
				saveCell(cells_[i][j], xml_element);
			}
		}
	}
}

/**
	Export a specific cell as XML
	@param cell Cell to be exported as XML
	@param xml_element XML element under which the \<cell\> element will be attached
	@param save_empty If true, the cell will be saved even if it is an empty one
*/
void TitleBlockTemplate::saveCell(TitleBlockCell *cell, QDomElement &xml_element, bool save_empty) const {
	if (!cell) return;
	if (cell -> spanner_cell) return;
	if (!save_empty && cell -> cell_type == TitleBlockCell::EmptyCell) return;
	
	
	QDomElement cell_elmt = xml_element.ownerDocument().createElement("cell");
	xml_element.appendChild(cell_elmt);
	
	// save information dependent from this template
	cell_elmt.setAttribute("row", cell -> num_row);
	cell_elmt.setAttribute("col", cell -> num_col);
	if (cell -> row_span) cell_elmt.setAttribute("rowspan", cell -> row_span);
	if (cell -> col_span) cell_elmt.setAttribute("colspan", cell -> col_span);
	
	// save other information
	cell -> saveContentToXml(cell_elmt);
}

/**
	Load the essential attributes of a cell: row and column indices and spans.
	@param xml_element XML element representing a cell, i.e. either an titleblock
	logo or an titleblock field.
	@param titleblock_cell_ptr Pointer to a TitleBlockCell object pointer - if non-zero and if
	this method returns true, will be filled with the created TitleBlockCell
	@return TRUE if the cell appears to be ok, FALSE otherwise
*/
bool TitleBlockTemplate::checkCell(const QDomElement &xml_element, TitleBlockCell **titleblock_cell_ptr) {
	int col_count = columns_width_.count(), row_count = rows_heights_.count();
	
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
	qDebug() << Q_FUNC_INFO << "begin" << row_count << col_count;
#endif
	
	int row_num, col_num, row_span, col_span;
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
	TitleBlockCell *cell_ptr = cells_[col_num][row_num];
	if (cell_ptr -> cell_type != TitleBlockCell::EmptyCell || cell_ptr -> spanner_cell) {
		return(false);
	}
	// ensure the num_row and num_col attributes are alright
	cell_ptr -> num_row = row_num;
	cell_ptr -> num_col = col_num;
	
	// parse the rowspan and colspan attributes
	if (QET::attributeIsAnInteger(xml_element, "rowspan", &row_span) && row_span > 0) {
		cell_ptr -> row_span = row_span;
	}
	
	if (QET::attributeIsAnInteger(xml_element, "colspan", &col_span) && col_span > 0) {
		cell_ptr -> col_span = col_span;
	}
	// these attributes are stored "as is" -- whether they can be applied directly or must be restricted will be checked later
	
	if (titleblock_cell_ptr) *titleblock_cell_ptr = cell_ptr;
	return(true);
}

/**
	Initialize the internal cells grid with the row and column counts.
	Note that this method does nothing if one of the internal lists
	columns_width_ and rows_heights_ is empty.
*/
void TitleBlockTemplate::initCells() {
	if (columns_width_.count() < 1 || rows_heights_.count() < 1) return;
	
	cells_.clear();
	qDeleteAll(registered_cells_);
	registered_cells_.clear();
	for (int i = 0 ; i < columns_width_.count() ; ++ i) {
		cells_ << createColumn();
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
			str += cells_[i][j] -> toString() + "    ";
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
	@return the information field attached to this template
*/
QString TitleBlockTemplate::information() const {
	return(information_);
}

/**
	@param info information to be attached to this template
*/
void TitleBlockTemplate::setInformation(const QString &info) {
	information_ = info;
}

/**
	@param i row index
	@return the height of the row at index i
*/
int TitleBlockTemplate::rowDimension(int i) {
	int index = (i == -1) ? rows_heights_.count() - 1 : i;
	if (index >= 0 && index < rows_heights_.count()) {
		return(rows_heights_.at(index));
	}
	return(-1);
}

/**
	Set the height of a row
	@param i row index
	@param dimension New height of the row at index i
*/
void TitleBlockTemplate::setRowDimension(int i, const TitleBlockDimension &dimension) {
	int index = (i == -1) ? rows_heights_.count() - 1 : i;
	if (index >= 0 || index < rows_heights_.count()) {
		rows_heights_[index] = dimension.value;
	}
}

/**
	@param i column index
	@return the width of the column at index i
*/
TitleBlockDimension TitleBlockTemplate::columnDimension(int i) {
	int index = (i == -1) ? columns_width_.count() - 1 : i;
	if (index >= 0 && index < columns_width_.count()) {
		return(columns_width_.at(index));
	}
	return(TitleBlockDimension(-1));
}

/**
	Set the width of a column
	@param i column index
	@param dimension New width of the column at index i
*/
void TitleBlockTemplate::setColumnDimension(int i, const TitleBlockDimension &dimension) {
	int index = (i == -1) ? columns_width_.count() - 1 : i;
	if (index >= 0 || index < columns_width_.count()) {
		columns_width_[index] = dimension;
	}
}

/**
	@return the number of columns in this template
*/
int TitleBlockTemplate::columnsCount() const {
	return(columns_width_.count());
}

/**
	@return the number of rows in this template
*/
int TitleBlockTemplate::rowsCount() const {
	return(rows_heights_.count());
}

/**
	@param total_width The total width of the titleblock to render
	@return the list of the columns widths for this rendering
*/
QList<int> TitleBlockTemplate::columnsWidth(int total_width) const {
	if (total_width < 0) return(QList<int>());
	
	// we first iter to determine the absolute and total-width-related widths
	QVector<int> final_widths(columns_width_.count());
	int abs_widths_sum = 0, rel_widths_sum = 0;
	QList<int> relative_columns;
	
	for (int i = 0 ; i < columns_width_.count() ; ++ i) {
		TitleBlockDimension icd = columns_width_.at(i);
		if (icd.type == QET::Absolute) {
			abs_widths_sum += icd.value;
			final_widths[i] = icd.value;
		} else if (icd.type == QET::RelativeToTotalLength) {
			int abs_value = qRound(total_width * icd.value / 100.0);
			relative_columns << i;
			abs_widths_sum += abs_value;
			final_widths[i] = abs_value;
		}
	}
	
	// we can now deduce the remaining width
	int remaining_width = total_width - abs_widths_sum;
	
	// we do a second iteration to build the final widths list
	for (int i = 0 ; i < columns_width_.count() ; ++ i) {
		TitleBlockDimension icd = columns_width_.at(i);
		if (icd.type == QET::RelativeToRemainingLength) {
			final_widths[i] = qRound(remaining_width * icd.value / 100.0);
			relative_columns << i;
			rel_widths_sum += final_widths[i];
		}
	}
	
	// Have we computed widths from percentage for relative columns?
	if (relative_columns.count()) {
		// Due to the rounding process, we may get a slight difference between the
		// sum of the columns widths and the total width.
		int difference = total_width - abs_widths_sum - rel_widths_sum;
	
		if (difference) {
			// We consider we should not attempt to compensate this difference if it is
			// under relative_columns_count * 0.5 (which means that each percent-based
			// columns can "bring" up to 0.5px of difference).
			qreal max_acceptable_difference = relative_columns.count() * 0.5;
			
			int share = difference > 0 ? 1 : -1;
			if (qAbs(difference) <= max_acceptable_difference) {
				while (difference) {
					foreach (int index, relative_columns) {
						final_widths[index] += share;
						difference -= share;
						if (!difference) break;
					}
				}
			}
		}
	}
	return(final_widths.toList());
}

/**
	@return the heights of all the rows in this template
*/
QList<int> TitleBlockTemplate::rowsHeights() const {
	return(rows_heights_);
}

/**
	@param a column type
	@return the count of \a type columns 
*/
int TitleBlockTemplate::columnTypeCount(QET::TitleBlockColumnLength type) {
	int count = 0;
	
	for (int i = 0 ; i < columns_width_.count() ; ++ i) {
		if (columns_width_.at(i).type == type) ++ count;
	}
	
	return(count);
}

/**
	@param a column type
	@return the sum of values attached to \a type columns 
*/
int TitleBlockTemplate::columnTypeTotal(QET::TitleBlockColumnLength type) {
	int total = 0;
	
	for (int i = 0 ; i < columns_width_.count() ; ++ i) {
		if (columns_width_.at(i).type == type) {
			total += columns_width_.at(i).value;
		}
	}
	
	return(total);
}

/**
	@return the minimum width for this template
*/
int TitleBlockTemplate::minimumWidth() {
	// Abbreviations: ABS: absolute, RTT: relative to total, RTR: relative to
	// remaining, TOT: total diagram/TBT width (variable).
	
	// Minimum size may be enforced by ABS and RTT widths:
	// TOT >= ((sum(REL)/100)*TOT)+sum(ABS)
	// => (1 - (sum(REL)/100))TOT >= sum(ABS)
	// => TOT >= sum(ABS) / (1 - (sum(REL)/100))
	// => TOT >= sum(ABS) / ((100 - sum(REL))/100))
	return(
		qRound(
			columnTypeTotal(QET::Absolute)
			/
			((100.0 - columnTypeTotal(QET::RelativeToTotalLength)) / 100.0)
		)
	);
}

/**
	@return the maximum width for this template, or -1 if it does not have any.
*/
int TitleBlockTemplate::maximumWidth() {
	if (columnTypeCount(QET::Absolute) == columns_width_.count()) {
		// The template is composed of absolute widths only,
		// therefore it may not extend beyond their sum.
		return(columnTypeTotal(QET::Absolute));
	}
	return(-1);
}

/**
	@return the total effective width of this template
	@param total_width The total width initially planned for the rendering
*/
int TitleBlockTemplate::width(int total_width) {
	int width = 0;
	foreach (int col_width, columnsWidth(total_width)) {
		width += col_width;
	}
	return(width);
}

/**
	@return the total height of this template
*/
int TitleBlockTemplate::height() const {
	int height = 0;
	foreach(int row_height, rows_heights_) {
		height += row_height;
	}
	return(height);
}

/**
	Move a row within this template.
	@param from Index of the moved row
	@param to Arrival index of the moved row
*/
bool TitleBlockTemplate::moveRow(int from, int to) {
	// checks from and to
	if (from >= rows_heights_.count()) return(false);
	if (to   >= rows_heights_.count()) return(false);
	for (int j = 0 ; j < columns_width_.count() ; ++ j) {
		cells_[j].move(from, to);
	}
	rows_heights_.move(from, to);
	rowColsChanged();
	return(true);
}

/**
	Add a new 25px-wide row at the provided index.
	@param i Index of the added row, -1 meaning "last position"
*/
void TitleBlockTemplate::addRow(int i) {
	insertRow(25, createRow(), i);
}

/**
	@param dimension Size of the row to be added (always absolute, in pixels)
	@param column Row to be added
	@param i Index of the column after insertion, -1 meaning "last position"
*/
bool TitleBlockTemplate::insertRow(int dimension, const QList<TitleBlockCell *> &row, int i) {
	int index = (i == -1) ? rows_heights_.count() : i;
	
	for (int j = 0 ; j < columns_width_.count() ; ++ j) {
		cells_[j].insert(index, row[j]);
	}
	rows_heights_.insert(index, dimension);
	rowColsChanged();
	return(true);
}

/**
	Removes the row at index i
	@param i Index of the column to be removed
	@return the removed column
*/
QList<TitleBlockCell *> TitleBlockTemplate::takeRow(int i) {
	QList<TitleBlockCell *> row;
	int index = (i == -1) ? rows_heights_.count() - 1 : i;
	if (index < 0 || index >= rows_heights_.count()) return(row);
	for (int j = 0 ; j < columns_width_.count() ; ++ j) {
		row << cells_[j].takeAt(index);
	}
	rows_heights_.removeAt(index);
	rowColsChanged();
	return(row);
}

/**
	@return a new row that fits the current grid
*/
QList<TitleBlockCell *> TitleBlockTemplate::createRow() {
	return(createCellsList(columns_width_.count()));
	
}

/**
	Move the column at index "from" to index "to".
	@param from Source index of the moved column
	@param to   Target index of the moved column
*/
bool TitleBlockTemplate::moveColumn(int from, int to) {
	// checks from and to
	if (from >= columns_width_.count()) return(false);
	if (to   >= columns_width_.count()) return(false);
	cells_.move(from, to);
	columns_width_.move(from, to);
	rowColsChanged();
	return(true);
}

/**
	Add a new 50px-wide column at the provided index.
	@param i Index of the added column, -1 meaning "last position"
*/
void TitleBlockTemplate::addColumn(int i) {
	insertColumn(TitleBlockDimension(50, QET::Absolute), createColumn(), i);
}

/**
	@param dimension Size of the column to be added
	@param column Column to be added
	@param i Index of the column after insertion, -1 meaning "last position"
*/
bool TitleBlockTemplate::insertColumn(const TitleBlockDimension &dimension, const QList<TitleBlockCell *> &column, int i) {
	int index = (i == -1) ? columns_width_.count() : i;
	cells_.insert(index, column);
	columns_width_.insert(index, dimension);
	rowColsChanged();
	return(true);
}

/**
	Removes the column at index i
	@param i Index of the column to be removed
	@return the removed column
*/
QList<TitleBlockCell *> TitleBlockTemplate::takeColumn(int i) {
	int index = (i == -1) ? columns_width_.count() - 1 : i;
	if (index < 0 || index >= columns_width_.count()) {
		return(QList<TitleBlockCell *>());
	}
	QList<TitleBlockCell *> column = cells_.takeAt(i);
	columns_width_.removeAt(i);
	rowColsChanged();
	return(column);
}

/**
	@return a new column that fits the current grid
*/
QList<TitleBlockCell *> TitleBlockTemplate::createColumn() {
	return(createCellsList(rows_heights_.count()));
}

/**
	@param row A row number (starting from 0)
	@param col A column number (starting from 0)
	@return the cell located at (row, col)
*/
TitleBlockCell *TitleBlockTemplate::cell(int row, int col) const {
	if (row >= rows_heights_.count()) return(0);
	if (col >= columns_width_.count()) return(0);
	
	return(cells_[col][row]);
}

/**
	@param cell A cell belonging to this title block template
	@param ignore_span_state (Optional, defaults to false) If true, will consider
	cells theoretically spanned (i.e. row_span and col_span attributes).
	Otherwise, will take span_state attribute into account.
	@return the set of cells spanned by the provided cell
	Note the returned set does not include the spanning, provided cell
*/
QSet<TitleBlockCell *> TitleBlockTemplate::spannedCells(const TitleBlockCell *given_cell, bool ignore_span_state) const {
	QSet<TitleBlockCell *> set;
	if (!given_cell) return(set);
	if (!ignore_span_state && given_cell -> span_state == TitleBlockCell::Disabled) return(set);
	
	int final_row_span = ignore_span_state ? given_cell -> row_span : given_cell -> applied_row_span;
	int final_col_span = ignore_span_state ? given_cell -> col_span : given_cell -> applied_col_span;
	if (!final_row_span && !final_col_span) return(set);
	
	for (int i = given_cell -> num_col ; i <= given_cell -> num_col + final_col_span ; ++ i) {
		for (int j = given_cell -> num_row ; j <= given_cell -> num_row + final_row_span ; ++ j) {
			if (i == given_cell -> num_col && j == given_cell -> num_row) continue;
			TitleBlockCell *current_cell = cell(j, i);
			if (current_cell) set << current_cell;
		}
	}
	return(set);
}

/**
	Export the span parameters of all cell in the current grid.
*/
QHash<TitleBlockCell *, QPair<int, int> > TitleBlockTemplate::getAllSpans() const {
	QHash<TitleBlockCell *, QPair<int, int> > spans;
	for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
		for (int i = 0 ; i < columns_width_.count() ; ++ i) {
			spans.insert(
				cells_[i][j],
				QPair<int, int>(
					cells_[i][j] -> row_span,
					cells_[i][j] -> col_span
				)
			);
		}
	}
	return(spans);
}

/**
	Restore a set of span parameters.
*/
void TitleBlockTemplate::setAllSpans(const QHash<TitleBlockCell *, QPair<int, int> > &spans) {
	foreach (TitleBlockCell *cell, spans.keys()) {
		cell -> row_span = spans[cell].first;
		cell -> col_span = spans[cell].second;
	}
}

/**
	@param logo_name Logo name to be added / replaced
	@param logo_data Logo data
*/
bool TitleBlockTemplate::addLogo(const QString &logo_name, QByteArray *logo_data, const QString &logo_type, const QString &logo_storage) {
	if (data_logos_.contains(logo_name)) {
		// we are replacing the logo
		removeLogo(logo_name);
	}
	
	// we can now create our image object from the byte array
	if (logo_type == "svg") {
		// SVG format is handled by the QSvgRenderer class
		QSvgRenderer *svg = new QSvgRenderer();
		if (!svg -> load(*logo_data)) {
			return(false);
		}
		vector_logos_.insert(logo_name, svg);
		
		// we also memorize the way to store them in the final XML output
		QString final_logo_storage = logo_storage;
		if (logo_storage != "xml" && logo_storage != "base64") {
			final_logo_storage = "xml";
		}
		storage_logos_.insert(logo_name, logo_storage);
	} else {
		
		// bitmap formats are handled by the QPixmap class
		QPixmap logo_pixmap;
		logo_pixmap.loadFromData(*logo_data);
		if (!logo_pixmap.width() || !logo_pixmap.height()) {
			return(false);
		}
		bitmap_logos_.insert(logo_name, logo_pixmap);
		
		// bitmap logos can only be stored using a base64 encoding
		storage_logos_.insert(logo_name, "base64");
	}
	
	// we systematically store the raw data
	data_logos_.insert(logo_name, *logo_data);
	type_logos_.insert(logo_name, logo_type);
	
	return(true);
}

/**
	@param filepath Path of the image file to add as a logo
	@param name Name used to store the logo; if none is provided, the
	basename of the first argument is used.
	@return true if the logo could be deleted, false otherwise
*/
bool TitleBlockTemplate::addLogoFromFile(const QString &filepath, const QString &name) {
	QFileInfo filepath_info(filepath);
	QString filename = name.isEmpty() ? filepath_info.fileName() : name;
	QString filetype = filepath_info.suffix();
	
	// we read the provided logo
	QFile logo_file(filepath);
	if (!logo_file.open(QIODevice::ReadOnly)) return(false);
	QByteArray file_content = logo_file.readAll();
	
	// first, we try to add it as an SVG image
	if (addLogo(filename, &file_content, "svg", "xml")) return(true);
	
	// we then try to add it as a bitmap image
	return addLogo(filename, &file_content, filepath_info.suffix(), "base64");
}

/*
	@param logo_name Name used to store the logo
	@param filepath Path the logo will be saved as
	@return true if the logo could be exported, false otherwise
*/
bool TitleBlockTemplate::saveLogoToFile(const QString &logo_name, const QString &filepath) {
	if (!data_logos_.contains(logo_name)) {
		return(false);
	}
	
	QFile target_file(filepath);
	if (!target_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return(false);
	}
	
	target_file.write(data_logos_[logo_name]);
	target_file.close();
	return(true);
}

/**
	@param logo_name Name of the logo to remove
	@return true if the logo could be deleted, false otherwise
*/
bool TitleBlockTemplate::removeLogo(const QString &logo_name) {
	if (!data_logos_.contains(logo_name)) {
		return(false);
	}
	
	/// TODO check existing cells using this logo.
	if (vector_logos_.contains(logo_name)) {
		delete vector_logos_.take(logo_name);
	}
	if (bitmap_logos_.contains(logo_name)) {
		bitmap_logos_.remove(logo_name);
	}
	data_logos_.remove(logo_name);
	storage_logos_.remove(logo_name);
	return(true);
}

/**
	Rename the \a logo_name logo to \a new_name
	@param logo_name Name of the logo to be renamed
	@param new_name New name of the renamed logo
*/
bool TitleBlockTemplate::renameLogo(const QString &logo_name, const QString &new_name) {
	if (!data_logos_.contains(logo_name) || data_logos_.contains(new_name)) {
		return(false);
	}
	
	/// TODO check existing cells using this logo.
	if (vector_logos_.contains(logo_name)) {
		vector_logos_.insert(new_name, vector_logos_.take(logo_name));
	}
	if (bitmap_logos_.contains(logo_name)) {
		bitmap_logos_.insert(new_name, bitmap_logos_.take(logo_name));
	}
	data_logos_.insert(new_name, data_logos_.take(logo_name));
	storage_logos_.insert(new_name, storage_logos_.take(logo_name));
	return(true);
}

/**
	Set the kind of storage for the \a logo_name logo.
	@param logo_name Name of the logo which kind of storage is to be changed
	@param storage The kind of storage to use for the logo, e.g. "xml" or "base64".
*/
void TitleBlockTemplate::setLogoStorage(const QString &logo_name, const QString &storage) {
	if (storage_logos_.contains(logo_name)) {
		storage_logos_[logo_name] = storage;
	}
}

/**
	@return The names of logos embedded within this title block template.
*/
QList<QString> TitleBlockTemplate::logos() const {
	return(data_logos_.keys());
}

/**
	@param logo_name Name of a logo embedded within this title block template.
	@return the kind of storage used for the required logo, or a null QString
	if no such logo was found in this template.
*/
QString TitleBlockTemplate::logoType(const QString &logo_name) const {
	if (type_logos_.contains(logo_name)) {
		return type_logos_[logo_name];
	}
	return(QString());
}

/**
	@param logo_name Name of a vector logo embedded within this title block template.
	@return the rendering object for the required vector logo, or 0 if no such
	vector logo was found in this template.
*/
QSvgRenderer *TitleBlockTemplate::vectorLogo(const QString &logo_name) const {
	if (vector_logos_.contains(logo_name)) {
		return vector_logos_[logo_name];
	}
	return(0);
}

/**
	@param logo_name Name of a logo embedded within this title block template.  
	@return the pixmap for the required bitmap logo, or a null pixmap if no
	such bitmap logo was found in this template.
*/
QPixmap TitleBlockTemplate::bitmapLogo(const QString &logo_name) const {
	if (bitmap_logos_.contains(logo_name)) {
		return bitmap_logos_[logo_name];
	}
	return(QPixmap());
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
	
	// run through each individual cell
	for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
		for (int i = 0 ; i < columns_width_.count() ; ++ i) {
			if (cells_[i][j] -> spanner_cell || cells_[i][j] -> cell_type == TitleBlockCell::EmptyCell) continue;
			
			// calculate the border rect of the current cell
			int x = lengthRange(0, cells_[i][j] -> num_col, widths);
			int y = lengthRange(0, cells_[i][j] -> num_row, rows_heights_);
			
			int row_span = 0, col_span = 0;
			if (cells_[i][j] -> span_state != TitleBlockCell::Disabled) {
				row_span = cells_[i][j] -> applied_row_span;
				col_span = cells_[i][j] -> applied_col_span;
			}
			int w = lengthRange(cells_[i][j] -> num_col, cells_[i][j] -> num_col + 1 + col_span, widths);
			int h = lengthRange(cells_[i][j] -> num_row, cells_[i][j] -> num_row + 1 + row_span, rows_heights_);
			QRect cell_rect(x, y, w, h);
			
			renderCell(painter, *cells_[i][j], diagram_context, cell_rect);
		}
	}
}

/**
	Render the titleblock in DXF.
	@param diagram_context Diagram context to use to generate the titleblock strings
	@param titleblock_width Width of the titleblock to render
*/
void TitleBlockTemplate::renderDxf(QRectF &title_block_rect, const DiagramContext &diagram_context,
								   int titleblock_width, QString &file_path, int color) const {
	QList<int> widths = columnsWidth(titleblock_width);

	// draw the titleblock border
	double xCoord    = title_block_rect.topLeft().x();
	double yCoord    = Createdxf::sheetHeight - title_block_rect.bottomLeft().y()*Createdxf::yScale;
	double recWidth  = title_block_rect.width()  * Createdxf::xScale;
	double recHeight = title_block_rect.height() * Createdxf::yScale;
	Createdxf::drawRectangle(file_path, xCoord, yCoord, recWidth, recHeight, color);

	// run through each individual cell
	for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
		for (int i = 0 ; i < columns_width_.count() ; ++ i) {
			if (cells_[i][j] -> spanner_cell || cells_[i][j] -> cell_type == TitleBlockCell::EmptyCell) continue;

			// calculate the border rect of the current cell
			double x = lengthRange(0, cells_[i][j] -> num_col, widths);
			double y = lengthRange(0, cells_[i][j] -> num_row, rows_heights_);

			int row_span = 0, col_span = 0;
			if (cells_[i][j] -> span_state != TitleBlockCell::Disabled) {
				row_span = cells_[i][j] -> applied_row_span;
				col_span = cells_[i][j] -> applied_col_span;
			}
			double w = lengthRange(cells_[i][j] -> num_col, cells_[i][j] -> num_col + 1 + col_span, widths);
			double h = lengthRange(cells_[i][j] -> num_row, cells_[i][j] -> num_row + 1 + row_span, rows_heights_);

			x = xCoord + x*Createdxf::xScale;
			h *= Createdxf::yScale;
			y = yCoord + recHeight - h - y*Createdxf::yScale;
			w *= Createdxf::xScale;

			Createdxf::drawRectangle(file_path, x, y, w, h, color);
			if (cells_[i][j] -> type() == TitleBlockCell::TextCell) {
				QString final_text = finalTextForCell(*cells_[i][j], diagram_context);
				renderTextCellDxf(file_path, final_text, *cells_[i][j], x, y, w, h, color);
			}
		}
	}
}



/**
	Render a titleblock cell.
	@param painter Painter to use to render the titleblock
	@param diagram_context Diagram context to use to generate the titleblock strings
	@param rect Rectangle the cell must be rendered into.
*/
void TitleBlockTemplate::renderCell(QPainter &painter, const TitleBlockCell &cell, const DiagramContext &diagram_context, const QRect &cell_rect) const {
	// draw the border rect of the current cell
	QPen pen(QBrush(), 0.0, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
	pen.setColor(Qt::black);
	painter.setPen(pen);
	painter.setBrush(Qt::white);
	painter.drawRect(cell_rect);
	
	painter.save();
	// render the inner content of the current cell
	if (cell.type() == TitleBlockCell::LogoCell) {
		if (!cell.logo_reference.isEmpty()) {
			// the current cell appears to be a logo - we first look for the
			// logo reference in our vector logos list, since they offer a
			// potentially better (or, at least, not resolution-limited) rendering
			if (vector_logos_.contains(cell.logo_reference)) {
				vector_logos_[cell.logo_reference] -> render(&painter, cell_rect);
			} else if (bitmap_logos_.contains(cell.logo_reference)) {
				painter.drawPixmap(cell_rect, bitmap_logos_[cell.logo_reference]);
			}
		}
	} else if (cell.type() == TitleBlockCell::TextCell) {
		QString final_text = finalTextForCell(cell, diagram_context);
		renderTextCell(painter, final_text, cell, cell_rect);
	}
	painter.restore();
	
	// draw again the border rect of the current cell, without the brush this time
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(cell_rect);
}




/**
	@param cell A cell from this template
	@param diagram_context Diagram context to use to generate the final text for the given cell
	@return the final text that has to be drawn in the given cell
*/
QString TitleBlockTemplate::finalTextForCell(const TitleBlockCell &cell, const DiagramContext &diagram_context) const {
	QString cell_text = cell.value.name();
	QString cell_label = cell.label.name();
	
	cell_text = interpreteVariables(cell_text, diagram_context);
	
	if (cell.display_label && !cell.label.isEmpty()) {
		cell_label = interpreteVariables(cell_label, diagram_context);
		cell_text = QString(tr(" %1 : %2", "titleblock content - please let the blank space at the beginning")).arg(cell_label).arg(cell_text);
	} else {
		cell_text = QString(tr(" %1")).arg(cell_text);
	}
	return(cell_text);
}

/**
	@param string A text containing 0 to n variables, e.g. "%var" or "%{var}"
	@param diagram_context Diagram context to use to interprete variables
	@return the provided string with variables replaced by the values from the diagram context
*/
QString TitleBlockTemplate::interpreteVariables(const QString &string, const DiagramContext &diagram_context) const {
	QString interpreted_string = string;
	foreach (QString key, diagram_context.keys(DiagramContext::DecreasingLength)) {
		interpreted_string.replace("%{" + key + "}", diagram_context[key].toString());
		interpreted_string.replace("%" + key,        diagram_context[key].toString());
	}
	return(interpreted_string);
}

/**
	@brief Get list of variables
	@return The list of string with variables
*/
QStringList TitleBlockTemplate::listOfVariables() {
	QStringList list;
	// run through each individual cell
	for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
		for (int i = 0 ; i < columns_width_.count() ; ++ i) {
			if (cells_[i][j] -> spanner_cell || cells_[i][j] -> cell_type == TitleBlockCell::EmptyCell) continue;
			// TODO: not works on all cases...
			list << cells_[i][j] -> value.name().replace("%","");
		}	
	}
	qDebug() << list;
	return list;
}

/**
	This method uses a \a painter to render the \a text of a \a cell
	into the \a cell_rect rectangle.
	The alignment, font_size and other cell parameters are taken into account
	when rendering.
	@param painter QPainter used to render the text
	@param text Text to render
	@param cell Cell the rendered text is rattached to
	@param cell_rect Rectangle delimiting the cell area
*/
void TitleBlockTemplate::renderTextCell(QPainter &painter, const QString &text, const TitleBlockCell &cell, const QRectF &cell_rect) const {
	if (text.isEmpty()) return;
	QFont text_font = TitleBlockTemplate::fontForCell(cell);
	painter.setFont(text_font);
	
	if (cell.hadjust) {
		QFontMetricsF font_metrics(text_font);
		QRectF font_rect = font_metrics.boundingRect(QRect(-10000, -10000, 10000, 10000), cell.alignment, text);
		
		if (font_rect.width() > cell_rect.width()) {
			qreal ratio = qreal(cell_rect.width()) / qreal(font_rect.width());
			painter.save();
			
			painter.translate(cell_rect.topLeft());
			qreal vertical_adjustment = cell_rect.height() * (1 - ratio) / 2.0;
			painter.translate(0.0, vertical_adjustment);
			painter.scale(ratio, ratio);
			
			QRectF new_world_cell_rect(cell_rect);
			new_world_cell_rect.moveTo(0, 0.0);
			new_world_cell_rect.setWidth(new_world_cell_rect.width() / ratio);
			painter.drawText(new_world_cell_rect, cell.alignment, text);
			
			painter.restore();
			return;
		}
	}
	
	// Still here? Let's draw the text normally
	painter.drawText(cell_rect, cell.alignment, text);
}


void TitleBlockTemplate::renderTextCellDxf(QString &file_path, const QString &text,
										   const TitleBlockCell &cell,
										   qreal x, qreal y, qreal w, qreal h, int color) const {
	if (text.isEmpty()) return;
	QFont text_font = TitleBlockTemplate::fontForCell(cell);
	double textHeight = text_font.pointSizeF();
	if (textHeight < 0)
		textHeight = text_font.pixelSize();

	qreal x2 = x + w;

	int vAlign = 0;
	int hAlign = 0;
	bool hALigned = false;

	if ( cell.alignment & Qt::AlignRight ) {
		hAlign = 2;
		hALigned = true;
	} else if ( cell.alignment & Qt::AlignHCenter ) {
		hAlign = 1;
		hALigned = true;
		x2 = x + w/2;
	} else if ( cell.alignment & Qt::AlignJustify ) {
		hAlign = 5;
		hALigned = true;
	}

	if ( cell.alignment & Qt::AlignTop ) {
		vAlign = 3;
		y += h - textHeight*Createdxf::yScale;
		if (!hALigned)
			x2 = x;
	} else if ( cell.alignment & Qt::AlignVCenter ) {
		vAlign = 2;
		y += h/2;
		if (!hALigned)
			x2 = x;
	} else if ( cell.alignment & Qt::AlignBottom ) {}


	//painter.setFont(text_font);

	if (cell.hadjust) {
		QFontMetricsF font_metrics(text_font);
		QRectF font_rect = font_metrics.boundingRect(QRect(-10000, -10000, 10000, 10000), cell.alignment, text);

		if (font_rect.width()*Createdxf::xScale > w) {
			qreal ratio = qreal(w) / qreal(font_rect.width()*Createdxf::xScale);
			textHeight *= ratio;
		}
	}

	Createdxf::drawTextAligned(file_path, text, x,
							   y, textHeight*Createdxf::yScale, 0, 0, hAlign, vAlign, x2, color, 0);

}


/**
	Set the spanner_cell attribute of every cell to 0.
*/
void TitleBlockTemplate::forgetSpanning() {
	for (int i = 0 ; i < columns_width_.count() ; ++ i) {
		for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
			cells_[i][j] -> spanner_cell = 0;
		}
	}
}

/**
	Set the spanner_cell attribute of every cell spanned by \a spanning_cell to 0.
	@param modify_cell (Optional, defaults to true) Whether to set row_span and col_span of \a spanning_cell to 0.
*/
void TitleBlockTemplate::forgetSpanning(TitleBlockCell *spanning_cell, bool modify_cell) {
	if (!spanning_cell) return;
	foreach (TitleBlockCell *spanned_cell, spannedCells(spanning_cell)) {
		spanned_cell -> spanner_cell = 0;
	}
	if (modify_cell) {
		spanning_cell -> row_span = 0;
		spanning_cell -> col_span = 0;
		spanning_cell -> applied_row_span = 0;
		spanning_cell -> applied_col_span = 0;
		spanning_cell -> span_state = TitleBlockCell::Enabled;
	}
}

/**
	Forget any previously applied span, then apply again all spans defined
	by existing cells.
*/
void TitleBlockTemplate::applyCellSpans() {
	forgetSpanning();
	for (int i = 0 ; i < columns_width_.count() ; ++ i) {
		for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
			checkCellSpan(cells_[i][j]);
			applyCellSpan(cells_[i][j]);
		}
	}
}

/**
	Check whether a given cell can be spanned according to its row_span and
	col_span attributes. the following attributes of \a cell are updated
	according to what is really possible:
	  * applied_col_span
	  * applied_row_span
	  * span_state
	@param cell Cell we want to check
	@return false if no check could be performed, true otherwise
*/
bool TitleBlockTemplate::checkCellSpan(TitleBlockCell *cell) {
	if (!cell) return(false);
	
	cell -> span_state = TitleBlockCell::Enabled;
	cell -> applied_row_span = cell -> row_span;
	cell -> applied_col_span = cell -> col_span;
	
	// ensure the cell can span as far as required
	if (cell -> num_col + cell -> col_span >= columnsCount()) {
		cell -> applied_col_span = columnsCount() - 1 - cell -> num_col;
		cell -> span_state = TitleBlockCell::Restricted;
	}
	if (cell -> num_row + cell -> row_span >= rowsCount()) {
		cell -> applied_row_span = rowsCount() - 1 - cell -> num_row;
		cell -> span_state = TitleBlockCell::Restricted;
	}
	
	// ensure cells that will be spanned are either empty or free
	for (int i = cell -> num_col ; i <= cell -> num_col + cell -> applied_col_span ; ++ i) {
		for (int j = cell -> num_row ; j <= cell -> num_row + cell -> applied_row_span ; ++ j) {
			if (i == cell -> num_col && j == cell -> num_row) continue;
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
			qDebug() << Q_FUNC_INFO << "span check" << i << j;
#endif
			TitleBlockCell *current_cell = cells_[i][j];
			if (current_cell -> cell_type != TitleBlockCell::EmptyCell || (current_cell -> spanner_cell && current_cell -> spanner_cell != cell)) {
				cell -> span_state = TitleBlockCell::Disabled;
				return(true);
			}
		}
	}
	
	return(true);
}

/**
	Ensure the spans of the provided cell are applied within the grid structure.
	Note: this function does not check whether the spans of the provided cell make sense.
	@param cell Potentially spanning cell
*/
void TitleBlockTemplate::applyCellSpan(TitleBlockCell *cell) {
	if (!cell || (!cell -> row_span && !cell -> col_span)) return;
	if (cell -> span_state == TitleBlockCell::Disabled) return;
	
	// goes through every spanned cell
	for (int i = cell -> num_col ; i <= cell -> num_col + cell -> applied_col_span ; ++ i) {
		for (int j = cell -> num_row ; j <= cell -> num_row + cell -> applied_row_span ; ++ j) {
			// avoid the spanning cell itself
			if (i == cell -> num_col && j == cell -> num_row) continue;
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
			qDebug() << Q_FUNC_INFO << "marking cell at" << j << i <<  "as spanned by cell at" << cell -> num_row <<  cell -> num_col;
#endif
			// marks all spanned cells with the spanning cell
			cells_[i][j] -> spanner_cell = cell;
		}
	}
}

/**
	Ensure all cells have the right col+row numbers.
*/
void TitleBlockTemplate::applyRowColNums() {
	for (int i = 0 ; i < columns_width_.count() ; ++ i) {
		for (int j = 0 ; j < rows_heights_.count() ; ++ j) {
			cells_[i][j] -> num_col = i;
			cells_[i][j] -> num_row = j;
		}
	}
}

/**
	Take care of consistency and span-related problematics when
	adding/moving/deleting rows and columns.
*/
void TitleBlockTemplate::rowColsChanged() {
	applyRowColNums();
	applyCellSpans();
}

/**
	@return the width between two borders
	@param start start border number
	@param end end border number
*/
int TitleBlockTemplate::lengthRange(int start, int end, const QList<int> &lengths_list) const {
	if (start > end || start >= lengths_list.count() || end > lengths_list.count()) {
#ifdef TITLEBLOCK_TEMPLATE_DEBUG
		qDebug() << Q_FUNC_INFO << "wont use" << start << "and" << end;
#endif
		return(0);
	}
	
	int length = 0;
	for (int i = start ; i < end ; ++i) {
		length += lengths_list[i];
	}
	return(length);
}

