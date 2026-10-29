/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "qetgraphicstableitem.h"
#include "../../ui/tableeditor15.h"
#include "../../shownkinds.h"
#include "../../qetproject.h"
#include "../../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../../cablelist/cablelistmodel.h"
#include "../../createdxf.h"
#include "../../diagram.h"
#include "../../elementprovider.h"
#include "../../qetmessagebox.h"
#include "../../qetxml.h"
#include "../../utils/qetutils.h"
#include "projectdbmodel.h"
#include "qetgraphicsheaderitem.h"

#include <QAbstractItemModel>
#include "../../ui/tableformat13.h"
#include <QFontMetrics>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QFocusEvent>
#include <cmath>
#include <QPainter>

static int no_model_height = 20;
static int no_model_width = 40;

/**
	@brief QetGraphicsTableItem::adjustTableToFolio
	Adjust the table to fit at best the folio
	@param table : table to adjust
	@param margins : margins between table and folio.
*/
void QetGraphicsTableItem::adjustTableToFolio(
		QetGraphicsTableItem *table,
		QMargins margins)
{
	if (!table->diagram()) {
		return;
	}

	auto drawable_rect = table->diagram()->border_and_titleblock.insideBorderRect();
    if(table->customFormatting() && table->model()) {
        const int offset=table->previousTable()?table->previousTable()->displayNRowOffset():0;
        const auto fallback=table->model()->index(0,0).data(Qt::FontRole).value<QFont>();
        const int remaining=qMax(0,table->model()->rowCount()-offset);
        const auto layout=QetFormatting::tableLayout(table->customFormat(),table->model(),offset,remaining,fallback);
        table->setPos(drawable_rect.left()+margins.left(),drawable_rect.top()+margins.top()+layout.headerHeight+layout.titleHeight);
        const qreal available=drawable_rect.bottom()-margins.bottom()-table->pos().y();
        int count=0,height=0;for(int rowHeight:layout.rowHeights){if(height+rowHeight>available)break;height+=rowHeight;++count;}
        // Zero means unlimited in the table model; keep one row when even the
        // first row is taller than the page, rather than displaying everything.
        table->setDisplayNRow(remaining?qMax(1,count):0);
        return;
    }
	table->setPos(
				drawable_rect.topLeft().x() + margins.left(),
				drawable_rect.topLeft().y()
				+ margins.top()
				+ table->headerItem()->rect().height());

	auto size_ = table->size();
	size_.setWidth(int(drawable_rect.width() - (margins.left() + margins.right())));
		//Size must be a multiple of 10, because the table adjusts itself by step of 10.
	while (size_.width()%10) {
		--size_.rwidth();
	}
	table->setSize(size_);

		//Calculate the maximum row to display to fit the nomenclature into diagram
	auto available_height = drawable_rect.height() - table->pos().y();
	auto min_row_height = table->minimumRowHeight();
	table->setDisplayNRow(int(floor(available_height/min_row_height))); //Convert a double to int, but max_row_to_display is already rounded an integer so we assume everything is ok
}

/**
	@brief QetGraphicsTableItem::checkInsufficientRowsCount
	Check if the number of rows of table + linked table is enough
	to display all content of the model,
	if not open a dialog to advise user what to do.
	@param table
*/
void QetGraphicsTableItem::checkInsufficientRowsCount(
		QetGraphicsTableItem *table)
{
	if (!table->diagram() || !table->model()) {
		return;
	}

	auto first_table = table;
	while (first_table->previousTable())
		first_table = first_table->previousTable();

	if (first_table->displayNRow() <= 0) //displayed rows is unlimited
		return;

	int count_ = first_table->displayNRow();
	bool several_table = false;
	while (first_table->nextTable())
	{
		several_table = true;
		first_table = first_table->nextTable();
		if (first_table->displayNRow() <= 0) { //displayed rows is unlimited
			return;
		} else {
			count_ += first_table->displayNRow();
			first_table->displayNRowOffset();
		}
	}

	if (count_ < first_table->model()->rowCount())
	{
		QWidget *parent = nullptr;
		if (first_table->diagram() && first_table->diagram()->views().size())
			parent = first_table->diagram()->views().first();

		QString text;
		if (several_table) {
			text = tr("The information to be displayed is greater than the maximum quantity that can be displayed by the tables.\n"
					  "Please add a new table or adjust the existing tables to show all information.");
		} else {
			text = tr("The information to be displayed is greater than the maximum quantity that can be displayed by the table.\n"
					  "Please add a new table or adjust the existing table to display all information.");
		}
		QET::QetMessageBox::information(parent, tr("Table limitation"), text);
	}

}

/**
	@brief QetGraphicsTableItem::QetGraphicsTableItem
	Default constructor
	@param parent
*/
QetGraphicsTableItem::QetGraphicsTableItem(QGraphicsItem *parent) :
	QetGraphicsItem(parent)
{
	ShownKinds::tag(this, ShownKinds::Tables);
	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsSelectable, true);
	setFlag(QGraphicsItem::ItemIsFocusable, true);
	setAcceptHoverEvents(true);
	setUpHandler();

		//A litle bounding rect before model is set,
		//then user can already grab this item, even if model is not already set
	m_bounding_rect.setRect(m_br_margin/-2, m_br_margin/-2, 50, 50);

	connect(this, &QetGraphicsTableItem::xChanged,
		this, &QetGraphicsTableItem::adjustHandlerPos);
	connect(this, &QetGraphicsTableItem::yChanged,
		this, &QetGraphicsTableItem::adjustHandlerPos);

	m_header_item = new QetGraphicsHeaderItem(this);
	connect(m_header_item, &QetGraphicsHeaderItem::heightResized,
		this, [this]()
	{
		m_header_item->setPos(0, 0-m_header_item->rect().height());
	});
	connect(m_header_item, &QetGraphicsHeaderItem::sectionResized,
		this, &QetGraphicsTableItem::changed);
		//Init the size of table without a model
	setModel();
}

QetGraphicsTableItem::~QetGraphicsTableItem()
{
	if (m_previous_table) {
		if (m_next_table) {
			m_previous_table->setNextTable(m_next_table);
		} else {
			m_previous_table->setNextTable(nullptr);
		}
	} else if (m_next_table) {
		m_next_table->setPreviousTable(nullptr);
	}
}

/**
	@brief QetGraphicsTableItem::setModel
	Set the model presented by this item.
	Since QetGraphicsTableItem don't take ownership of model,
	if item already have a model, it's your responsibility to delete it.
	@param model
*/
void QetGraphicsTableItem::setModel(QAbstractItemModel *model)
{
	if (m_model)
	{
		disconnect(m_model, &QAbstractItemModel::dataChanged,
			   this, &QetGraphicsTableItem::dataChanged);
		disconnect(m_model, &QAbstractItemModel::modelReset,
			   this, &QetGraphicsTableItem::modelReseted);
	}
	m_model = model;
	m_header_item->setModel(m_model);

	setUpColumnAndRowMinimumSize();
	adjustSize();

	m_header_item->setPos(0, -m_header_item->rect().height());
	if (m_model)
	{
		connect(m_model, &QAbstractItemModel::dataChanged,
			this, &QetGraphicsTableItem::dataChanged);
		connect(m_model, &QAbstractItemModel::modelReset,
			this, &QetGraphicsTableItem::modelReseted);
	}

	if (m_next_table) {
		m_next_table->setModel(m_model);
	}
	emit changed();
}

/**
	@brief QetGraphicsTableItem::model
	@return The model that this item is presenting
*/
QAbstractItemModel *QetGraphicsTableItem::model() const
{
	return m_model.data();
}

/**
	@brief QetGraphicsTableItem::boundingRect
	Reimplemented from QGraphicsObject
	@return
*/
QRectF QetGraphicsTableItem::boundingRect() const
{
	return m_bounding_rect;
}

