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
#include "dxftoelmt.h"
#include "../ui/thirdpartybinaryinstalldialog.h"
#include "../qetapp.h"

#include <QFile>
#include <QProcess>
#include <QMessageBox>

/**
 * @brief dxftoElmt
 * Return the dxf at @a file_path converted to elmt.
 * The returned value is a QByteArray, instead of a
 * QDomDocument or QString, to let user do what they want with that.
 * If something goes wrong the QByteArray returned is empty.
 * @param file_path
 * @return
 */
QByteArray dxfToElmt(const QString &file_path)
{
	if (!dxf2ElmtIsPresent(false)) {
		return QByteArray();
	}

	QProcess process_;
	const QString program{dxf2ElmtBinaryPath()};
	const QStringList arguments{file_path, QStringLiteral("-v")};

	process_.start(program, arguments);
	
	
	if (process_.waitForFinished())
	{
		const auto byte_array{process_.readAllStandardOutput()};
		const auto error_output{process_.readAllStandardError()};
		
			process_.close();
			if (error_output.length() > 0) {
			// inform the user about log-output via QMessageBox
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.setText(QObject::tr("Dxf2elmt: \nError: Make sure the file %1 is a valid .dxf file").arg(file_path));
			msgBox.setInformativeText (QObject::tr("See details here:"));
			msgBox.setDetailedText(error_output);
			msgBox.exec();
		}
			return byte_array;
	}
	else
	{
			//Something goes wrong we return an empty QByteArray
		return QByteArray();
	}
}

QString dxf2ElmtDirPath()
{
	return QETApp::dataDir() % "/binary";
}

/**
 * @brief dxf2ElmtBinaryPath
 * @return the path to the dxf2elmt program
 */
QString dxf2ElmtBinaryPath()
{
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
	return dxf2ElmtDirPath() % QStringLiteral("/dxf2elmt.exe");
#elif defined(Q_OS_MACOS)
	return dxf2ElmtDirPath() % QStringLiteral("/./dxf2elmt");
#else
	return dxf2ElmtDirPath() % QStringLiteral("/dxf2elmt");
#endif
}

/**
 * @brief dxf2ElmtIsPresent
 * Return true if dxf2elmt is present in the system
 * @param install_dialog
 * True to display a dialog with the explanations
 * of how to install the dxf2elmt program
 * if not present in the system.
 * @return
 */
bool dxf2ElmtIsPresent(bool install_dialog, QWidget *parent)
{
	const bool exist{QFile::exists(dxf2ElmtBinaryPath())};
	if (!exist && install_dialog)
	{
		auto string_{QObject::tr("L'import dxf nécessite le logiciel dxf2elmt.\n"
								 "Veuillez télécharger celui-ci en suivant le lien ci dessous et le dézipper dans le dossier d'installation")};

		ThirdPartyBinaryInstallDialog dialog_(string_,
											  QStringLiteral("https://github.com/Vadoola/dxf2elmt/releases"),
											  dxf2ElmtDirPath(),
											  parent);
		dialog_.exec();
	}
	return exist;
}
