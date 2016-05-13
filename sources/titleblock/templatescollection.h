/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef TITLEBLOCK_SLASH_TEMPLATES_COLLECTION_H
#define TITLEBLOCK_SLASH_TEMPLATES_COLLECTION_H
#define TITLEBLOCKS_FILE_EXTENSION ".titleblock"
#include <QtCore>
#include <QtXml>
#include "templatelocation.h"
#include "qet.h"
class TitleBlockTemplate;
class QETProject;

/**
	This abstract class represents a generic collection of title block templates.
	Unlike elements collections, title block templates collections do not provide
	a tree structure. Instead, they provide a simple, flat list of available
	templates.
*/
class TitleBlockTemplatesCollection : public QObject {
	Q_OBJECT
	
	// Constructors, destructor
	public:
	TitleBlockTemplatesCollection(QObject *parent);
	virtual ~TitleBlockTemplatesCollection();
	private:
	TitleBlockTemplatesCollection(const TitleBlockTemplatesCollection &);
	
	// methods
	public:
	virtual QStringList templates() = 0;
	virtual TitleBlockTemplate *getTemplate(const QString &) = 0;
	virtual QDomElement getTemplateXmlDescription(const QString &) = 0;
	virtual bool setTemplateXmlDescription(const QString &, const QDomElement &) = 0;
	virtual void removeTemplate(const QString &) = 0;
	virtual TitleBlockTemplateLocation location(const QString & = QString()) = 0;
	virtual bool hasFilePath() = 0;
	virtual QString filePath() = 0;
	virtual bool isReadOnly(const QString & = QString()) const = 0;
	virtual QString title() const;
	virtual void setTitle(const QString &);
	virtual QString protocol() const;
	virtual void setProtocol(const QString &);
	virtual QET::QetCollection collection () const;
	virtual void setCollection (QET::QetCollection);
	virtual QETProject *parentProject();
	virtual QList<TitleBlockTemplateLocation> templatesLocations();
	
	signals:
	void changed(TitleBlockTemplatesCollection *, const QString & = QString());
	void aboutToRemove(TitleBlockTemplatesCollection *, const QString &);
	
	// attributes
	protected:
	/// Human-readable title for this collection
	QString title_;
	/// Protocol used to designate this collection
	QString protocol_;
	QET::QetCollection m_collection;
	/// Already parsed embedded titleblock templates
	QHash<QString, TitleBlockTemplate *> titleblock_templates_;
};

/**
	This class represents a simple abastraction layer for a collection of title
	block templates embedded within a project.
*/
class TitleBlockTemplatesProjectCollection : public TitleBlockTemplatesCollection {
	Q_OBJECT
	
	// Constructors, destructor
	public:
	TitleBlockTemplatesProjectCollection(QETProject *, QObject *parent = 0);
	virtual ~TitleBlockTemplatesProjectCollection();
	private:
	TitleBlockTemplatesProjectCollection(const TitleBlockTemplatesProjectCollection &);
	
	// methods
	public:
	virtual QString title() const;
	virtual QString protocol() const;
	virtual QETProject *parentProject();
	virtual QStringList templates();
	virtual TitleBlockTemplate *getTemplate(const QString &);
	virtual QDomElement getTemplateXmlDescription(const QString &);
	virtual bool setTemplateXmlDescription(const QString &, const QDomElement &);
	virtual void removeTemplate(const QString &);
	virtual TitleBlockTemplateLocation location(const QString & = QString());
	virtual bool hasFilePath();
	virtual QString filePath();
	virtual bool isReadOnly(const QString & = QString()) const;
	virtual void fromXml(const QDomElement &);
	virtual void deleteUnusedTitleBlocKTemplates();
	
	// attributes
	private:
	/// Parent project
	QETProject *project_;
	/// Parent QDomDocument for stored QDomElements
	QDomDocument xml_document_;
	/// XML descriptions of embedded titleblock templates
	QHash<QString, QDomElement> titleblock_templates_xml_;
};

/**
	This classe represents a simple abastraction layer for a file-based title
	block templates directory. 
*/
class TitleBlockTemplatesFilesCollection : public TitleBlockTemplatesCollection {
	Q_OBJECT
	
	// Constructors, destructor
	public:
	TitleBlockTemplatesFilesCollection(const QString &, QObject * = 0);
	virtual ~TitleBlockTemplatesFilesCollection();
	private:
	TitleBlockTemplatesFilesCollection(const TitleBlockTemplatesFilesCollection &);
	
	// methods
	public:
	QString path(const QString & = QString()) const;
	virtual QStringList templates();
	virtual TitleBlockTemplate *getTemplate(const QString &);
	virtual QDomElement getTemplateXmlDescription(const QString &);
	virtual bool setTemplateXmlDescription(const QString &, const QDomElement &);
	virtual void removeTemplate(const QString &);
	virtual TitleBlockTemplateLocation location(const QString & = QString());
	virtual bool hasFilePath();
	virtual QString filePath();
	virtual bool isReadOnly(const QString & = QString()) const;
	
	static QString toTemplateName(const QString &);
	static QString toFileName(const QString &);
	
	private slots:
	void fileSystemChanged(const QString &str);
	
	// attributes
	private:
	/// File System Watcher object to track the files changes made outside the application
	QFileSystemWatcher watcher_;
	/// Collection real directory
	QDir dir_;
};
#endif
