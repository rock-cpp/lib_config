#include <boost/test/unit_test.hpp>
#include "Bundle.hpp"
#include "stdlib.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include <thread>         // std::this_thread::sleep_until
#include <chrono>         // std::chrono::system_clock
#include <ctime>          // std::time_t, std::tm, std::localtime, std::mktime

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
    std::ofstream fs((bundle_root / "config" / "orogen" / "my::Task.yml").string(),
                  std::ios_base::out);
    fs << "--- name:default\n";
    fs << "axisScale: []\n";
    fs << "name: defaultname\n";

    fs << "--- name:specialized\n";
    fs << "axisScale: [1,2,3]\n";
    fs << "name: " << name << "\n";
    fs << "--- name:"<<name << "\n";
    fs << "axisScale: [1,2,3]\n";
    fs << "name: " << name << "\n";
    fs.close();
    fs::create_directory(bundle_root / "logs");

    fs::path bundle_config(bundle_root / "config" / "bundle.yml");
    std::ofstream os(bundle_config.c_str(), std::ios_base::out);
    if(deps.size() > 0){
        os << "bundle:\n";
        os << "    dependencies:\n";
        for(std::string& dep : deps){
            os << "        - " << dep << "\n";
        }
        os << "\n";
    }
    os.close();
}

/*
 * Creates the following bundle structure:
 *
 * root/first   (depends on "second" and "third")
 * root/second  (depends on fourth)
 * root/third   (no depdendencies)
 * root/fourth  (no depdendencies)
 *
 * first
 *    |- second
 *         | fourth
 *    |- third
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
    deps.push_back("fourth");
    prepare_bundle("second", deps);
    prepare_bundle("fourth", deps);

    deps.push_back("fourth");
    prepare_bundle("third", deps);
}

BOOST_AUTO_TEST_CASE(construction)
{
    clear_environment_variables();
    prepare_bundles();
    //Can initialize from absolute path in ROCK_BUNDLE environment variable
    //and with no ROCK_BUNDLE_PATH set
    fs::path selected_bundle_dir = fs::path(bundle_path) / "first";
    setenv("ROCK_BUNDLE", selected_bundle_dir.c_str(), 1);
    libConfig::Bundle& inst = libConfig::Bundle::getInstance();
    BOOST_CHECK_EQUAL(inst.getActiveBundleName(), "first");

    //Finding files is working properly with disabled dependencies
    std::string s = inst.findFileByName("config/bundle.yml");
    BOOST_CHECK_EQUAL(s, selected_bundle_dir.string()+"/config/bundle.yml");
    inst.deleteInstance();

    //Can initialize from bundle name ROCK_BUNDLE environment
    clear_environment_variables();
    setenv("ROCK_BUNDLE_PATH", bundle_path.c_str(), 1);
    setenv("ROCK_BUNDLE", "first", 1);
    libConfig::Bundle& inst2 = libConfig::Bundle::getInstance();
    BOOST_CHECK_EQUAL(inst2.getActiveBundleName(), "first");

    //Should have depdendencies resolved.
    //Check that dependency resolution is breadth-first and that file finding
    //is done correctly (breadth-first)
    std::vector<std::string> candidates = inst.findFilesByName("config/bundle.yml");
    BOOST_ASSERT(candidates.size() == 4);
    BOOST_CHECK_NE(candidates[0].find("first"), std::string::npos);
    BOOST_CHECK_NE(candidates[1].find("second"), std::string::npos);
    BOOST_CHECK_NE(candidates[2].find("third"), std::string::npos);
    BOOST_CHECK_NE(candidates[3].find("fourth"), std::string::npos);

    //check that log folders are incremented if one already exists and that
    //they are created correctly
    using std::chrono::system_clock;
    std::time_t tt = system_clock::to_time_t (system_clock::now());
    struct std::tm * ptm = std::localtime(&tt);
    ptm->tm_min; ++ptm->tm_sec; //Increment one second
    //Sleep until next second to begin
    std::this_thread::sleep_until (system_clock::from_time_t (mktime(ptm)));

    std::vector<std::string> folders;
    for(uint i=0; i<10; i++){
        inst.createLogDirectory();
        folders.push_back(inst.getLogDirectory());
    }

    for(uint i=0; i<10; i++){
        if(i > 0){
            int val;
            std::string::size_type sz;
            std::string folder = folders.at(i);
            std::string number = folder.substr(folder.size()-1,folder.size());
            val = std::stoi(number, &sz);
            BOOST_CHECK_EQUAL(val, i);
        }
    }

    //Can detect config files by Task prototype
    std::string p = inst.getConfigurationPath("my::Task");
    BOOST_CHECK_EQUAL(p, selected_bundle_dir.string()+"/config/orogen/my::Task.yml");
    candidates = inst.getConfigurationPathsForTaskModel("my::Task");
    BOOST_ASSERT(candidates.size() == 4);
    BOOST_CHECK_NE(candidates[0].find("first"), std::string::npos);
    BOOST_CHECK_NE(candidates[1].find("second"), std::string::npos);
    BOOST_CHECK_NE(candidates[2].find("third"), std::string::npos);
    BOOST_CHECK_NE(candidates[3].find("fourth"), std::string::npos);

    //!Can find files by extesion
    //findFilesByExtension("config/orogen","yml");

    //Throws when bundle could not be found
    inst.deleteInstance();
    setenv("ROCK_BUNDLE", "bullshit", 1);
    BOOST_REQUIRE_THROW(inst2 = libConfig::Bundle::getInstance(), std::runtime_error);
    inst.deleteInstance();
}

BOOST_AUTO_TEST_CASE(task_configuration)
{
    clear_environment_variables();
    setenv("ROCK_BUNDLE_PATH", bundle_path.c_str(), 1);
    setenv("ROCK_BUNDLE", "first", 1);
    libConfig::Bundle& inst = libConfig::Bundle::getInstance();
    inst.initialize();

    //Active bundles are first,second,third,fourth (highest prio first)
    //Two sections are resent in each bundle. Each bundle adds one additional
    //section. Expected numer of sections --> 6
    libConfig::MultiSectionConfiguration mcfg = inst.taskConfigurations.getMultiConfig("my::Task");
    BOOST_CHECK_EQUAL(mcfg.getSubsections().size(), 6);

    //Bundle "first" should overwrite all others, section
    //'specialized' should overwrite 'default'
    libConfig::Configuration cfg = inst.taskConfigurations.getConfig(
                "my::Task", {"default", "specialized"});
    std::map< std::string, std::shared_ptr<libConfig::ConfigValue> > vals =
            cfg.getValues();
    std::shared_ptr<libConfig::SimpleConfigValue> val =
            std::dynamic_pointer_cast<libConfig::SimpleConfigValue>(vals["name"]);
    BOOST_CHECK_EQUAL(val->getValue(), "first");

    //Acces to section imported from lower priorized bundle is possible
    cfg = inst.taskConfigurations.getConfig(
                    "my::Task", {"default", "fourth"});
    vals = cfg.getValues();
    val = std::dynamic_pointer_cast<libConfig::SimpleConfigValue>(vals["name"]);
    BOOST_CHECK_EQUAL(val->getValue(), "fourth");
    BOOST_ASSERT(true);
}
