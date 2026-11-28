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
#include "macropackagetree.h"

#include "../../qetapp.h"
#include "../../qetmessagebox.h"
#include "../eplandxf/eplanpartimporter.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QSettings>
#include <QShowEvent>
#include <QStyle>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {
	const int TypeRole = Qt::UserRole;        ///< type number of a type entry
	const int LoadedRole = Qt::UserRole + 1;
}

MacroPackageTree::MacroPackageTree(QWidget *parent) :
	QWidget(parent)
{
	m_filter = new QLineEdit(this);
	m_filter->setPlaceholderText(tr("Type or text, e.g. EL1809"));
	m_filter->setClearButtonEnabled(true);
	connect(m_filter, &QLineEdit::textChanged, this, &MacroPackageTree::filter);

	m_tree = new QTreeWidget(this);
	m_tree->setHeaderHidden(true);
	m_tree->setUniformRowHeights(true);
	connect(m_tree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item) { activate(item); });

	m_hint = new QLabel(this);
	m_hint->setWordWrap(true);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(2);
	layout->addWidget(m_filter);
	layout->addWidget(m_tree, 1);
	layout->addWidget(m_hint);
}

/// the package is read when the tab is looked at, not at program start
void MacroPackageTree::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	const QString path = QSettings().value(QStringLiteral("macropackage/path")).toString();
	if (path != m_path || m_tree->topLevelItemCount() == 0) fill();
}

/// where the elements of @p article lie, or will lie, in the company collection
QString MacroPackageTree::folderOf(const MacroPackageArticle &article) const
{
	return EplanPartImporter::macroPackageFolder(m_package, article, QString(), QETApp::companyElementsDir());
}

void MacroPackageTree::fill()
{
	m_tree->clear();
	m_path = QSettings().value(QStringLiteral("macropackage/path")).toString();
	if (m_path.isEmpty() || !QFileInfo::exists(m_path)) {
		m_hint->setText(tr("No macro package chosen yet: Part import > From a manufacturer macro package (ZIP)…"));
		return;
	}
	QGuiApplication::setOverrideCursor(Qt::WaitCursor);
	const bool ok = m_package.open(m_path);
	QGuiApplication::restoreOverrideCursor();
	if (!ok) {
		m_hint->setText(m_package.errorString());
		return;
	}
	// manufacturer > product family > type
	QMap<QString, QTreeWidgetItem *> makers, families;
	QList<QTreeWidgetItem *> tops;
	for (const MacroPackageArticle &a : m_package.articles()) {
		const QString maker = m_package.manufacturerName(a);
		QTreeWidgetItem *&maker_item = makers[maker];
		if (!maker_item) {
			maker_item = new QTreeWidgetItem({maker});
			maker_item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
			tops << maker_item;
		}
		const QString family = a.category.isEmpty() ? QStringLiteral("General") : a.category;
		QTreeWidgetItem *&family_item = families[maker + QLatin1Char('/') + family];
		if (!family_item) {
			family_item = new QTreeWidgetItem(maker_item, {family});
			family_item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
		}
		auto *item = new QTreeWidgetItem(
			family_item, {a.description.isEmpty() ? a.type() : a.type() + QStringLiteral(" – ") + a.description});
		item->setData(0, TypeRole, a.type());
		mark(item, QDir(folderOf(a)).exists());
	}
	m_tree->addTopLevelItems(tops);
	for (QTreeWidgetItem *top : std::as_const(tops)) {
		top->setExpanded(true);
		top->sortChildren(0, Qt::AscendingOrder);
	}
	m_hint->setText(tr("%1 articles in %2. Grey entries are not converted yet: a double click "
			   "converts the type and shows it in the company collection.")
			.arg(m_package.articles().size()).arg(QFileInfo(m_path).fileName()));
	filter(m_filter->text());
}

/// grey with a download arrow until the type is in the collection
void MacroPackageTree::mark(QTreeWidgetItem *item, bool loaded)
{
	item->setData(0, LoadedRole, loaded);
	item->setIcon(0, style()->standardIcon(loaded ? QStyle::SP_DialogApplyButton : QStyle::SP_ArrowDown));
	item->setForeground(0, palette().brush(loaded ? QPalette::Active : QPalette::Disabled, QPalette::Text));
	item->setToolTip(0, loaded ? tr("In the company collection. Double click: show it there.")
				   : tr("Not converted yet. Double click: convert this type into the company collection."));
}

void MacroPackageTree::filter(const QString &text)
{
	const QStringList words = text.toLower().split(QLatin1Char(' '), Qt::SkipEmptyParts);
	for (int m = 0; m < m_tree->topLevelItemCount(); ++m) {
		QTreeWidgetItem *maker = m_tree->topLevelItem(m);
		for (int f = 0; f < maker->childCount(); ++f) {
			QTreeWidgetItem *family = maker->child(f);
			int shown = 0;
			for (int t = 0; t < family->childCount(); ++t) {
				QTreeWidgetItem *type = family->child(t);
				const QString hay = (type->text(0) + QLatin1Char(' ') + family->text(0)).toLower();
				const bool match = std::all_of(words.cbegin(), words.cend(),
							       [&hay](const QString &w) { return hay.contains(w); });
				type->setHidden(!match);
				if (match) ++shown;
			}
			family->setHidden(shown == 0);
			// a search opens what it found, unless that is half the package
			family->setExpanded(!words.isEmpty() && shown > 0 && shown <= 200);
		}
	}
}

/**
	A type was asked for: convert it if it is not in the collection yet,
	then tell where it lies.
*/
void MacroPackageTree::activate(QTreeWidgetItem *item)
{
	const QString type = item ? item->data(0, TypeRole).toString() : QString();
	if (type.isEmpty()) return;
	const MacroPackageArticle *found = m_package.article(type);
	if (!found) return;
	const QString folder = folderOf(*found);
	if (QDir(folder).exists()) {
		mark(item, true);
		emit typeLoaded(folder, false);
		return;
	}
	if (!m_terms_accepted) {
		// the terms of the other part import dialogs, asked once per session
		const auto answer = QET::QetMessageBox::question(
			this, tr("Manufacturer macro package"),
			QCoreApplication::translate("EplanImportDialog",
				"Manufacturer data from the EPLAN Data Portal or a manufacturer's "
				"website is subject to the terms of use of that source. By importing "
				"you confirm that you are authorised to use it here; the import is "
				"at your own responsibility.")
				+ QStringLiteral("\n\n")
				+ QCoreApplication::translate("EplanImportDialog", "I accept these terms."),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
		if (answer != QMessageBox::Yes) return;
		m_terms_accepted = true;
	}
	EplanPartImporter importer;
	QGuiApplication::setOverrideCursor(Qt::WaitCursor);
	const bool ok = importer.importMacroPackage(m_package, {type}, QString(), QETApp::companyElementsDir());
	QGuiApplication::restoreOverrideCursor();
	if (!ok || importer.writtenPaths().isEmpty()) {
		const QString why = !importer.errorString().isEmpty() ? importer.errorString()
								      : importer.failed().join(QLatin1Char('\n'));
		QET::QetMessageBox::warning(this, tr("Manufacturer macro package"),
					    tr("%1 could not be converted:\n%2").arg(type, why));
		return;
	}
	mark(item, true);
	emit typeLoaded(folder, true);
}
