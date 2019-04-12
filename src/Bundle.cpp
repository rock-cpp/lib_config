#include "Bundle.hpp"
#include <stdlib.h>
#include <stdexcept>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <base/Time.hpp>
#include <yaml-cpp/yaml.h>
#include "YAMLConfiguration.hpp"


namespace fs = boost::filesystem;
using namespace libConfig;

Bundle * Bundle::instance;

//Splits a string into a vector at the position of a specific tokens that are
//present in the input string.
std::vector<std::string> tokenize(std::string data, char delim)
{
    boost::char_separator<char> sep(&delim);
    boost::tokenizer<boost::char_separator<char> > tokens(data, sep);

    std::vector<std::string> ret;
    for(const std::string &token : tokens)
    {
        ret.push_back(token);
    }
    return ret;
}

//!
//! @brief Checks if in one of multiple possibel search paths a given folder
//!        exists
//! @Returns full ppath of folder if it was found. Returns empty string
//!          if the given fodler could not be found
std::string findFolderInSearchPaths(std::string folderName,
                       std::vector<std::string> searchPaths)
{
    for(const std::string &path : searchPaths)
    {
        fs::path candidate = fs::path(path) / folderName;

        if(boost::filesystem::exists(candidate))
        {
            return candidate.string();
        }
    }
    return "";
}

SingleBundle::SingleBundle() :
    name(""), path(""), logBaseDir(""), dataDir(""), configDir(""),
    orogenConfigDir("")
{
}

SingleBundle::~SingleBundle()
{

}

SingleBundle SingleBundle::fromNameAndSearchPaths(
        std::string name,
        std::vector<std::string> bundleSearchPaths)
{
    SingleBundle bundle;
    bundle.name = name;
    bundle.path = findFolderInSearchPaths(name, bundleSearchPaths);
    if(bundle.path.empty()){
        std::string searchPathsString;
        for (const std::string &path : bundleSearchPaths){
            searchPathsString += path;
            searchPathsString += ",";
        }

        std::stringstream ss;
        ss << "Could not determine path of bundle " << name << ". Looked " <<
              "into the following search paths: " << searchPathsString;
        throw(std::runtime_error(ss.str()));
    }

    bundle.setAndValidatePaths();
    return bundle;
}

SingleBundle SingleBundle::fromFullPath(std::string bundlePath)
{
    SingleBundle bundle;
    bundle.path = bundlePath;
    std::vector<std::string> tkn = tokenize(bundlePath, '/');
    bundle.name = tkn.back();
    bundle.setAndValidatePaths();
    return bundle;
}

//Throws if the bundle path is not valid. Writes warning message, if data and
//config folder cannot be found
void SingleBundle::setAndValidatePaths()
{
    if(path.empty() || !fs::exists(path)){
        std::stringstream ss;
        ss << "Bundle '" << name << "' has the following path: '" <<
              path << "', but this path does not exist.";
        throw(std::runtime_error(ss.str()));
    }

    //Config folder (config)
    fs::path subpath = fs::path("config");
    configDir = (path / subpath).string();
    if(!fs::exists(configDir)){
        std::clog << "The path of bundle '" << name << "' was resolved to '" <<
                     path << "', but the path does not contain a '" <<
                     subpath.string() << "' subfolder.";
    }

    //Config folder (config/orogen)
    subpath = fs::path("config") / "orogen";
    orogenConfigDir = (path / subpath).string();
    if(!fs::exists(orogenConfigDir)){
        std::clog << "The path of bundle '" << name << "' was resolved to '" <<
                     path << "', but the path does not contain a '" <<
                     subpath.string() << "' subfolder.";
    }

    //Data folder
    subpath = fs::path("data");
    dataDir = (path / subpath).string();
    if(!fs::exists(configDir)){
        std::clog << "The path of bundle '" << name << "' was resolved to '" <<
                     dataDir << "', but the path does not contain a '" <<
                     subpath.string() << "' subfolder.";
    }

    //Existence of log dir is not validated because there is no requirement for
    //it to exist. It often gets deleted when deleting log files
    logBaseDir = (fs::path(path) / "log").string();
}

Bundle::Bundle()
{
}

bool Bundle::initialized()
{
    return activeBundles.size() > 0;
}

bool Bundle::createLogDirectory()
{
    base::Time curTime = base::Time::now();
    fs::path logDir = fs::path(selectedBundle().logBaseDir) / curTime.toString(
                                                            base::Time::Seconds,
                                                            "%Y%m%d-%H%M");

    //use other directory if log dir already exists
    std::string base = logDir.string();
    int i = 1;
    while(boost::filesystem::exists(logDir))
    {
        logDir = base + "." + boost::lexical_cast<std::string>(i);
        i++;
    }

    if(!boost::filesystem::create_directories(logDir))
    {
        throw std::runtime_error("Failed to create log directory :" +
                                 logDir.string());
    }

    //create symlink to current
    const fs::path currentPath(fs::path(selectedBundle().logBaseDir) / "current");
    if(boost::filesystem::is_symlink(currentPath))
    {
        boost::filesystem::remove(currentPath);
    }
    boost::filesystem::create_directory_symlink(logDir, currentPath);
    currentLogDir = logDir.string();

    return true;
}


