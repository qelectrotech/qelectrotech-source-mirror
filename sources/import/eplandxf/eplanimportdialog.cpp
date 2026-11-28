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
#include "eplanimportdialog.h"

#include "../../qetapp.h"
#include "../../qetmessagebox.h"
#include "../edz/edzarchive.h"
#include "../edz/edzpart.h"
#include "eplanpartimporter.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

EplanImportDialog::EplanImportDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Part import"));
	setMinimumWidth(520);
	setAcceptDrops(true);

	m_source = new QLineEdit(this);
	m_source->setReadOnly(true);
	m_source->setPlaceholderText(tr("ZIP package, .edz part or DXF; or drop it here"));
	auto *change = new QPushButton(tr("Change…"), this);
	connect(change, &QPushButton::clicked, this, &EplanImportDialog::chooseSource);
	// an unpacked package is a folder, which a file dialog cannot pick
	auto *folder = new QPushButton(tr("Folder…"), this);
	folder->setToolTip(tr("Unpacked EPLAN DXF package (folder)"));
	connect(folder, &QPushButton::clicked, this, &EplanImportDialog::chooseFolder);
	auto *source_row = new QHBoxLayout;
	source_row->addWidget(m_source);
	source_row->addWidget(change);
	source_row->addWidget(folder);

	m_manufacturer = new QLineEdit(this);
	m_category = new QLineEdit(this);
	m_category->setPlaceholderText(QStringLiteral("General"));
	m_type = new QLineEdit(this);
	for (QLineEdit *e : {m_manufacturer, m_category, m_type})
		connect(e, &QLineEdit::textChanged, this, &EplanImportDialog::updateButtons);

	m_edz = new QLineEdit(this);
	m_edz->setReadOnly(true);
	m_edz->setPlaceholderText(tr("optional: order number and description"));
	m_edz_button = new QPushButton(tr("Choose…"), this);
	connect(m_edz_button, &QPushButton::clicked, this, &EplanImportDialog::chooseEdz);
	auto *edz_row = new QHBoxLayout;
	edz_row->addWidget(m_edz);
	edz_row->addWidget(m_edz_button);

	QSettings settings;
	m_other_dir = settings.value(QStringLiteral("eplanimport/otherdir")).toString();
	m_target = new QComboBox(this);
	m_target->addItem(tr("User collection"), QETApp::customElementsDir());
	m_target->addItem(tr("Company collection"), QETApp::companyElementsDir());
	m_target->addItem(m_other_dir.isEmpty() ? tr("Other folder…")
						: QDir::toNativeSeparators(m_other_dir),
			  m_other_dir);
	m_target->setCurrentIndex(settings.value(QStringLiteral("eplanimport/target"), 0).toInt());
	connect(m_target, &QComboBox::activated, this, [this](int index) {
		if (index != 2) return;
		const QString dir = QFileDialog::getExistingDirectory(
			this, tr("Folder of an element collection"), m_other_dir);
		if (dir.isEmpty()) {
			if (m_other_dir.isEmpty()) m_target->setCurrentIndex(0);
			return;
		}
		m_other_dir = dir;
		m_target->setItemText(2, QDir::toNativeSeparators(dir));
		m_target->setItemData(2, dir);
	});

	auto *form = new QFormLayout;
	form->addRow(tr("Source:"), source_row);
	form->addRow(tr("Manufacturer:"), m_manufacturer);
	form->addRow(tr("Category:"), m_category);
	form->addRow(tr("Type:"), m_type);
	form->addRow(tr("Part data (.edz):"), edz_row);
	form->addRow(tr("Import into:"), m_target);

	auto *hint = new QLabel(
		tr("The elements are stored in manufacturer / category / type folders. "
		   "Each empty EPLAN field (PLC address, symbolic address, function "
		   "text) becomes a text of its own that is filled in per placement."),
		this);
	hint->setWordWrap(true);

	// a check box text does not wrap: the terms go into a label above it
	auto *terms_label = new QLabel(
		tr("Manufacturer data from the EPLAN Data Portal or a manufacturer's "
		   "website is subject to the terms of use of that source. By importing "
		   "you confirm that you are authorised to use it here; the import is "
		   "at your own responsibility."),
		this);
	terms_label->setWordWrap(true);
	m_terms = new QCheckBox(tr("I accept these terms."), this);
	connect(m_terms, &QCheckBox::toggled, this, &EplanImportDialog::updateButtons);
	m_missing = new QLabel(this);
	m_missing->setStyleSheet(QStringLiteral("color: #c0392b;"));

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
	m_import = buttons->addButton(tr("Import"), QDialogButtonBox::AcceptRole);
	m_import->setDefault(true);
	connect(buttons, &QDialogButtonBox::accepted, this, &EplanImportDialog::runImport);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(hint);
	layout->addSpacing(8);
	layout->addWidget(terms_label);
	layout->addWidget(m_terms);
	layout->addWidget(m_missing);
	layout->addWidget(buttons);

	m_edz->setEnabled(false);
	m_edz_button->setEnabled(false);
	updateButtons();
}

