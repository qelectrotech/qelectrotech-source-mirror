// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "../../sources/dataBase/qetparity.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QtTest>

#include <algorithm>
#include <numeric>

/**
	The repair data of a .qetdb (QETDB-S4C-PLAN.md P1): any m lost pages of
	a group, data or parity, rebuilt exactly; one more is reported; the
	same bytes as the prototype tools/qet-sqlite/rescue/qetparity.c.
*/
class TestQetParity : public QObject
{
	Q_OBJECT

	static constexpr int page = 4096;

	static QByteArray randomPages(int n, quint32 seed)
	{
		QByteArray data(qsizetype(n) * page, Qt::Uninitialized);
		QRandomGenerator rng(seed);
		rng.fillRange(reinterpret_cast<quint32 *>(data.data()), data.size() / 4);
		return data;
	}

		//The pages of group @p g, local index li, when there are @p groups
	static int pageOf(int g, int li, int groups) { return g + li * groups; }

	private slots:
	void cleanup()
	{
		QetParity::setPortableOnly(false);
	}

		//As parity.py's layout()
	void layoutAsPrototype_data()
	{
		QTest::addColumn<int>("pages");
		QTest::addColumn<int>("k");
		QTest::addColumn<int>("m");
		QTest::addColumn<int>("groups");
		QTest::newRow("one page") << 1 << 1 << 8 << 1;
		QTest::newRow("7") << 7 << 7 << 8 << 1;
		QTest::newRow("201") << 201 << 101 << 11 << 2;
		QTest::newRow("300") << 300 << 150 << 15 << 2;
		QTest::newRow("401") << 401 << 134 << 14 << 3;
		QTest::newRow("f2099") << 13625 << 198 << 20 << 69;
	}

	void layoutAsPrototype()
	{
		QFETCH(int, pages);
		const QetParity::Layout l = QetParity::layout(pages, 10);
		QFETCH(int, k);
		QFETCH(int, m);
		QFETCH(int, groups);
		QCOMPARE(l.k, k);
		QCOMPARE(l.m, m);
		QCOMPARE(l.groups, groups);
	}

		//qetparity encode, over the same 300 pages, gave this
	void sameBytesAsPrototype_data()
	{
		QTest::addColumn<bool>("portable");
		QTest::newRow("portable") << true;
		QTest::newRow("fastest here") << false;
	}

	void sameBytesAsPrototype()
	{
		QFETCH(bool, portable);
		QetParity::setPortableOnly(portable);
		const int n = 300;
		QByteArray data(qsizetype(n) * page, Qt::Uninitialized);
		for (qsizetype i = 0 ; i < data.size() ; ++i)
			data[i] = char((i * 131 + (i >> 12) * 7 + 13) & 0xff);
		const QetParity::Layout l = QetParity::layout(n, 10);
		const QByteArray parity = QetParity::encode(data, page, l.k, l.m);
		QCOMPARE(parity.size(), qsizetype(l.groups) * l.m * page);
		QCOMPARE(QCryptographicHash::hash(parity, QCryptographicHash::Sha256).toHex(),
				 QByteArray("db32297ea6344496a92070e40e19bcdb007a86b36f6903cd8032cfb750275cf2"));
	}

	void portableAndAvx2Agree()
	{
		if (!QetParity::usesAvx2())
			QSKIP("no AVX2 here");
		const QByteArray data = randomPages(450, 7);
		const QetParity::Layout l = QetParity::layout(450, 10);
		const QByteArray fast = QetParity::encode(data, page, l.k, l.m);
		QetParity::setPortableOnly(true);
		QCOMPARE(QetParity::encode(data, page, l.k, l.m), fast);
	}

		//Every loss count from none to m in each group, data and parity
		//pages mixed, rebuilt exactly
	void lossesUpToParityRebuilt_data()
	{
		QTest::addColumn<bool>("portable");
		QTest::newRow("portable") << true;
		QTest::newRow("fastest here") << false;
	}

