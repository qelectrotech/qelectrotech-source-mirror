#include <QtTest>
#include <QCoreApplication>

// add necessary includes here

class mytest : public QObject
{
	Q_OBJECT

public:
	mytest();
	~mytest();

private slots:
	void initTestCase();
	void cleanupTestCase();
	void test_case1();

};

mytest::mytest()
{

}

mytest::~mytest()
{

}

void mytest::initTestCase()
{

}

void mytest::cleanupTestCase()
{

}

void mytest::test_case1()
{

}

QTEST_MAIN(mytest)

#include "tst_mytest.moc"
