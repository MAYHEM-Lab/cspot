#include <woofc-access.h>
#ifndef __APPLE__
#include "doctest.h"
#include <woofc-priv.h>


namespace {
TEST_CASE("Computing port hashes work") {
    auto foobar_hash = WooFPortHash("foobar");
    auto barfoo_hash = WooFPortHash("barfoo");

    REQUIRE_NE(foobar_hash, barfoo_hash);
}

TEST_CASE("Computing name hashes work") {
    auto foobar_hash = WooFNameHash("foobar");
    auto barfoo_hash = WooFNameHash("barfoo");

    REQUIRE_NE(foobar_hash, barfoo_hash);
}
} // namespace
#endif
