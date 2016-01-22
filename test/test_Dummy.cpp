#include <boost/test/unit_test.hpp>
#include <lib_config/Dummy.hpp>

using namespace lib_config;

BOOST_AUTO_TEST_CASE(it_should_not_crash_when_welcome_is_called)
{
    lib_config::DummyClass dummy;
    dummy.welcome();
}