/**
	@brief QetGraphicsTableItem::paint
	Draw the table
	@param painter
	@param option
	@param widget
*/
void QetGraphicsTableItem::paint(
		QPainter *painter,
		const QStyleOptionGraphicsItem *option,
		QWidget *widget)
{
	Q_UNUSED(option)
    Q_UNUSED(widget)
    if(customFormatting()) {
        int offset=m_previous_table?m_previous_table->displayNRowOffset():0;
        QFont fallback=m_model?m_model->index(0,0).data(Qt::FontRole).value<QFont>():QFont();
        QetFormatting::paintTable(*painter,m_custom_format,m_model,offset,formattedRows(),fallback);
        if(widget&&isSelected()&&scene()&&!scene()->views().isEmpty()){painter->save();painter->setPen(QPen(QColor("#2678d4"),0));painter->setBrush(QColor(38,120,212,35));const auto g=QetFormatting::tableLayout(m_custom_format,m_model,offset,formattedRows(),fallback);QVector<int> xs{0},ys{0};for(int w:g.widths)xs<<xs.last()+w;for(int h:g.rowHeights)ys<<ys.last()+h;for(const auto &cell:m_cells20){const int c=cell.first,r=cell.second-offset;if(cell.second==-2)painter->drawRect(QRectF(0,-g.headerHeight-g.titleHeight,g.width,g.titleHeight));else if(c>=0&&c<g.widths.size()){if(cell.second==-1)painter->drawRect(QRectF(xs[c],-g.headerHeight,g.widths[c],g.headerHeight));else if(r>=0&&r<g.rowHeights.size())painter->drawRect(QRectF(xs[c],ys[r],g.widths[c],g.rowHeights[r]));}}painter->restore();}
        if(isSelected()){painter->save();painter->setPen(QPen(Qt::blue,0,Qt::DashLine));painter->setBrush(Qt::NoBrush);painter->drawRect(boundingRect().adjusted(5,5,-5,-5));painter->restore();}
        return;
    }

	painter->save();

	QPen pen;
	pen.setWidthF(0.7);
	pen.setColor(Qt::black);
	painter->setPen(pen);
	painter->drawRect(0,0, m_header_item->rect().width(), m_current_size.height());

	if (isSelected())
	{
		painter->save();
		QColor color(Qt::darkBlue);
		color.setAlpha(20);
		painter->setBrush(QBrush (color));
		painter->setPen(Qt::NoPen);
		painter->drawRect(0,0, m_header_item->rect().width(), m_current_size.height());
		painter->restore();
	}

	if(!m_model)
	{
		painter->restore();
		return;
	}
	painter->setFont(m_model->data(m_model->index(0,0), Qt::FontRole).value<QFont>());

		//Draw vertical lines
	auto offset= 0;
	for(auto i=0 ; i<m_model->columnCount() ; ++i)
	{
		QPointF p1(offset+m_header_item->sectionSize(i), 0);
		QPointF p2(offset+m_header_item->sectionSize(i), m_current_size.height());
		painter->drawLine(p1, p2);
		offset += m_header_item->sectionSize(i);
	}

		//calculate the number of rows to display.
	auto row_count = m_model->rowCount();

	if (m_previous_table) //Remove the number of row already displayed by previous tables
		row_count -= m_previous_table->displayNRowOffset();

	if (m_number_of_row_to_display > 0) //User override the number of row to display
		row_count = std::min(row_count, m_number_of_row_to_display);

		//Draw horizontal lines
	auto cell_height =  static_cast<double>(m_current_size.height())/static_cast<double>(row_count);
	for(auto i= 1 ; i-1<row_count ; ++i)
	{
		QPointF p1(m_header_item->rect().left(), cell_height*i);
		QPointF p2(m_header_item->rect().right(), cell_height*i);
		painter->drawLine(p1, p2);
	}

		//Write text of each cell
	for (auto i=0 ; i<row_count ; ++i)
	{
		auto margin_ = QETUtils::marginsFromString(m_model->index(0,0).data(Qt::UserRole+1).toString());
		QPointF top_left(margin_.left(), i==0? margin_.top() : cell_height*i + margin_.top());

		for(auto j= 0 ; j<m_model->columnCount() ; ++j)
		{
				//In first iteration the top left X is margin left,
				// in all other iteration the top left X is stored in m_column_size
			if (j>0) {
				top_left.setX(top_left.x() + m_header_item->sectionSize(j-1));
			}
			QSize size(m_header_item->sectionSize(j) - margin_.left() - margin_.right(),
					   static_cast<int>(cell_height) - margin_.top() - margin_.bottom());
			auto index_row = m_previous_table ? i + m_previous_table->displayNRowOffset() : i;
			painter->drawText(
						QRectF(top_left, size),
						m_model->data(
							m_model->index(0,0),
							Qt::TextAlignmentRole
							).toInt()
						| Qt::AlignVCenter,
						m_model->index(index_row, j)
						.data().toString());
		}
	}

	painter->restore();
}

/**
	@brief QetGraphicsTableItem::setSize
	Set the current size of the table to size
	@param size
*/
void QetGraphicsTableItem::setSize(const QSize &size)
{
	if(customFormatting()){refreshCustomGeometry();return;}
	auto new_size = size;
	if (new_size.width() < minimumSize().width()) {
		new_size.setWidth(minimumSize().width());
	}
	if (new_size.height() < minimumSize().height()) {
		new_size.setHeight(minimumSize().height());
	}
	while (new_size.width()%10) {
		new_size.rwidth()++;
	}

	prepareGeometryChange();
	m_current_size = new_size;
	adjustColumnsWidth();
	setUpBoundingRect();
	emit changed();
}

/**
	@brief QetGraphicsTableItem::size
	@return The current size of the table
*/
QSize QetGraphicsTableItem::size() const
{
	if(customFormatting())return m_current_size;
	QSize size_(m_header_item->rect().width(), m_current_size.height());
	return size_;
}

/**
	@brief QetGraphicsTableItem::minimumSize
	@return the minimum size the table can be
	The returned size take care of the table's minimum width,
	but also the header item's minimum width
*/
QSize QetGraphicsTableItem::minimumSize() const
{
	if(customFormatting()){
        const int offset=m_previous_table?m_previous_table->displayNRowOffset():0;
        const QFont fallback=m_model?m_model->index(0,0).data(Qt::FontRole).value<QFont>():QFont();
        const auto g=QetFormatting::tableLayout(m_custom_format,m_model,offset,formattedRows(),fallback);
        return QSize(g.width,g.bodyHeight);
    }
	if (!m_model) {
		return QSize(no_model_width, no_model_height);
	}

	auto row_count = m_model->rowCount();
	if (m_previous_table) {
		row_count -= m_previous_table->displayNRowOffset();
	}

	if (m_number_of_row_to_display > 0)
		row_count = std::min(row_count, m_number_of_row_to_display);


		//m_minimum_column_width already take in count the minimum size of header
	QSize size_(
				std::accumulate(
					m_minimum_column_width.begin(),
					m_minimum_column_width.end(),
					0),
				m_minimum_row_height*row_count);
		//make sure that the width is a multiple of 10
	while (size_.width()%10) {
		size_.rwidth()++;
	}
	return size_;
}

/**
	@brief QetGraphicsTableItem::setDisplayNRow
	Limit the number of row to display
	@param number : set to 0 or less to disabled the limit of row to display
*/
void QetGraphicsTableItem::setDisplayNRow(const int &number)
{
	m_number_of_row_to_display = number;
	if(customFormatting())m_custom_format["rows"]=qMax(0,number);
	setToMinimumHeight();
	if (m_next_table)
		m_next_table->previousTableDisplayRowChanged();
	emit changed();
}

/**
	@brief QetGraphicsTableItem::displayNRow
	@return the number of row displayed.
	A value of 0 or less mean there is no limit
*/
int QetGraphicsTableItem::displayNRow() const
{
	return customFormatting()?QetFormatting::number(m_custom_format,"rows",0,0,10000):m_number_of_row_to_display;
}

