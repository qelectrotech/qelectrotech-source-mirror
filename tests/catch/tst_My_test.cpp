#include "../../sources/borderproperties.h"

#include <catch2/catch.hpp>

TEST_CASE("My first test with Catch2", "[fancy]") { REQUIRE(0 == 0); }

TEST_CASE("BorderProperties")
{
	int				 var = 5;
	BorderProperties My;
	My.columns_count		 = var;
	My.columns_header_height = var;

	CHECK(My.columns_count - 1 == var); // test zal falen maar test de vogende
	CHECK(
		My.columns_header_height - 1
		== var); // test zal falen maar test de vogende
	REQUIRE(0 == 0);
	REQUIRE(1 == 0); // test zal falen en stopt
	REQUIRE(0 == 0); // not testit
	REQUIRE(1 == 0); // not testit
}
