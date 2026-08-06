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
#include "diagnosticsreportdialog.h"

#include "../../qetmessagebox.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

DiagnosticsReportDialog::DiagnosticsReportDialog(
		const QString &title,
		const QString &intro,
		const QByteArray &content,
		QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(title);
	resize(700, 500);

	auto *layout = new QVBoxLayout(this);

	auto *intro_label = new QLabel(intro, this);
	intro_label->setWordWrap(true);
	layout->addWidget(intro_label);

	auto *preview = new QPlainTextEdit(this);
	preview->setReadOnly(true);
	preview->setLineWrapMode(QPlainTextEdit::NoWrap);
	preview->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	preview->setPlainText(QString::fromUtf8(content));
	layout->addWidget(preview);

	auto *buttons = new QDialogButtonBox(this);
	QPushButton *save_button = buttons->addButton(tr("Enregistrer..."), QDialogButtonBox::ActionRole);
	buttons->addButton(QDialogButtonBox::Close);
	connect(save_button, &QPushButton::clicked, this, &DiagnosticsReportDialog::saveToFile);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(buttons->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &QDialog::accept);
	layout->addWidget(buttons);

	// Stash the content for saveToFile(); the preview widget already
	// holds a QString copy but we save the original UTF-8 bytes to avoid
	// any round-trip surprises.
	setProperty("qet_report_content", content);
}

void DiagnosticsReportDialog::saveToFile()
{
	const QString path = QFileDialog::getSaveFileName(
			this,
			tr("Enregistrer le rapport de diagnostic"),
			QStringLiteral("qet-diagnostic-report.txt"),
			tr("Fichiers texte (*.txt);;Tous les fichiers (*)"));

	if (path.isEmpty()) {
		return;
	}

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		QET::QetMessageBox::critical(
				this,
				tr("Erreur"),
				tr("Impossible d'écrire dans le fichier « %1 ».").arg(path));
		return;
	}

	file.write(property("qet_report_content").toByteArray());
	file.close();
}
