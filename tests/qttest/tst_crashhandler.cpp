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

#include "logging/crashhandler.h"

#include <QTest>
#include <climits>
#include <csignal>
#include <cstring>

/**
	@brief The tst_CrashHandler class

	Covers CrashHandler::formatInt(), the decimal formatter the signal
	handler uses to write "Signal: 11" into a crash dump.

	It is worth a test out of proportion to its size. It cannot use
	snprintf(), which is not on the POSIX async-signal-safe list, so it is
	hand-rolled; it runs only inside a signal handler, where nothing can
	assert and a fault produces no diagnostic; and it is exercised only
	when the application is already crashing, so a defect here would
	corrupt or truncate exactly the dumps that matter and would never be
	noticed in ordinary use.

	The negation is the interesting part: -value on INT_MIN is undefined
	behaviour, so the implementation goes through unsigned.
*/
class tst_CrashHandler : public QObject
{
	Q_OBJECT

	private slots:
		void formatsZero();
		void formatsPositive();
		void formatsTheHandledSignals();
		void formatsNegative();
		void formatsIntMinWithoutOverflow();
		void formatsIntMax();
		void truncatesRatherThanOverflowing();
		void writesNothingWhenThereIsNoRoom();

	private:
		/// Formats into a buffer poisoned with a sentinel, and fails if
		/// anything past the returned length was touched.
		static QByteArray format(int value, int size = 32);
};

QByteArray tst_CrashHandler::format(int value, int size)
{
	char buffer[64];
	memset(buffer, '\xAB', sizeof(buffer));

	const int len = CrashHandler::formatInt(buffer, size, value);

	// Nothing may be written past what was reported, nor past `size`.
	for (int i = qMax(len, 0) ; i < static_cast<int>(sizeof(buffer)) ; ++i) {
		if (buffer[i] != '\xAB') {
			return QByteArray("WROTE PAST END at ") + QByteArray::number(i);
		}
	}
	if (len < 0 || len > size) {
		return QByteArray("BAD LENGTH ") + QByteArray::number(len);
	}
	return QByteArray(buffer, len);
}

void tst_CrashHandler::formatsZero()
{
	QCOMPARE(format(0), QByteArray("0"));
}

void tst_CrashHandler::formatsPositive()
{
	QCOMPARE(format(1), QByteArray("1"));
	QCOMPARE(format(9), QByteArray("9"));
	QCOMPARE(format(10), QByteArray("10"));
	QCOMPARE(format(1234567), QByteArray("1234567"));
}

/**
	The values this actually sees in the field: kHandledSignals, as
	written into the "Signal: N" line of every dump.
*/
void tst_CrashHandler::formatsTheHandledSignals()
{
	QCOMPARE(format(SIGSEGV), QByteArray::number(SIGSEGV));
	QCOMPARE(format(SIGABRT), QByteArray::number(SIGABRT));
#if defined(SIGBUS)
	QCOMPARE(format(SIGBUS), QByteArray::number(SIGBUS));
#endif
	QCOMPARE(format(SIGFPE), QByteArray::number(SIGFPE));
	QCOMPARE(format(SIGILL), QByteArray::number(SIGILL));
}

void tst_CrashHandler::formatsNegative()
{
	QCOMPARE(format(-1), QByteArray("-1"));
	QCOMPARE(format(-42), QByteArray("-42"));
}

/**
	-INT_MIN is undefined behaviour; the implementation negates through
	unsigned instead. A build that got this wrong would either trap under
	-ftrapv/UBSan or silently print the wrong number.
*/
void tst_CrashHandler::formatsIntMinWithoutOverflow()
{
	QCOMPARE(format(INT_MIN), QByteArray::number(INT_MIN));
}

void tst_CrashHandler::formatsIntMax()
{
	QCOMPARE(format(INT_MAX), QByteArray::number(INT_MAX));
}

/**
	A buffer too small must be filled and stopped at, never run past --
	the handler passes a fixed 64-byte stack buffer and subtracts what it
	has already used.
*/
void tst_CrashHandler::truncatesRatherThanOverflowing()
{
	QCOMPARE(format(12345, 3), QByteArray("123"));
	QCOMPARE(format(-12345, 3), QByteArray("-12"));
	QCOMPARE(format(7, 1), QByteArray("7"));
}

void tst_CrashHandler::writesNothingWhenThereIsNoRoom()
{
	QCOMPARE(format(123, 0), QByteArray());
	QCOMPARE(format(0, 0), QByteArray());
	QCOMPARE(format(-5, 0), QByteArray());
}

QTEST_APPLESS_MAIN(tst_CrashHandler)
#include "tst_crashhandler.moc"
