/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef QET_ARGUMENTS_H
#define QET_ARGUMENTS_H
#include <QtCore>
/**
	This class represents a set of arguments the application has received as
	parameters. Initialized from a list of strings, an instance of this class
	provides access to the differents options and files given on the command line.
*/
class QETArguments : public QObject {
	Q_OBJECT
	
	// constructors, destructor
	public:
	QETArguments(QObject * = 0);
	QETArguments(const QList<QString> &, QObject * = 0);
	QETArguments(const QETArguments &);
	QETArguments &operator=(const QETArguments &);
	virtual ~QETArguments();
	
	// methods
	public:
	virtual void setArguments(const QList<QString> &);
	virtual QList<QString> arguments() const;
	virtual QList<QString> files() const;
	virtual QList<QString> projectFiles() const;
	virtual QList<QString> elementFiles() const;
	virtual QList<QString> titleBlockTemplateFiles() const;
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
	virtual bool commonElementsDirSpecified() const;
	virtual QString commonElementsDir() const;
#endif
#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
	virtual bool commonTitleBlockTemplatesDirSpecified() const;
	virtual QString commonTitleBlockTemplatesDir() const;
#endif
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
	virtual bool configDirSpecified() const;
	virtual QString configDir() const;
#endif
	virtual bool langDirSpecified() const;
	virtual QString langDir() const;
	virtual bool printHelpRequested() const;
	virtual bool printLicenseRequested() const;
	virtual bool printVersionRequested() const;
	virtual QList<QString> options() const;
	virtual QList<QString> unknownOptions() const;
	
	private:
	void clear();
	void parseArguments(const QList<QString> &);
	void handleFileArgument(const QString &);
	void handleOptionArgument(const QString &);
	
	// attributes
	private:
	QList<QString> project_files_;
	QList<QString> element_files_;
	QList<QString> tbt_files_;
	QList<QString> options_;
	QList<QString> unknown_options_;
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
	QString common_elements_dir_;
#endif
#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
	QString common_tbt_dir_;
#endif
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
	QString config_dir_;
#endif
	QString lang_dir_;
	bool print_help_;
	bool print_license_;
	bool print_version_;
};
#endif
