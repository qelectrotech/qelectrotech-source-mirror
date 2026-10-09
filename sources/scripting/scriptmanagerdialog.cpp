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
#include "scriptmanagerdialog.h"

#include "scriptlibrary.h"
#include "../qetmessagebox.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

ScriptManagerDialog::ScriptManagerDialog(Runner runner, QWidget *parent) :
	QDialog(parent),
	m_runner(std::move(runner))
{
	setWindowTitle(tr("Gérer les scripts"));
	setAttribute(Qt::WA_DeleteOnClose);

	m_list = new QListWidget(this);
	m_list->setIconSize(QSize(24, 24));
	m_list->setMinimumWidth(220);
	auto *new_button = new QPushButton(tr("&Nouveau"), this);
	m_delete = new QPushButton(tr("Supp&rimer"), this);
	auto *left_buttons = new QHBoxLayout();
	left_buttons->addWidget(new_button);
	left_buttons->addWidget(m_delete);
	auto *left = new QVBoxLayout();
	left->addWidget(m_list);
	left->addLayout(left_buttons);

	m_name = new QLineEdit(this);
	m_name->setPlaceholderText(tr("Le texte du bouton"));
	m_icon = new QLineEdit(this);
	m_icon->setPlaceholderText(tr("vide : les initiales du nom ; builtin:<nom> : une icône du thème"));
	m_icon_preview = new QToolButton(this);
	m_icon_preview->setIconSize(QSize(24, 24));
	m_icon_preview->setToolTip(tr("Choisir une image (SVG ou PNG)…"));
	auto *icon_row = new QHBoxLayout();
	icon_row->addWidget(m_icon);
	icon_row->addWidget(m_icon_preview);
	m_tooltip = new QLineEdit(this);
	m_shortcut = new QKeySequenceEdit(this);
	m_context = new QComboBox(this);
	m_context->addItem(tr("Toujours"), QStringLiteral("canvas"));
	m_context->addItem(tr("Avec une sélection"), QStringLiteral("selection"));
	m_context->addItem(tr("Avec un conducteur sélectionné"), QStringLiteral("conductor"));

	auto *form = new QFormLayout();
	form->addRow(tr("Nom :"), m_name);
	form->addRow(tr("Icône :"), icon_row);
	form->addRow(tr("Info-bulle :"), m_tooltip);
	form->addRow(tr("Raccourci :"), m_shortcut);
	form->addRow(tr("Actif :"), m_context);

	m_body = new QPlainTextEdit(this);
	m_body->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	m_body->setLineWrapMode(QPlainTextEdit::NoWrap);
	m_body->setMinimumSize(480, 260);

	m_status = new QLabel(this);
	m_status->setWordWrap(true);
	m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);

	m_test = new QPushButton(tr("&Tester"), this);
	m_test->setToolTip(tr("Enregistre puis exécute le script sur le projet courant"
			      " (Ctrl+Z annule l'exécution)"));
	m_save = new QPushButton(tr("&Enregistrer"), this);
	auto *folder_button = new QPushButton(tr("Ouvrir le &dossier"), this);
	auto *close_button = new QPushButton(tr("&Fermer"), this);
		//Enter in a field saves; in the script it is a new line
	m_save->setDefault(true);
	auto *buttons = new QHBoxLayout();
	buttons->addWidget(m_test);
	buttons->addWidget(m_save);
	buttons->addStretch();
	buttons->addWidget(folder_button);
	buttons->addWidget(close_button);

	auto *right = new QVBoxLayout();
	right->addLayout(form);
	right->addWidget(new QLabel(tr("Script (l'objet qet ; qet.currentFolio() est le folio affiché) :"), this));
	right->addWidget(m_body, 1);
	right->addWidget(m_status);
	right->addLayout(buttons);

	auto *layout = new QHBoxLayout(this);
	layout->addLayout(left);
	layout->addLayout(right, 1);

	connect(new_button, &QPushButton::clicked, this, &ScriptManagerDialog::newScript);
	connect(m_delete, &QPushButton::clicked, this, &ScriptManagerDialog::deleteScript);
	connect(m_save, &QPushButton::clicked, this, &ScriptManagerDialog::save);
	connect(m_test, &QPushButton::clicked, this, &ScriptManagerDialog::testScript);
	connect(m_icon_preview, &QToolButton::clicked, this, &ScriptManagerDialog::chooseIcon);
	connect(close_button, &QPushButton::clicked, this, &QDialog::close);
	connect(folder_button, &QPushButton::clicked, this, []() {
		QDir().mkpath(ScriptLibrary::folder());
		QDesktopServices::openUrl(QUrl::fromLocalFile(ScriptLibrary::folder()));
	});

	auto modified = [this]() { if (!m_loading) setModified(true); };
	connect(m_name, &QLineEdit::textEdited, this, modified);
	connect(m_icon, &QLineEdit::textEdited, this, modified);
	connect(m_icon, &QLineEdit::textChanged, this, &ScriptManagerDialog::updateIconPreview);
	connect(m_name, &QLineEdit::textChanged, this, &ScriptManagerDialog::updateIconPreview);
	connect(m_tooltip, &QLineEdit::textEdited, this, modified);
	connect(m_shortcut, &QKeySequenceEdit::keySequenceChanged, this, modified);
	connect(m_context, &QComboBox::currentIndexChanged, this, modified);
	connect(m_body, &QPlainTextEdit::textChanged, this, modified);

	connect(m_list, &QListWidget::currentItemChanged, this,
		[this](QListWidgetItem *current, QListWidgetItem *previous) {
		if (m_loading || !current) return;
		if (!confirmDiscard()) {
			m_loading = true;
			m_list->setCurrentItem(previous);
			m_loading = false;
			return;
		}
		showScript(current->data(Qt::UserRole).toString());
	});

		//A file written by hand or by an assistant shows up here too
	connect(&ScriptLibrary::instance(), &ScriptLibrary::changed,
		this, &ScriptManagerDialog::reload);

	reload();
	if (m_list->count()) {
		m_list->setCurrentRow(0);
	} else {
		newScript();
	}
}