/**
	Ask for the file to import: a Data Portal package, an .edz part or a
	single DXF, all in one file dialog.
	@return false when the user cancels
*/
bool EplanImportDialog::chooseSource()
{
	QSettings settings;
	const QString last = settings.value(QStringLiteral("eplanimport/lastdir")).toString();
	const QStringList filters {
		tr("Parts (*.zip *.edz *.dxf)"),
		tr("EPLAN Data Portal DXF package (*.zip)"),
		tr("EPLAN parts (*.edz)"),
		tr("DXF drawings (*.dxf)")};
	const QString path = QFileDialog::getOpenFileName(this, tr("Import part"), last,
							  filters.join(QStringLiteral(";;")));
	if (path.isEmpty()) return false;
	settings.setValue(QStringLiteral("eplanimport/lastdir"), QFileInfo(path).absolutePath());
	return setSource(path);
}

/**
	Ask for an unpacked Data Portal package.
	@return false when the user cancels
*/
bool EplanImportDialog::chooseFolder()
{
	QSettings settings;
	const QString last = settings.value(QStringLiteral("eplanimport/lastdir")).toString();
	const QString path = QFileDialog::getExistingDirectory(this, tr("Unpacked EPLAN DXF package"), last);
	if (path.isEmpty()) return false;
	settings.setValue(QStringLiteral("eplanimport/lastdir"), path);
	return setSource(path);
}

/**
	Take @p path as the source; its kind is told by the file: a folder is an
	unpacked package, else the suffix .zip, .edz or .dxf.
	@return false, with a message, when it is none of these
*/
bool EplanImportDialog::setSource(const QString &path)
{
	const QFileInfo info(path);
	const QString suffix = info.suffix().toLower();
	Source kind;
	if (info.isDir()) kind = Source::Folder;
	else if (suffix == QLatin1String("zip")) kind = Source::Package;
	else if (suffix == QLatin1String("edz")) kind = Source::Edz;
	else if (suffix == QLatin1String("dxf")) kind = Source::Dxf;
	else {
		QET::QetMessageBox::warning(this, tr("Part import"),
					    tr("%1 is no part file: choose a ZIP package, an .edz "
					       "part, a DXF or the folder of an unpacked package.")
					    .arg(QDir::toNativeSeparators(path)));
		return false;
	}
	m_kind = kind;
	// what the previous source told does not hold for this one
	m_manufacturer->clear();
	m_type->clear();
	m_edz->clear();
	m_category->setPlaceholderText(QStringLiteral("General"));
	// a DXF may take its part data from an .edz
	const bool dxf = m_kind == Source::Dxf;
	m_edz->setEnabled(dxf);
	m_edz_button->setEnabled(dxf);
	m_source->setText(QDir::toNativeSeparators(path));
	sourceChanged();
	return true;
}