/**
	@brief QetGraphicsTableItem::setPreviousTable
	Set the previous table to table.
	If this table already have a previous table,
	the previous table will be replaced.
	Set new table to nullptr to remove an existing previous table.
	The table uses the model of the new previous table.
	Since the table does not take ownership of the model,
	it is your responsibility to manage the old model.
	Linked tables (table with next and/or previous table)
	share the same model, a table always take the model of the previous table.
	When remove a previous table (set to nullptr) from a table,
	the model is also removed, you need to set a new model
	@param table
*/
void QetGraphicsTableItem::setPreviousTable(QetGraphicsTableItem *table)
{
	if (m_previous_table == table ||
		this == table) {
		return;
	}

	auto old_previous_table = m_previous_table;
	m_previous_table = table;
	if (m_previous_table) //set previous table and get her model
	{
		m_previous_table->setNextTable(this);
		setModel(m_previous_table->m_model);
	}
	else if (old_previous_table->model()) //Copie the model of old previous table
	{
		auto *old_model = old_previous_table ? old_previous_table->model() : nullptr;
		if (auto *db_model = dynamic_cast<ProjectDBModel *>(old_model)) {
			setModel(new ProjectDBModel(*db_model));
		}
		else if (auto *cable_model = dynamic_cast<CableListModel *>(old_model)) {
			setModel(new CableListModel(*cable_model));
		}
	}
	emit changed();

	if (old_previous_table &&
		old_previous_table->nextTable() == this) {
		old_previous_table->setNextTable(nullptr);
	}
}

/**
	@brief QetGraphicsTableItem::setNextTable
	Set the next table to table
	nullptr will remove an existing next table.
	@param table
*/
void QetGraphicsTableItem::setNextTable(QetGraphicsTableItem *table)
{
	if (m_next_table == table ||
		this == table) {
		return;
	}

	auto old_next_table = m_next_table;

	m_next_table = table;
	if (m_next_table) {
		m_next_table->setPreviousTable(this);
	}

	if (old_next_table &&
		old_next_table->previousTable() == this) {
		old_next_table->setPreviousTable(nullptr);
	}

}

void QetGraphicsTableItem::setTableName(const QString &name)
{
	m_name = name;
	emit changed();
}

QString QetGraphicsTableItem::tableName() const
{
	return m_name;
}

/**
	@brief QetGraphicsTableItem::displayNRowOffset
	@return the offset (aka the last displayed row) of displayed row.
	If this item have a previous table, the previous offset is added.
*/
int QetGraphicsTableItem::displayNRowOffset() const
{
	auto offset_ = customFormatting()?formattedRows():m_number_of_row_to_display;
	if(m_previous_table)
		offset_ += m_previous_table->displayNRowOffset();

	return offset_;
}

/**
 * @brief QetGraphicsTableItem::displayedRowCount
 * @return the number of row displayed by this table
 */
int QetGraphicsTableItem::displayedRowCount() const
{
	if(customFormatting())return formattedRows();
	if (!m_model) {
		return 0;
	}

		//calculate the number of rows to display.
	auto row_count = m_model->rowCount();

	if (m_previous_table) //Remove the number of row already displayed by previous tables
		row_count -= m_previous_table->displayNRowOffset();

	if (m_number_of_row_to_display > 0) //User override the number of row to display
		row_count = std::min(row_count, m_number_of_row_to_display);

	return row_count;
}

QPointer<QetGraphicsTableItem> QetGraphicsTableItem::previousTable() const
{
	return m_previous_table;
}

QPointer<QetGraphicsTableItem> QetGraphicsTableItem::nextTable() const
{
	return m_next_table;
}

/**
	@brief QetGraphicsTableItem::setToMinimumHeight
	Set the height to the the minimum.
	The width stay unchanged.
*/
void QetGraphicsTableItem::setToMinimumHeight()
{
	auto size_ = size();
	size_.setHeight(1);
	setSize(size_);
}

void QetGraphicsTableItem::initLink()
{
	if (!m_pending_previous_table_uuid.isNull())
	{
			//The built folios only: a previous table on a folio not built
			//yet (lazy folios) is linked when that folio is
		QETProject *project = this->diagram()->project();
		ElementProvider provider_(project->builtFolios());
		auto previous_table = provider_.tableFromUuid(m_pending_previous_table_uuid);
		if (previous_table) {
			setPreviousTable(previous_table);
		}
		if (previous_table || !project->unloadedFolioCount()) {
			m_pending_previous_table_uuid = QUuid(); //Set to null in case initLink is called again
		}
	}
	setSize(m_pending_size);
}

/**
	@brief QetGraphicsTableItem::previousTableUuid
	@return the uuid of the table before this one in its chain, linked or
	still waiting for its folio to be built (lazy folios); null if none
*/
QUuid QetGraphicsTableItem::previousTableUuid() const
{
	if (m_previous_table) {
		return m_previous_table->uuid();
	}
	return m_pending_previous_table_uuid;
}

/**
	@brief QetGraphicsTableItem::minimumRowHeight
	@return the minimum height of a row
*/
int QetGraphicsTableItem::minimumRowHeight() const
{
	return customFormatting()?QetFormatting::number(m_custom_format,"rowHeight",20):m_minimum_row_height;
}

/**
	@brief QetGraphicsTableItem::toXml
	Save the table to xml
	@param dom_document : parent document
	@return the dom_element that describe the table
*/
QDomElement QetGraphicsTableItem::toXml(QDomDocument &dom_document) const
{
	auto dom_table = dom_document.createElement(xmlTagName());
	dom_table.setAttribute("x", QString::number(pos().x()));
	dom_table.setAttribute("y", QString::number(pos().y()));
	dom_table.setAttribute("width", QString::number(m_current_size.width()));
	dom_table.setAttribute("height", QString::number(m_current_size.height()));
	dom_table.setAttribute("uuid", m_uuid.toString());
	dom_table.setAttribute("name", m_name);
	if(!m_custom_format.isEmpty())dom_table.setAttribute("format-v13",QetFormatting::encode(m_custom_format));
	dom_table.setAttribute("display_n_row", QString::number(m_number_of_row_to_display));

		//Add the header xml
	dom_table.appendChild(m_header_item->toXml(dom_document));

		//Add previous table, the model is save by the previous table.
		//A folio deleted with undo still possible keeps its tables in the
		//chain, but it is not saved: name the nearest previous table that
		//is, or save the model if none is.
	const auto project_ = diagram() ? diagram()->project() : nullptr;
	auto in_project = [project_](const QetGraphicsTableItem *table) {
		return project_ && table->diagram()
				&& project_->folios().contains(table->diagram());
	};
	QetGraphicsTableItem *previous_ = m_previous_table;
	while (previous_ && !in_project(previous_)) {
		previous_ = previous_->previousTable();
	}
	if (previous_)
	{
		auto dom_previous_table = dom_document.createElement("previous_table");
		dom_previous_table.setAttribute("uuid", previous_->m_uuid.toString());
		dom_table.appendChild(dom_previous_table);
	}
	else if (m_model) //There is not a previous table, we need to save the model
	{
			//Add model
		auto dom_model = dom_document.createElement("model");
		if (auto project_db_model = dynamic_cast<ProjectDBModel *>(m_model.data())) {
			dom_model.appendChild(project_db_model->toXml(dom_document));
		}
		else if (auto cable_model = dynamic_cast<CableListModel *>(m_model.data())) {
			dom_model.appendChild(cable_model->toXml(dom_document));
		}
		dom_table.appendChild(dom_model);

	}

	return dom_table;
}

