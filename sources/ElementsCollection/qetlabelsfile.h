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
#ifndef QETLABELSFILE_H
#define QETLABELSFILE_H

#include <QCoreApplication>
#include <QDomDocument>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

class QIODevice;

/**
	@brief The QetLabelsFile class
	
	Represents the qet_labels.xml file of an elements collection : the file
	that maps every directory of the collection to the prefix (the ISO/IEC
	81346 label) given to the elements it contains.
	
	Two kind of users use this class :
	- the prefix lookup performed when an element is placed in a project
	  (prefixForPath(), see also autonum::elementPrefixForLocation())
	- the "Configurer les préfixes…" editor, which loads a file (creating a
	  skeleton in memory when there is none yet), shows the folder tree of
	  the collection next to the stored prefixes, and writes everything back
	  in one go when the user validates.
	
	Nothing is written to the disk before save() is called, so cancelling the
	editor really leaves the collection untouched.
*/
class QetLabelsFile
{
	Q_DECLARE_TR_FUNCTIONS(QetLabelsFile)

	public:
		QetLabelsFile() = default;
		
		// methods
		public:
			static QString labelsFilePath(const QString &collection_dir);
			static QList<QStringList> scanFolders(const QString &collection_dir);
			static QString prefixForPath(const QString &filepath, const QStringList &path, int dirLevel);
			static QString prefixInDocument(const QDomDocument &document, const QStringList &path, int dirLevel);
			
			bool load(const QString &collection_dir);
			QString prefix(const QStringList &relative_path) const;
			bool hasPrefix(const QStringList &relative_path) const;
			QStringList orphanPaths(const QList<QStringList> &folders) const;
			void ensureStructure(const QList<QStringList> &folders);
			void setPrefix(const QStringList &relative_path, const QString &prefix);
			int removeOrphans(const QList<QStringList> &folders);
			bool save();
			
			QString filePath() const {return m_file_path;}
			QString backupPath() const {return m_backup_path;}
			QString errorString() const {return m_error;}
				///true when the existing file was found unusable and an
				///empty document is used instead. Its copy is only made by
				///save(), right before the file is replaced, so repairing
				///the file instead of rebuilding it leaves no copy behind
			bool isBroken() const {return m_broken;}
				///what exactly is wrong with that file (line and column of
				///the syntax error for instance), so the caller can tell the
				///user how to repair it instead of silently rebuilding it
			QString brokenReason() const {return m_broken_reason;}
		
		private:
			static bool parse(QIODevice &device, QDomDocument &document, QString *reason);
			QDomElement categoryForPath(const QStringList &relative_path, bool create);
			void createEmptyDocument();
			QString backupBrokenFile();
			static void scanRec(const QString &path, const QStringList &relative_path, QList<QStringList> &out);
			static QDomElement directChildCategory(const QDomElement &parent, const QString &name);
			static void collectOrphans(const QDomElement &parent, const QString &relative_path, const QSet<QString> &folders_key, QList<QDomElement> &orphans);
			static QString serialize(const QDomDocument &document);
		
		// attributes
		private:
			QString m_file_path;
			QString m_backup_path;
			QString m_error;
			QString m_broken_reason;
			QString m_snapshot;
			bool m_file_exists = false;
			bool m_force_save = false;
			bool m_broken = false;
			QDomDocument m_document;
};

#endif // QETLABELSFILE_H
