/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef QET_SINGLE_APPLICATION_H
#define QET_SINGLE_APPLICATION_H
#include <QApplication>
#include <QSharedMemory>
#include <QLocalServer>
/**
	This class represents a Qt Application executing only a single instance
	depending on a unique string key.
*/
class QETSingleApplication : public QApplication {
	Q_OBJECT
	// constructors, destructor
	public:
	QETSingleApplication(int &, char **, const QString);
	virtual ~QETSingleApplication();
	
	private:
	QETSingleApplication(const QETSingleApplication &);
	
	// methods
	public:
	bool isRunning();
	bool sendMessage(const QString &);
	
	public slots:
	void receiveMessage();
	
	signals:
	void messageAvailable(QString);
	
	// attributes
	private:
	bool is_running_;
	QString unique_key_;
	QSharedMemory shared_memory_;
	QLocalServer *local_server_;
	static const int timeout_;
};
#endif
