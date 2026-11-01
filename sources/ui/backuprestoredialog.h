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

#ifndef BACKUPRESTOREDIALOG_H
#define BACKUPRESTOREDIALOG_H

#include <QDateTime>
#include <QDialog>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

class KAutoSaveFile;
class QComboBox;

/**
	@brief Lets the user pick, per crashed project, which recovery
	generation to reopen when the crash-recovery rotation
	(@see QETProject::writeBackup) left more than one snapshot behind.
*/
class BackupRestoreDialog : public QDialog
{
	Q_OBJECT

	public:
		/// @param groups : managed project file path -> its stale recovery
		/// generations, each list already sorted newest-first. Ownership of
		/// the KAutoSaveFile objects stays with the caller.
		/// @param working_copies : managed project file path -> when the
		/// working copy it left was last committed (QET-SQL-FORMAT S3z3),
		/// offered above its generations; a project never saved has the
		/// key neverSavedKey().
		explicit BackupRestoreDialog(
			const QHash<QString, QList<KAutoSaveFile *>> &groups,
			QWidget *parent = nullptr,
			const QHash<QString, QDateTime> &working_copies = {});
		~BackupRestoreDialog() override;

		/// Valid once accepted: one entry per project whose working copy is
		/// not picked, the chosen generation (defaults to the most recent one).
		QList<KAutoSaveFile *> selectedFiles() const;
		/// The generations the user did not pick; the caller should discard
		/// (release + delete) these.
		QList<KAutoSaveFile *> discardedFiles() const;
		/// The projects whose working copy is picked (the default), by key.
		QStringList selectedWorkingCopies() const;

		/// The key of a project never saved, whose working copy is
		/// @p working_copy: no file path starts with a line feed.
		static QString neverSavedKey(const QString &working_copy);

	private:
		int backupIndex(const QString &path) const;

		QHash<QString, QList<KAutoSaveFile *>> m_groups;
		QHash<QString, QDateTime> m_working_copies;
		QHash<QString, QComboBox *> m_combo_for_path;
};

#endif // BACKUPRESTOREDIALOG_H
