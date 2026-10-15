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
#ifndef SYNCFOLDER_H
#define SYNCFOLDER_H

#include <QFile>
#include <QFileInfo>
#include <QString>

#ifndef Q_OS_WIN
#include <fcntl.h>
#include <unistd.h>
#endif

namespace QET {
	/**
		Force to disk the folder holding @p file_path, once
		QSaveFile::commit() renamed a new file into it. QSaveFile syncs the
		file it wrote but not the folder, and on some file systems (ext4,
		XFS) a power cut soon after can then lose the rename: the previous
		file comes back, or none. Does nothing on Windows, where a folder
		cannot be synced this way.
	*/
	inline void syncFolderOf(const QString &file_path)
	{
#ifndef Q_OS_WIN
		const QByteArray folder = QFile::encodeName(QFileInfo(file_path).absolutePath());
		const int fd = ::open(folder.constData(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
		if (fd >= 0) {
			::fsync(fd);
			::close(fd);
		}
#else
		Q_UNUSED(file_path)
#endif
	}
}

#endif // SYNCFOLDER_H
