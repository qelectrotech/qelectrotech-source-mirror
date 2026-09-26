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
#include "connexionbackend.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

#include <cstring>

namespace {
		//The part of 3DconnexionClient's interface QET uses, with the
		//values Blender uses (intern/ghost/intern/GHOST_NDOFManagerCocoa.mm).
	constexpr quint32 MSG_DEVICE_STATE = 0x33645352;	// '3dSR'
	constexpr quint16 CMD_HANDLE_BUTTONS = 2;
	constexpr quint16 CMD_HANDLE_AXIS = 3;
	constexpr quint16 MODE_TAKE_OVER = 1;
	constexpr quint32 MASK_ALL = 0x3fff;
	constexpr quint32 MASK_ALL_BUTTONS = 0xffffffff;
	constexpr quint32 SIGNATURE = 0x51456c54;		// 'QElT'

		//ConnexionDeviceState is packed to 2 bytes. Offsets of the fields read.
	constexpr int STATE_CLIENT = 2;
	constexpr int STATE_COMMAND = 4;
	constexpr int STATE_AXIS = 30;
	constexpr int STATE_BUTTONS = 44;

	using MessageHandler = void (*)(quint32, quint32, void *);
	using DeviceHandler = void (*)(quint32);
	using SetConnexionHandlers = qint16 (*)(MessageHandler, DeviceHandler, DeviceHandler, bool);
	using RegisterConnexionClient = quint16 (*)(quint32, const quint8 *, quint16, quint32);
	using SetConnexionClientButtonMask = void (*)(quint16, quint32);
	using CleanupConnexionHandlers = void (*)();
	using UnregisterConnexionClient = void (*)(quint16);

		//3DxWare's callbacks carry no context, so the one client lives here.
		//The mutex is shared with 3DxWare's thread.
	QMutex instance_mutex;
	ConnexionBackend *instance = nullptr;

	void deviceChanged(quint32) {}
}

const char ConnexionBackend::DEFAULT_LIBRARY[] =
	"/Library/Frameworks/3DconnexionClient.framework/3DconnexionClient";

/**
	@brief ConnexionBackend::ConnexionBackend
	Load 3DxWare's client library and register with it. Not installed, or
	installed but not running, leaves isAvailable() false: the ordinary
	case, never reported as an error.
	@param parent
	@param library : the client library; only tests pass another
*/
ConnexionBackend::ConnexionBackend(QObject *parent, const QString &library) :
	SpaceMouseBackend(parent),
	m_library(library)
{
	{
		QMutexLocker lock(&instance_mutex);
		if (instance) {
			return;		//3DxWare takes one set of handlers per process
		}
	}
	if (!m_library.load()) {
		return;
	}

	const auto set_handlers = reinterpret_cast<SetConnexionHandlers>(
		m_library.resolve("SetConnexionHandlers"));
	const auto register_client = reinterpret_cast<RegisterConnexionClient>(
		m_library.resolve("RegisterConnexionClient"));
	const auto button_mask = reinterpret_cast<SetConnexionClientButtonMask>(
		m_library.resolve("SetConnexionClientButtonMask"));
	m_cleanup = reinterpret_cast<CleanupConnexionHandlers>(
		m_library.resolve("CleanupConnexionHandlers"));
	m_unregister = reinterpret_cast<UnregisterConnexionClient>(
		m_library.resolve("UnregisterConnexionClient"));
	if (!set_handlers || !register_client || !m_cleanup || !m_unregister) {
		return;
	}

	{
		QMutexLocker lock(&instance_mutex);
		instance = this;
	}
		//Fails while 3DxWare is installed but its driver is not running.
	if (set_handlers(&ConnexionBackend::onMessage, deviceChanged, deviceChanged, true) != 0) {
		shutdown();
		return;
	}
	m_handlers_installed = true;

		//3DxWare only sends a client its messages while that application
		//is in front, and recognises it by its executable's name (a Pascal
		//string). Take-over mode stops 3DxWare's own actions in QET, so the
		//view never moves twice.
	QByteArray name = QFileInfo(QCoreApplication::applicationFilePath())
				  .fileName().toUtf8().left(255);
	name.prepend(char(name.size()));
	m_client = register_client(SIGNATURE,
				   reinterpret_cast<const quint8 *>(name.constData()),
				   MODE_TAKE_OVER, MASK_ALL);
	if (!m_client) {
		shutdown();
		return;
	}
	if (button_mask) {
		button_mask(m_client, MASK_ALL_BUTTONS);
	}
}