/**
	@brief QetGraphicsTableItem::fromXml
	Restore the table from xml.
	Make this item is already in a diagram to
	@param dom_element
*/
void QetGraphicsTableItem::fromXml(const QDomElement &dom_element)
{
	if (dom_element.tagName() != xmlTagName()) {
		return;
	}

	this->setPos(
				dom_element.attribute(
					"x",
					QString::number(10)).toDouble(),
				dom_element.attribute(
					"y",
					QString::number(10)).toDouble());
	//Size is not set now because will change during the whole process of opening a project from the xml
	m_pending_size = QSize(
				dom_element.attribute(
					"width",
					QString::number(no_model_width)).toInt(),
				dom_element.attribute(
					"height",
					QString::number(no_model_height)).toInt());

	m_uuid = QUuid(
				dom_element.attribute(
					"uuid",
					QUuid::createUuid().toString()));
	m_name = dom_element.attribute("name");
	m_custom_format=QetFormatting::decode(dom_element.attribute("format-v13"));
	m_header_item->setVisible(!customFormatting());
	m_number_of_row_to_display = dom_element.attribute(
				"display_n_row",
				QString::number(0)).toInt();

	auto vector_ = QETXML::directChild(dom_element, "previous_table");
	if (vector_.size()) { //Table have a previous table
		m_pending_previous_table_uuid = QUuid(
					vector_.first().attribute("uuid"));
	}
	else if (this->diagram()) //The table haven't got a previous table, so there should be a model save to xml
	{
			//Get table: which model wrote itself in there decides which
			//class reads it back -- a cable list is not a database query
		auto model_element = dom_element.firstChildElement("model");
		auto project_model_element = model_element.firstChildElement(
					ProjectDBModel::xmlTagName());
		auto cable_model_element = model_element.firstChildElement(
					CableListModel::xmlTagName());

		if (!cable_model_element.isNull())
		{
			auto model_ = new CableListModel(
						this->diagram()->project(),
						this->diagram()->project());
			model_->fromXml(cable_model_element);
			this->setModel(model_);
		}
		else
		{
			auto model_ = new ProjectDBModel(
						this->diagram()->project(),
						this->diagram()->project());
			model_->fromXml(project_model_element);
			this->setModel(model_);
		}
	}

	//Restore the header from xml
	m_header_item->fromXml(
				dom_element.firstChildElement(
					QetGraphicsHeaderItem::xmlTagName()));
	emit changed();
}

/**
	@brief QetGraphicsTableItem::toDXF
	Draw this table to the dxf document
	@param filepath file path of the the dxf document
	@return true if draw success
*/
bool QetGraphicsTableItem::toDXF(const QString &filepath)
{
	// Header
	m_header_item->toDXF(filepath);

	//QRectF rect = boundingRect();
	QRectF rect(0,0, m_header_item->rect().width(), m_current_size.height());
	QPolygonF poly(rect);
	Createdxf::drawPolygon(filepath,mapToScene(poly),0);

	//Draw vertical lines
	auto offset= 0;
	for(auto i=0 ; i<m_model->columnCount() ; ++i)
	{
		QPointF p1(offset+m_header_item->sectionSize(i), 0);
		QPointF p2(
					offset+m_header_item->sectionSize(i),
					m_current_size.height());
		Createdxf::drawLine(
					filepath,
					QLineF(mapToScene(p1),mapToScene(p2)),
					0);
		offset += m_header_item->sectionSize(i);
	}
	//Calculate the number of rows to display.
	auto row_count = m_model->rowCount();

	if (m_previous_table) //Remove the number of row already displayed by previous tables
		row_count -= m_previous_table->displayNRowOffset();

	if (m_number_of_row_to_display > 0) //User override the number of row to display
		row_count = std::min(row_count, m_number_of_row_to_display);

		//Draw horizontal lines
	auto cell_height =  static_cast<double>(m_current_size.height())/static_cast<double>(row_count);
	for(auto i= 1 ; i-1<row_count ; ++i)
	{
		QPointF p1(m_header_item->rect().left(), cell_height*i);
		QPointF p2(m_header_item->rect().right(), cell_height*i);
		Createdxf::drawLine(
					filepath,
					QLineF(mapToScene(p1),mapToScene(p2))
					,0);
	}

	//Write text of each cell
	for (auto i=0 ; i<row_count ; ++i)
	{
		auto margin_ = QETUtils::marginsFromString(m_model->index(0,0).data(Qt::UserRole+1).toString());
		QPointF top_left(margin_.left(), i==0? margin_.top() : cell_height*i + margin_.top());

		for(auto j= 0 ; j<m_model->columnCount() ; ++j)
		{
			//In first iteration the top left X is margin left,
			// in all other iteration the top left X is stored in m_column_size
			if (j>0) {
				top_left.setX(top_left.x() + m_header_item->sectionSize(j-1));
			}
			QSize size(m_header_item->sectionSize(j) - margin_.left() - margin_.right(),
				   static_cast<int>(cell_height) - margin_.top() - margin_.bottom());
			auto index_row = m_previous_table ? i + m_previous_table->displayNRowOffset() : i;

			QPointF qm = mapToScene(top_left);
			qreal h = size.height();//  * Createdxf::yScale;
			qreal x = qm.x() * Createdxf::xScale;
			qreal y = Createdxf::sheetHeight -  ((qm.y() + h/2) * Createdxf::yScale);
			qreal h1 = h * 0.5 * Createdxf::yScale;

			int valign = 2;

			Createdxf::drawTextAligned(
						filepath,
						m_model->index(index_row, j).data().toString(),
						x,y,h1,0,0,0,valign,x,0,0);
		}
	}
	return true;
}

/**
	@brief QetGraphicsTableItem::hoverEnterEvent
	Reimplemented from QetGraphicsItem
	@param event
*/
void QetGraphicsTableItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	if (m_model && !customFormatting())
	{
		adjustHandlerPos();
		this->scene()->addItem(&m_handler_item);
		m_handler_item.setSize(QETUtils::graphicsHandlerSize(this));
		m_handler_item.installSceneEventFilter(this);
	}
	QGraphicsObject::hoverEnterEvent(event);
}

/**
	@brief QetGraphicsTableItem::hoverLeaveEvent
	Reimplemented from QetGraphicsItem
	@param event
*/
void QetGraphicsTableItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if(!m_resize_active18)unsetCursor();
	if (m_handler_item.scene() &&
		!m_handler_item.contains(mapToItem(&m_handler_item, event->pos())))
	{
		m_handler_item.scene()->removeItem(&m_handler_item);
	}
	QGraphicsObject::hoverLeaveEvent(event);
}

/**
	@brief QetGraphicsTableItem::sceneEventFilter
	Reimplemented from QetGraphicsItem
	@param watched
	@param event
	@return
*/
bool QetGraphicsTableItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
	if (watched == &m_handler_item)
	{
		if(event->type() == QEvent::GraphicsSceneMousePress) //Click
		{
			handlerMousePressEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
			return true;
		}
		else if(event->type() == QEvent::GraphicsSceneMouseMove) //Move
		{
			handlerMouseMoveEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
			return true;
		}
		else if (event->type() == QEvent::GraphicsSceneMouseRelease) //Release
		{
			handlerMouseReleaseEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
			return true;
		}
	}

	return false;
}

/**
	@brief QetGraphicsTableItem::itemChange
	@param change
	@param value
	@return
*/
QVariant QetGraphicsTableItem::itemChange(
		QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
		//item was removed from scene, we remove the handler
	if ((change==ItemSelectedHasChanged && !value.toBool()) || change==ItemSceneChange)cancelResize18();
	if (change == ItemSceneHasChanged) {
		if (!scene() && m_handler_item.scene()) {
			m_handler_item.scene()->removeItem(&m_handler_item);
		}
	}

	return QetGraphicsItem::itemChange(change, value);
}

/**
 * @brief QetGraphicsTableItem::modelReseted
 */
void QetGraphicsTableItem::modelReseted()
{
	dataChanged(m_model->index(0,0), m_model->index(0,0), QVector<int>());
	setToMinimumHeight();

	if (!previousTable() && !m_model->property("qetEditDefinition15").toBool()) // Preserve continuation tables during undoable definition edits.
	{
		checkInsufficientRowsCount(this);
		removeUselessNextTable();
	}
}

