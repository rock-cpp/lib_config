#include <boost/test/unit_test.hpp>
#include "Bundle.hpp"
#include "stdlib.h"
#include <boost/filesystem.hpp>
#include <iostream>

namespace fs = boost::filesystem;
const std::string bundle_path="/tmp/lib_config_bundles";

void clear_environment_variables(){
    unsetenv ("ROCK_BUNDLE");
    unsetenv ("ROCK_BUNDLE_PATH");
}

void prepare_bundle(std::string name,
                    std::vector<std::string> deps)
{
    //Create new bundle folder
    fs::path bundle_root(fs::path(bundle_path) / name);
    fs::create_directories(bundle_root);
    fs::create_directory(bundle_root / "config");
    fs::create_directory(bundle_root / "config" / "orogen");
    fs::create_directory(bundle_root / "logs");

    fs::path bundle_config(bundle_root / "config" / "bundle.yml");
    std::ofstream os(bundle_config.c_str(), std::ios_base::out);
    if(deps.size() > 0){
        os << "bundle:";
        os << "    dependencies:";
        for(std::string& dep : deps){
            os << "        - " <<dep;
        }
        os << "\n";
    }
}

/*
 * Creates the following bundle structure:
 *
 * root/first   (depends on "second" and "third")
 * root/second  (no dependencies)
 * root/third   (depends on fourth)
 * root/fourth  (no depdendencies)
 */
void prepare_bundles()
{
    //Clear if already present
    boost::system::error_code ec;
    if(fs::exists(bundle_path)){
        fs::remove_all(bundle_path, ec);
    }

    std::vector<std::string> deps;
    deps.push_back("second");
    deps.push_back("third");
    prepare_bundle("first", deps);

    deps.clear();
    prepare_bundle("second", deps);
    prepare_bundle("fourth", deps);

    deps.push_back("fourth");
    prepare_bundle("third", deps);
}

BOOST_AUTO_TEST_CASE(construction)
{
    prepare_bundles();
    //Can initialize from absolute path in ROCK_BUNDLE environment variable
    fs::path selected_bundle_dir = fs::path(bundle_path) / "first";
    setenv("ROCK_BUNDLE", selected_bundle_dir.c_str(), 1);
    libConfig::Bundle& inst = libConfig::Bundle::getInstance();
    BOOST_ASSERT(inst.getActiveBundleName() == "first");
    inst.deleteInstance();

    //Can initialize from relative path in ROCK_BUNDLE environment
    clear_environment_variables();
    setenv("ROCK_BUNDLE_PATH", bundle_path.c_str(), 1);
    setenv("ROCK_BUNDLE", "second", 1);
    libConfig::Bundle& inst2 = libConfig::Bundle::getInstance();
    BOOST_ASSERT(inst2.getActiveBundleName() == "second");
    inst.deleteInstance();

    //Throws when bundle could not be found
    setenv("ROCK_BUNDLE", "bullshit", 1);
    inst2 = libConfig::Bundle::getInstance();
    inst.deleteInstance();
    //Can resolve bundle configuration file
    //Can handle malformated bundle configuration file
    //Can find files in bundle
    //Can detect config files by Task prototype
}
