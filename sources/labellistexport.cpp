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
#include "labellistexport.h"
#include "bomexport.h"
#include "conductornumexport.h"
#include "qetapp.h"
#include "qetproject.h"
#include "dataBase/projectdatabase.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QSqlQuery>
#include <QStringBuilder>
#include <QVBoxLayout>

#include <algorithm>
#include <climits>

namespace {

	/**
		@brief The Row struct
		One label and where it is drawn, to sort by
	*/
	struct Row
	{
		QString label;
		int folio = INT_MAX;	///< folio number, INT_MAX for none
		int column = INT_MAX;	///< column of the cell, "G4" -> 4
		QString row;			///< row of the cell, "G4" -> "G"
	};

	/**
		@brief cell
		Reads a cell of the folio grid as written in the database ("G4":
		row G, column 4) into @p row; leaves a cell it can't read last.
	*/
	void cell(const QString &position, Row &row)
	{
		static const QRegularExpression rx(QStringLiteral("^([A-Za-z]+)(\\d+)$"));
		const QRegularExpressionMatch match = rx.match(position.trimmed());
		if (match.hasMatch()) {
			row.row = match.captured(1).toUpper();
			row.column = match.captured(2).toInt();
		}
	}

	/**
		@brief The natural order of labels: digit runs compare as numbers (2
		before 10), the rest case-insensitively. Done by hand because
		QCollator's numeric mode depends on the locale (the C locale of a
		CI runner ignores it).
	*/
	struct NaturalOrder
	{
		int compare(const QString &a, const QString &b) const
		{
			int i = 0, j = 0;
			while (i < a.size() && j < b.size()) {
				if (a[i].isDigit() && b[j].isDigit()) {
					int ie = i, je = j;
					while (ie < a.size() && a[ie].isDigit()) ++ie;
					while (je < b.size() && b[je].isDigit()) ++je;
					QStringView da = QStringView(a).mid(i, ie - i);
					QStringView db = QStringView(b).mid(j, je - j);
					while (da.size() > 1 && da.startsWith(QLatin1Char('0'))) da = da.mid(1);
					while (db.size() > 1 && db.startsWith(QLatin1Char('0'))) db = db.mid(1);
					if (da.size() != db.size()) return da.size() < db.size() ? -1 : 1;
					if (const int c = da.compare(db)) return c < 0 ? -1 : 1;
					i = ie;
					j = je;
				} else {
					const QChar x = a[i].toCaseFolded(), y = b[j].toCaseFolded();
					if (x != y) return x < y ? -1 : 1;
					++i;
					++j;
				}
			}
			if (i < a.size()) return 1;
			if (j < b.size()) return -1;
			return 0;
		}
	};

	NaturalOrder naturalOrder()
	{
		return NaturalOrder();
	}

	/**
		@brief inFolioOrder
		Folio after folio; on a folio, column after column, top to bottom;
		then by label, 2 before 10
	*/
	void inFolioOrder(QList<Row> &rows)
	{
		const NaturalOrder collator = naturalOrder();
		std::stable_sort(rows.begin(), rows.end(), [&collator](const Row &a, const Row &b) {
			if (a.folio != b.folio) return a.folio < b.folio;
			if (a.column != b.column) return a.column < b.column;
			if (a.row.size() != b.row.size()) return a.row.size() < b.row.size();
			if (a.row != b.row) return a.row < b.row;
			return collator.compare(a.label, b.label) < 0;
		});
	}

	QStringList labelsOf(const QList<Row> &rows)
	{
		QStringList list;
		for (const Row &row : rows) list << row.label;
		return list;
	}

	/**
		@brief label
		The label of a row of @p query: the plain label in column 0, or the
		full designation in column 1 when asked for and there is one. An
		item with no plain label has nothing to print: empty.
	*/
	QString label(const QSqlQuery &query, bool full_designation)
	{
		const QString plain = query.value(0).toString().trimmed();
		if (plain.isEmpty()) return QString();
		const QString full = query.value(1).toString().trimmed();
		return full_designation && !full.isEmpty() ? full : plain;
	}

	QStringList devices(projectDataBase *db, bool full_designation)
	{
		QList<Row> rows;
			//A terminal gets a terminal marker, not a device tag
		QSqlQuery query = db->newQuery(QStringLiteral(
				"SELECT l.label, l.structure_id, l.folio_number, l.position FROM qet_device_list l "
				"JOIN qet_devices d ON d.device_uuid = l.head_uuid "
				"WHERE COALESCE(d.kind, '') <> 'terminal'"));
		query.exec();
		while (query.next()) {
			Row row;
			row.label = label(query, full_designation);
			if (row.label.isEmpty()) continue;
			if (!query.value(2).isNull()) row.folio = query.value(2).toInt();
			cell(query.value(3).toString(), row);
			rows << row;
		}
		inFolioOrder(rows);
		return labelsOf(rows);
	}