Bundle& Bundle::getInstance()
{
    if(!instance){
        instance = new Bundle();
        bool st = instance->initialize();
        if(!st){
            instance = nullptr;
            throw(std::runtime_error(std::string()+"Error, no active bundle " +
            "configured. Please use 'rock-bundle-default'"+
            " to set one."));
        }
    }

    return *instance;
}

void Bundle::deleteInstance()
{
    delete instance;
    instance = nullptr;
}

bool Bundle::initialize()
{
    std::clog << "Initializing Bundles" << std::endl;
    activeBundles.clear();
    std::clog << "Determining selected bundle" << std::endl;
    //Read environment variables:
    //  ROCK_BUNDLE: contains currently selected bundle (name or absolute path)
    //  ROCK_BUNDLE_PATH: contains search paths, where to look for a bundle.
    //                    multiple paths are separated by ':'
    const char *activeBundleC = getenv("ROCK_BUNDLE");
    if(!activeBundleC)
    {
        std::cerr << "ROCK_BUNDLE not set. Bundle cannot be initialized." <<
                     std::endl;
        return false;
    }

    const char *pathsC = getenv("ROCK_BUNDLE_PATH");
    if(pathsC)
    {
        std::string paths = pathsC;
        bundleSearchPaths = tokenize(paths, ':');
    }else
    {
        std::clog << "No bundle search path set. Consider setting environment "\
                     "variable ROCK_BUNDLE_PATH." << std::endl;
    }

    //Determine if ROCK_BUNDLE contained absolute path or bundle name
    fs::path pathCheck(activeBundleC);
    SingleBundle bundle;
    if(pathCheck.is_absolute() && fs::exists(pathCheck))
    {
        //ROCK_BUNDLE contains an absolute path
        bundle = SingleBundle::fromFullPath(pathCheck.string());
    }else
    {
        //ROCK_BUNDLE contains bundle name
        bundle = SingleBundle::fromNameAndSearchPaths(activeBundleC,
                                                      bundleSearchPaths);
    }
    std::clog << "Currently selected bundle: "<<bundle.name << " at " <<
                 bundle.path << std::endl;

    activeBundles.push_back(bundle);

    std::clog << "Discovering bundle dependencies" << std::endl;
    if(bundleSearchPaths.empty())
    {
        std::clog << "Bundle search path is not set. Dependency resolutibn " <<
                     "to other bundles is disabled.";
    }else{
        discoverDependencies(selectedBundle(), activeBundles);
    }
    std::clog << "Active bundles: " << std::endl;
    for(SingleBundle& b : activeBundles){
        std::clog << "   " << b.name << " at " << b.path << "\n";
    }
    std::clog << std::endl;

    //Initialze TaskConfigurations
    std::clog << "Loading task configuration files from bundle" << std::endl;
    std::vector<std::string> configs = findFilesByExtension(
                (fs::path("config") / "orogen").string(), ".yml");
    taskConfigurations.initialize(configs);
    std::clog << "Bundles successfully initialized" << std::endl;

    return true;
}

const std::string &Bundle::getActiveBundleName()
{
    return selectedBundle().name;
}

SingleBundle& Bundle::selectedBundle()
{
    return activeBundles.front();
}

const std::string &Bundle::getConfigurationDirectory()
{
    return selectedBundle().configDir;
}

std::string Bundle::getConfigurationPath(const std::string &task)
{
    std::string relativePath = "config/orogen/"+ task + ".yml";
    try{
        std::string result = findFileByName(relativePath);
        return result;
    } catch (std::runtime_error &e){
        throw std::runtime_error("Bundle::getConfigurationPath : Error, could not find config path for task " + task);
    }
}

std::string Bundle::getLogDirectory()
{
    if(currentLogDir.empty()){
        createLogDirectory();
    }
    return currentLogDir;
}

const std::string& Bundle::getDataDirectory()
{
    return selectedBundle().dataDir;
}

std::string Bundle::findFileByName(const std::string& relativePath)
{
    for(const SingleBundle &bundle : activeBundles)
    {
        fs::path curPath = fs::path(bundle.path) / relativePath;
        if(boost::filesystem::exists(curPath))
            return curPath.string();
    }
    throw std::runtime_error("Bundle::findFile : Error, could not find file " + relativePath);
}

std::vector<std::string> Bundle::findFilesByName(const std::string& relativePath)
{
    std::vector<std::string> paths;
    for(const SingleBundle &bundle : activeBundles)
    {
        fs::path curPath = fs::path(bundle.path) / relativePath;
        if(boost::filesystem::exists(curPath))
            paths.push_back(curPath.string());
    }
    return paths;
}

std::vector<std::string> Bundle::findFilesByExtension(
        const std::string &relativePath, const std::string &ext)
{
    std::vector<std::string> ret;
    for(const SingleBundle &bundle : activeBundles)
    {
        fs::path root = fs::path(bundle.path) / relativePath;
        if(!fs::exists(root) || !fs::is_directory(root))
            continue;

        fs::recursive_directory_iterator it(root);
        fs::recursive_directory_iterator endit;

        while(it != endit)
        {
            fs::path p = it->path();
            if(fs::is_regular_file(*it) && it->path().extension() == ext)
            {
                ret.push_back(it->path().string());
            }
            ++it;
        }
    }
    return ret;
}

