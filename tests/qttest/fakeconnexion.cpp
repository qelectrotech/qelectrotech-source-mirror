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
/*
	A stand-in for 3DxWare's 3DconnexionClient.framework, for
	tst_spacemouseconnexion: it exports the calls ConnexionBackend makes,
	plus fake*() controls the test uses to script 3DxWare's side. Like the
	real driver, it delivers messages on a thread of its own.
*/
#include <QtGlobal>

#include <cstdint>
#include <cstring>
#include <string>
#include <thread>

#define FAKE_EXPORT extern "C" Q_DECL_EXPORT

namespace {
	using MessageHandler = void (*)(uint32_t, uint32_t, void *);
	using DeviceHandler = void (*)(uint32_t);

	MessageHandler handler = nullptr;
	int16_t handlers_result = 0;
	uint16_t client_result = 7;
	std::string registered_name;
	uint32_t registered_mode = 0;
	uint32_t registered_mask = 0;
	uint32_t registered_buttons = 0;
	int unregisters = 0;
	int cleanups = 0;
}

FAKE_EXPORT int16_t SetConnexionHandlers(MessageHandler message, DeviceHandler, DeviceHandler, bool)
{
	if (handlers_result == 0) {
		handler = message;
	}
	return handlers_result;
}

FAKE_EXPORT void CleanupConnexionHandlers()
{
	handler = nullptr;
	++cleanups;
}

FAKE_EXPORT uint16_t RegisterConnexionClient(uint32_t, const uint8_t *name, uint16_t mode, uint32_t mask)
{
	registered_name.assign(reinterpret_cast<const char *>(name) + 1, name[0]);
	registered_mode = mode;
	registered_mask = mask;
	return client_result;
}

FAKE_EXPORT void SetConnexionClientButtonMask(uint16_t, uint32_t mask)
{
	registered_buttons = mask;
}

FAKE_EXPORT void UnregisterConnexionClient(uint16_t)
{
	++unregisters;
}

/// Reset, and set what SetConnexionHandlers and RegisterConnexionClient return.
FAKE_EXPORT void fakeReset(int16_t handlers, uint16_t client)
{
	handler = nullptr;
	handlers_result = handlers;
	client_result = client;
	registered_name.clear();
	registered_mode = registered_mask = registered_buttons = 0;
	unregisters = cleanups = 0;
}

/// What the backend registered with: name, mode, mask, button mask, and
/// how many times it unregistered and cleaned up.
FAKE_EXPORT const char *fakeRegistration(uint32_t *mode, uint32_t *mask, uint32_t *buttons,
					 int *unregistered, int *cleaned)
{
	*mode = registered_mode;
	*mask = registered_mask;
	*buttons = registered_buttons;
	*unregistered = unregisters;
	*cleaned = cleanups;
	return registered_name.c_str();
}

/// Send one ConnexionDeviceState (packed to 2 bytes, 48 bytes long) from
/// another thread, as 3DxWare does.
FAKE_EXPORT void fakeSend(uint16_t client, uint16_t command, const int16_t axis[6], uint32_t buttons)
{
	unsigned char state[48] = {};
	std::memcpy(state + 2, &client, 2);
	std::memcpy(state + 4, &command, 2);
	std::memcpy(state + 30, axis, 12);
	std::memcpy(state + 44, &buttons, 4);
	std::thread driver([&state]() {
		if (handler) {
			handler(0, 0x33645352, state);	// '3dSR'
		}
	});
	driver.join();
}
