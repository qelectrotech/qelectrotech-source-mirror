#include "../../../sources/qet.h"

#include <catch2/catch_all.hpp>

TEST_CASE("qet stringToFileName")
{
	CHECK(QET::stringToFileName("test") == QString("test"));
	CHECK(QET::stringToFileName("t_est") == QString("t_est"));
	CHECK(QET::stringToFileName("t-est") == QString("t-est"));
	CHECK(QET::stringToFileName("te.st") == QString("te.st"));

	CHECK(QET::stringToFileName("tesMt") != QString("tesMt"));
	CHECK(QET::stringToFileName("te<st") != QString("te<st"));
	CHECK(QET::stringToFileName("té<st") != QString("té<st"));
}
