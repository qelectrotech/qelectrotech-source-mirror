#include "../../../sources/borderproperties.h"

#include <catch2/catch_all.hpp>

TEST_CASE("BorderProperties")
{
	BorderProperties My;
	CHECK(My.columns_count == 17);
	CHECK(My.columns_width == 60.0);
	CHECK(My.columns_header_height == 20.0);
	CHECK(My.display_columns == true);
	CHECK(My.rows_count == 8);
	CHECK(My.rows_height == 80.0);
	CHECK(My.rows_header_width == 20.0);
	CHECK(My.display_rows == true);

	BorderProperties My2;

	CHECK((My2 == My) == true);

	My2 = My.defaultProperties();

	CHECK((My2 == My) == true);

	CHECK(My2.columns_count == 17);
	CHECK(My2.columns_width == 60.0);
	CHECK(My2.columns_header_height == 20.0);
	CHECK(My2.display_columns == true);
	CHECK(My2.rows_count == 8);
	CHECK(My2.rows_height == 80.0);
	CHECK(My2.rows_header_width == 20.0);
	CHECK(My2.display_rows == true);
}
