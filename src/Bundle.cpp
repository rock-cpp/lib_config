#include "Bundle.hpp"
#include <stdlib.h>
#include <stdexcept>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <base/Time.hpp>

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
    for(const std::string &bp: bundlePaths)
    {
        // Find it in the default bundle, in case ACTIVE_BUNDLES is not set
        curPath = bp + "/" +activeBundle + "/" + relativePath;
        if(boost::filesystem::exists(curPath))
            return curPath;
        const char *activeBundleC = getenv("ACTIVE_BUNDLES");
        if (activeBundleC)
        {
            std::string selected_bundles = activeBundleC;
            boost::char_separator<char> sep(":");
            boost::tokenizer<boost::char_separator<char> > tokens(selected_bundles, sep);
            for(const std::string &token : tokens)
            {
                curPath = bp + "/" + token + "/" + relativePath;
                if(boost::filesystem::exists(curPath))
                    return curPath;
            }        
        }
    }
    throw std::runtime_error("Bundle::findFile : Error, could not find file " + relativePath);
}
