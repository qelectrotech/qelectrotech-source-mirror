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
#include "qetapptest.h"
#include "scripting/assistantinfo.h"
#include "ui/configpage/generalconfigurationpage.h"

#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalBlocker>

/**
	qet-assistant.json names the user and company collections, and the qet
	MCP server hands them to every headless run it starts. Accepting the
	settings dialog wrote the file before the dialog applied the new
	collection paths, so the file kept the old ones: a company collection
	chosen in the dialog stayed unknown to the assistant, and company://
	paths failed in its runs, until QElectroTech was started again.
*/
class tst_assistantinfopaths : public QObject
{
	Q_OBJECT

	static QString writtenPath(const QString &key)
	{
		QFile file(AssistantInfo::path());
		if (!file.open(QIODevice::ReadOnly))
			return QString();
		const QJsonObject folders = QJsonDocument::fromJson(file.readAll())
				.object().value(QStringLiteral("folders")).toObject();
		return QDir::cleanPath(folders.value(key).toString());
	}

	static void choose(GeneralConfigurationPage &page, const QString &box, const QString &path)
	{
		auto *combo = page.findChild<QComboBox *>(box);
		QVERIFY(combo);
			//The state the page itself leaves once a folder is picked;
			//signals blocked, or index 1 opens the folder dialog
		const QSignalBlocker blocker(combo);
		combo->setCurrentIndex(1);
		combo->setItemData(1, path, Qt::DisplayRole);
	}

private slots:
	void companyCollectionChosenInTheDialogIsWritten()
	{
		QTemporaryDir company;
		QVERIFY(company.isValid());

		GeneralConfigurationPage page;
		choose(page, QStringLiteral("m_company_elmt_path_cb"), company.path());
		page.applyConf();

		QCOMPARE(writtenPath(QStringLiteral("elements_company")),
				 QDir::cleanPath(company.path()));
	}

	void customCollectionChosenInTheDialogIsWritten()
	{
		QTemporaryDir custom;
		QVERIFY(custom.isValid());

		GeneralConfigurationPage page;
		choose(page, QStringLiteral("m_custom_elmt_path_cb"), custom.path());
		page.applyConf();

		QCOMPARE(writtenPath(QStringLiteral("elements_custom")),
				 QDir::cleanPath(custom.path()));
	}
};

QET_APP_TEST_MAIN(tst_assistantinfopaths)

#include "tst_assistantinfopaths.moc"
