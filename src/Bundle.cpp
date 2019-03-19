#include "Bundle.hpp"
#include <stdlib.h>
#include <stdexcept>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <base/Time.hpp>
#include <yaml-cpp/yaml.h>

using namespace libConfig;

Bundle * Bundle::instance;

Bundle::Bundle()
{
    const char *activeBundleC = getenv("ROCK_BUNDLE");
    if(!activeBundleC)
    {
        throw std::runtime_error("Error, no active bundle configured. Please use 'rock-bundle-default' to set one.");
    }
    activeBundle = activeBundleC;

    boost::filesystem::path abs = boost::filesystem::absolute(activeBundle);

    if(abs == activeBundle && boost::filesystem::exists(activeBundle))
    {
        //new style, ROCK_BUNDLE contains full path
        activeBundlePath = activeBundle;
        
        boost::char_separator<char> sep("/");
        boost::tokenizer<boost::char_separator<char> > tokens(activeBundle, sep);
        
        //a bit inefective, but ok for this use case
        for(const std::string &token : tokens)
        {
            activeBundle = token;
        }        
    }
    else
    {
        //old style, we need to build the path our own
        const char *pathsC = getenv("ROCK_BUNDLE_PATH");
        if(!pathsC)
        {
            throw std::runtime_error("Internal Error, no bundle path found.");
        }
        std::string paths = pathsC;
        
        boost::char_separator<char> sep(":");
        boost::tokenizer<boost::char_separator<char> > tokens(paths, sep);

        for(const std::string &path : tokens)
        {
            bundlePaths.push_back(path);
            
            std::string candidate = path + "/" + activeBundle;
            
            if(boost::filesystem::exists(candidate))
            {
                activeBundlePath = candidate;
            }
        }
        
        if(activeBundlePath.empty())
        {
            std::cout << pathsC << " active bundle " << activeBundleC << std::endl; 
            throw std::runtime_error("Error, could not determine bundle path");
        }
    }
    
    configDir = activeBundlePath + "/config/orogen/";
    dataDir = activeBundlePath + "/data/";

    activeBundles.push_back(activeBundle);
    discoverDependencies(activeBundle, activeBundles);

    for(const std::string& b : activeBundles)
        activeBundlePaths.push_back(findBundle(b));
}

bool Bundle::createLogDirectory()
{
    base::Time curTime = base::Time::now();
    logDir = activeBundlePath + "/logs/" + curTime.toString(base::Time::Seconds, "%Y%m%d-%H%M");

    //use other directory if log dir already exists
    std::string base = logDir;
    int i = 1;
    while(boost::filesystem::exists(logDir))
    {
        logDir = base + "." + boost::lexical_cast<std::string>(i);
        i++;
    }

    if(!boost::filesystem::create_directories(logDir))
    {
        throw std::runtime_error("Failed to create log directory :" + logDir);
    }
    
    const std::string currentPath(activeBundlePath + "/logs/current");
    
    //create symlink to current
    if(boost::filesystem::is_symlink(currentPath))
    {
        boost::filesystem::remove(currentPath);
    }

    boost::filesystem::create_directory_symlink(logDir, currentPath);
    
    return true;
}


Bundle& Bundle::getInstance()
{
    if(!instance)
        instance = new Bundle();
    
    return *instance;
}

Bundle &Bundle::deleteInstance()
{
    delete instance;
    instance = nullptr;
}

const std::string &Bundle::getActiveBundleName()
{
    return activeBundle;
}

const std::string &Bundle::getConfigurationDirectory()
{
    return configDir;
}

std::string Bundle::getConfigurationPath(const std::string &task)
{
    std::string relativePath = "config/orogen/"+ task + ".yml";
    try{
        std::string result = findFile(relativePath);
        return result;
    } catch (std::runtime_error &e){
        throw std::runtime_error("Bundle::getConfigurationPath : Error, could not find config path for task " + task);
    }
}

const std::string& Bundle::getLogDirectory()
{
    if(logDir.empty())
        createLogDirectory();
    
    return logDir;
}

const std::string& Bundle::getDataDirectory()
{
    return dataDir;
}

std::string Bundle::findFile(const std::string& relativePath)
{
    std::string curPath = activeBundlePath + "/" + relativePath;
    if(boost::filesystem::exists(curPath))
        return curPath;

    for(const std::string &path : activeBundlePaths)
    {
        curPath = path + "/" + relativePath;
        if(boost::filesystem::exists(curPath))
            return curPath;
    }
    throw std::runtime_error("Bundle::findFile : Error, could not find file " + relativePath);
}

std::vector<std::string> Bundle::getConfigurationPaths(const std::string &task_model_name)
{
    std::string relativePath = "config/orogen/"+ task_model_name + ".yml";
    std::vector<std::string> paths = findFiles(relativePath);
    if(paths.empty())
        std::runtime_error("Bundle::getConfigurationPaths: Could not find configuration file for task " + task_model_name + " in any of the active bundles");
    return paths;
}

std::vector<std::string> Bundle::findFiles(const std::string& relativePath)
{
    std::vector<std::string> paths;
    for(const std::string &path : activeBundlePaths)
    {
        std::string curPath = path + "/" + relativePath;
        if(boost::filesystem::exists(curPath))
            paths.push_back(curPath);
    }
    return paths;
}

void Bundle::discoverDependencies(const std::string &bundle_name, std::vector<std::string> &dependencies)
{
    const std::string &bundle_path = findBundle(bundle_name);
    if(bundle_path.empty())
        throw std::runtime_error("Bundle::discoverDependencies: Bundle " + bundle_name + " could not be found.");

    const std::string &config_file = bundle_path + "/config/bundle.yml";
    if(!boost::filesystem::exists(config_file)) // No bundle.yml file exists. We assume that this bundle has no depending bundles
        return;

    std::vector<std::string> deps = loadDependenciesFromYAML(config_file);
    dependencies.insert(dependencies.end(), deps.begin(), deps.end());
    for(const std::string& d : deps)
    {
        // Don't consider duplicates and avoid cyclic dependencies.
        // TODO: When we do it this way, cyclic dependencies are ignored. Better throw here?
        if(std::find(dependencies.begin(), dependencies.end(), d) != dependencies.end())
            continue;

        dependencies.push_back(d);
        discoverDependencies(d, dependencies);
    }
}

std::vector<std::string> Bundle::loadDependenciesFromYAML(const std::string &config_file)
{
    YAML::Node node = YAML::LoadFile(config_file);

    std::vector<std::string> deps;
    if(node["bundle"]["dependencies"])
        deps = node["bundle"]["dependencies"].as<std::vector<std::string>>();

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
    for(const std::string &bp : bundlePaths)
    {
        const std::string curPath = bp + "/" + bundle_name;
        if(boost::filesystem::is_directory(curPath))
            return curPath;
    }
    return "";
}