	QStringList terminals(projectDataBase *db, bool full_designation)
	{
		QStringList list;
			//Strip by strip, as the terminal strip editor shows them
		QSqlQuery query = db->newQuery(QStringLiteral(
				"SELECT label, terminal_id FROM qet_terminals "
				"ORDER BY strip_id, strip_uuid, position, level"));
		query.exec();
		while (query.next()) {
			const QString text = label(query, full_designation);
			if (!text.isEmpty()) list << text;
		}

			//Then the terminals on no strip, by label: XE:2 before XE:10
		QList<Row> loose;
		query = db->newQuery(QStringLiteral(
				"SELECT label, structure_id FROM qet_devices WHERE kind = 'terminal' "
				"AND device_uuid NOT IN (SELECT element_uuid FROM qet_terminals "
				"WHERE element_uuid IS NOT NULL)"));
		query.exec();
		while (query.next()) {
			Row row;
			row.label = label(query, full_designation);
			if (!row.label.isEmpty()) loose << row;
		}
		const NaturalOrder collator = naturalOrder();
		std::stable_sort(loose.begin(), loose.end(), [&collator](const Row &a, const Row &b) {
			return collator.compare(a.label, b.label) < 0;
		});
		return list + labelsOf(loose);
	}

	QStringList cables(projectDataBase *db, bool full_designation)
	{
		QList<Row> rows;
		QSqlQuery query = db->newQuery(QStringLiteral(
				"SELECT designation, structure_id, folio_number FROM qet_cables"));
		query.exec();
		while (query.next()) {
			Row row;
			row.label = label(query, full_designation);
			if (row.label.isEmpty()) continue;
			if (!query.value(2).isNull()) row.folio = query.value(2).toInt();
			rows << row;
		}
		inFolioOrder(rows);
		return labelsOf(rows);
	}

	const QString kind_key = QStringLiteral("export/label_list/kind");
	const QString full_key = QStringLiteral("export/label_list/full_designation");
}

/**
	@brief LabelListExport::kindFromString
	@param name : devices, terminals, cables or wires
	@param kind : set to the kind @p name names
	@return true if @p name names a kind
*/
bool LabelListExport::kindFromString(const QString &name, Kind *kind)
{
	for (const Kind k : {Devices, Terminals, Cables, Wires}) {
		if (kindName(k) == name) {
			*kind = k;
			return true;
		}
	}
	return false;
}

/**
	@brief LabelListExport::kindName
	@return the name of @p kind on the command line and in the settings
*/
QString LabelListExport::kindName(Kind kind)
{
	switch (kind) {
		case Devices: return QStringLiteral("devices");
		case Terminals: return QStringLiteral("terminals");
		case Cables: return QStringLiteral("cables");
		case Wires: return QStringLiteral("wires");
	}
	return QString();
}

/**
	@brief LabelListExport::labels
	@param project
	@param kind
	@param full_designation : =plant+location-tag instead of the tag as
	drawn, for devices, terminals and cables; wires have only their number
	@return the labels of @p kind in @p project, one per marker to print:
	a tag per device (not per symbol), a marker per terminal, a number per
	cable, and a wire number per wire as "Export the list of names of wires"
	writes them, in natural order (2 before 10)
*/
QStringList LabelListExport::labels(QETProject *project, Kind kind, bool full_designation)
{
	if (!project) return QStringList();
	if (kind == Wires) {
		QStringList wires = ConductorNumExport(project).wiresNum().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
			//2 before 10, as the other lists
		const NaturalOrder collator = naturalOrder();
		std::stable_sort(wires.begin(), wires.end(), [&collator](const QString &a, const QString &b) {
			return collator.compare(a, b) < 0;
		});
		return wires;
	}
	projectDataBase *db = project->dataBase();
	if (!db) return QStringList();
		//Up to date without telling the views of the project: a list
		//that is only counted or written changes nothing they show
	db->refresh();
	switch (kind) {
		case Devices: return devices(db, full_designation);
		case Terminals: return terminals(db, full_designation);
		case Cables: return cables(db, full_designation);
		case Wires: break;
	}
	return QStringList();
}

/**
	@brief LabelListExport::toCsv
	@return @p labels as CSV in UTF-8, one per line, no header: what label
	printer software reads as a list of markers. Quoted, as the other list
	exports (BomExport::csvRecord()), so that a ';' or ',' in a label stays
	in its cell whatever separator the spreadsheet expects
*/
QByteArray LabelListExport::toCsv(const QStringList &labels)
{
	QByteArray csv;
	for (const QString &label : labels) {
		csv += BomExport::csvRecord({label});
	}
	return csv;
}

