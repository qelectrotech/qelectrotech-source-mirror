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
#include "macropackagedialog.h"

#include "../../qetapp.h"
#include "../../qetmessagebox.h"
#include "../eplandxf/eplanpartimporter.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {
	/// more rows than this are of no use to look through: narrow the search
	constexpr int MaxRows = 500;
}

MacroPackageDialog::MacroPackageDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Part import from a manufacturer macro package"));
	resize(760, 560);

	m_path = new QLineEdit(this);
	m_path->setReadOnly(true);
	m_path->setPlaceholderText(tr("ZIP file with the EPLAN macros of a manufacturer"));
	auto *choose = new QPushButton(tr("Choose ZIP…"), this);
	connect(choose, &QPushButton::clicked, this, [this]() { choosePackage(false); });
	// an unpacked package is a folder, which a file dialog cannot pick
	auto *folder = new QPushButton(tr("Folder…"), this);
	folder->setToolTip(tr("Unpacked macro package (folder)"));
	connect(folder, &QPushButton::clicked, this, [this]() { choosePackage(true); });
	auto *path_row = new QHBoxLayout;
	path_row->addWidget(m_path);
	path_row->addWidget(choose);
	path_row->addWidget(folder);

	auto *hint = new QLabel(
		tr("Some manufacturers offer all their EPLAN macros in one download instead of "
		   "single parts in the EPLAN Data Portal. Beckhoff: on beckhoff.com open any "
		   "product page > Documentation and downloads > Macros > \"EPLAN macros from "
		   "2.7\" (ZIP; needs a myBeckhoff login). Download it once and choose it here; "
		   "the package is remembered."),
		this);
	hint->setWordWrap(true);

	m_search = new QLineEdit(this);
	m_search->setPlaceholderText(tr("Type, order number or text, e.g. EL1809 or \"analog 4-channel\""));
	m_search->setClearButtonEnabled(true);
	connect(m_search, &QLineEdit::textChanged, this, &MacroPackageDialog::showArticles);
	m_count = new QLabel(this);

	m_results = new QTreeWidget(this);
	m_results->setHeaderLabels({tr("Type"), tr("Description"), tr("Category")});
	m_results->setRootIsDecorated(false);
	m_results->setAlternatingRowColors(true);
	m_results->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_results->setUniformRowHeights(true);
	m_results->header()->setStretchLastSection(true);
	m_results->setColumnWidth(0, 170);
	m_results->setColumnWidth(1, 380);
	connect(m_results, &QTreeWidget::itemSelectionChanged, this, &MacroPackageDialog::updateButtons);

	QSettings settings;
	m_target = new QComboBox(this);
	m_target->addItem(QCoreApplication::translate("EplanImportDialog", "User collection"),
			  QETApp::customElementsDir());
	m_target->addItem(QCoreApplication::translate("EplanImportDialog", "Company collection"),
			  QETApp::companyElementsDir());
	m_target->setCurrentIndex(qBound(0, settings.value(QStringLiteral("macropackage/target"), 0).toInt(), 1));

	auto *form = new QFormLayout;
	form->addRow(tr("Package:"), path_row);
	form->addRow(tr("Search:"), m_search);

	auto *target_row = new QFormLayout;
	target_row->addRow(QCoreApplication::translate("EplanImportDialog", "Import into:"), m_target);

	// the terms and their check box as in the other part import dialogs
	auto *terms_label = new QLabel(
		QCoreApplication::translate("EplanImportDialog",
			"Manufacturer data from the EPLAN Data Portal or a manufacturer's "
			"website is subject to the terms of use of that source. By importing "
			"you confirm that you are authorised to use it here; the import is "
			"at your own responsibility."),
		this);
	terms_label->setWordWrap(true);
	m_terms = new QCheckBox(QCoreApplication::translate("EplanImportDialog", "I accept these terms."), this);
	connect(m_terms, &QCheckBox::toggled, this, &MacroPackageDialog::updateButtons);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	m_import = buttons->addButton(tr("Import selected"), QDialogButtonBox::ActionRole);
	m_import->setDefault(true);
	connect(m_import, &QPushButton::clicked, this, [this]() { runImport(false); });
	m_import_all = buttons->addButton(tr("Import all"), QDialogButtonBox::ActionRole);
	m_import_all->setToolTip(tr("Every article of the package. Articles whose type folder "
				    "exists already are left as they are."));
	connect(m_import_all, &QPushButton::clicked, this, [this]() { runImport(true); });
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_results, &QTreeWidget::itemDoubleClicked, this, [this]() {
		if (m_import->isEnabled()) runImport(false);
	});

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(hint);
	layout->addWidget(m_results, 1);
	layout->addWidget(m_count);
	layout->addLayout(target_row);
	layout->addWidget(terms_label);
	layout->addWidget(m_terms);
	layout->addWidget(buttons);

	updateButtons();
	// the package of last time, once the dialog is on screen: reading
	// its list of articles takes a moment
	const QString last = settings.value(QStringLiteral("macropackage/path")).toString();
	if (!last.isEmpty() && QFileInfo::exists(last))
		QTimer::singleShot(0, this, [this, last]() { openPackage(last); });
	else
		m_count->setText(tr("Choose the macro package first."));
}

