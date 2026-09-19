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
#ifndef QET_KAUTOSAVEFILE_H
#define QET_KAUTOSAVEFILE_H

#include <QFile>
#include <QList>
#include <QUrl>

#include <memory>

class QLockFile;

/**
	Small Qt-only replacement for the KAutoSaveFile API used by QET.

	It stores crash-recovery files below the application data autosave folder
	and protects each recovery file with QLockFile.  The original managed file
	path is kept in a sidecar file so allStaleFiles() can present recoverable
	projects on the next startup.
*/
class KAutoSaveFile : public QFile
{
	public:
		explicit KAutoSaveFile(const QUrl &filename, QObject *parent = nullptr);
		explicit KAutoSaveFile(QObject *parent = nullptr);
		~KAutoSaveFile() override;

		QUrl managedFile() const;
		void setManagedFile(const QUrl &filename);
		virtual void releaseLock();
		bool open(OpenMode openmode) override;

		static QList<KAutoSaveFile *> staleFiles(
			const QUrl &url,
			const QString &applicationName = QString());
		static QList<KAutoSaveFile *> allStaleFiles(
			const QString &applicationName = QString());

	private:
		QUrl m_managed_file;
		std::unique_ptr<QLockFile> m_lock;
		bool m_managed_file_name_changed = false;
};

#endif // QET_KAUTOSAVEFILE_H