/**
	@brief LabelListExport::write
	Writes @p labels to @p file_name as CSV in UTF-8 with a byte order mark,
	as the other list exports do, so that Excel reads it right
	@return true if written; else false and @p error says why
*/
bool LabelListExport::write(const QStringList &labels, const QString &file_name, QString *error)
{
	QSaveFile file(file_name);
	if (!file.open(QIODevice::WriteOnly)) {
		if (error) *error = file.errorString();
		return false;
	}
	file.write("\xEF\xBB\xBF");
	file.write(toCsv(labels));
	if (!file.commit()) {
		if (error) *error = file.errorString();
		return false;
	}
	return true;
}

/**
	@brief LabelListExportDialog::LabelListExportDialog
	@param project : the project to export the labels of
	@param parent
*/
LabelListExportDialog::LabelListExportDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project)
{
	setWindowTitle(tr("Export labels for printing"));

	auto layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("One label per line, for label printer software:"), this));

	m_kinds = new QButtonGroup(this);
	const QList<QPair<LabelListExport::Kind, QString>> kinds {
		{LabelListExport::Devices, tr("Device tags (one per device, in folio order)")},
		{LabelListExport::Terminals, tr("Terminal markers (strip by strip, in strip order)")},
		{LabelListExport::Cables, tr("Cable numbers (in folio order)")},
		{LabelListExport::Wires, tr("Wire numbers (one per wire)")}
	};
	QSettings settings;
	LabelListExport::Kind saved = LabelListExport::Devices;
	LabelListExport::kindFromString(settings.value(kind_key).toString(), &saved);
	for (const auto &kind : kinds) {
		auto button = new QRadioButton(kind.second, this);
		button->setChecked(kind.first == saved);
		m_kinds->addButton(button, kind.first);
		layout->addWidget(button);
	}

	m_full = new QCheckBox(tr("Full designation (=plant+location-tag)"), this);
	m_full->setChecked(settings.value(full_key, false).toBool());
	layout->addWidget(m_full);

	m_count = new QLabel(this);
	layout->addWidget(m_count);

	auto buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, this);
	buttons->button(QDialogButtonBox::Save)->setText(tr("Save as CSV..."));
	connect(buttons, &QDialogButtonBox::accepted, this, &LabelListExportDialog::save);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);

	connect(m_kinds, &QButtonGroup::idClicked, this, &LabelListExportDialog::refresh);
	connect(m_full, &QCheckBox::toggled, this, &LabelListExportDialog::refresh);
	refresh();
}

LabelListExport::Kind LabelListExportDialog::kind() const {
	return static_cast<LabelListExport::Kind>(m_kinds->checkedId());
}

/**
	@brief LabelListExportDialog::refresh
	Reads the list again, and says how many labels it has
*/
void LabelListExportDialog::refresh()
{
	const bool wires = kind() == LabelListExport::Wires;
		//A wire has only its number
	m_full->setEnabled(!wires);
	m_labels = LabelListExport::labels(m_project, kind(), m_full->isChecked() && !wires);
	if (m_labels.isEmpty()) {
		m_count->setText(tr("This project has none."));
	} else {
		m_count->setText(tr("%n label(s), the first: %1", nullptr, m_labels.size())
						 .arg(m_labels.first()));
	}
}

/**
	@brief LabelListExportDialog::save
	Asks where, and writes the list
*/
void LabelListExportDialog::save()
{
	if (m_labels.isEmpty()) {
		QMessageBox::information(this, windowTitle(), tr("There are no labels of this kind to export."));
		return;
	}
	QString dir = m_project->currentDir();
	if (dir.isEmpty()) dir = QETApp::documentDir();
		//A title may hold what a file name cannot
	QString title = m_project->title();
	title.replace(QRegularExpression(QStringLiteral("[/\\\\:*?\"<>|]")), QStringLiteral("_"));
	const QString name = dir % QLatin1Char('/') % tr("labels") % QLatin1Char('_')
			% LabelListExport::kindName(kind()) % QLatin1Char('_') % title % QStringLiteral(".csv");
	const QString file_name = QFileDialog::getSaveFileName(this, tr("Save As..."), name,
														   tr("CSV files (*.csv)"));
	if (file_name.isEmpty()) return;

	QString error;
	if (!LabelListExport::write(m_labels, file_name, &error)) {
		QMessageBox::critical(this, tr("Error"), tr("Unable to write %1:\n%2").arg(file_name, error));
		return;
	}
	QSettings settings;
	settings.setValue(kind_key, LabelListExport::kindName(kind()));
	settings.setValue(full_key, m_full->isChecked());
	accept();
}
