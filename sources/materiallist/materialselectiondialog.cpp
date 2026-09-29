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
#include "materialselectiondialog.h"

#include "materialentrydialog.h"
#include "../qetmessagebox.h"
#include "ui_materialselectiondialog.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QWheelEvent>

/**
	@brief MaterialTableModel::MaterialTableModel
	Constructor
	@param parent parent object
*/
MaterialTableModel::MaterialTableModel(QObject *parent) :
	QAbstractTableModel(parent)
{
}

/**
	@brief MaterialTableModel::setMaterial
	Replace the displayed file content
	@param columns
	@param records
*/
void MaterialTableModel::setMaterial(const QStringList &columns, const QList<MaterialRecord> &records)
{
	beginResetModel();
	m_columns = columns;
	m_records = records;
	endResetModel();
}

/**
	@brief MaterialTableModel::record
	@param row
	@return the record of that row, a null one when out of range
*/
MaterialRecord MaterialTableModel::record(int row) const
{
	return m_records.value(row);
}

/**
	@brief MaterialTableModel::rowCount
	@param parent
	@return
*/
int MaterialTableModel::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : m_records.size();
}

/**
	@brief MaterialTableModel::columnCount
	@param parent
	@return
*/
int MaterialTableModel::columnCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : m_columns.size();
}

/**
	@brief MaterialTableModel::data
	@param index
	@param role
	@return
*/
QVariant MaterialTableModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()
		|| index.row() < 0 || index.row() >= m_records.size()
		|| index.column() < 0 || index.column() >= m_columns.size())
	{
		return QVariant();
	}

	const QString value = m_records.at(index.row()).value(m_columns.at(index.column()));

	switch (role)
	{
		case Qt::DisplayRole:
		case Qt::ToolTipRole:
			//The tooltip shows the whole cell once the column is too narrow
			return value;
		default:
			return QVariant();
	}
}

/**
	@brief MaterialTableModel::headerData
	@param section
	@param orientation
	@param role
	@return
*/
QVariant MaterialTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) {
		return QVariant();
	}

	if (orientation == Qt::Horizontal)
	{
		if (section >= 0 && section < m_columns.size()) {
			return MaterialList::translatedColumn(m_columns.at(section));
		}
		return QVariant();
	}

	return section + 1;
}

/**
	@brief MaterialFilterProxy::MaterialFilterProxy
	Constructor
	@param model the material table model, also used as source model
	@param parent parent object
*/
MaterialFilterProxy::MaterialFilterProxy(MaterialTableModel *model, QObject *parent) :
	QSortFilterProxyModel(parent),
	m_model(model)
{
	setSourceModel(model);
	setFilterCaseSensitivity(Qt::CaseInsensitive);
	setDynamicSortFilter(true);
}

/**
	@brief MaterialFilterProxy::setTokens
	Set the words every row must hold to be shown.
	@param tokens
*/
void MaterialFilterProxy::setTokens(const QStringList &tokens)
{
	if (m_tokens == tokens) {
		return;
	}
	m_tokens = tokens;
		//The plain string filter of the base class is not what keeps the
		//rows (filterAcceptsRow() below is), setting it is only how the
		//base class is told to run the filter again.
	setFilterFixedString(tokens.join(QLatin1Char(' ')));
}

