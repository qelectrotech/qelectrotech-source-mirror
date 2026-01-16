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
#include "thirdpartybinaryinstalldialog.h"
#include "ui_thirdpartybinaryinstalldialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>

ThirdPartyBinaryInstallDialog::ThirdPartyBinaryInstallDialog(const QString &text,
															 const QString &downloadLink,
															 const QString &binaryFolderPath,
															 QWidget *parent) :
	QDialog{parent},
	ui{new Ui::ThirdPartyBinaryInstallDialog}
{
	ui->setupUi(this);
	ui->m_label->setText(text);

	connect(ui->m_download_pb, &QPushButton::clicked, [downloadLink](){
		QDesktopServices::openUrl(QUrl(downloadLink));
	});
	connect(ui->m_install_dir_pb, &QPushButton::clicked, [binaryFolderPath]() {
			//Make sure the path exist
		QDir dir_;
		dir_.mkpath(binaryFolderPath);
		QDesktopServices::openUrl(QUrl("file:///" + binaryFolderPath));
	});
}

ThirdPartyBinaryInstallDialog::~ThirdPartyBinaryInstallDialog()
{
	delete ui;
}
