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
	ConnexionBackend, the macOS backend that reads a 3D mouse through
	3DxWare. It is tested on every platform against fakeconnexion, which
	exports the calls 3DxWare's client library does and sends messages from
	its own thread as 3DxWare does. What only a Mac with 3DxWare can check
	-- that the real library behaves like the fake -- is not tested here.
*/
#include "spacemouse/connexionbackend.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QLibrary>
#include <QSignalSpy>
#include <QtTest>

#include <functional>

namespace {
	constexpr quint16 CLIENT = 7;
	constexpr quint16 AXIS = 3;
	constexpr quint16 BUTTONS = 2;

	using Reset = void (*)(qint16, quint16);
	using Registration = const char *(*)(quint32 *, quint32 *, quint32 *, int *, int *);
	using Send = void (*)(quint16, quint16, const qint16 *, quint32);
}

class TstSpaceMouseConnexion : public QObject
{
	Q_OBJECT

	private slots:
		void initTestCase();
		void init();
		void noLibrary();
		void driverNotRunning();
		void registration();
		void motion();
		void otherClients();
		void buttons();
		void deletedBeforeDelivery();
		void oneClientPerProcess();
		void realLibrary();

	private:
		QList<SpaceMouseSample> collect(ConnexionBackend &backend,
						const std::function<void()> &send);

		QLibrary m_fake{QStringLiteral(FAKE_CONNEXION)};
		Reset m_reset = nullptr;
		Registration m_registration = nullptr;
		Send m_send = nullptr;
};

/*
	A fake that fails to load fails every test that uses it, in init(),
	rather than here: realLibrary() does not use it, and CI runs it alone
	under code-signing settings that may refuse the fake too.
*/
void TstSpaceMouseConnexion::initTestCase()
{
	if (m_fake.load()) {
		m_reset = reinterpret_cast<Reset>(m_fake.resolve("fakeReset"));
		m_registration = reinterpret_cast<Registration>(m_fake.resolve("fakeRegistration"));
		m_send = reinterpret_cast<Send>(m_fake.resolve("fakeSend"));
	}
}

void TstSpaceMouseConnexion::init()
{
	if (qstrcmp(QTest::currentTestFunction(), "realLibrary") == 0) {
		return;
	}
	QVERIFY2(m_reset && m_registration && m_send, qPrintable(m_fake.errorString()));
	m_reset(0, CLIENT);
}

QList<SpaceMouseSample> TstSpaceMouseConnexion::collect(
		ConnexionBackend &backend, const std::function<void()> &send)
{
	QList<SpaceMouseSample> samples;
	auto c = connect(&backend, &SpaceMouseBackend::motion,
			 [&samples](const SpaceMouseSample &s) { samples.append(s); });
	send();
	QCoreApplication::processEvents();
	disconnect(c);
	return samples;
}

/// 3DxWare not installed: the ordinary case.
void TstSpaceMouseConnexion::noLibrary()
{
	ConnexionBackend backend(nullptr, QStringLiteral("/nonexistent/3DconnexionClient"));
	QVERIFY(!backend.isAvailable());
}

/// Installed but its driver is not running: SetConnexionHandlers fails, and
/// nothing is left registered for 3DxWare to call.
void TstSpaceMouseConnexion::driverNotRunning()
{
	m_reset(-36, CLIENT);
	{
		ConnexionBackend backend(nullptr, QStringLiteral(FAKE_CONNEXION));
		QVERIFY(!backend.isAvailable());
	}
	quint32 mode, mask, buttons;
	int unregistered, cleaned;
	m_registration(&mode, &mask, &buttons, &unregistered, &cleaned);
	QCOMPARE(unregistered, 0);
	QCOMPARE(cleaned, 0);

		//And a registration refused (client 0) leaves nothing either.
	m_reset(0, 0);
	{
		ConnexionBackend backend(nullptr, QStringLiteral(FAKE_CONNEXION));
		QVERIFY(!backend.isAvailable());
	}
	m_registration(&mode, &mask, &buttons, &unregistered, &cleaned);
	QCOMPARE(cleaned, 1);
}

/// Registers by executable name, in take-over mode, for everything, and
/// undoes it all on destruction.
void TstSpaceMouseConnexion::registration()
{
	quint32 mode, mask, buttons;
	int unregistered, cleaned;
	{
		ConnexionBackend backend(nullptr, QStringLiteral(FAKE_CONNEXION));
		QVERIFY(backend.isAvailable());
		const QString name = QString::fromUtf8(
			m_registration(&mode, &mask, &buttons, &unregistered, &cleaned));
		QCOMPARE(name, QFileInfo(QCoreApplication::applicationFilePath()).fileName());
		QCOMPARE(mode, 1u);
		QCOMPARE(mask, 0x3fffu);
		QCOMPARE(buttons, 0xffffffffu);
		QCOMPARE(unregistered, 0);
	}
	m_registration(&mode, &mask, &buttons, &unregistered, &cleaned);
	QCOMPARE(unregistered, 1);
	QCOMPARE(cleaned, 1);
}