/**
	@brief QetGraphicsTableItem::setUpColumnAndRowMinimumSize
	Calculate the minimum row height and the minimum column width for each columns
	this function doesn't change the geometry of the table.
*/
void QetGraphicsTableItem::setUpColumnAndRowMinimumSize()
{
	if (!m_model)
	{
		m_minimum_row_height = no_model_height;
		m_minimum_column_width = m_header_item->minimumSectionWidth();
		return;
	}

	QFontMetrics metrics(m_model->data(model()->index(0,0), Qt::FontRole).value<QFont>());
	auto margin_ = QETUtils::marginsFromString(model()->index(0,0).data(Qt::UserRole+1).toString());
		//Set the height of row;
	m_minimum_row_height = metrics.boundingRect("HEIGHT TEST").height() + margin_.top() + margin_.bottom();

	m_minimum_column_width = m_header_item->minimumSectionWidth();

		//Get the maximum width of each columns
	for (auto row= 0 ; row<m_model->rowCount() ; ++row)
	{
		for(auto col= 0 ; col<m_model->columnCount() ; ++col)
		{
			auto index = m_model->index(row, col);
			auto width = metrics.boundingRect(index.data().toString()).width();
			m_minimum_column_width.replace(
						col,
						std::max(
							m_minimum_column_width.at(col),
							width + margin_.left() + margin_.right()));
		}
	}
}

/**
	@brief QetGraphicsTableItem::setUpBoundingRect
*/
void QetGraphicsTableItem::setUpBoundingRect()
{
	if(customFormatting()){refreshCustomGeometry();return;}
	QSize header_size = m_header_item->rect().size();
	QRect rect(
				0,
				-header_size.height(),
				header_size.width(),
				m_current_size.height() + header_size.height());
	m_bounding_rect = rect.adjusted(
				-m_br_margin,
				-m_br_margin,
				m_br_margin,
				m_br_margin);
}

/**
	@brief QetGraphicsTableItem::adjustHandlerPos
	Adjust the pos of the handler item
*/
void QetGraphicsTableItem::adjustHandlerPos()
{
	m_handler_item.setPos(mapToScene(QRect(QPoint(0,0), size()).bottomRight()));
}

/**
	@brief QetGraphicsTableItem::setUpHandler
*/
void QetGraphicsTableItem::setUpHandler()
{
	m_handler_item.setColor(Qt::blue);
	m_handler_item.setZValue(this->zValue() + 1);
}

void QetGraphicsTableItem::handlerMousePressEvent(
		QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event)
	diagram()->clearSelection();
	this->setSelected(true);
	m_old_size = size();
}

void QetGraphicsTableItem::handlerMouseMoveEvent(
		QGraphicsSceneMouseEvent *event)
{
	auto new_handler_pos = Diagram::snapToGrid(event->scenePos());
	QSize size_ = QRectF(QPointF(0,0), mapFromScene(new_handler_pos)).size().toSize();

	QPoint new_pos(
				std::max(minimumSize().width(), size_.width()),
				std::max(minimumSize().height(), size_.height()));
	m_handler_item.setPos(mapToScene(new_pos));

	QSize new_size(new_pos.x(), new_pos.y());
	if (new_size != size()) {
		setSize(new_size);
	}
}

void QetGraphicsTableItem::handlerMouseReleaseEvent(
		QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event)
	if (diagram())
	{
		auto undo = new QPropertyUndoCommand(this, "size", m_old_size, size());
		undo->setAnimated();
		undo->setText(tr("Modify the geometry of a table"));
		diagram()->undoStack().push(undo);
	}
}

/**
	@brief QetGraphicsTableItem::adjustColumnsWidth
	Adjust the size of each column according to the current table width
	by setting the sectionSize of the header item
*/
void QetGraphicsTableItem::adjustColumnsWidth()
{
	if(customFormatting()){refreshCustomGeometry();return;}
	if (!m_model)
	{
		auto h_  = m_header_item->minimumSectionWidth();
		for (auto i=0 ; i<h_.size() ; ++i) {
			m_header_item->resizeSection(i, h_.at(i));
		}
		return;
	}

	auto a = m_current_size.width() - minimumSize().width();
	auto b = a/std::max(1,m_model->columnCount()); //avoid divide by 0

	int sum_=0;
	for(auto i= 0 ; i<m_model->columnCount() ; ++i)
	{
		auto at_a = std::min((int)m_minimum_column_width.size()-1, i);               //In case of the I is higher than m_minimum_column_width or
		auto at_b = std::min((int)m_header_item->minimumSectionWidth().size()-1, i); //m_header_item->minimumSectionWidth().size()
		m_header_item->resizeSection(
					i,
					std::max(
						m_minimum_column_width.at(at_a),
						m_header_item->minimumSectionWidth().at(at_b))+b);
		sum_+= m_header_item->sectionSize(i);
	}


	//The sum of the header sections width can be less than width of
	// @m_current_size we adjust it in order to have the same width
	if (m_model->columnCount() > 0 &&
		sum_ < m_current_size.width())
	{
		//add the quotient of the division to each columns
		auto result = (m_current_size.width()-sum_)/m_model->columnCount();
		for(auto i= 0 ; i<m_model->columnCount() ; ++i) {
			m_header_item->resizeSection(
						i,
						m_header_item->sectionSize(i) + result);
		}

			//add the rest of the division to the last column
		auto last_section = m_model->columnCount()-1;
		m_header_item->resizeSection(
					last_section,
					m_header_item->sectionSize(last_section)
					+ ((m_current_size.width()-sum_)%m_model->columnCount()));
	}

	update();
}

void QetGraphicsTableItem::dataChanged(
		const QModelIndex &topLeft,
		const QModelIndex &bottomRight,
		const QVector<int> &roles)
{
	Q_UNUSED(topLeft)
	Q_UNUSED(bottomRight)
	Q_UNUSED(roles)

	setUpColumnAndRowMinimumSize();
	adjustSize();
	update();
	emit changed();
}

/**
	@brief QetGraphicsTableItem::adjustSize
	If needed, this function resize the current height and width of table and/or the size of columns.
	according to there minimum
*/
void QetGraphicsTableItem::adjustSize()
{
	if(customFormatting()){refreshCustomGeometry();return;}
		//If there is no model, set the size to minimum
	if (!m_model) {
		setSize(minimumSize());
		return;
	}

	if (m_current_size.height() < minimumSize().height())
	{
		prepareGeometryChange();
		m_current_size.setHeight(minimumSize().height());
		setUpBoundingRect();
		update();
	}

	if (m_current_size.width() < minimumSize().width())		//The current width is shorter than the the minimum width, adjust it
	{
		prepareGeometryChange();
		m_current_size.setWidth(minimumSize().width());
		adjustColumnsWidth();
		setUpBoundingRect();
	}
	else { //The current width is ok, but we need to adjust all columns width
		adjustColumnsWidth();
	}
}

void QetGraphicsTableItem::previousTableDisplayRowChanged()
{
	setToMinimumHeight();
	if (m_next_table) {
		m_next_table->previousTableDisplayRowChanged();
	}
}

/**
 * @brief QetGraphicsTableItem::removeUselessNextTable
 * Remove next table if useless, an useless table with 0 row displayed.
 * If \p recursive is true check and remove for all sub next table.
 * @param recursive
 */
void QetGraphicsTableItem::removeUselessNextTable(bool recursive)
{
	if (m_next_table)
	{
		if (recursive) {
			m_next_table->removeUselessNextTable();
		}
		if (m_next_table->displayedRowCount() <= 0)
		{
			m_next_table.data()->deleteLater();
			m_next_table.clear();
		}
	}
}