void ScriptManagerDialog::reload()
{
	m_loading = true;
	const QString current = m_path;
	m_list->clear();
	const ScriptLibrary &library = ScriptLibrary::instance();
	for (const ScriptLibrary::Script &script : library.scripts()) {
		auto *item = new QListWidgetItem(ScriptLibrary::icon(script), script.header.name, m_list);
		item->setData(Qt::UserRole, script.path);
		item->setToolTip(QFileInfo(script.path).fileName());
		if (script.path == current) m_list->setCurrentItem(item);
	}
		//Listed so they can be opened and fixed here
	for (const QString &error : library.errors()) {
		const QString file = error.section(QStringLiteral(": "), 0, 0);
		const QString path = QDir(ScriptLibrary::folder()).filePath(file);
		auto *item = new QListWidgetItem(file, m_list);
		item->setData(Qt::UserRole, path);
		item->setForeground(palette().color(QPalette::PlaceholderText));
		item->setToolTip(tr("Pas de bouton : %1").arg(error.section(QStringLiteral(": "), 1)));
		if (path == current) m_list->setCurrentItem(item);
	}
	m_loading = false;
		//Changed on disk and not here: show what is on disk now
	if (!m_modified && !m_path.isEmpty()) {
		if (QFileInfo::exists(m_path)) showScript(m_path);
		else clearForm();
	}
}

void ScriptManagerDialog::showScript(const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
	const QString text = QString::fromUtf8(file.readAll());
	const ScriptHeader h = ScriptHeader::parse(text, QFileInfo(path).completeBaseName());

	m_loading = true;
	m_path = path;
	m_name->setText(h.name);
	m_icon->setText(h.icon);
	m_tooltip->setText(h.tooltip);
	m_shortcut->setKeySequence(QKeySequence::fromString(h.shortcut));
	m_context->setCurrentIndex(qMax(0, m_context->findData(h.context)));
	m_body->setPlainText(ScriptHeader::bodyOf(text));
	m_loading = false;
	setModified(false);
	m_status->setText(h.isValid()
			  ? QDir::toNativeSeparators(path)
			  : tr("Pas de bouton pour ce fichier : %1").arg(h.error));
}

void ScriptManagerDialog::clearForm()
{
	m_loading = true;
	m_path.clear();
	m_name->clear();
	m_icon->clear();
	m_tooltip->clear();
	m_shortcut->clear();
	m_context->setCurrentIndex(0);
	m_body->clear();
	m_loading = false;
	setModified(false);
	m_status->clear();
}

void ScriptManagerDialog::newScript()
{
	if (!confirmDiscard()) return;
	m_loading = true;
	m_list->clearSelection();
	m_list->setCurrentItem(nullptr);
	m_loading = false;
	clearForm();
	m_loading = true;
	m_name->setText(tr("Nouveau script"));
	m_body->setPlainText(tr(
		"// qet.currentFolio() est le folio affiché ; un clic s'annule\n"
		"// d'un seul Ctrl+Z. Liste des appels : qet.apiSignatures()\n"
		"var f = qet.currentFolio();\n"
		"qet.addText(f, \"Texte\", 40, 40);\n"));
	m_loading = false;
	setModified(true);
	m_status->setText(tr("Pas encore enregistré"));
	m_name->setFocus();
	m_name->selectAll();
}

