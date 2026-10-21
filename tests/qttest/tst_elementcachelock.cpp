// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QUuid>

#include "elementscollectioncache.h"

// Every QElectroTech on a computer shares one elements_cache.sqlite. The cache
// used PRAGMA locking_mode = EXCLUSIVE, so a QElectroTech that had once read
// the file kept it locked until it closed, and every write from a second one
// waited SQLite's 5 s busy timeout and failed -- on the GUI thread, twice per
// symbol shown in the collection (#1571). Two caches on the same file in one
// process lock each other exactly as two processes do.
class tst_elementcachelock : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	const QUuid m_uuid = QUuid::createUuid();

private slots:
	void secondCacheWritesWithoutWaiting()
	{
		QVERIFY(m_dir.isValid());
		const QString path = m_dir.filePath(QStringLiteral("elements_cache.sqlite"));

		ElementsCollectionCache first(path);
		QVERIFY(first.cacheName(QStringLiteral("embed://a.elmt"), m_uuid));
		QVERIFY(first.fetchNameFromCache(QStringLiteral("embed://a.elmt"), m_uuid));
		QVERIFY(!first.fetchNameFromCache(QStringLiteral("embed://missing.elmt"), m_uuid));

		ElementsCollectionCache second(path);
		QElapsedTimer timer;
		timer.start();
		QVERIFY(second.cacheName(QStringLiteral("embed://b.elmt"), m_uuid));
		QVERIFY(second.cachePixmap(QStringLiteral("embed://b.elmt"), m_uuid));
		QVERIFY2(timer.elapsed() < 1000, qPrintable(QString::number(timer.elapsed()) + " ms"));

		// and each one sees what the other wrote
		QVERIFY(second.fetchNameFromCache(QStringLiteral("embed://a.elmt"), m_uuid));
		QVERIFY(first.fetchNameFromCache(QStringLiteral("embed://b.elmt"), m_uuid));
		QVERIFY(first.cacheName(QStringLiteral("embed://c.elmt"), m_uuid));
	}
};

QTEST_MAIN(tst_elementcachelock)
#include "tst_elementcachelock.moc"
