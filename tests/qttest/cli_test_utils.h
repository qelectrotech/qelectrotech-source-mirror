#ifndef CLI_TEST_UTILS_H
#define CLI_TEST_UTILS_H

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSet>
#include <QStringList>

namespace CliTestUtils {

struct CliResult {
	int exit_code = -1;
	QString stdout_text;
	QString stderr_text;
};

inline CliResult runQetCli(const QStringList &arguments, int timeout_ms = 30000)
{
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));

	QProcess proc;
	proc.setProcessEnvironment(env);
	proc.start(QStringLiteral(QET_TEST_BINARY_PATH), arguments);
	if (!proc.waitForFinished(timeout_ms)) {
		proc.kill();
		proc.waitForFinished();
		return {-1, QString::fromUtf8(proc.readAllStandardOutput()),
				QStringLiteral("timed out or failed to start: ")
					+ QString::fromUtf8(proc.readAllStandardError())};
	}

	return {proc.exitCode(),
			QString::fromUtf8(proc.readAllStandardOutput()),
			QString::fromUtf8(proc.readAllStandardError())};
}

inline QByteArray readFile(const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return {};
	return file.readAll();
}

inline QJsonObject readJsonObject(const QString &path)
{
	const QJsonDocument doc = QJsonDocument::fromJson(readFile(path));
	return doc.object();
}

inline QList<QStringList> parseSemicolonCsv(QString text)
{
	if (text.startsWith(QChar(0xfeff)))
		text.remove(0, 1);
	text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
	text.replace(QLatin1Char('\r'), QLatin1Char('\n'));

	QList<QStringList> rows;
	QStringList row;
	QString field;
	bool quoted = false;

	for (qsizetype i = 0; i < text.size(); ++i) {
		const QChar ch = text.at(i);
		if (quoted) {
			if (ch == QLatin1Char('"')) {
				if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('"')) {
					field += QLatin1Char('"');
					++i;
				} else {
					quoted = false;
				}
			} else {
				field += ch;
			}
			continue;
		}

		if (ch == QLatin1Char('"')) {
			quoted = true;
		} else if (ch == QLatin1Char(';')) {
			row << field;
			field.clear();
		} else if (ch == QLatin1Char('\n')) {
			row << field;
			field.clear();
			if (!row.isEmpty() && !(row.size() == 1 && row.first().isEmpty()))
				rows << row;
			row.clear();
		} else {
			field += ch;
		}
	}

	if (!field.isEmpty() || !row.isEmpty()) {
		row << field;
		if (!(row.size() == 1 && row.first().isEmpty()))
			rows << row;
	}

	return rows;
}

inline QSet<QString> columnSet(const QList<QStringList> &rows, const QString &header)
{
	QSet<QString> values;
	if (rows.isEmpty())
		return values;

	const int index = rows.first().indexOf(header);
	if (index < 0)
		return values;

	for (int i = 1; i < rows.size(); ++i) {
		if (index < rows.at(i).size())
			values.insert(rows.at(i).at(index));
	}
	return values;
}

} // namespace CliTestUtils

#endif // CLI_TEST_UTILS_H
