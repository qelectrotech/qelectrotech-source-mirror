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

#ifdef BUILD_WITHOUT_KF
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
	@param working_copies : managed project path -> when the working copy
	it left was last committed (QET-SQL-FORMAT S3z3), offered first
*/
BackupRestoreDialog::BackupRestoreDialog(
	const QHash<QString, QList<KAutoSaveFile *>> &groups,
	QWidget *parent,
	const QHash<QString, QDateTime> &working_copies) :
	QDialog(parent),
	m_groups(groups),
	m_working_copies(working_copies)
{
	for (auto it = m_working_copies.constBegin(); it != m_working_copies.constEnd(); ++it) {
		if (!m_groups.contains(it.key())) {
			m_groups.insert(it.key(), {});
		}
	}

	setWindowTitle(tr("Recovery files", "window title"));

	auto main_layout = new QVBoxLayout(this);

	auto intro = new QLabel(
		tr("<b>Recovery files were found,<br>do you want to open "
		   "them?</b><br>For a project with several recovery "
		   "versions, the newest is selected by default.",
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
		QString display_path = path;
#	endif
		if (path.startsWith(QLatin1Char('\n'))) {
			display_path = tr("A project never saved", "recovery files");
		}
		grid->addWidget(new QLabel(display_path), row, 0);

		auto combo = new QComboBox(this);
		if (m_working_copies.contains(path)) {
			combo->addItem(tr("%1 (after the last edit)", "recovery generation label")
						   .arg(QLocale::system().toString(m_working_copies.value(path).toLocalTime(),
														  QLocale::ShortFormat)));
		}
		for (int i = 0; i < generations.size(); ++i)
		{
			const QDateTime modified =
				QFileInfo(*generations.at(i)).lastModified();
			QString label = QLocale::system().toString(
				modified, QLocale::ShortFormat);
			if (i == 0 && !m_working_copies.contains(path)) {
				label = tr("%1 (newest)", "recovery generation label")
							.arg(label);
			}
			combo->addItem(label);
		}
		combo->setEnabled(combo->count() > 1);
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
		const int index = backupIndex(it.key());
		if (index >= 0) {
			selected << it.value().at(index);
		}
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
		const int index = backupIndex(it.key());
		const QList<KAutoSaveFile *> &generations = it.value();
		for (int i = 0; i < generations.size(); ++i) {
			if (i != index) {
				discarded << generations.at(i);
			}
		}
	}
	return discarded;
}

/**
	@return the projects whose working copy is picked (the default when
	there is one), by their key
*/
QStringList BackupRestoreDialog::selectedWorkingCopies() const
{
	QStringList selected;
	for (auto it = m_working_copies.constBegin(); it != m_working_copies.constEnd(); ++it) {
		if (m_combo_for_path.value(it.key())->currentIndex() == 0) {
			selected << it.key();
		}
	}
	return selected;
}

/**
	@return the key of a project never saved, whose working copy is
	@p working_copy: no file path starts with a line feed
*/
QString BackupRestoreDialog::neverSavedKey(const QString &working_copy)
{
	return QLatin1Char('\n') + working_copy;
}

/**
	@return the recovery generation of the project @p path that is picked,
	or -1 if its working copy is
*/
int BackupRestoreDialog::backupIndex(const QString &path) const
{
	const int index = m_combo_for_path.value(path)->currentIndex();
	return m_working_copies.contains(path) ? index - 1 : index;
}