/**
	@brief MaterialFilterProxy::filterAcceptsRow
	A row is kept when each token is found in at least one of its cells :
	the words may be spread over different columns, as a search for
	"Hilfsschalter Schneider" expects.

	Every token walks every cell of the row, which means the whole file
	on every keystroke : well within reach for a catalogue, to be
	revisited before someone imports a supplier export of tens of
	thousands of lines.
	@param source_row
	@param source_parent
	@return
*/
bool MaterialFilterProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	Q_UNUSED(source_parent)

	if (m_tokens.isEmpty() || !m_model) {
		return true;
	}

	const QStringList columns = m_model->columns();
	for (const QString &token : m_tokens)
	{
		bool found = false;
		for (int column = 0; column < columns.size(); ++column)
		{
			const QModelIndex index = m_model->index(source_row, column);
			const QString value = m_model->data(index, Qt::DisplayRole).toString();
			if (value.contains(token, Qt::CaseInsensitive))
			{
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}

	return true;
}

/**
	@brief MaterialSelectionDialog::MaterialSelectionDialog
	Constructor
	@param path path of the material file
	@param parent parent widget
*/
MaterialSelectionDialog::MaterialSelectionDialog(const QString &path,
												 QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MaterialSelectionDialog),
	m_path(path)
{
	ui->setupUi(this);

	m_model = new MaterialTableModel(this);
	m_proxy = new MaterialFilterProxy(m_model, this);

	ui->m_table_view->setModel(m_proxy);
	ui->m_table_view->setSortingEnabled(true);
	ui->m_table_view->horizontalHeader()->setStretchLastSection(true);

		//The wheel is taken in hand here : it moves the rows, and the
		//columns when the movement is horizontal or Shift is held.
	ui->m_table_view->viewport()->installEventFilter(this);

		//The row numbers are the order of the file : clicking them, or the
		//corner just above them, sorts the table by them, which means the
		//table is no longer sorted at all.
		//
		//QTableView keeps that corner button to itself, so the only way to
		//reach it is the name Qt gives that private class. Nothing breaks
		//if the name ever changes : the row numbers below stay connected
		//and only this shortcut disappears.
	for (QAbstractButton *button : ui->m_table_view->findChildren<QAbstractButton *>())
	{
		if (button->inherits("QTableCornerButton")) {
			connect(button, &QAbstractButton::clicked,
					this, &MaterialSelectionDialog::resetSorting);
		}
	}
	ui->m_table_view->verticalHeader()->setSectionsClickable(true);
	connect(ui->m_table_view->verticalHeader(), &QHeaderView::sectionClicked,
			this, &MaterialSelectionDialog::resetSorting);

	ui->m_button_box->button(QDialogButtonBox::Ok)->setText(tr("Appliquer"));
	ui->m_button_box->button(QDialogButtonBox::Cancel)->setText(tr("Annuler"));

	connect(ui->m_search_edit, &QLineEdit::textChanged,
			this, &MaterialSelectionDialog::searchChanged);
	if (QItemSelectionModel *selection_model = ui->m_table_view->selectionModel())
	{
		connect(selection_model, &QItemSelectionModel::selectionChanged, this,
				[this](const QItemSelection &, const QItemSelection &) { updateApplyButton(); });
	}

	reload();

	if (const QSize saved_size = MaterialList::savedDialogSize(); saved_size.isValid()) {
		resize(saved_size);
	}

	const int description_column = m_data.columns.indexOf(QStringLiteral("description"));
	if (description_column >= 0) {
		ui->m_table_view->sortByColumn(description_column, Qt::AscendingOrder);
	}

		//Last, so that the columns the user widened, moved or sorted last
		//time are the ones he finds again.
	restoreViewState();

	updateApplyButton();
}

/**
	@brief MaterialSelectionDialog::~MaterialSelectionDialog
	Destructor
*/
MaterialSelectionDialog::~MaterialSelectionDialog()
{
	delete ui;
}

/**
	@brief MaterialSelectionDialog::restoreViewState
	Give the columns back the width, the place and the order they had the
	last time the window was closed.
*/
void MaterialSelectionDialog::restoreViewState()
{
	const QByteArray state = MaterialList::savedHeaderState();
	if (state.isEmpty()) {
		return;
	}
	ui->m_table_view->horizontalHeader()->restoreState(state);

		//Every column of the file is shown, even one an older version of
		//the window had hidden because no record used it yet.
	for (int column = 0; column < ui->m_table_view->model()->columnCount(); ++column) {
		ui->m_table_view->setColumnHidden(column, false);
	}
}

/**
	@brief MaterialSelectionDialog::saveViewState
	Remember how the columns were laid out, and how big the window was.
*/
void MaterialSelectionDialog::saveViewState()
{
	MaterialList::saveHeaderState(ui->m_table_view->horizontalHeader()->saveState());
	MaterialList::saveDialogSize(size());
}

/**
	@brief MaterialSelectionDialog::resetSorting
	Give the table back the order of the file: no column sorted and no
	arrow in the header, the rows counting from one down again.
*/
void MaterialSelectionDialog::resetSorting()
{
		//Sorting by the row numbers is sorting by nothing : without a
		//column to sort by, the proxy writes the rows the way the file
		//does.
	m_proxy->sort(-1, Qt::AscendingOrder);
	ui->m_table_view->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
}

/**
	@brief MaterialSelectionDialog::done
	Every way out of the window goes through here, so a column resized by
	the user is never lost, be it "Apply", "Cancel" or the window manager
	close button.
	@param result
*/
void MaterialSelectionDialog::done(int result)
{
	saveViewState();
	QDialog::done(result);
}

/**
	@brief MaterialSelectionDialog::eventFilter
	The table takes the wheel in hand: the rows must move up and down and
	the columns left and right, whether the platform does anything with
	wheel events or not.
	@param watched
	@param event
	@return true when the event was handled here
*/
bool MaterialSelectionDialog::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == ui->m_table_view->viewport() && event->type() == QEvent::Wheel)
	{
		scrollTable(static_cast<QWheelEvent *>(event));
		return true;
	}
	return QDialog::eventFilter(watched, event);
}