/// Motion arrives from 3DxWare's thread and is emitted on this one, in
/// QET's axis convention.
void TstSpaceMouseConnexion::motion()
{
	ConnexionBackend backend(nullptr, QStringLiteral(FAKE_CONNEXION));
	QVERIFY(backend.isAvailable());

	QThread *emitted_on = nullptr;
	connect(&backend, &SpaceMouseBackend::motion,
		[&emitted_on]() { emitted_on = QThread::currentThread(); });
	const qint16 axis[6] = {10, 20, 30, 40, 50, 60};
	const QList<SpaceMouseSample> samples = collect(backend, [&]() {
		m_send(CLIENT, AXIS, axis, 0);
	});
	QCOMPARE(samples.size(), 1);
	QCOMPARE(emitted_on, QThread::currentThread());

	const SpaceMouseSample &s = samples.first();
	QCOMPARE(s.x, 10);
	QCOMPARE(s.y, -30);
	QCOMPARE(s.z, -20);
	QCOMPARE(s.rx, 40);
	QCOMPARE(s.ry, -60);
	QCOMPARE(s.rz, -50);
}

/// 3DxWare sends every state to every client; only ours counts.
void TstSpaceMouseConnexion::otherClients()
{
	ConnexionBackend backend(nullptr, QStringLiteral(FAKE_CONNEXION));
	const qint16 axis[6] = {1, 2, 3, 4, 5, 6};
	QCOMPARE(collect(backend, [&]() { m_send(CLIENT + 1, AXIS, axis, 0); }).size(), 0);
}

/// A press is a bit set that was clear; 0-based, as HidBackend numbers them.
void TstSpaceMouseConnexion::buttons()
{
	ConnexionBackend backend(nullptr, QStringLiteral(FAKE_CONNEXION));
	QSignalSpy spy(&backend, &SpaceMouseBackend::buttonPressed);
	const qint16 still[6] = {};

	m_send(CLIENT, BUTTONS, still, 0b101);
	QCoreApplication::processEvents();
	QCOMPARE(spy.size(), 2);
	QCOMPARE(spy.at(0).at(0).toInt(), 0);
	QCOMPARE(spy.at(1).at(0).toInt(), 2);

	m_send(CLIENT, BUTTONS, still, 0b111);
	m_send(CLIENT, BUTTONS, still, 0);
	m_send(CLIENT, BUTTONS, still, 0b100);
	QCoreApplication::processEvents();
	QCOMPARE(spy.size(), 4);
	QCOMPARE(spy.at(2).at(0).toInt(), 1);
	QCOMPARE(spy.at(3).at(0).toInt(), 2);

	QCOMPARE(ConnexionBackend::newlyPressed(0, 0x80000000u), QList<int>{31});
	QVERIFY(ConnexionBackend::newlyPressed(0b11, 0b01).isEmpty());
}

/// A message queued for a backend deleted before the main thread reads it
/// is dropped, not delivered to freed memory.
void TstSpaceMouseConnexion::deletedBeforeDelivery()
{
	auto *backend = new ConnexionBackend(nullptr, QStringLiteral(FAKE_CONNEXION));
	const qint16 axis[6] = {1, 2, 3, 4, 5, 6};
	m_send(CLIENT, AXIS, axis, 0);
	delete backend;
	QCoreApplication::processEvents();
		//And 3DxWare calling after shutdown finds no one to call.
	m_send(CLIENT, AXIS, axis, 0);
	QCoreApplication::processEvents();
}

/// 3DxWare takes one set of handlers per process.
void TstSpaceMouseConnexion::oneClientPerProcess()
{
	ConnexionBackend first(nullptr, QStringLiteral(FAKE_CONNEXION));
	ConnexionBackend second(nullptr, QStringLiteral(FAKE_CONNEXION));
	QVERIFY(first.isAvailable());
	QVERIFY(!second.isAvailable());

	const qint16 axis[6] = {5, 0, 0, 0, 0, 0};
	QCOMPARE(collect(first, [&]() { m_send(CLIENT, AXIS, axis, 0); }).size(), 1);
}

/// On a Mac with 3DxWare installed: this process can load 3DxWare's real
/// library. A Developer ID build with the hardened runtime needs
/// misc/qelectrotech.entitlements for that; an ad-hoc signature does not
/// enforce library validation, so CI cannot show the difference. Whether
/// 3DxWare's driver then answers depends on the machine, so it is not checked.
void TstSpaceMouseConnexion::realLibrary()
{
	QLibrary library(QString::fromLatin1(ConnexionBackend::DEFAULT_LIBRARY));
	if (!QFileInfo::exists(library.fileName())) {
		QSKIP("3DxWare is not installed");
	}
	QVERIFY2(library.load(), qPrintable(library.errorString()));
	QVERIFY(library.resolve("SetConnexionHandlers"));

	ConnexionBackend backend;
	qInfo("3DxWare's driver %s", backend.isAvailable() ? "answered" : "did not answer");
}

QTEST_GUILESS_MAIN(TstSpaceMouseConnexion)
#include "tst_spacemouseconnexion.moc"