int QetGraphicsTableItem::formattedRows() const {
    int rows=QetFormatting::number(m_custom_format,"rows",0,0,10000);
    if(rows)return rows;
    int offset=m_previous_table?m_previous_table->displayNRowOffset():0;
    return qMax(0,m_model?m_model->rowCount()-offset:0);
}
void QetGraphicsTableItem::refreshCustomGeometry() {
    prepareGeometryChange();
    const int offset=m_previous_table?m_previous_table->displayNRowOffset():0;
    const QFont fallback=m_model?m_model->index(0,0).data(Qt::FontRole).value<QFont>():QFont();
    const auto g=QetFormatting::tableLayout(m_custom_format,m_model,offset,formattedRows(),fallback);
    m_current_size=QSize(g.width,g.bodyHeight);
    m_bounding_rect=g.bounds.adjusted(-10,-10,10,10);
    if(m_header_item)m_header_item->hide();adjustHandlerPos();update();
}
void QetGraphicsTableItem::setCustomFormat(const QVariantMap &format) {
    const bool changed18=m_custom_format!=format;
    prepareGeometryChange();m_custom_format=format;m_header_item->setVisible(!customFormatting());
    if(customFormatting()&&m_handler_item.scene())m_handler_item.scene()->removeItem(&m_handler_item);
    if(customFormatting())refreshCustomGeometry();else{setUpColumnAndRowMinimumSize();adjustSize();setUpBoundingRect();}
    if(m_next_table)m_next_table->previousTableDisplayRowChanged();update();
    if(changed18)emit customFormatChanged18(m_custom_format);
}

void QetGraphicsTableItem::editProperty() { QetTableEditor15::edit(this); }

bool QetGraphicsTableItem::canResize18() const {
    return customFormatting()&&m_model&&isMovable()&&flags().testFlag(ItemIsMovable)
            &&(!diagram()||!diagram()->isReadOnly());
}

bool QetGraphicsTableItem::resizeBoundary18(const QPointF &position,QWidget *viewport,int &column,int &row) const {
    column=row=-1;
    if(!isSelected()||!canResize18())return false;
    QGraphicsView *view=nullptr;
    if(scene())for(auto *candidate:scene()->views())if(!viewport||candidate->viewport()==viewport){view=candidate;break;}
    qreal tx=5,ty=5;
    if(view){const auto transform=deviceTransform(view->viewportTransform());
        tx=5/qMax(qreal(0.001),std::hypot(transform.m11(),transform.m12()));
        ty=5/qMax(qreal(0.001),std::hypot(transform.m21(),transform.m22()));}
    const int offset=m_previous_table?m_previous_table->displayNRowOffset():0;
    const auto g=QetFormatting::tableLayout(m_custom_format,m_model,offset,formattedRows(),m_model->index(0,0).data(Qt::FontRole).value<QFont>());

    qreal columnDistance=2,rowDistance=2;
    if(position.y()>=-g.headerHeight-g.titleHeight-ty&&position.y()<=g.bodyHeight+ty){
        if(qAbs(position.x())<=tx){column=-2;columnDistance=qAbs(position.x())/tx;}
        int x=0;for(int c=0;c<g.widths.size();++c){x+=g.widths[c];qreal d=qAbs(position.x()-x)/tx;if(d<=1&&d<columnDistance){column=c;columnDistance=d;}}
    }
    if(position.x()>=-tx&&position.x()<=g.width+tx){
        auto candidate=[&](qreal y,int id){qreal d=qAbs(position.y()-y)/ty;if(d<=1&&d<rowDistance){row=id;rowDistance=d;}};
        candidate(-g.headerHeight-g.titleHeight,g.titleHeight?-2:-3);
        if(g.titleHeight)candidate(-g.headerHeight,-4);
        int y=0;for(int rr=0;rr<g.rowHeights.size();++rr){y+=g.rowHeights[rr];candidate(y,offset+rr);}
    }
    if(column!=-1&&row!=-1){if(columnDistance<=rowDistance)row=-1;else column=-1;}
    return column!=-1||row!=-1;
}

bool QetGraphicsTableItem::commitResize18(const QVariantMap &before,const QVariantMap &after,bool undo,const QString &text) {
    if(before==after)return false;
    if(undo&&diagram()){
        // A live drag may already display after. Restore the starting state so
        // the command owns both the initial redo and every later Undo/Redo.
        setCustomFormat(before);
        // A completed gesture is one independent action. QPropertyUndoCommand
        // normally merges successive writes to the same property, which would
        // silently combine separate column/row drags. The parent has the
        // default nonmergeable id and owns the actual property operation.
        auto *command=new QUndoCommand(text);
        new QPropertyUndoCommand(this,"customFormat",before,after,command);
        diagram()->undoStack().push(command);
    }else setCustomFormat(after);
    return true;
}

bool QetGraphicsTableItem::resizeColumn18(int column,int width,bool undo) {
    if(!canResize18()||column<0||column>=m_model->columnCount())return false;
    const auto before=m_custom_format;auto after=before;
    auto widths=QetFormatting::tableWidths(before,m_model->columnCount());widths[column]=qBound(10,width,10000);
    QVariantList values;for(int value:widths)values<<value;after["widths"]=values;
    auto automatic=before.value("autoWidths").toList();while(automatic.size()<widths.size())automatic<<false;automatic[column]=false;after["autoWidths"]=automatic;
    return commitResize18(before,after,undo,tr("Resize table column"));
}

bool QetGraphicsTableItem::resizeRow18(int absoluteModelRow,int height,bool undo) {
    if(!canResize18()||absoluteModelRow<0)return false;
    const auto before=m_custom_format;auto after=before;auto rows=after.value("rowHeights18").toMap();
    rows[QString::number(absoluteModelRow)]=qBound(6,height,10000);after["rowHeights18"]=rows;
    return commitResize18(before,after,undo,tr("Resize table row"));
}

bool QetGraphicsTableItem::autoFitColumn18(int column,bool undo) {
    if(!canResize18()||column<0||column>=m_model->columnCount())return false;
    const int offset=m_previous_table?m_previous_table->displayNRowOffset():0;
    const auto widths=QetFormatting::fittedColumnWidths(m_custom_format,m_model,offset,formattedRows(),m_model->index(0,0).data(Qt::FontRole).value<QFont>());
    return resizeColumn18(column,widths[column],undo);
}

bool QetGraphicsTableItem::autoFitRow18(int absoluteModelRow,bool undo) {
    if(!canResize18()||absoluteModelRow<0)return false;
    const int offset=m_previous_table?m_previous_table->displayNRowOffset():0;const auto font=m_model->index(0,0).data(Qt::FontRole).value<QFont>();
    const auto geometry=QetFormatting::tableLayout(m_custom_format,m_model,offset,formattedRows(),font);
    return resizeRow18(absoluteModelRow,QetFormatting::fittedRowHeight18(m_custom_format,m_model,absoluteModelRow,geometry.widths,font),undo);
}

void QetGraphicsTableItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    int column,row;
    if(resizeBoundary18(event->pos(),event->widget(),column,row))setCursor(column!=-1?Qt::SplitHCursor:Qt::SplitVCursor);
    else unsetCursor();
    QetGraphicsItem::hoverMoveEvent(event);
}

void QetGraphicsTableItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if(m_resize_active18&&event->button()==Qt::RightButton){cancelResize18();event->accept();return;}
    m_resize_cancelled18=false;
    int column,row;
    if(event->button()==Qt::LeftButton&&!(event->modifiers()&Qt::ControlModifier)&&resizeBoundary18(event->pos(),event->widget(),column,row)){
        setFocus(Qt::MouseFocusReason);m_resize_before18=m_custom_format;m_resize_press18=event->pos();m_resize_scene25=event->scenePos();m_resize_position25=pos();m_resize_column18=column;m_resize_row18=row;
        const int offset=m_previous_table?m_previous_table->displayNRowOffset():0;
        const auto g=QetFormatting::tableLayout(m_custom_format,m_model,offset,formattedRows(),m_model->index(0,0).data(Qt::FontRole).value<QFont>());
        m_resize_start18=0;m_resize_active18=true;event->accept();return;
    }
    if(event->button()==Qt::LeftButton&&isSelected()&&customFormatting()){
        const auto cell=cellAt20(event->pos());
        if(cell.second>=-2){
            if((event->modifiers()&Qt::ShiftModifier)&&m_anchor20.second>=-1&&cell.second>=-1){QSet<QPair<int,int>> cells;for(int r=qMin(cell.second,m_anchor20.second);r<=qMax(cell.second,m_anchor20.second);++r)for(int c=qMin(cell.first,m_anchor20.first);c<=qMax(cell.first,m_anchor20.first);++c)cells.insert({c,r});setSelectedCells20(cells);}
            else if(event->modifiers()&Qt::ControlModifier){auto cells=m_cells20;if(cells.contains(cell))cells.remove(cell);else cells.insert(cell);setSelectedCells20(cells);}
            else {m_anchor20=cell;setSelectedCells20({cell});}
            if(event->modifiers()&(Qt::ShiftModifier|Qt::ControlModifier)){event->accept();return;}
        }
    }
    QetGraphicsItem::mousePressEvent(event);
}

void QetGraphicsTableItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if(m_resize_cancelled18){event->accept();return;}
    if(m_resize_active18){
        if(!canResize18()){cancelResize18();event->accept();return;}
        auto delta=mapFromScene(event->scenePos())-mapFromScene(m_resize_scene25);
        if(m_resize_column18==-2){
            const int off=m_previous_table?m_previous_table->displayNRowOffset():0;
            auto g=QetFormatting::tableLayout(m_resize_before18,m_model,off,formattedRows(),m_model->index(0,0).data(Qt::FontRole).value<QFont>());
            if(!g.widths.isEmpty()){
                int dx=qBound(g.widths[0]-10000,qRound(delta.x()),g.widths[0]-10);
                auto after=m_resize_before18;g.widths[0]-=dx;QVariantList widths,automatic;
                for(int w:g.widths){widths<<w;automatic<<false;}after["widths"]=widths;after["autoWidths"]=automatic;
                setCustomFormat(after);setPos(m_resize_position25+mapToParent(QPointF(dx,0))-mapToParent(QPointF()));
            }
        }else resizeBoundary20(m_resize_column18,m_resize_row18,qRound(m_resize_column18>=0?delta.x():delta.y()),m_resize_before18,false);
        event->accept();return;
    }
    QetGraphicsItem::mouseMoveEvent(event);
}

void QetGraphicsTableItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if(m_resize_cancelled18){m_resize_cancelled18=false;event->accept();return;}
    if(m_resize_active18){const auto before=m_resize_before18,after=m_custom_format;m_resize_active18=false;m_resize_column18=m_resize_row18=-1;unsetCursor();
        auto finalPos=pos();setCustomFormat(before);setPos(m_resize_position25);
        if(diagram()&&(before!=after||finalPos!=m_resize_position25)){
            auto *command=new QUndoCommand(tr("Resize table boundary"));
            new QPropertyUndoCommand(this,"customFormat",before,after,command);
            if(finalPos!=m_resize_position25)new QPropertyUndoCommand(this,"pos",m_resize_position25,finalPos,command);
            diagram()->undoStack().push(command);
        }else {setCustomFormat(after);setPos(finalPos);}
        event->accept();return;}
    QetGraphicsItem::mouseReleaseEvent(event);
}

void QetGraphicsTableItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    int column,row;
    if(event->button()==Qt::LeftButton&&resizeBoundary18(event->pos(),event->widget(),column,row)){
        cancelResize18();if(column>=0)autoFitColumn18(column);else if(row>=0)autoFitRow18(row);event->accept();return;
    }
    if(event->button()==Qt::LeftButton&&isSelected()&&canResize18()){
        auto cell=cellAt20(event->pos());
        if(cell.second>=-2){setSelectedCells20({cell});editCellText25(cell.first,cell.second,event->widget());event->accept();return;}
    }
    QetGraphicsItem::mouseDoubleClickEvent(event);
}

void QetGraphicsTableItem::cancelResize18() {
    if(!m_resize_active18)return;
    m_resize_active18=false;m_resize_cancelled18=true;m_resize_column18=m_resize_row18=-1;setCustomFormat(m_resize_before18);setPos(m_resize_position25);unsetCursor();
}
void QetGraphicsTableItem::keyPressEvent(QKeyEvent *event) {
    if(event->key()==Qt::Key_Escape&&m_resize_active18){cancelResize18();event->accept();return;}
    if(event->key()==Qt::Key_F2&&m_cells20.size()==1&&canResize18()){
        auto cell=*m_cells20.cbegin();editCellText25(cell.first,cell.second);event->accept();return;
    }
    QGraphicsObject::keyPressEvent(event);
}
void QetGraphicsTableItem::focusOutEvent(QFocusEvent *event) {cancelResize18();QGraphicsObject::focusOutEvent(event);}
bool QetGraphicsTableItem::sceneEvent(QEvent *event) {
    if(event->type()==QEvent::UngrabMouse)cancelResize18();
    return QetGraphicsItem::sceneEvent(event);
}

QRectF QetGraphicsTableItem::cellRect20(int c,int r) const {
 if(!m_model)return {};const int off=m_previous_table?m_previous_table->displayNRowOffset():0;
 const auto g=QetFormatting::tableLayout(m_custom_format,m_model,off,formattedRows(),m_model->index(0,0).data(Qt::FontRole).value<QFont>());
 if(r==-2)return QRectF(0,-g.headerHeight-g.titleHeight,g.width,g.titleHeight);
 if(c<0||c>=g.widths.size())return {};int x=0;for(int i=0;i<c;++i)x+=g.widths[i];
 if(r==-1)return QRectF(x,-g.headerHeight,g.widths[c],g.headerHeight);
 if(r<off||r>=off+g.rowHeights.size())return {};int y=0;for(int i=0;i<r-off;++i)y+=g.rowHeights[i];return QRectF(x,y,g.widths[c],g.rowHeights[r-off]);
}
QPair<int,int> QetGraphicsTableItem::cellAt20(const QPointF &p) const {
 if(!m_model)return {-1,-3};const int off=m_previous_table?m_previous_table->displayNRowOffset():0;
 const auto g=QetFormatting::tableLayout(m_custom_format,m_model,off,formattedRows(),m_model->index(0,0).data(Qt::FontRole).value<QFont>());
 if(p.x()<0||p.x()>g.width)return {-1,-3};
 if(g.titleHeight&&p.y()>=-g.headerHeight-g.titleHeight&&p.y()<-g.headerHeight)return {-1,-2};
 int c=-1,x=0;for(int i=0;i<g.widths.size();++i){x+=g.widths[i];if(p.x()<=x){c=i;break;}}if(c<0)return {-1,-3};
 if(p.y()>=-g.headerHeight&&p.y()<0)return {c,-1};if(p.y()<0)return {-1,-3};
 int y=0;for(int r=0;r<g.rowHeights.size();++r){y+=g.rowHeights[r];if(p.y()<=y)return {c,off+r};}return {-1,-3};
}
bool QetGraphicsTableItem::resizeBoundary20(int column,int row,int delta,const QVariantMap &start,bool undo){
 if(!canResize18())return false;const int off=m_previous_table?m_previous_table->displayNRowOffset():0;
 const auto g=QetFormatting::tableLayout(start,m_model,off,formattedRows(),m_model->index(0,0).data(Qt::FontRole).value<QFont>());auto after=start;
 if(column==-1&&row<-1){
  if(row==-2)after["titleHeight"]=qBound(6,g.titleHeight-delta,10000);
  else if(row==-3){after["headerHeight"]=qBound(6,g.headerHeight-delta,10000);after["autoHeaderHeight"]=false;}
  else if(row==-4){delta=qBound(6-g.titleHeight,delta,g.headerHeight-6);after["titleHeight"]=g.titleHeight+delta;after["headerHeight"]=g.headerHeight-delta;after["autoHeaderHeight"]=false;}
  return commitResize18(start,after,undo,tr("Resize table title / header"));
 }
 if(column>=0){if(column>=g.widths.size())return false;auto widths=g.widths;auto automatic=start.value("autoWidths").toList();while(automatic.size()<widths.size())automatic<<false;
  const bool paired=start.value("columnResize20","keep").toString()=="keep"&&column+1<widths.size();
  delta=qBound(10-widths[column],delta,paired?qMin(10000-widths[column],widths[column+1]-10):10000-widths[column]);widths[column]+=delta;automatic[column]=false;
  if(paired){widths[column+1]-=delta;automatic[column+1]=false;}QVariantList values;for(int w:widths)values<<w;after["widths"]=values;after["autoWidths"]=automatic;
 }else {int index=row-off;if(index<0||index>=g.rowHeights.size())return false;auto heights=g.rowHeights;const bool paired=start.value("rowResize20","expand").toString()=="keep"&&index+1<heights.size();
  delta=qBound(6-heights[index],delta,paired?qMin(10000-heights[index],heights[index+1]-6):10000-heights[index]);auto overrides=start.value("rowHeights18").toMap();overrides[QString::number(row)]=heights[index]+delta;if(paired)overrides[QString::number(row+1)]=heights[index+1]-delta;after["rowHeights18"]=overrides;
 }
 return commitResize18(start,after,undo,tr("Resize adjacent table cells"));
}

