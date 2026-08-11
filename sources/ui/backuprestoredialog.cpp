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

#include "backuprestoredialog.h"

#ifdef BUILD_WITHOUT_KF5
#	include "nokde/kautosavefile.h"
#else
#	include <KAutoSaveFile>
#endif

#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLocale>
#include <QVBoxLayout>

/**
	@brief BackupRestoreDialog::BackupRestoreDialog
	@param groups : managed project path -> its recovery generations
	(newest-first)
	@param parent : parent widget
*/
BackupRestoreDialog::BackupRestoreDialog(
	const QHash<QString, QList<KAutoSaveFile *>> &groups,
	QWidget *parent) :
	QDialog(parent),
	m_groups(groups)
{
	setWindowTitle(tr("Fichiers de restauration", "window title"));

	auto main_layout = new QVBoxLayout(this);

	auto intro = new QLabel(
		tr("<b>Des fichiers de restauration ont été trouvés,<br>"
		   "voulez-vous les ouvrir ?</b><br>"
		   "Pour un projet ayant plusieurs versions de restauration, "
		   "la plus récente est sélectionnée par défaut.",
		   "dialog message"));
	intro->setWordWrap(true);
	main_layout->addWidget(intro);

	auto grid = new QGridLayout();
	int row = 0;
	for (auto it = m_groups.constBegin(); it != m_groups.constEnd(); ++it)
	{
		const QString &path = it.key();
		const QList<KAutoSaveFile *> &generations = it.value();

#	ifdef Q_OS_WIN
		QString display_path = path;
		display_path.remove(0, 1);
#	else
		const QString &display_path = path;
#	endif
		grid->addWidget(new QLabel(display_path), row, 0);

		auto combo = new QComboBox(this);
		for (int i = 0; i < generations.size(); ++i)
		{
			const QDateTime modified =
				QFileInfo(*generations.at(i)).lastModified();
			QString label = QLocale::system().toString(
				modified, QLocale::ShortFormat);
			if (i == 0) {
				label = tr("%1 (la plus récente)", "recovery generation label")
							.arg(label);
			}
			combo->addItem(label);
		}
		combo->setEnabled(generations.size() > 1);
		grid->addWidget(combo, row, 1);

		m_combo_for_path.insert(path, combo);
		++row;
	}
	main_layout->addLayout(grid);

	auto buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	main_layout->addWidget(buttons);
}

/**
	@brief BackupRestoreDialog::~BackupRestoreDialog
*/
BackupRestoreDialog::~BackupRestoreDialog() = default;

/**
	@return one recovery generation per project, matching the combo box
	selection (the newest generation, unless the user picked another).
*/
QList<KAutoSaveFile *> BackupRestoreDialog::selectedFiles() const
{
	QList<KAutoSaveFile *> selected;
	for (auto it = m_groups.constBegin(); it != m_groups.constEnd(); ++it)
	{
		const int index = m_combo_for_path.value(it.key())->currentIndex();
		selected << it.value().at(index);
	}
	return selected;
}

/**
	@return every recovery generation the user did not pick; the caller
	should release and delete these.
*/
QList<KAutoSaveFile *> BackupRestoreDialog::discardedFiles() const
{
	QList<KAutoSaveFile *> discarded;
	for (auto it = m_groups.constBegin(); it != m_groups.constEnd(); ++it)
	{
		const int index = m_combo_for_path.value(it.key())->currentIndex();
		const QList<KAutoSaveFile *> &generations = it.value();
		for (int i = 0; i < generations.size(); ++i) {
			if (i != index) {
				discarded << generations.at(i);
			}
		}
	}
	return discarded;
}