/**
	@brief ConnexionBackend::~ConnexionBackend
*/
ConnexionBackend::~ConnexionBackend()
{
	shutdown();
}

/**
	@brief ConnexionBackend::sampleFromAxes
	3DxWare reports y up and z away from the user; the raw USB reports, and
	so every other backend, have y towards the user and z down. Derived from
	Blender, whose 3DxWare and spacenavd code paths must agree.
	@param axis : TX, TY, TZ, RX, RY, RZ as 3DxWare sends them
	@return the same movement in QET's convention
*/
SpaceMouseSample ConnexionBackend::sampleFromAxes(const qint16 axis[6])
{
	SpaceMouseSample sample;
	sample.x = axis[0];
	sample.y = -axis[2];
	sample.z = -axis[1];
	sample.rx = axis[3];
	sample.ry = -axis[5];
	sample.rz = -axis[4];
	return sample;
}

/**
	@brief ConnexionBackend::newlyPressed
	@param before : the button bitmask of the previous message
	@param now : the button bitmask of this one
	@return the 0-based buttons pressed since \a before, as HidBackend numbers them
*/
QList<int> ConnexionBackend::newlyPressed(quint32 before, quint32 now)
{
	QList<int> pressed;
	const quint32 down = now & ~before;
	for (int bit = 0; bit < 32; ++bit) {
		if (down & (quint32(1) << bit)) {
			pressed.append(bit);
		}
	}
	return pressed;
}

/**
	@brief ConnexionBackend::onMessage
	Runs on 3DxWare's thread. Copies what matters out of the message and
	hands it to the main thread.
	@param type : the message type
	@param argument : a ConnexionDeviceState for MSG_DEVICE_STATE
*/
void ConnexionBackend::onMessage(quint32, quint32 type, void *argument)
{
	if (type != MSG_DEVICE_STATE || !argument) {
		return;
	}
	const char *state = static_cast<const char *>(argument);
	quint16 client;
	quint16 command;
	qint16 axis[6];
	quint32 buttons;
	std::memcpy(&client, state + STATE_CLIENT, sizeof client);
	std::memcpy(&command, state + STATE_COMMAND, sizeof command);
	std::memcpy(axis, state + STATE_AXIS, sizeof axis);
	std::memcpy(&buttons, state + STATE_BUTTONS, sizeof buttons);

		//Queued with the backend as context: if it is deleted before the
		//main thread gets to it, Qt drops the call.
	QMutexLocker lock(&instance_mutex);
	ConnexionBackend *backend = instance;
	if (!backend) {
		return;
	}
	QMetaObject::invokeMethod(backend, [=]() {
		backend->handleState(client, command, axis, buttons);
	}, Qt::QueuedConnection);
}

/**
	@brief ConnexionBackend::handleState
	On the main thread: turn one device state into signals.
*/
void ConnexionBackend::handleState(quint16 client, quint16 command,
				   const qint16 axis[6], quint32 buttons)
{
	if (client != m_client) {
		return;		//3DxWare sends every state to every client
	}
	if (command == CMD_HANDLE_AXIS) {
		emit motion(sampleFromAxes(axis));
	} else if (command == CMD_HANDLE_BUTTONS) {
			//State first: a button can open a dialog whose event loop
			//delivers the next message before emit returns.
		const QList<int> pressed = newlyPressed(m_buttons, buttons);
		m_buttons = buttons;
		for (int button : pressed) {
			emit buttonPressed(button);
		}
	}
}

/**
	@brief ConnexionBackend::shutdown
	Unregister and stop 3DxWare's callbacks. The library stays loaded, since
	3DxWare's thread may still be returning from one.
*/
void ConnexionBackend::shutdown()
{
	if (m_client && m_unregister) {
		m_unregister(m_client);
	}
	m_client = 0;
	if (m_handlers_installed && m_cleanup) {
		m_cleanup();
	}
	m_handlers_installed = false;

	QMutexLocker lock(&instance_mutex);
	if (instance == this) {
		instance = nullptr;
	}
}
