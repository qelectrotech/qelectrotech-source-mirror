// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "projectlanguages.h"
#include "projectlanguagesdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

/**
	@brief ProjectLanguagesDialog::ProjectLanguagesDialog
	@param languages the project's texts in other languages now
	@param project_texts the texts drawn in the project, offered to translate
	@param parent
*/
ProjectLanguagesDialog::ProjectLanguagesDialog(const ProjectLanguages &languages,
											   const QStringList &project_texts,
											   QWidget *parent) :
	QDialog(parent),
	m_languages(languages),
	m_project_texts(project_texts)
{
	setWindowTitle(tr("Project languages"));
	resize(720, 460);
	auto *layout = new QVBoxLayout(this);

	auto *note = new QLabel(tr("Add a language, collect the texts of the project, and type the words "
							   "to show in each language. The folios keep their own text; "
							   "only what is shown and printed changes."));
	note->setWordWrap(true);
	layout->addWidget(note);

	auto *top = new QHBoxLayout;
	top->addWidget(new QLabel(tr("Show the folios in")));
	m_active = new QComboBox;
	top->addWidget(m_active);
	top->addStretch();
	auto *add = new QPushButton(tr("Add language..."));
	auto *remove = new QPushButton(tr("Remove language"));
	auto *collect = new QPushButton(tr("Collect texts of the project"));
	top->addWidget(add);
	top->addWidget(remove);
	top->addWidget(collect);
	layout->addLayout(top);

	m_table = new QTableWidget;
	m_table->horizontalHeader()->setStretchLastSection(true);
	layout->addWidget(m_table, 1);

	auto *bottom = new QHBoxLayout;
	auto *export_csv = new QPushButton(tr("Export CSV..."));
	auto *import_csv = new QPushButton(tr("Import CSV..."));
	auto *add_text = new QPushButton(tr("Add text..."));
	add_text->setToolTip(tr("Add a text that is not collected, such as a column title of a table."));
	bottom->addWidget(add_text);
	bottom->addWidget(export_csv);
	bottom->addWidget(import_csv);
	bottom->addStretch();
	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	bottom->addWidget(buttons);
	layout->addLayout(bottom);

	connect(add, &QPushButton::clicked, this, &ProjectLanguagesDialog::addLanguage);
	connect(remove, &QPushButton::clicked, this, &ProjectLanguagesDialog::removeLanguage);
	connect(collect, &QPushButton::clicked, this, &ProjectLanguagesDialog::collectTexts);
	connect(add_text, &QPushButton::clicked, this, &ProjectLanguagesDialog::addText);
	connect(export_csv, &QPushButton::clicked, this, &ProjectLanguagesDialog::exportCsv);
	connect(import_csv, &QPushButton::clicked, this, &ProjectLanguagesDialog::importCsv);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	refreshActive();
	fillTable();
}

/**
	@brief ProjectLanguagesDialog::languages
	@return the languages and words as edited
*/
ProjectLanguages ProjectLanguagesDialog::languages() const
{
	ProjectLanguages result = m_languages;
	const QString active = m_active->currentData().toString();
	result.setActiveLanguage(active);
	const QStringList added = result.languages().mid(1);
	for (int row = 0 ; row < m_table->rowCount() ; ++row) {
		const QString source = m_table->item(row, 0)->text();
		for (int column = 0 ; column < added.size() ; ++column)
			if (auto *item = m_table->item(row, column + 1))
				result.setTranslation(source, added.at(column), item->text());
	}
	return result;
}

/**
	@brief ProjectLanguagesDialog::readTable
	Keep what was typed in m_languages before the table is rebuilt.
*/
void ProjectLanguagesDialog::readTable()
{
	const QString active = m_active->currentData().toString();
	m_languages = languages();
	m_languages.setActiveLanguage(active);
}

void ProjectLanguagesDialog::refreshActive()
{
	m_active->clear();
	for (const QString &code : m_languages.languages())
		m_active->addItem(code == m_languages.sourceLanguage() ? tr("%1 (as drawn)").arg(code) : code, code);
	m_active->setCurrentIndex(qMax(0, m_active->findData(m_languages.activeLanguage())));
}

