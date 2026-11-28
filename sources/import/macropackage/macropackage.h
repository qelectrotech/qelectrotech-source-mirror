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
#ifndef MACROPACKAGE_H
#define MACROPACKAGE_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

/// One article of a manufacturer's macro package.
struct MacroPackageArticle
{
	QString part_number;    ///< "BEC.EK9300"
	QString order_number;   ///< "EK9300"
	QString type_number;    ///< "EK9300"
	QString manufacturer;   ///< EPLAN's short code, "BEC"
	QString description;    ///< German where the package has it, else English
	QString macro;          ///< the macro's file in the package, '/'-separated; empty: none
	QString category;       ///< folder of the macro, without the manufacturer's name
	QString source;         ///< the article XML the part is read from
	qint64 source_size = 0;

	/// what the article is called: its type number, else its order number
	QString type() const { return type_number.isEmpty() ? order_number : type_number; }
};

/**
	@brief The macro package of a manufacturer: all its EPLAN macros in one
	download, as Beckhoff offers it ("EPLAN Makros ab 2.7") instead of
	single parts in the EPLAN Data Portal.

	A package is a ZIP file or its unpacked folder with
	- window macros (*.ema, the "EplanPxfRoot" XML that an .edz carries too),
	  sorted into folders by product family, and
	- article data as EPLAN parts management XML ("partsmanagement" with one
	  <part> per article, the format of the part.xml of an .edz): one file
	  for all articles and/or one beside each macro. A part names its macro
	  in P_ARTICLE_GROUPSYMBOLMACRO ("$(MD_MACROS)\<path in the package>");
	  several articles may share one macro.

	The package is only indexed when opened (the articles, the names of the
	files); a macro and the data of a part are read when they are asked for.
	A ZIP is never unpacked to disk. Its list of articles is kept in the
	cache folder and read from there as long as the ZIP is the same file
	(size and date), so opening it again takes no time and the archive is
	only read once a macro is wanted.
*/
class MacroPackage
{
	public:
		MacroPackage();
		~MacroPackage();
		MacroPackage(const MacroPackage &) = delete;
		MacroPackage &operator=(const MacroPackage &) = delete;

		bool open(const QString &path);
		QString path() const { return m_path; }
		QString errorString() const { return m_error; }

		const QVector<MacroPackageArticle> &articles() const { return m_articles; }
		int macroCount() const { return m_macro_count; }
		/// display name of the manufacturer of @p article ("Beckhoff")
		QString manufacturerName(const MacroPackageArticle &article) const;
		/// the articles whose type, order number, description or category
		/// hold every word of @p text; all articles for an empty text
		QVector<MacroPackageArticle> find(const QString &text) const;
		/// the article of that type or order number (case and blanks do
		/// not count); nullptr when the package has none
		const MacroPackageArticle *article(const QString &type) const;

		/// the macro of @p article; empty when it has none
		QByteArray macro(const MacroPackageArticle &article);
		/// a parts management document with this article alone
		QByteArray partXml(const MacroPackageArticle &article);

	private:
		struct Zip;
		QByteArray read(const QString &entry);
		bool loadArchive();
		bool buildIndex();
		QString indexCacheFile() const;
		bool readIndexCache();
		void writeIndexCache() const;
		void indexParts(const QString &entry, const QByteArray &xml);

		QString m_path;
		QString m_error;
		QString m_top_folder;                 ///< the one folder all files lie in, if so
		Zip *m_zip = nullptr;                 ///< a ZIP package
		QHash<QString, QString> m_entries;    ///< lower-case path -> path in the package
		QVector<MacroPackageArticle> m_articles;
		QHash<QString, int> m_by_number;      ///< part number -> index
		int m_macro_count = 0;
		bool m_loaded = false;                ///< the files of the package are listed
};

#endif // MACROPACKAGE_H
