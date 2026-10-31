// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef PROJECTLANGUAGES_H
#define PROJECTLANGUAGES_H

#include <QDomElement>
#include <QMap>
#include <QString>
#include <QStringList>

/**
	@brief The ProjectLanguages class
	The texts of a project in other languages. A text is found by its
	source text (the words as drawn on the folio); each language adds the
	words to show in its place. Nothing on the folio is changed: the
	active language only changes what is displayed.
*/
class ProjectLanguages
{
	public:
		struct CsvResult {
			bool ok = false;
			int imported = 0;
			QString error;
			QStringList warnings;
		};

		bool isEmpty() const;
		bool operator==(const ProjectLanguages &other) const;
		bool operator!=(const ProjectLanguages &other) const {return !(*this == other);}

		QString sourceLanguage() const {return m_source;}
		void setSourceLanguage(const QString &code);
		QString activeLanguage() const;
		void setActiveLanguage(const QString &code);
		QStringList languages() const;
		bool addLanguage(const QString &code);
		void removeLanguage(const QString &code);

		QStringList phrases() const;
		void addPhrase(const QString &source);
		QString translation(const QString &source, const QString &language) const;
		void setTranslation(const QString &source, const QString &language, const QString &text);
		QString translate(const QString &source) const;

		QDomElement toXml(QDomDocument &document) const;
		static ProjectLanguages fromXml(const QDomElement &element);

		QByteArray exportCsv() const;
		CsvResult importCsv(const QByteArray &bytes);

		static bool validCode(const QString &code);
		static bool isTranslatable(const QString &text);
		static QStringList placeholders(const QString &text);

	private:
		QString m_source = QStringLiteral("en");
		QString m_active;
		QStringList m_languages;
			/// source text -> language -> text; a phrase may have no language yet
		QMap<QString, QMap<QString, QString>> m_phrases;
};

#endif // PROJECTLANGUAGES_H