void ProjectLanguagesDialog::fillTable()
{
	const QStringList added = m_languages.languages().mid(1);
	const QStringList phrases = m_languages.phrases();
	m_table->clear();
	m_table->setColumnCount(1 + added.size());
	m_table->setRowCount(phrases.size());
	m_table->setColumnWidth(0, 240);
	m_table->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Text as drawn")));
	for (int c = 0 ; c < added.size() ; ++c)
		m_table->setHorizontalHeaderItem(c + 1, new QTableWidgetItem(added.at(c)));
	for (int row = 0 ; row < phrases.size() ; ++row) {
		auto *source = new QTableWidgetItem(phrases.at(row));
		source->setFlags(source->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(row, 0, source);
		for (int c = 0 ; c < added.size() ; ++c)
			m_table->setItem(row, c + 1, new QTableWidgetItem(m_languages.translation(phrases.at(row), added.at(c))));
	}
}

void ProjectLanguagesDialog::addLanguage()
{
	bool ok = false;
	const QString code = QInputDialog::getText(this, tr("Add language"),
			tr("Language code, such as de, fr or pt-BR:"), QLineEdit::Normal, QString(), &ok).trimmed();
	if (!ok || code.isEmpty()) return;
	readTable();
	if (!m_languages.addLanguage(code)) {
		QMessageBox::warning(this, tr("Add language"),
							 tr("\"%1\" is not a usable language code, or is already there.").arg(code));
		return;
	}
	refreshActive();
	fillTable();
}

void ProjectLanguagesDialog::removeLanguage()
{
	const int column = m_table->currentColumn();
	if (column < 1) {
		QMessageBox::information(this, tr("Remove language"), tr("Select a cell in the column of the language to remove."));
		return;
	}
	const QString code = m_table->horizontalHeaderItem(column)->text();
	if (QMessageBox::question(this, tr("Remove language"),
							  tr("Remove %1 and all its words?").arg(code)) != QMessageBox::Yes) return;
	readTable();
	m_languages.removeLanguage(code);
	refreshActive();
	fillTable();
}

void ProjectLanguagesDialog::collectTexts()
{
	readTable();
	int added = 0;
	const int before = m_languages.phrases().size();
	for (const QString &text : std::as_const(m_project_texts))
		m_languages.addPhrase(text);
	added = m_languages.phrases().size() - before;
	fillTable();
	QMessageBox::information(this, tr("Collect texts"), tr("%n new text(s) added.", nullptr, added));
}

void ProjectLanguagesDialog::addText()
{
	bool ok = false;
	const QString text = QInputDialog::getText(this, tr("Add text"),
			tr("Text as drawn, for example a column title of a table:"),
			QLineEdit::Normal, QString(), &ok).trimmed();
	if (!ok || text.isEmpty()) return;
	readTable();
	m_languages.addPhrase(text);
	fillTable();
}

void ProjectLanguagesDialog::exportCsv()
{
	readTable();
	const QString path = QFileDialog::getSaveFileName(this, tr("Export CSV"), QString(), tr("CSV files (*.csv)"));
	if (path.isEmpty()) return;
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly) || file.write(m_languages.exportCsv()) < 0)
		QMessageBox::warning(this, tr("Export CSV"), tr("Could not write %1.").arg(path));
}

void ProjectLanguagesDialog::importCsv()
{
	const QString path = QFileDialog::getOpenFileName(this, tr("Import CSV"), QString(), tr("CSV files (*.csv)"));
	if (path.isEmpty()) return;
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(this, tr("Import CSV"), tr("Could not read %1.").arg(path));
		return;
	}
	readTable();
	const auto result = m_languages.importCsv(file.readAll());
	if (!result.ok) {
		QMessageBox::warning(this, tr("Import CSV"), result.error);
		return;
	}
	refreshActive();
	fillTable();
	QString message = tr("%n translation(s) imported.", nullptr, result.imported);
	if (!result.warnings.isEmpty())
		message += QLatin1Char('\n') + result.warnings.mid(0, 10).join(QLatin1Char('\n'));
	QMessageBox::information(this, tr("Import CSV"), message);
}
