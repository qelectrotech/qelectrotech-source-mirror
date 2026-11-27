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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "projecttemplates.h"

#include <QCoreApplication>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QUuid>

QList<ProjectTemplates::Entry> ProjectTemplates::list(const QList<Folder> &folders)
{
	QList<Entry> found;
	for (const Folder &folder : folders) {
		const QDir dir(folder.path);
		const QFileInfoList files = dir.entryInfoList(QStringList{QStringLiteral("*.qet")},
													  QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
		for (const QFileInfo &file : files)
			found << Entry{file.completeBaseName(), file.absoluteFilePath(), folder.label};
	}
	return found;
}

bool ProjectTemplates::prepareCopy(const QString &template_path, const QString &out_path, QString *error)
{
	auto fail = [error](const QString &text) {
		if (error) *error = text;
		return false;
	};

	QFile in(template_path);
	if (!in.open(QIODevice::ReadOnly))
		return fail(QCoreApplication::translate("ProjectTemplates", "Cannot read %1: %2")
					.arg(template_path, in.errorString()));
	QDomDocument document;
	if (!document.setContent(&in) || document.documentElement().tagName() != QLatin1String("project"))
		return fail(QCoreApplication::translate("ProjectTemplates",
												"%1 is not a QElectroTech project in the .qet format.")
					.arg(QFileInfo(template_path).fileName()));

	QDomElement root = document.documentElement();
	root.removeAttribute(QStringLiteral("title"));
	root.setAttribute(QStringLiteral("uuid"), QUuid::createUuid().toString());

	QFile out(out_path);
	if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return fail(QCoreApplication::translate("ProjectTemplates", "Cannot write %1: %2")
					.arg(out_path, out.errorString()));
	out.write(document.toByteArray(2));
	return true;
}
