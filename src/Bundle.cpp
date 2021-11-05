#include "Bundle.hpp"
#include <stdlib.h>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <base/Time.hpp>
#include <yaml-cpp/yaml.h>
#include "YAMLConfiguration.hpp"
#include <base-logging/Logging.hpp>


namespace fs = boost::filesystem;
using namespace libConfig;

Bundle * Bundle::instance;
std::vector<std::string> Bundle::_bundleSearchPaths;

//Splits a string into a vector at the position of a specific tokens that are
//present in the input string.
std::vector<std::string> tokenize(std::string data, std::string delim)
{
    boost::char_separator<char> sep(delim.c_str());
    boost::tokenizer<boost::char_separator<char> > tokens(data, sep);

    std::vector<std::string> ret;
    for(const std::string &token : tokens)
    {
        ret.push_back(token);
    }
    return ret;
}


std::vector<std::string> findFoldersInSearchPaths(const std::string& folderName,
                       const std::vector<std::string>& searchPaths)
{
    std::vector<std::string> ret;
    for(const std::string &path : searchPaths)
    {
        fs::path candidate = fs::path(path) / folderName;

        if(boost::filesystem::exists(candidate))
        {
            ret.push_back(candidate.string());
        }
    }
    return ret;
}

std::vector<std::string> findFoldersInSearchPaths(const std::vector<std::string>& searchPaths)
{
    std::vector<std::string> r;
    for(const std::string& s : searchPaths){
        for(fs::directory_entry& p : fs::directory_iterator(s))
            if (p.status().type() == fs::file_type::directory_file){
                r.push_back(p.path().string());
            }
    }
    return r;
}

//!
//! @brief Checks if in one of multiple possibel search paths a given folder
//!        exists
//! @Returns full ppath of folder if it was found. Returns empty string
//!          if the given fodler could not be found
std::string findFolderInSearchPaths(std::string folderName,
                       std::vector<std::string> searchPaths)
{
    std::vector<std::string> candidates = findFoldersInSearchPaths(folderName, searchPaths);
    if(candidates.empty()){
        return "";
    }else{
        return candidates[0];
    }
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
    std::vector<std::string> tkn = tokenize(bundlePath, "/");
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
        configDir = "";
    }

    //Config folder (config/orogen)
    subpath = fs::path("config") / "orogen";
    orogenConfigDir = (path / subpath).string();
    if(!fs::exists(orogenConfigDir)){
        orogenConfigDir = "";
    }

    //Data folder
    subpath = fs::path("data");
    dataDir = (path / subpath).string();
    if(!fs::exists(dataDir)){
        dataDir = "";
    }

    //Existence of log dir is not validated because there is no requirement for
    //it to exist. It often gets deleted when deleting log files
    logBaseDir = (fs::path(path) / "logs").string();
}

Bundle::Bundle()
{
}

std::string Bundle::getSelectedBundleName()
{
    const char *activeBundleC = getenv("ROCK_BUNDLE");
    if(activeBundleC){
        return activeBundleC;
    }else{
        return "";
    }
}

bool Bundle::isBundleSelected()
{
    return !getSelectedBundleName().empty();
}

bool Bundle::setSelectedBundle(const std::string& bundle_name)
{
    if(!findBundle(bundle_name).empty()){
        setenv("ROCK_BUNDLE", bundle_name.c_str(), true);
        return true;
    }else{
        LOG_ERROR_S << "Could not select bundle " << bundle_name << ", because no bundle with that name could be found in ROCK_BUNDLE_PATH";
        return false;
    }
}

void Bundle::unselectBundle()
{
    setenv("ROCK_BUNDLE", "", true);
}

std::vector<SingleBundle> Bundle::getAvailableBundles()
{
    std::vector<SingleBundle> ret;
    std::vector<std::string> bnames = getAvailableBundleNames();
    for(const std::string& bname : bnames)
    {
        ret.push_back(SingleBundle::fromNameAndSearchPaths(bname, bundleSearchPaths()));
    }
    return ret;
}

std::vector<std::string> Bundle::getAvailableBundleNames()
{
    std::vector<std::string> ret;
    for(const std::string& bp : findFoldersInSearchPaths(bundleSearchPaths()))
    {
        ret.push_back(fs::path(bp).filename().string());
    }
    return ret;
}