void EplanImportDialog::dragEnterEvent(QDragEnterEvent *event)
{
	const QList<QUrl> urls = event->mimeData()->urls();
	if (urls.size() == 1 && urls.first().isLocalFile()) event->acceptProposedAction();
}

void EplanImportDialog::dropEvent(QDropEvent *event)
{
	const QList<QUrl> urls = event->mimeData()->urls();
	if (urls.size() != 1 || !urls.first().isLocalFile()) return;
	event->acceptProposedAction();
	setSource(urls.first().toLocalFile());
}

/**
	Fill in what the source itself tells: the part data of a package or of
	an .edz part.
*/
void EplanImportDialog::sourceChanged()
{
	const QString path = QDir::fromNativeSeparators(m_source->text());
	const bool package = m_kind == Source::Package || m_kind == Source::Folder;
	// a package names manufacturer and type itself
	m_manufacturer->setReadOnly(package);
	m_type->setReadOnly(package);
	if (package) {
		QString error;
		const QMap<QString, QString> art = EplanPartImporter::packageInfo(path, &error);
		if (art.isEmpty()) {
			QET::QetMessageBox::warning(this, tr("Part import"),
						    tr("This is no EPLAN DXF package:\n%1").arg(error));
			m_manufacturer->clear();
			m_type->clear();
			updateButtons();
			return;
		}
		m_manufacturer->setText(EplanPartImporter::manufacturerName(
			art.value(QStringLiteral("Manufacturer (full name)"))));
		// a package may hold several articles, each gets a folder of its own
		const QString articles = art.value(QStringLiteral("Articles"));
		QString type = art.value(QStringLiteral("Type number"));
		if (type.isEmpty()) type = art.value(QStringLiteral("Order number"));
		m_type->setText(articles.contains(QLatin1Char(',')) ? articles : type);
		const QString group = art.value(QStringLiteral("Product group"));
		m_category->setPlaceholderText(group.isEmpty() ? QStringLiteral("General") : group);
	} else if (m_kind == Source::Edz) {
		readEdz(path);
	} else {
		// a DXF out of an unpacked Data Portal package: its commercialdata.csv
		// a few folders up names manufacturer and type
		QDir dir = QFileInfo(path).absoluteDir();
		bool found = false;
		for (int up = 0; up < 5 && !found; ++up) {
			if (dir.exists(QStringLiteral("commercialdata.csv"))) {
				found = true;
				const QStringList rel = dir.relativeFilePath(path).split(QLatin1Char('/'));
				const QString macro = rel.size() > 1 && rel.first().compare(QLatin1String("dxf"), Qt::CaseInsensitive) == 0
							      ? rel.at(1) : QString();
				const QMap<QString, QString> art = EplanPartImporter::packageInfo(dir.absolutePath(), nullptr, macro);
				if (!art.isEmpty()) {
					m_manufacturer->setText(EplanPartImporter::manufacturerName(
						art.value(QStringLiteral("Manufacturer (full name)"))));
					QString type = art.value(QStringLiteral("Type number"));
					if (type.isEmpty()) type = art.value(QStringLiteral("Order number"));
					if (!type.isEmpty()) m_type->setText(type);
				}
			} else if (!dir.cdUp()) {
				break;
			}
		}
		if (m_type->text().isEmpty()) m_type->setText(QFileInfo(path).completeBaseName());
	}
	updateButtons();
}

void EplanImportDialog::chooseEdz()
{
	QSettings settings;
	const QString path = QFileDialog::getOpenFileName(
		this, tr("EPLAN part"),
		settings.value(QStringLiteral("eplanimport/lastdir")).toString(),
		tr("EPLAN parts (*.edz)"));
	if (path.isEmpty()) return;
	m_edz->setText(QDir::toNativeSeparators(path));
	readEdz(path);
}

