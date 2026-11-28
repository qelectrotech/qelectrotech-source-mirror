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
#ifndef AASXPACKAGE_H
#define AASXPACKAGE_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

/// one leaf of a submodel: a Property, MultiLanguageProperty, Range or File
struct AasxElement
{
	QString path;         ///< idShorts from the submodel down, '/'-separated
	QString id_short;
	QString kind;         ///< Property, MultiLanguageProperty, Range, File, Blob
	QString value;        ///< a File: its path in the package (/aasx/...)
	QString semantic_id;  ///< first key of semanticId, e.g. 0173-1#02-AAO677#002
	QString content_type; ///< of a File
};

struct AasxSubmodel
{
	QString id;
	QString id_short;     ///< e.g. Nameplate, TechnicalData, HandoverDocumentation
	QString semantic_id;
	QVector<AasxElement> elements;
};

/**
	@brief An Asset Administration Shell package (.aasx, IEC 63278 / IDTA).

	An AASX is an OPC package (ZIP): _rels/.rels names the aasx-origin,
	whose relationships name the AAS file (aas-spec) -- XML or JSON, AAS
	metamodel 2.0 or 3.0 -- and the supplementary files (datasheets,
	CAD models, pictures). Read here by element names, not by schema,
	so both versions and both serialisations give the same leaves.
	Manufacturers fill the submodel templates of the IDTA: Digital
	Nameplate (IDTA 02006), Technical Data (IDTA 02003), Handover
	Documentation (IDTA 02004), CAD (IDTA 02020 ...).
*/
class AasxPackage
{
	public:
		bool open(const QString &path);
		bool openData(const QByteArray &zip);
		QString errorString() const { return m_error; }

		QVector<AasxSubmodel> submodels() const { return m_submodels; }
		/// the AAS file inside the package, e.g. aasx/xml/content.xml
		QString specPath() const { return m_spec; }
		/// the files of the package that are neither AAS nor OPC bookkeeping
		QStringList files() const;
		/// a file of the package by its path ("/aasx/doc.pdf" or "aasx/doc.pdf")
		QByteArray file(const QString &path) const;

		/**
			The value of the first leaf named one of @p id_shorts, from the
			submodels whose idShort contains @p submodel first (e.g.
			"Nameplate"), then from all; empty when there is none.
		*/
		QString value(const QStringList &id_shorts, const QString &submodel = QString()) const;
		/// the File elements of all submodels (documents, CAD models, pictures)
		QVector<AasxElement> fileElements() const;

			/// the part as the IDTA templates name it
		QString manufacturer() const;
		QString orderNumber() const;
		QString typeNumber() const;
		QString designation() const;

	private:
		bool parseXml(const QByteArray &data);
		bool parseJson(const QByteArray &data);

		QHash<QString, QByteArray> m_entries;    ///< path without leading '/'
		QVector<AasxSubmodel> m_submodels;
		QString m_spec;
		QString m_error;
};

#endif // AASXPACKAGE_H