std::vector<std::string> Bundle::tokenize(std::string data, std::string delim)
{
    boost::char_separator<char> sep(delim.c_str());
    boost::tokenizer<boost::char_separator<char> > tokens(data, sep);

    std::vector<std::string> ret;
    for(const std::string &token : tokens)
    {
        ret.push_back(token);
    }
    return ret;
}

std::string Bundle::findBundle(const std::string &bundle_name)
{
    SingleBundle b = SingleBundle::fromNameAndSearchPaths(bundle_name,
                                                          bundleSearchPaths());
    return b.path;
}

std::vector<std::string> Bundle::bundleSearchPaths()
{
    if(!_bundleSearchPaths.empty()){
        return _bundleSearchPaths;
    }

    const char *pathsC = getenv("ROCK_BUNDLE_PATH");
    if(pathsC)
    {
        std::string paths = pathsC;
        _bundleSearchPaths = tokenize(paths, ":");
    }else
    {
        std::clog << "No bundle search path set. Consider setting environment "\
                     "variable ROCK_BUNDLE_PATH." << std::endl;
    }
    return _bundleSearchPaths;
}


bool Bundle::initialized() const
{
    return activeBundles.size() > 0;
}

std::string time_to_string(timeval tv)
{
    int uSecs = tv.tv_usec;

    time_t when = tv.tv_sec;
    struct tm *tm = localtime(&when);

    char time[50];
    strftime(time, 50, "%Y%m%d-%H%M%S", tm);

    char buffer[57];
    sprintf(buffer,"%s.%04d", time, (int) (uSecs/1000.0));
    return std::string(buffer);
}