	void lossesUpToParityRebuilt()
	{
		QFETCH(bool, portable);
		QetParity::setPortableOnly(portable);
		const int n = 450;
		const QByteArray saved = randomPages(n, 1);
		const QetParity::Layout l = QetParity::layout(n, 10);
		QCOMPARE(l.groups, 3);
		const QByteArray parity = QetParity::encode(saved, page, l.k, l.m);
		QRandomGenerator rng(2);
		for (int losses = 0 ; losses <= l.m ; ++losses) {
			for (int parity_losses = 0 ; parity_losses <= losses ; parity_losses += std::max(1, losses / 3)) {
				QByteArray data = saved;
				QByteArray bad_parity = parity;
				QList<int> lost;
				QList<std::pair<int, int>> lost_parity;
				for (int g = 0 ; g < l.groups ; ++g) {
					const int size = (n - g + l.groups - 1) / l.groups;
					std::vector<int> local(static_cast<size_t>(size));
					std::iota(local.begin(), local.end(), 0);
					std::shuffle(local.begin(), local.end(), rng);
					for (int x = 0 ; x < losses - parity_losses ; ++x) {
						const int p = pageOf(g, local[size_t(x)], l.groups);
						data.replace(qsizetype(p) * page, page, QByteArray(page, char(0xa5)));
						lost << p;
					}
					std::vector<int> js(size_t(l.m));
					std::iota(js.begin(), js.end(), 0);
					std::shuffle(js.begin(), js.end(), rng);
					for (int x = 0 ; x < parity_losses ; ++x) {
						bad_parity.replace((qsizetype(g) * l.m + js[size_t(x)]) * page, page, QByteArray(page, '\0'));
						lost_parity << std::pair{g, js[size_t(x)]};
					}
				}
				QCOMPARE(QetParity::decode(data, page, bad_parity, l.k, l.m, lost, lost_parity), 0);
				QVERIFY2(data == saved, qPrintable(QStringLiteral("%1 lost, %2 of them parity")
												   .arg(losses).arg(parity_losses)));
			}
		}
	}

		//One loss more than the parity in one group: that group is
		//reported and left as it was, the others still rebuilt
	void oneLossTooManyReported()
	{
		const int n = 450;
		const QByteArray saved = randomPages(n, 3);
		const QetParity::Layout l = QetParity::layout(n, 10);
		const QByteArray parity = QetParity::encode(saved, page, l.k, l.m);
		QByteArray data = saved;
		QList<int> lost;
		for (int li = 0 ; li <= l.m ; ++li)
			lost << pageOf(0, li, l.groups);
		lost << pageOf(1, 5, l.groups) << pageOf(2, 9, l.groups);
		for (int p : std::as_const(lost))
			data.replace(qsizetype(p) * page, page, QByteArray(page, char(0x5a)));
		QCOMPARE(QetParity::decode(data, page, parity, l.k, l.m, lost, {}), 1);
		for (int p : {pageOf(1, 5, l.groups), pageOf(2, 9, l.groups)})
			QCOMPARE(data.mid(qsizetype(p) * page, page), saved.mid(qsizetype(p) * page, page));
		QCOMPARE(data.mid(0, page), QByteArray(page, char(0x5a)));
	}

		//A wrong page not named as lost is not found by the code: the
		//caller's page hashes find it (P3). Named, it is rebuilt
	void unnamedDamageIsNotGuessed()
	{
		const int n = 200;
		const QByteArray saved = randomPages(n, 4);
		const QetParity::Layout l = QetParity::layout(n, 10);
		const QByteArray parity = QetParity::encode(saved, page, l.k, l.m);
		QByteArray data = saved;
		data[qsizetype(17) * page + 100] = char(data.at(qsizetype(17) * page + 100) ^ 1);
		QCOMPARE(QetParity::decode(data, page, parity, l.k, l.m, {}, {}), 0);
		QVERIFY(data != saved);
		QCOMPARE(QetParity::decode(data, page, parity, l.k, l.m, {17}, {}), 0);
		QCOMPARE(data, saved);
	}

	void wrongSizesRefused()
	{
		const QByteArray data = randomPages(10, 5);
		const QetParity::Layout l = QetParity::layout(10, 10);
		const QByteArray parity = QetParity::encode(data, page, l.k, l.m);
		QByteArray cut = data.left(data.size() - 1);
		QCOMPARE(QetParity::decode(cut, page, parity, l.k, l.m, {0}, {}), -1);
		QByteArray whole = data;
		QCOMPARE(QetParity::decode(whole, page, parity.left(page), l.k, l.m, {0}, {}), -1);
		QCOMPARE(QetParity::encode(data, page, 250, 10), QByteArray());
	}
};

QTEST_GUILESS_MAIN(TestQetParity)
#include "tst_qetparity.moc"
