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

#include <QDialog>
#include <QHash>
#include <QList>
#include <QString>

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
		explicit BackupRestoreDialog(
			const QHash<QString, QList<KAutoSaveFile *>> &groups,
			QWidget *parent = nullptr);
		~BackupRestoreDialog() override;

		/// Valid once accepted: one entry per project, the chosen generation
		/// (defaults to the most recent one).
		QList<KAutoSaveFile *> selectedFiles() const;
		/// The generations the user did not pick; the caller should discard
		/// (release + delete) these.
		QList<KAutoSaveFile *> discardedFiles() const;

	private:
		QHash<QString, QList<KAutoSaveFile *>> m_groups;
		QHash<QString, QComboBox *> m_combo_for_path;
};

#endif // BACKUPRESTOREDIALOG_H
