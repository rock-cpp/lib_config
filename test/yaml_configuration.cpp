#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include "YAMLConfiguration.hpp"
#include <string>
#include <map>
#include <fstream>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

std::string prepare_config_file(){
    std::string filepath = (fs::path("/tmp/") / "my::Task.yml").string();
    std::ofstream fs(filepath,
                  std::ios_base::out);
    fs << "--- name:default\n";
    fs << "axisScale: []\n";
    fs << "name: defaultname\n";

    fs << "--- name:specialized\n";
    fs << "axisScale: [1,2,3]\n";
    fs << "name: specialized\n";
    fs << "--- name:other\n";
    fs << "axisScale: [1,2,3]\n";
    fs << "name: other\n";
    fs.close();
    return filepath;
}

std::string complex_type(){
    return "axisScale: [1,2,3]\nname: hallo";
}

BOOST_AUTO_TEST_CASE(load_from_file)
{
    std::string filepath = prepare_config_file();

    libConfig::YAMLConfigParser parser;
    std::map<std::string, libConfig::Configuration> configs;
    parser.loadConfigFile(filepath, configs);
    BOOST_CHECK_EQUAL(configs.size(), 3);
}

BOOST_AUTO_TEST_CASE(load_from_string)
{
    std::string filepath = prepare_config_file();
    std::ifstream t(filepath);
    std::string yamlstring((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());

    libConfig::YAMLConfigParser parser;
    std::map<std::string, libConfig::Configuration> configs;
    parser.loadConfigString(yamlstring, configs);

    BOOST_CHECK_EQUAL(configs.size(), 3);
}

BOOST_AUTO_TEST_CASE(get_value)
{
    libConfig::YAMLConfigParser parser;
    std::shared_ptr<libConfig::ConfigValue> val = parser.getConfigValue(complex_type());
    std::shared_ptr<libConfig::ComplexConfigValue> cval = std::dynamic_pointer_cast<libConfig::ComplexConfigValue>(val);
    BOOST_CHECK(cval != nullptr);

    BOOST_CHECK_EQUAL(cval->getValues().size(), 2);

}

BOOST_AUTO_TEST_CASE(string_variable_insertions)
{
    std::string res;
    std::string exp;
    putenv("AUTOPROJ_CURRENT_ROOT=/abs/path");
    res = libConfig::YAMLConfigParser::applyStringVariableInsertions("<%= #{ENV['AUTOPROJ_CURRENT_ROOT']}/models/robots/coyote3/urdf/coyote3.urdf %>");
    exp = "/abs/path/models/robots/coyote3/urdf/coyote3.urdf";
    BOOST_CHECK_EQUAL(res, exp);

    //res = libConfig::YAMLConfigParser::applyStringVariableInsertions("<%= \"#{ENV[''AUTOPROJ_CURRENT_ROOT'']}/models/robots/coyote3/urdf/coyote3.urdf\" %>");
    //exp = "/abs/path/myfile.txt";
    //BOOST_CHECK_EQUAL(res, exp);

    //res = libConfig::YAMLConfigParser::applyStringVariableInsertions("<%= \"#{ENV['AUTOPROJ_CURRENT_ROOT']}/models/robots/coyote3/urdf/coyote3.urdf\" %>");
    //exp = "/abs/path/models/robots/coyote3/urdf/coyote3.urdf";
    //BOOST_CHECK_EQUAL(res, exp);
}
