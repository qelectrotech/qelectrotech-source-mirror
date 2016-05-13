/*
	Copyright 2006-2016 The QElectroTech Team
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTAvBILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "qetsingleapplication.h"
#include <QLocalSocket>

const int QETSingleApplication::timeout_ = 10000;

/**
	Constructeur
	@param argc Nombre d'arguments passes au programme par le systeme
	@param argv Tableau des arguments passes au programme par le systeme
	@param unique_key Cle unique
*/
QETSingleApplication::QETSingleApplication(int &argc, char **argv, const QString unique_key) :
	QApplication(argc, argv),
	unique_key_(unique_key)
{
	// verifie s'il y a un segment de memoire partage correspondant a la cle unique
#if defined (Q_OS_OS2)
#define QT_NO_SHAREDMEMORY
	{
#else
	shared_memory_.setKey(unique_key_);
	if (shared_memory_.attach()) {
		// oui : l'application est deja en cours d'execution
		is_running_ = true;
	} else {
		// non : il s'agit du premier demarrage de l'application pour cette cle unique
		is_running_ = false;
		
		// initialisation du segment de memoire partage
		if (!shared_memory_.create(1)) {
			qDebug() << "QETSingleApplication::QETSingleApplication() : Impossible de créer l'instance unique" << qPrintable(unique_key_);
			return;
		}
#endif
		// initialisation d'un serveur local pour recevoir les messages des autres instances
		local_server_ = new QLocalServer(this);
		connect(local_server_, SIGNAL(newConnection()), this, SLOT(receiveMessage()));
		// la cle unique est egalement utilise pour le serveur
		local_server_ -> listen(unique_key_);
	}
}

/**
	Destructeur
*/
QETSingleApplication::~QETSingleApplication() {
}

/**
	Slot gerant la reception des messages.
	Lorsque l'application recoit un message, ce slot emet le signal
	messageAvailable avec le message recu.
*/
void QETSingleApplication::receiveMessage() {
	QLocalSocket *local_socket = local_server_ -> nextPendingConnection();
	if (!local_socket -> waitForReadyRead(timeout_)) {
		qDebug() << "QETSingleApplication::receiveMessage() :" << qPrintable(local_socket -> errorString()) << "(" << qPrintable(unique_key_) << ")";
		return;
	}
	QByteArray byteArray = local_socket -> readAll();
	QString message = QString::fromUtf8(byteArray.constData());
	emit(messageAvailable(message));
	local_socket -> disconnectFromServer();
}

/**
	@return true si l'application est deja en cours d'execution
*/
bool QETSingleApplication::isRunning() {
	return(is_running_);
}

/**
	Envoie un message a l'application. Si celle-ci n'est pas en cours
	d'execution, cette methode ne fait rien.
	@param message Message a transmettre a l'application
	@return true si le message a ete tranmis, false sinon
*/
bool QETSingleApplication::sendMessage(const QString &message) {
	// l'application doit etre en cours d'execution
	if (!is_running_) {
		return(false);
	}
	
	// se connecte a l'application, avec gestion du timeout
	QLocalSocket local_socket(this);
	local_socket.connectToServer(unique_key_, QIODevice::WriteOnly);
	if (!local_socket.waitForConnected(timeout_)) {
		qDebug() << "QETSingleApplication::sendMessage() :" << qPrintable(local_socket.errorString()) << "(" << qPrintable(unique_key_) << ")";
		return(false);
	}
	
	// envoi du message, avec gestion du timeout
	local_socket.write(message.toUtf8());
	if (!local_socket.waitForBytesWritten(timeout_)) {
		qDebug() << "QETSingleApplication::sendMessage() :" << qPrintable(local_socket.errorString()) << "(" << qPrintable(unique_key_) << ")";
		return(false);
	}
	
	// deconnexion
	local_socket.disconnectFromServer();
	return(true);
}
