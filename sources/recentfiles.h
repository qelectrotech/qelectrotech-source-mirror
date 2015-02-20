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
#ifndef RECENT_FILES_H
#define RECENT_FILES_H
#include <QtCore>
#include <QIcon>
class QMenu;
/**
	This class provides a way to manage recently opened files.
*/
class RecentFiles : public QObject {
	Q_OBJECT
	
	// constructors, destructor
	public:
	RecentFiles(const QString &, int = 10, QObject * = 0);
	virtual ~RecentFiles();
	private:
	RecentFiles(const RecentFiles &);
	
	// methods
	public:
	int size() const;
	QMenu *menu() const;
	QIcon iconForFiles() const;
	void setIconForFiles(const QIcon &);
	
	public slots:
	void clear();
	void save();
	void fileWasOpened(const QString &);
	
	signals:
	void fileOpeningRequested(const QString &);
	
	private:
	void extractFilesFromSettings();
	void insertFile(const QString &);
	void saveFilesToSettings();
	void buildMenu();
	
	private slots:
	void handleMenuRequest(const QString &);
	
	// attributes
	private:
	QString identifier_;
	int size_;
	QList<QString> list_;
	QMenu *menu_;
	QSignalMapper *mapper_;
	QIcon files_icon_;
};
#endif
