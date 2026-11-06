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
#include "../../sources/ui/nokde/kautosavefile.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>

#include <cstdio>

/*
	Run by kautosavefile_test.cpp, with an application name and the path
	of a project: writes a crash-recovery file of that project, says
	"ready" and waits, until the test kills it as a crash would end
	QElectroTech. The recovery file is the managed path, in UTF-8.
*/
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	const QStringList args = QCoreApplication::arguments();
	if (args.size() != 3)
		return 1;

	QStandardPaths::setTestModeEnabled(true);
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech"));
	QCoreApplication::setApplicationName(args.at(1));

	const QString managed_path = args.at(2);
	KAutoSaveFile backup(QUrl::fromLocalFile(managed_path));
	if (!backup.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return 2;
	const QByteArray payload = managed_path.toUtf8();
	if (backup.write(payload) != payload.size() || !backup.flush())
		return 3;

	std::fputs("ready\n", stdout);
	std::fflush(stdout);
	for (;;)
		QThread::sleep(60);
}