bool Bundle::createLogDirectory()
{
    base::Time curTime = base::Time::now();
    fs::path logDir = fs::path(selectedBundle().logBaseDir) / time_to_string(curTime.toTimeval());

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

void Bundle::loadTaskConfigurations()
{
    std::vector<std::string> configs = findFilesByExtension(
                (fs::path("config") / "orogen").string(), ".yml");
    taskConfigurations.initialize(configs);
}

bool Bundle::initialize(bool loadTaskConfigs)
{
    LOG_DEBUG_S << "Initializing Bundles";
    activeBundles.clear();
    LOG_DEBUG_S << "Determining selected bundle";
    //Read environment variables:
    //  ROCK_BUNDLE: contains currently selected bundle (name or absolute path)
    //  ROCK_BUNDLE_PATH: contains search paths, where to look for a bundle.
    //                    multiple paths are separated by ':'
    std::string activeBundle = getSelectedBundleName();
    if(activeBundle.empty())
    {
        LOG_ERROR_S << "ROCK_BUNDLE not set. Bundle cannot be initialized.";
        return false;
    }

    
    //Determine if ROCK_BUNDLE contained absolute path or bundle name
    fs::path pathCheck(activeBundle);
    SingleBundle bundle;
    if(pathCheck.is_absolute() && fs::exists(pathCheck))
    {
        //ROCK_BUNDLE contains an absolute path
        bundle = SingleBundle::fromFullPath(pathCheck.string());
    }else
    {
        //ROCK_BUNDLE contains bundle name
        bundle = SingleBundle::fromNameAndSearchPaths(activeBundle,
                                                      bundleSearchPaths());
    }
    LOG_INFO_S << "Currently selected bundle: "<<bundle.name << " at " <<
                 bundle.path;

    activeBundles.push_back(bundle);

    LOG_DEBUG_S << "Discovering bundle dependencies";
    if(bundleSearchPaths().empty())
    {
        LOG_WARN_S << "Bundle search path is not set. Dependency resolutibn " <<
                     "to other bundles is disabled.";
    }else{
        discoverDependencies(selectedBundle(), activeBundles);
    }
    LOG_INFO_S << "Active bundles: ";
    std::string active_bundles_string;
    for(SingleBundle& b : activeBundles){
        active_bundles_string += "   " + b.name + " at " + b.path + "\n";
    }
    LOG_INFO_S << active_bundles_string;

    //Initialze TaskConfigurations
    if(loadTaskConfigs){
        LOG_DEBUG_S << "Loading task configuration files from bundle";
        loadTaskConfigurations();
    }
    LOG_INFO_S << "Bundles successfully initialized";

    return true;
}

const std::string &Bundle::getActiveBundleName()
{
	return selectedBundle().name;
}

const std::vector<SingleBundle>& Bundle::getActiveBundles()
{
	return activeBundles;
}

const std::vector<std::string> Bundle::getActiveBundleNames()
{
    std::vector<std::string> ret;
	for(const SingleBundle& sb : getActiveBundles())
    {
        ret.push_back(sb.name);
    }
    return ret;
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

const bool Bundle::hasConfigForTask(const std::string &taskModelName) const
{
    return taskConfigurations.hasConfigForTask(taskModelName);
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


template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

std::string Bundle::findFileByName(const std::string& relativePath)
{
    if (relativePath.empty())
        throw std::runtime_error("The path to the file is empty. relativePath = " + relativePath);

    LOG_DEBUG_S << "relativePath = " << relativePath << std::endl;
    // remove any arguments, e.g. from syscall template
    std::vector<std::string> split_str = split(relativePath, ' ');
    const std::string& relPath = split_str.at(0);
    LOG_DEBUG_S << "relPath = " << relPath << std::endl;
    for(const SingleBundle &bundle : activeBundles)
    {
        fs::path curPath = fs::path(bundle.path) / relPath;
        fs::path fullPath = fs::path(bundle.path) / relativePath;
        if(boost::filesystem::exists(curPath))
            return fullPath.string();
    }
    throw std::runtime_error("Could not find file. relativePath = " + relativePath + ", relPath = " + relPath);
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
        std::runtime_error(std::string() + "Could not find configuration file for task " +
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
        LOG_INFO_S << "Bundle '" + bundle.name + "' does not contain a bundle" <<
                     " configuration file '."<<config_file;
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
                                                              bundleSearchPaths());
        dependencies.push_back(d);
        to_inspect.push_back(d);
    }

    for(const SingleBundle& bundle : to_inspect){
        discoverDependencies(bundle, dependencies);
    }
}

std::vector<std::string> Bundle::loadDependenciesFromYAML(const std::string &config_file)
{
    LOG_DEBUG_S << "Parsing bundle configuration file " << config_file;
    YAML::Node node;
    try{
        node = YAML::LoadFile(config_file);
    }catch(std::runtime_error& ex){
        LOG_FATAL_S << "Could not parse bundle config file " << config_file <<
                     ": " << ex.what();
        throw(ex);
    }

    std::vector<std::string> deps;
    try{
        if(node["bundle"] && node["bundle"]["dependencies"]){
            deps = node["bundle"]["dependencies"].as<std::vector<std::string>>();
        }
    }catch(std::runtime_error& ex){
        LOG_ERROR_S << "Error extracting dependecies from " << config_file;
    }

    // Erase duplicates
    std::vector<std::string> ret;
    for(const std::string& d : deps){
        if(std::find(ret.begin(), ret.end(), d) == ret.end())
            ret.push_back(d);
    }
    return ret;
}


TaskConfigurations::TaskConfigurations()
{

}

void TaskConfigurations::initialize(const std::vector<std::string> &configFiles)
{
    taskConfigurations.clear();

    for(const std::string& cfgFilePath : configFiles)
    {
        MultiSectionConfiguration cfgFile;
        LOG_DEBUG_S << "Loading config file " << cfgFilePath;
        bool st = cfgFile.loadFromBundle(cfgFilePath);
        if(!st){
            LOG_WARN_S << "File " << cfgFilePath << " could not be parsed";
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
                                            const std::vector<std::string> &sections) const
{
    return getMultiConfig( taskModelName ).getConfig( sections );
}

const MultiSectionConfiguration &TaskConfigurations::getMultiConfig(const std::string &taskModelName) const
{
    if(hasConfigForTask(taskModelName)){
        return taskConfigurations.at(taskModelName);
    }
    else{
        throw std::out_of_range("No task configuration for task model name " + taskModelName + " found.");
    }
}

const bool TaskConfigurations::hasConfigForTask(const std::string &taskModelName) const
{
    return taskConfigurations.find( taskModelName ) != taskConfigurations.end();
}
