//
// Copyright (c) 2017-2022 Olaf (<ibis-hdl@users.noreply.github.com>).
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

namespace boost::test_tools {
}
namespace boost::unit_test::framework {
}
namespace boost::unit_test::data {
}

namespace testsuite {

// ToDo: Check for correct and intuitive aliases!
namespace btt = boost::test_tools;
namespace utf = boost::unit_test;
namespace utf_data = boost::unit_test::data;

namespace butf = boost::unit_test::framework;

}  // namespace testsuite

// global for BOOST_DATA_TEST_CASE()
namespace btt = testsuite::btt;
namespace utf_data = testsuite::utf_data;