/**
	@brief wheelAmount
	Turn one wheel movement into an amount to scroll.

	@param delta the movement, in eighths of a degree (120 per notch) or
	in pixels, depending on the device
	@param counting_items true when the scroll bar counts rows and columns
	instead of pixels, which is how the scroll bars of a table work
	@return how far the scroll bar moves, in its own unit
*/
static int wheelAmount(int delta, bool counting_items)
{
	if (counting_items) {
		return 3 * qRound(static_cast<double>(delta) / static_cast<int>(QWheelEvent::DefaultDeltasPerStep));
	}
	return delta;
}

/**
	@brief MaterialSelectionDialog::scrollTable
	Scroll the table with the wheel, in both directions.

	A wheel which only turns moves the rows. A movement reported on the
	horizontal axis (a wheel which tilts, a touchpad swiped sideways), or
	Shift held down on a plain wheel, moves the columns instead, which is
	how the rest of a wide catalogue is reached.

	Three rows or columns travel with every notch, the same amount Qt
	itself scrolls when it gets the wheel.
	@param event
*/
void MaterialSelectionDialog::scrollTable(QWheelEvent *event)
{
	QPoint delta = event->angleDelta();
	if (delta.isNull()) {
		delta = event->pixelDelta();
	}

		//Shift turns the wheel movement sideways.
	if ((event->modifiers() & Qt::ShiftModifier) && delta.x() == 0 && delta.y() != 0) {
		delta.setX(delta.y());
		delta.setY(0);
	}
	if (delta.isNull()) {
		return;
	}

	if (delta.y() != 0 && ui->m_table_view->verticalScrollBar())
	{
		const bool counting_rows =
				ui->m_table_view->verticalScrollMode() == QAbstractItemView::ScrollPerItem;
		QScrollBar *bar = ui->m_table_view->verticalScrollBar();
		bar->setValue(bar->value() - wheelAmount(delta.y(), counting_rows));
	}

	if (delta.x() != 0 && ui->m_table_view->horizontalScrollBar())
	{
		const bool counting_columns =
				ui->m_table_view->horizontalScrollMode() == QAbstractItemView::ScrollPerItem;
		QScrollBar *bar = ui->m_table_view->horizontalScrollBar();
		bar->setValue(bar->value() - wheelAmount(delta.x(), counting_columns));
	}
}

/**
	@brief MaterialSelectionDialog::reload
	Read the material file again, so that anything the user saved from his
	spreadsheet since the dialog was built shows up.
*/
void MaterialSelectionDialog::reload()
{
	QString error;
	MaterialListData data;

	if (!MaterialList::load(m_path, &data, &error))
	{
		QET::QetMessageBox::warning(this,
									tr("Lecture impossible"),
									tr("Impossible de lire le fichier :\n%1\n%2")
										.arg(m_path, error));
		data = MaterialListData();
	}

	m_data = data;
	m_model->setMaterial(m_data.columns, m_data.records);
	updateCount();
}

/**
	@brief MaterialSelectionDialog::updateCount
	Tell how many entries are shown.
*/
void MaterialSelectionDialog::updateCount()
{
	const int total = m_model->rowCount();
	const int shown = m_proxy->rowCount();

	if (m_proxy->tokens().isEmpty()) {
		ui->m_count_label->setText(tr("Entrées : %1").arg(total));
	} else {
		ui->m_count_label->setText(tr("Entrées : %1 sur %2").arg(shown).arg(total));
	}
}

/**
	@brief MaterialSelectionDialog::updateApplyButton
	Apply is only meaningful with an entry selected.
*/
void MaterialSelectionDialog::updateApplyButton()
{
	QPushButton *button = ui->m_button_box->button(QDialogButtonBox::Ok);
	if (button) {
		button->setEnabled(!selectedRecord().values.isEmpty());
	}
}

