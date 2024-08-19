/*
	Copyright 2024 The QElectroTech Team
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

#include <QFile>
#include <QProcess>
#include <QInputDialog>
#include <QDir>

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
										Qt::WindowFlags(), 1);
	QString sFactorX = "1.0";
	if (ok)
		sFactorX = (QString::number(fx, 'f', 5));
	else
		return QByteArray();

	double fy = QInputDialog::getDouble(parent, QObject::tr("Entrer le facteur d'échelle"),
										QObject::tr("Facteur Y:"), fx, 0.1, 100, 5, &ok,
										Qt::WindowFlags(), 1);
	QString sFactorY = "1.0";
	if (ok)
		sFactorY = (QString::number(fy, 'f', 5));
	else
		return QByteArray();

	QProcess process_;
	const QString program{ElementScalerBinaryPath()};
	const QStringList arguments{QStringLiteral("-x"), sFactorX,
								QStringLiteral("-y"), sFactorY,
								QStringLiteral("-o"), QStringLiteral("-f"), file_path};

	process_.start(program, arguments);

	if (process_.waitForFinished())
	{
		const auto byte_array{process_.readAll()};
		process_.close();
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
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
	return (QDir::homePath() + QStringLiteral("/Application Data/qet/binary"));
#elif defined(Q_OS_MACOS)
	return (QDir::homePath() + QStringLiteral("/.qet/binary"));
#else
	return (QDir::homePath() + QStringLiteral("/.qet/binary"));
#endif
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