std::vector<std::string> Bundle::getConfigurationPathsForTaskModel(
        const std::string &task_model_name)
{
    fs::path relativePath = fs::path("config") / "orogen" / (task_model_name+".yml");
    std::vector<std::string> paths = findFilesByName(relativePath.string());
    if(paths.empty()){
        std::runtime_error(std::string() + "Bundle::getConfigurationPaths: " +
                           "Could not find configuration file for task " +
                           task_model_name + " in any of the active bundles");
    }
    return paths;
}

void Bundle::discoverDependencies(const SingleBundle &bundle,
                                  std::vector<SingleBundle> &dependencies)
{
    fs::path config_file = bundle.configDir / fs::path("bundle.yml");
    if(!fs::exists(config_file)){
        // No bundle.yml file exists.
        // We assume that this bundle has no depending bundles
        std::clog << "Bundle '" + bundle.name + "' does not contain a bundle" <<
                     " configuration file '."<<config_file << std::endl;
        return;
    }

    //The vector dependencies encodes the priority bundles are search for.
    //i.e. first element in vector is the first to look for a certain file.
    //We want to have a breadth-first discovery behaviour. This means we first
    //first level dependecies are considered more relvant than second level
    //dependencies. An example: This depenedcy tree
    // bundle_1
    //   |- bundle_2
    //        |- bundle_4
    //   |- bundle_3
    // is resolved to this priorization:
    //  [bundle_1, bundle_2, bundle_3, bundle_4]
    std::vector<std::string> deps = loadDependenciesFromYAML(config_file.string());
    std::vector<SingleBundle> to_inspect;
    for(const std::string& bundleName : deps){
        // Don't consider duplicates and avoid cyclic dependencies.
        // TODO: When we do it this way, cyclic dependencies are ignored.
        //       Better throw here?
        std::vector<SingleBundle>::iterator it = std::find(dependencies.begin(),
                                                           dependencies.end(),
                                                           bundleName);
        if(it != dependencies.end()){
            continue;
        }
        SingleBundle d = SingleBundle::fromNameAndSearchPaths(bundleName,
                                                              bundleSearchPaths);
        dependencies.push_back(d);
        to_inspect.push_back(d);
    }

    for(const SingleBundle& bundle : to_inspect){
        discoverDependencies(bundle, dependencies);
    }
}

std::vector<std::string> Bundle::loadDependenciesFromYAML(const std::string &config_file)
{
    std::clog << "Parsing bundle configuration file " << config_file << std::endl;
    YAML::Node node;
    try{
        node = YAML::LoadFile(config_file);
    }catch(std::runtime_error ex){
        std::cerr << "Could not parse bundle config file " << config_file <<
                     ": " << ex.what() << std::endl;
        throw(ex);
    }

    std::vector<std::string> deps;
    try{
        if(node["bundle"] && node["bundle"]["dependencies"]){
            deps = node["bundle"]["dependencies"].as<std::vector<std::string>>();
        }
    }catch(std::runtime_error ex){
        std::cerr << "Error extracting dependecies from " << config_file << std::endl;
    }

    // Erase duplicates
    std::vector<std::string> ret;
    for(const std::string& d : deps){
        if(std::find(ret.begin(), ret.end(), d) == ret.end())
            ret.push_back(d);
    }
    return ret;
}

std::string Bundle::findBundle(const std::string &bundle_name)
{
    SingleBundle b = SingleBundle::fromNameAndSearchPaths(bundle_name,
                                                          bundleSearchPaths);
    return b.path;
}


void TaskConfigurations::initialize(const std::vector<std::string> &configFiles)
{
    taskConfigurations.clear();

    for(const std::string& cfgFilePath : configFiles)
    {
        MultiSectionConfiguration cfgFile;
        std::clog << "Loading config file " << cfgFilePath <<std::endl;
        bool st = cfgFile.load(cfgFilePath);
        if(!st){
            std::clog << "File " << cfgFilePath << " could not be parsed" << std::endl;
            continue;
        }
        std::string& task = cfgFile.taskModelName;
        if(taskConfigurations.find(task) == taskConfigurations.end()){
            //First config file for that task
            taskConfigurations.insert(std::make_pair(task, cfgFile));
        }else
        {
            //There was already a config file laoded. Due to ordering in vector
            //the new config file must be of lower priority
            taskConfigurations.at(task).mergeConfigFile(cfgFile);
        }
    }
}

Configuration TaskConfigurations::getConfig(const std::string &taskModelName,
                                            const std::vector<std::string> &sections)
{
    MultiSectionConfiguration& mcfg = taskConfigurations.at(taskModelName);
    Configuration cfg = mcfg.getConfig(sections);
    return cfg;
}

const MultiSectionConfiguration &TaskConfigurations::getMultiConfig(const std::string &taskModelName)
{
    return taskConfigurations.at(taskModelName);
}
