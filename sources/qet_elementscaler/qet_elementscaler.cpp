/*
	Copyright 2024-2025 The QElectroTech Team
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
#include "qet_elementscaler.h"
#include "../ui/thirdpartybinaryinstalldialog.h"
#include "../qetapp.h"

#include <QFile>
#include <QProcess>
#include <QInputDialog>
#include <QMessageBox>

/**
 * @brief QET_ElementScaler
 * Return the scaled element from @a file_path.
 * The returned value is a QByteArray, instead of a
 * QDomDocument or QString, to let user do what he/she wants.
 * If something goes wrong the QByteArray returned is empty.
 * @param file_path
 * @return
 */
QByteArray ElementScaler(const QString &file_path, QWidget *parent)
{
	if (!ElementScalerIsPresent(false)) {
		return QByteArray();
	}

	bool ok;
	double fx = QInputDialog::getDouble(parent, QObject::tr("Entrer le facteur d'échelle"),
										QObject::tr("Facteur X:"), 1.0, 0.1, 100, 5, &ok,
										Qt::WindowFlags());
	QString sFactorX = "1.0";
	if (ok)
		sFactorX = (QString::number(fx, 'f', 5));
	else
		return QByteArray();

	double fy = QInputDialog::getDouble(parent, QObject::tr("Entrer le facteur d'échelle"),
										QObject::tr("Facteur Y:"), fx, 0.1, 100, 5, &ok,
										Qt::WindowFlags());
	QString sFactorY = "1.0";
	if (ok)
		sFactorY = (QString::number(fy, 'f', 5));
	else
		return QByteArray();

	const QStringList items{QObject::tr("sans"),
							QObject::tr("horizontal"),
							QObject::tr("vertical"),
							QObject::tr("horizontal + vertical")};
	QString item = QInputDialog::getItem(parent,
										 QObject::tr("Retourner l'élément :"),
										 QObject::tr("direction"), items, 0, false, &ok);
	int8_t mirrorIndex = 0;
	if (ok && !item.isEmpty()) {
		mirrorIndex = items.indexOf(item, 0);
	}
	else
		return QByteArray();

	QProcess process_;
	const QString program{ElementScalerBinaryPath()};
	QStringList arguments{QStringLiteral("-x"), sFactorX,
						  QStringLiteral("-y"), sFactorY};
	switch (mirrorIndex) {
		case 1: arguments << QStringLiteral("--FlipHorizontal");
				break;
		case 2: arguments << QStringLiteral("--FlipVertical");
				break;
		case 3: arguments << QStringLiteral("--FlipHorizontal")
						  << QStringLiteral("--FlipVertical");
				break;
	}
	arguments << QStringLiteral("-o") << QStringLiteral("-f") << file_path;

	process_.start(program, arguments);

	if (process_.waitForFinished())
	{
		const auto byte_array{process_.readAllStandardOutput()};
		const auto error_output{process_.readAllStandardError()};
		process_.close();
		if (error_output.length() > 0) {
			// inform the user about log-output via QMessageBox
			QMessageBox msgBox;
			msgBox.setText(QObject::tr("QET_ElementScaler: \nadditional information about %1 import / scaling").arg(file_path));
			msgBox.setInformativeText(QObject::tr("See details here:"));
			msgBox.setDetailedText(error_output);
			msgBox.exec();
		}
		return byte_array;
	}
	else
	{
			//If something went wrong we return an empty QByteArray
		return QByteArray();
	}
}

QString ElementScalerDirPath()
{
	return QETApp::dataDir() + "/binary";
}

/**
 * @brief ElementScalerBinaryPath
 * @return the path to the QET_ElementScaler program
 */
QString ElementScalerBinaryPath()
{
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
	return ElementScalerDirPath() + QStringLiteral("/QET_ElementScaler.exe");
#elif defined(Q_OS_MACOS)
	return ElementScalerDirPath() + QStringLiteral("/./QET_ElementScaler");
#else
	return ElementScalerDirPath() + QStringLiteral("/QET_ElementScaler");
#endif
}

/**
 * @brief ElementScalerIsPresent
 * Return true if QET_ElementScaler is present in the system
 * @param install_dialog
 * True to display a dialog with the explanations
 * of how to install the QET_ElementScaler program
 * if not present in the system.
 * @return
 */
bool ElementScalerIsPresent(bool install_dialog, QWidget *parent)
{
	const bool exist{QFile::exists(ElementScalerBinaryPath())};

	if (!exist && install_dialog)
	{
		auto string_{QObject::tr("Le logiciel QET_ElementScaler est nécessaire pour mettre les éléments à l'échelle.\n"
								 "Veuillez télécharger celui-ci en suivant le lien ci dessous et le dézipper dans le dossier d'installation")};

		ThirdPartyBinaryInstallDialog dialog_(string_,
											  QStringLiteral("https://github.com/plc-user/QET_ElementScaler/releases"),
											  ElementScalerDirPath(),
											  parent);
		dialog_.exec();
	}
	return exist;
}
