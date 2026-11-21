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
#include "reportsdialog.h"

#include <qet.h>
#include <qetmessagebox.h>
#include <qetproject.h>

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QProgressDialog>
#include <QPushButton>
#include <QScopeGuard>
#include <QTableWidget>
#include <QVBoxLayout>

ReportsDialog::ReportsDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project),
	m_report(new QComboBox(this)),
	m_place(new QComboBox(this)),
	m_summary(new QLabel(this)),
	m_table(new QTableWidget(this)),
	m_reports(ProjectReports::all())
{
	setWindowTitle(tr("Reports"));
	resize(980, 600);
	auto *layout = new QVBoxLayout(this);

	auto *choices = new QHBoxLayout();
	choices->addWidget(new QLabel(tr("List:"), this));
	m_report->setObjectName(QStringLiteral("reportChoice"));
	for (const auto &report : std::as_const(m_reports))
		m_report->addItem(report.title);
	choices->addWidget(m_report, 1);
	choices->addWidget(new QLabel(tr("Place:"), this));
	m_place->setObjectName(QStringLiteral("reportPlace"));
	choices->addWidget(m_place, 1);
	layout->addLayout(choices);

	m_summary->setObjectName(QStringLiteral("reportSummary"));
	layout->addWidget(m_summary);

	m_table->setObjectName(QStringLiteral("reportTable"));
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setAlternatingRowColors(true);
	m_table->verticalHeader()->setVisible(false);
	m_table->horizontalHeader()->setStretchLastSection(true);
	layout->addWidget(m_table);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	auto *refresh_button = buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
	m_csv = buttons->addButton(tr("Export to CSV..."), QDialogButtonBox::ActionRole);
	m_csv->setObjectName(QStringLiteral("reportCsv"));
	m_pdf = buttons->addButton(tr("Export to PDF..."), QDialogButtonBox::ActionRole);
	m_pdf->setObjectName(QStringLiteral("reportPdf"));
	connect(refresh_button, &QPushButton::clicked, this, &ReportsDialog::refresh);
	connect(m_csv, &QPushButton::clicked, this, [this]() {exportCsv();});
	connect(m_pdf, &QPushButton::clicked, this, [this]() {exportPdf();});
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
	if (project) connect(project, &QObject::destroyed, this, &QDialog::close);
	QET::trackDialogGeometry(this);

	buildFolios();
	m_places = ProjectReports::places(m_project);
	m_place->addItem(tr("Whole project"));
	for (const auto &place : std::as_const(m_places))
		m_place->addItem(place.text());
	connect(m_report, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReportsDialog::refresh);
	connect(m_place, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReportsDialog::refresh);
	refresh();
}

/**
	@brief ReportsDialog::buildFolios
	A list covers every folio: those not built yet are built first.
*/
void ReportsDialog::buildFolios()
{
	if (!m_project || !m_project->unloadedFolioCount()) return;
	QProgressDialog progress(tr("Reading the folios..."), QString(), 0,
							 m_project->unloadedFolioCount(),
							 parentWidget() ? parentWidget() : this);
	progress.setWindowTitle(windowTitle());
	progress.setWindowModality(Qt::ApplicationModal);
	progress.setCancelButton(nullptr);
	progress.setMinimumDuration(0);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const auto restore = qScopeGuard([this]() {
		if (m_project) m_project->setFolioBuildObserver({});
		QApplication::restoreOverrideCursor();
	});
	m_project->setFolioBuildObserver([&progress](int built, int total) {
		progress.setMaximum(total);
		progress.setLabelText(tr("Reading folio %1 of %2...").arg(built).arg(total));
		progress.setValue(built);
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	});
	m_project->loadFolios();
}

ProjectReports::Report ReportsDialog::currentReport() const
{
	return m_reports.value(m_report->currentIndex());
}

ProjectReports::Place ReportsDialog::currentPlace() const
{
		//The first entry is the whole project
	return m_places.value(m_place->currentIndex() - 1);
}

void ReportsDialog::refresh()
{
	m_shown = ProjectReports::read(m_project, currentReport(), currentPlace());
	m_table->setSortingEnabled(false);
	m_table->clear();
	m_table->setColumnCount(int(m_shown.headers.size()));
	m_table->setRowCount(int(m_shown.rows.size()));
	m_table->setHorizontalHeaderLabels(m_shown.headers);
	for (int r = 0 ; r < m_shown.rows.size() ; ++r)
		for (int c = 0 ; c < m_shown.headers.size() ; ++c)
			m_table->setItem(r, c, new QTableWidgetItem(m_shown.rows.at(r).at(c)));
	m_table->resizeColumnsToContents();
	if (m_shown.error.isEmpty())
		m_summary->setText(tr("%n row(s).", nullptr, int(m_shown.rows.size())));
	else
		m_summary->setText(m_shown.error);
	m_csv->setEnabled(m_shown.error.isEmpty());
	m_pdf->setEnabled(m_shown.error.isEmpty() && !m_shown.rows.isEmpty());
}

QString ReportsDialog::chooseFile(const QString &filter, const QString &suffix)
{
	QString name = currentReport().id;
	if (!currentPlace().text().isEmpty())
		name += QLatin1Char(' ') + currentPlace().text();
	const QString directory = m_project && !m_project->filePath().isEmpty()
			? QFileInfo(m_project->filePath()).absolutePath() : QDir::homePath();
	QString path = QFileDialog::getSaveFileName(
				this, tr("Export the list"),
				QDir(directory).filePath(name + suffix), filter);
	if (!path.isEmpty() && QFileInfo(path).suffix().isEmpty())
		path += suffix;
	return path;
}

bool ReportsDialog::exportCsv(const QString &given)
{
	const QString path = given.isEmpty() ? chooseFile(tr("CSV files (*.csv)"), QStringLiteral(".csv")) : given;
	if (path.isEmpty()) return false;
	QString error;
	if (!ProjectReports::writeCsv(m_shown, path, &error)) {
		if (given.isEmpty())
			QET::QetMessageBox::critical(this, tr("Export to CSV"), tr("Cannot write the file:\n%1").arg(error));
		return false;
	}
	m_summary->setText(tr("%n row(s) written to %1.", nullptr, int(m_shown.rows.size())).arg(path));
	return true;
}

bool ReportsDialog::exportPdf(const QString &given)
{
	const QString path = given.isEmpty() ? chooseFile(tr("PDF files (*.pdf)"), QStringLiteral(".pdf")) : given;
	if (path.isEmpty()) return false;
	QString subtitle = m_project ? m_project->title() : QString();
	const QString place = currentPlace().text();
	if (!place.isEmpty())
		subtitle += (subtitle.isEmpty() ? QString() : QStringLiteral(", ")) + place;
	QString error;
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const int pages = ProjectReports::writePdf(m_shown, currentReport().title, subtitle, path, &error);
	QApplication::restoreOverrideCursor();
	if (!pages) {
		if (given.isEmpty())
			QET::QetMessageBox::critical(this, tr("Export to PDF"), tr("Cannot write the file:\n%1").arg(error));
		return false;
	}
	m_summary->setText(tr("%n page(s) written to %1.", nullptr, pages).arg(path));
	return true;
}
