#include <QtGui/QGuiApplication>
#include <gmock/gmock.h>

int main(int argc, char** argv)
{
	QGuiApplication app(argc, argv);

	// disable the whole debug output (we want only the output from gtest)
	// Debug::instance()->setDebugLevelLogFile(Debug::DebugLevel_t::Nothing);
	// Debug::instance()->setDebugLevelStderr(Debug::DebugLevel_t::Nothing);

	// init gtest and run all tests
	::testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}