void MacroPackageDialog::choosePackage(bool folder)
{
	QSettings settings;
	QString start = settings.value(QStringLiteral("macropackage/path")).toString();
	if (!start.isEmpty()) start = QFileInfo(start).absolutePath();
	const QString path = folder
		? QFileDialog::getExistingDirectory(this, tr("Unpacked macro package"), start)
		: QFileDialog::getOpenFileName(this, tr("Manufacturer macro package"), start,
					       tr("Macro packages (*.zip)"));
	if (!path.isEmpty()) openPackage(path);
}

void MacroPackageDialog::openPackage(const QString &path)
{
	m_count->setText(tr("Reading the package…"));
	QGuiApplication::setOverrideCursor(Qt::WaitCursor);
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	const bool ok = m_package.open(path);
	QGuiApplication::restoreOverrideCursor();
	if (!ok) {
		m_path->clear();
		m_results->clear();
		m_count->setText(tr("Choose the macro package first."));
		QET::QetMessageBox::warning(this, windowTitle(),
					    tr("This is no macro package:\n%1").arg(m_package.errorString()));
		updateButtons();
		return;
	}
	m_path->setText(QDir::toNativeSeparators(path));
	QSettings().setValue(QStringLiteral("macropackage/path"), path);
	showArticles();
	m_search->setFocus();
}

void MacroPackageDialog::showArticles()
{
	m_results->clear();
	if (m_path->text().isEmpty()) return;
	const QVector<MacroPackageArticle> found = m_package.find(m_search->text());
	QList<QTreeWidgetItem *> items;
	for (const MacroPackageArticle &a : found) {
		if (items.size() >= MaxRows) break;
		auto *item = new QTreeWidgetItem({a.type(), a.description, a.category});
		if (!a.order_number.isEmpty() && a.order_number != a.type())
			item->setToolTip(0, tr("Order number: %1").arg(a.order_number));
		if (a.macro.isEmpty()) item->setToolTip(2, tr("no macro: terminals from the part data only"));
		items << item;
	}
	m_results->addTopLevelItems(items);
	QString count = tr("%1 of %2 articles").arg(found.size()).arg(m_package.articles().size());
	if (found.size() > MaxRows) count += QStringLiteral(" – ") + tr("the first %1 are shown, narrow the search").arg(MaxRows);
	m_count->setText(count);
	// one hit is the one that is meant
	if (items.size() == 1) items.first()->setSelected(true);
	updateButtons();
}

void MacroPackageDialog::updateButtons()
{
	m_import->setEnabled(!m_results->selectedItems().isEmpty() && m_terms->isChecked());
	m_import->setToolTip(m_terms->isChecked() ? QString() : tr("Accept the terms first."));
	m_import_all->setEnabled(!m_path->text().isEmpty() && m_terms->isChecked());
}

/**
	Import the selected articles, or all of the package: those with a
	progress bar that can stop the import.
*/
void MacroPackageDialog::runImport(bool all)
{
	QStringList types;
	if (all) {
		for (const MacroPackageArticle &a : m_package.articles()) types << a.type();
	} else {
		for (const QTreeWidgetItem *item : m_results->selectedItems()) types << item->text(0);
	}
	const QString dir = m_target->currentData().toString();
	if (types.isEmpty() || dir.isEmpty()) return;
	QSettings().setValue(QStringLiteral("macropackage/target"), m_target->currentIndex());

	EplanPartImporter importer;
	QProgressDialog progress(tr("Importing the macro package…"), tr("Cancel"), 0, int(types.size()), this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(all ? 0 : 1500);
	const bool ok = importer.importMacroPackage(
		m_package, types, QString(), dir,
		[&progress](int done, int total, const QString &type) {
			progress.setMaximum(total);
			progress.setValue(done);
			progress.setLabelText(tr("Importing the macro package…") + QLatin1Char('\n') + type);
			return !progress.wasCanceled();
		});
	progress.reset();
	if (!ok) {
		QET::QetMessageBox::critical(this, windowTitle(),
					     tr("The import failed:\n%1").arg(importer.errorString()));
		return;
	}
	m_written << importer.writtenPaths();
	if (all || importer.writtenPaths().isEmpty()) {
		QString text = tr("%1 articles imported (%2 elements), %3 left as they were because "
				  "their type folder exists already, %4 failed.")
				       .arg(importer.importedCount()).arg(importer.writtenPaths().size())
				       .arg(importer.skippedCount()).arg(importer.failed().size());
		if (!importer.failed().isEmpty())
			text += QStringLiteral("\n\n") + importer.failed().mid(0, 20).join(QLatin1Char('\n'));
		QET::QetMessageBox::information(this, windowTitle(), text);
		return;
	}
	// one line per folder: the elements of an article lie together
	QStringList folders;
	for (const QString &p : importer.writtenPaths()) {
		const QString f = QDir::toNativeSeparators(QFileInfo(p).absolutePath());
		if (!folders.contains(f)) folders << f;
	}
	QET::QetMessageBox::information(
		this, windowTitle(),
		tr("%n element(s) imported into", "", int(importer.writtenPaths().size()))
			+ QLatin1Char('\n') + folders.join(QLatin1Char('\n')));
}