/// manufacturer and type from an .edz part, where not yet filled in
void EplanImportDialog::readEdz(const QString &path)
{
	QGuiApplication::setOverrideCursor(Qt::WaitCursor);
	EdzArchive archive;
	EdzPart part;
	const bool ok = archive.extract(path) && part.parse(archive.partXmlPath());
	QGuiApplication::restoreOverrideCursor();
	if (!ok) return;
	const bool from_dxf = m_kind == Source::Dxf;
	if (!from_dxf || m_manufacturer->text().isEmpty())
		m_manufacturer->setText(EplanPartImporter::manufacturerName(part.manufacturer()));
	// a DXF file name is only a stand-in for the type
	const bool type_from_file =
		m_type->text() == QFileInfo(m_source->text()).completeBaseName();
	if (!part.typeNumber().isEmpty() && (!from_dxf || m_type->text().isEmpty() || type_from_file))
		m_type->setText(part.typeNumber());
}

void EplanImportDialog::updateButtons()
{
	QStringList missing;
	if (m_source->text().isEmpty()) missing << tr("source");
	if (m_manufacturer->text().trimmed().isEmpty()) missing << tr("manufacturer");
	if (m_type->text().trimmed().isEmpty()) missing << tr("type");
	if (!m_terms->isChecked()) missing << tr("accept the terms");
	m_missing->setText(missing.isEmpty() ? QString()
					     : tr("Still missing: %1").arg(missing.join(QStringLiteral(", "))));
	m_import->setEnabled(missing.isEmpty());
}

QString EplanImportDialog::targetDir()
{
	return m_target->currentData().toString();
}

void EplanImportDialog::runImport()
{
	const QString dir = targetDir();
	if (dir.isEmpty()) {
		QET::QetMessageBox::warning(this, tr("Part import"), tr("Choose the folder to import into."));
		return;
	}
	QSettings settings;
	settings.setValue(QStringLiteral("eplanimport/target"), m_target->currentIndex());
	if (!m_other_dir.isEmpty()) settings.setValue(QStringLiteral("eplanimport/otherdir"), m_other_dir);

	const QString source = QDir::fromNativeSeparators(m_source->text());
	const QString manufacturer = m_manufacturer->text().trimmed();
	QString category = m_category->text().trimmed();
	// a package falls back to its product group itself
	if (category.isEmpty() && m_kind != Source::Package && m_kind != Source::Folder)
		category = QStringLiteral("General");
	const QString type = m_type->text().trimmed();

	EplanPartImporter importer;
	QGuiApplication::setOverrideCursor(Qt::WaitCursor);
	bool ok = false;
	switch (m_kind) {
		case Source::Package:
		case Source::Folder:
			ok = importer.importPackage(source, category, dir);
			break;
		case Source::Dxf:
			ok = importer.importDxf(source, QDir::fromNativeSeparators(m_edz->text()),
						manufacturer, category, type, dir);
			break;
		case Source::Edz:
			ok = importer.importEdz(source, manufacturer, category, type, dir);
			break;
	}
	QGuiApplication::restoreOverrideCursor();
	if (!ok) {
		QET::QetMessageBox::critical(this, tr("Part import"),
					     tr("The import failed:\n%1").arg(importer.errorString()));
		return;
	}
	m_written = importer.writtenPaths();
	// all of them may be kept, edited since an earlier import
	const QString folder = (m_written.isEmpty() ? importer.keptPaths() : m_written).value(0);
	QString text = tr("%n element(s) imported into\n%1", "", int(m_written.size()))
			       .arg(QDir::toNativeSeparators(QFileInfo(folder).absolutePath()));
	if (!importer.messages().isEmpty())
		text += QStringLiteral("\n\n") + importer.messages().join(QLatin1Char('\n'));
	QET::QetMessageBox::information(this, tr("Part import"), text);
	accept();
}