/**
	@brief MaterialSelectionDialog::searchChanged
	Refilter the table each time the search field changes.
	@param text
*/
void MaterialSelectionDialog::searchChanged(const QString &text)
{
	m_proxy->setTokens(text.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts));
	updateCount();
	updateApplyButton();
}

/**
	@brief MaterialSelectionDialog::selectedRecord
	@return the selected entry, a null one when nothing is selected
*/
MaterialRecord MaterialSelectionDialog::selectedRecord() const
{
	if (!m_proxy || !ui->m_table_view->selectionModel()) {
		return MaterialRecord();
	}

	const QModelIndexList rows = ui->m_table_view->selectionModel()->selectedRows();
	if (rows.isEmpty()) {
		return MaterialRecord();
	}

	const QModelIndex source = m_proxy->mapToSource(rows.first());
	if (!source.isValid()) {
		return MaterialRecord();
	}

	return m_model->record(source.row());
}

/**
	@brief sameEntry
	Tell whether two records describe the same article of the file.

	The maps are not compared as they are: an entry built by the entry
	form leaves the empty columns out, load() gives every column to every
	record, and a file may hold spaces the form has trimmed. Comparing
	per column makes both shapes say the same thing. Cells the file holds
	past the header are not part of it: they say nothing about which
	article a line is.
	@param lhs
	@param rhs
	@return true when both records describe the same article
*/
static bool sameEntry(const MaterialRecord &lhs, const MaterialRecord &rhs)
{
	QStringList columns = lhs.values.keys();
	for (const QString &column : rhs.values.keys())
	{
		if (!columns.contains(column)) {
			columns.append(column);
		}
	}

	for (const QString &column : columns)
	{
		if (lhs.value(column).trimmed() != rhs.value(column).trimmed()) {
			return false;
		}
	}

	return true;
}

/**
	@brief MaterialSelectionDialog::selectRecord
	Select the row holding that record, scrolling to it.
	@param record
	@return true when the record was found and is visible
*/
bool MaterialSelectionDialog::selectRecord(const MaterialRecord &record)
{
	const QList<MaterialRecord> records = m_model->records();
	int row = -1;
		//The newest match wins : appending an entry twice in a row selects
		//the line which was just written.
	for (int i = records.size() - 1; i >= 0; --i)
	{
		if (sameEntry(records.at(i), record))
		{
			row = i;
			break;
		}
	}
	if (row < 0) {
		return false;
	}

	const QModelIndex proxy = m_proxy->mapFromSource(m_model->index(row, 0));
	if (!proxy.isValid()) {
		return false;
	}

	ui->m_table_view->setCurrentIndex(proxy);
	ui->m_table_view->selectionModel()->select(
		proxy, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
	ui->m_table_view->scrollTo(proxy);
	updateApplyButton();

	return true;
}

/**
	@brief MaterialSelectionDialog::accept
	Refuse to close the dialog without a selection: "Apply" with nothing
	chosen would silently do nothing.
*/
void MaterialSelectionDialog::accept()
{
	if (selectedRecord().values.isEmpty())
	{
		QET::QetMessageBox::information(this,
										tr("Aucune sélection"),
										tr("Sélectionnez d'abord un article dans la liste."));
		return;
	}

	QDialog::accept();
}

/**
	@brief MaterialSelectionDialog::on_m_new_entry_btn_clicked
	Open the entry form and, once it is filled, append the article to the
	material file, then select it in the table.
*/
void MaterialSelectionDialog::on_m_new_entry_btn_clicked()
{
	QStringList columns = m_data.columns;
	if (columns.isEmpty()) {
		columns = MaterialList::defaultColumns();
	}

	MaterialEntryDialog entry_dialog(columns, this);
	if (entry_dialog.exec() != QDialog::Accepted) {
		return;
	}

	const MaterialRecord record = entry_dialog.record();

	QString error;
	if (!MaterialList::appendRecord(m_path, record, &error))
	{
		QET::QetMessageBox::critical(this,
									 tr("Écriture impossible"),
									 tr("Impossible d'écrire dans le fichier :\n%1\n%2")
										.arg(m_path, error));
		return;
	}

		//The file is read again so the new line is found where it really
		//is, and not guessed from where it was before the write.
	reload();

		//A search hiding the new article would leave the user with a form
		//he just filled and no line to see.
	if (!selectRecord(record))
	{
		ui->m_search_edit->clear();
		selectRecord(record);
	}
}