bool QetGraphicsTableItem::setCellText25(int column,int row,const QString &text,bool restore) {
    if(!canResize18()||row<-2)return false;
    auto before=m_custom_format,after=before;
    if(row==-2){
        auto *db=qobject_cast<ProjectDBModel*>(m_model);auto data=db&&db->project()?db->project()->languageData():QVariantMap();
        auto lang=QetManualCells25::language(m_model);
        if(lang==QetLanguages16::sourceLanguage(data))after["title"]=restore?QString():text;
        else {auto titles=after.value("titleTexts26").toMap();if(restore)titles.remove(lang);else titles[lang]=text;after["titleTexts26"]=titles;}
    }
    else if(row==-1){
        if(column<0||column>=m_model->columnCount())return false;
        auto *db=qobject_cast<ProjectDBModel*>(m_model);auto data=db&&db->project()?db->project()->languageData():QVariantMap();
        auto lang=QetManualCells25::language(m_model);auto key=QetManualCells25::headerKey(m_model,column);
        if(lang!=QetLanguages16::sourceLanguage(data)){
            if(key.isEmpty())return false;auto languages=after.value("headerTexts25").toMap();auto edits=languages.value(lang).toMap();
            if(restore)edits.remove(key);else edits[key]=text;
            languages[lang]=edits;after["headerTexts25"]=languages;
            return commitResize18(before,after,true,restore?tr("Restore translated header"):tr("Edit translated header"));
        }
        auto headers=after.value("headers").toList();
        while(headers.size()<m_model->columnCount()){
            int c=headers.size();auto raw=m_model->headerData(c,Qt::Horizontal,QetLanguages16::SourceTextRole16);
            headers<<(raw.isValid()?raw:m_model->headerData(c,Qt::Horizontal));
        }
        auto raw=m_model->headerData(column,Qt::Horizontal,QetLanguages16::SourceTextRole16);
        headers[column]=restore?(raw.isValid()?raw:m_model->headerData(column,Qt::Horizontal)):QVariant(text);
        after["headers"]=headers;
    }else{
        QString key=QetManualCells25::key(m_model,row,column);if(key.isEmpty())return false;
        auto edits=after.value("manualCells25").toMap();
        if(restore)edits.remove(key);
        else edits[key]=QVariantMap{{"text",text},{"column",QetManualCells25::cache(m_model)->columns[column]},
             {"language",QetManualCells25::language(m_model)},{"original",row<m_model->rowCount()?m_model->index(row,column).data().toString():QString()}};
        after["manualCells25"]=edits;
    }
    return commitResize18(before,after,true,restore?tr("Restore generated table text"):tr("Edit table cell text"));
}
void QetGraphicsTableItem::editCellText25(int column,int row,QWidget *parent){
    if(!canResize18()||row<-2)return;
    if(row>=0&&QetManualCells25::key(m_model,row,column).isEmpty()){
        QMessageBox::information(parent,tr("Cell text"),tr("This row or column is not unique. Include a unique identifier in the table fields before editing, so your text cannot move to a different row when the table refreshes."));return;
    }
    QDialog d(parent);d.setWindowTitle(tr("Edit cell text"));d.setObjectName("editCellText25");d.resize(480,300);
    auto *layout=new QVBoxLayout(&d);
    auto *note=new QLabel(row==-2?tr("Editing title in drawing language: %1. Other language versions are preserved.").arg(QetManualCells25::language(m_model)):row==-1?tr("This header is edited for the current drawing language. Other languages and the underlying field name are preserved."):tr("Your text is saved with this table. Generated device data is unchanged. If the source row changes, the edit is retained for review."));note->setWordWrap(true);layout->addWidget(note);
    auto *editor=new QPlainTextEdit;editor->setObjectName("cellText25");
    editor->setPlainText(row==-2?QetFormatting::tableTitle26(m_custom_format,m_model):row==-1?QetFormatting::columnHeading(m_custom_format,m_model,column):QetManualCells25::text(m_custom_format,m_model,row,column));layout->addWidget(editor);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    auto *restore=buttons->addButton(row==-2?tr("Restore / clear this language title"):tr("Restore generated value"),QDialogButtonBox::ResetRole);
    bool reset=false;
    connect(restore,&QPushButton::clicked,&d,[&]{reset=true;d.accept();});
    connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);layout->addWidget(buttons);
    if(d.exec()==QDialog::Accepted)setCellText25(column,row,editor->toPlainText(),reset);
}
void QetGraphicsTableItem::reviewCellText25(QWidget *parent){
    if(!canResize18())return;
    auto before=m_custom_format;auto edits=before.value("manualCells25").toMap();
    auto keys=QetManualCells25::currentKeys(m_model,formattedRows()+(m_previous_table?m_previous_table->displayNRowOffset():0));
    QDialog d(parent);d.setWindowTitle(tr("Saved cell text"));d.resize(750,400);auto *layout=new QVBoxLayout(&d);
    auto *note=new QLabel(tr("Unmatched edits are saved but are not displayed on a different source row. Copy any text you want to reuse, or select edits and restore their generated values."));note->setWordWrap(true);layout->addWidget(note);
    auto *list=new QTableWidget(edits.size(),5);list->setHorizontalHeaderLabels({tr("Status"),tr("Field"),tr("Language"),tr("Original text"),tr("Your text")});list->setSelectionBehavior(QAbstractItemView::SelectRows);list->setEditTriggers(QAbstractItemView::NoEditTriggers);list->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);layout->addWidget(list);
    int r=0;for(auto it=edits.cbegin();it!=edits.cend();++it,++r){auto v=it.value().toMap();QString status=keys.contains(it.key())?tr("Matched"):v.value("language").toString()!=QetManualCells25::language(m_model)?tr("Other language"):tr("Unmatched");QStringList values{status,v.value("column").toString(),v.value("language").toString(),v.value("original").toString(),v.value("text").toString()};for(int c=0;c<5;++c)list->setItem(r,c,new QTableWidgetItem(values[c]));list->item(r,0)->setData(Qt::UserRole,it.key());}
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close);auto *restore=buttons->addButton(tr("Restore selected values"),QDialogButtonBox::ActionRole);layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);
    connect(restore,&QPushButton::clicked,&d,[&]{QSet<int> rows;for(auto *item:list->selectedItems())rows.insert(item->row());for(int row:rows)edits.remove(list->item(row,0)->data(Qt::UserRole).toString());if(!rows.isEmpty())d.accept();});
    if(d.exec()==QDialog::Accepted){auto after=before;after["manualCells25"]=edits;commitResize18(before,after,true,tr("Restore generated table values"));}
}
