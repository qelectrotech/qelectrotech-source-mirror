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
#include "qetapp.h"

#include <QList>
#include <QThread>
#include <QtTest>

#include <atomic>
#include <vector>

/*
	The elements collection loads on worker threads, and the name of each
	folder asks QETApp::langFromSetting() for the language. Without a
	QETApp, as in the tests, nothing asks it before them, so the first call
	came from several threads at once. It filled the language on the first
	call without a lock, and a thread could copy the string while another
	one replaced it: tst_collectionmodelquit aborted on the CI with
	ASSERT: "this->d->ref_.loadRelaxed() == 0" (qarraydataops.h).

	Only the first call in a process can show it, so nothing here calls
	langFromSetting() before the threads do. With the old code a run
	aborted now and then, not every time; ThreadSanitizer reports it every
	time.
*/
class tst_LangFromSettingThreads : public QObject
{
	Q_OBJECT
	private slots:
		void firstCallFromSeveralThreads()
		{
			const int count = 8;
			std::atomic<bool> start{false};
			std::vector<QString> languages(count);
			QList<QThread *> threads;
			for (int i = 0 ; i < count ; ++i)
			{
				threads << QThread::create([&start, &languages, i]()
				{
					while (!start.load())
						QThread::yieldCurrentThread();
					languages[i] = QETApp::langFromSetting();
				});
				threads.last()->start();
			}
			start = true;
			for (QThread *thread : std::as_const(threads))
			{
				QVERIFY(thread->wait(10000));
				delete thread;
			}

			QVERIFY(!languages.front().isEmpty());
			for (const QString &language : std::as_const(languages))
				QCOMPARE(language, languages.front());
			QCOMPARE(QETApp::langFromSetting(), languages.front());
		}
};

QTEST_MAIN(tst_LangFromSettingThreads)
#include "tst_langfromsettingthreads.moc"