/**
	@brief ScriptManagerDialog::save
	Write the file; the library's watcher turns it into a button.
	@return true if written
*/
bool ScriptManagerDialog::save()
{
	const QString name = m_name->text().simplified();
	if (name.isEmpty()) {
		m_status->setText(tr("Le script doit avoir un nom."));
		m_name->setFocus();
		return false;
	}
	QDir dir(ScriptLibrary::folder());
	dir.mkpath(QStringLiteral("."));
	QString path = m_path;
	if (path.isEmpty()) {
		QStringList taken;
		for (const QFileInfo &info : dir.entryInfoList({QStringLiteral("*.js")}, QDir::Files))
			taken << info.completeBaseName();
		path = dir.filePath(ScriptHeader::idFor(name, taken) + QStringLiteral(".js"));
	}

	const QString text = ScriptHeader::compose(name, m_icon->text(), m_tooltip->text(),
				     m_shortcut->keySequence().toString(QKeySequence::PortableText),
				     m_context->currentData().toString(), m_body->toPlainText());
	const ScriptHeader h = ScriptHeader::parse(text, QFileInfo(path).completeBaseName());
	if (!h.isValid()) {
		m_status->setText(tr("Non enregistré : %1").arg(h.error));
		return false;
	}

		//QSaveFile renames into place, so the watcher never reads half a file
	QSaveFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)
	    || file.write(text.toUtf8()) < 0 || !file.commit()) {
		m_status->setText(tr("Impossible d'écrire %1").arg(QDir::toNativeSeparators(path)));
		return false;
	}
	m_path = path;
	setModified(false);
	m_status->setText(QDir::toNativeSeparators(path));
	return true;
}

void ScriptManagerDialog::deleteScript()
{
	if (m_path.isEmpty()) {
		clearForm();
		return;
	}
	const auto answer = QET::QetMessageBox::question(
		this, tr("Supprimer le script"),
		tr("Supprimer « %1 » et son bouton ?").arg(m_name->text()),
		QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
	if (answer != QMessageBox::Yes) return;

	const QString icon = m_icon->text();
	QFile::remove(m_path);
		//Its icon too, unless another script uses it
	if (!icon.isEmpty() && !icon.startsWith(QLatin1String("builtin:"))
	    && !icon.contains(QLatin1Char('/')) && !icon.contains(QLatin1Char('\\'))) {
		bool used = false;
		for (const ScriptLibrary::Script &s : ScriptLibrary::instance().scripts())
			if (s.path != m_path && s.header.icon == icon) used = true;
		if (!used) QFile::remove(QDir(ScriptLibrary::folder()).filePath(icon));
	}
	clearForm();
}

void ScriptManagerDialog::testScript()
{
	if (m_modified || m_path.isEmpty()) {
		if (!save()) return;
	}
	if (m_runner) m_runner(m_path, m_name->text().simplified());
}

/**
	@brief ScriptManagerDialog::chooseIcon
	Copy an image into the scripts folder, so the script and its icon
	travel together, and name it in the header.
*/
void ScriptManagerDialog::chooseIcon()
{
	const QString source = QFileDialog::getOpenFileName(
		this, tr("Icône du script"), QString(),
		tr("Images (*.svg *.png)"));
	if (source.isEmpty()) return;
	QDir dir(ScriptLibrary::folder());
	dir.mkpath(QStringLiteral("."));
	const QString name = QFileInfo(source).fileName();
	const QString target = dir.filePath(name);
	if (QFileInfo(source).absoluteFilePath() != QFileInfo(target).absoluteFilePath()) {
		QFile::remove(target);
		if (!QFile::copy(source, target)) {
			m_status->setText(tr("Impossible de copier %1").arg(QDir::toNativeSeparators(source)));
			return;
		}
	}
	m_icon->setText(name);
	setModified(true);
}

void ScriptManagerDialog::updateIconPreview()
{
	ScriptLibrary::Script script;
	script.header.name = m_name->text();
	script.header.icon = m_icon->text().trimmed();
	script.path = m_path.isEmpty() ? QDir(ScriptLibrary::folder()).filePath(QStringLiteral("new.js"))
				       : m_path;
	m_icon_preview->setIcon(ScriptLibrary::icon(script));
}

void ScriptManagerDialog::setModified(bool modified)
{
	m_modified = modified;
	m_save->setEnabled(modified);
	m_delete->setEnabled(!m_path.isEmpty() || modified);
	setWindowTitle(tr("Gérer les scripts") + (modified ? QStringLiteral(" *") : QString()));
}

bool ScriptManagerDialog::confirmDiscard()
{
	if (!m_modified) return true;
	const auto answer = QET::QetMessageBox::question(
		this, tr("Gérer les scripts"),
		tr("Le script affiché n'est pas enregistré. Enregistrer ?"),
		QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
		QMessageBox::Save);
	if (answer == QMessageBox::Cancel) return false;
	if (answer == QMessageBox::Save) return save();
	setModified(false);
	return true;
}

void ScriptManagerDialog::closeEvent(QCloseEvent *event)
{
	if (confirmDiscard()) event->accept();
	else event->ignore();
}
