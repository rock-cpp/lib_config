#ifndef BUNDLE_H
#define BUNDLE_H

#include <string>
#include <vector>

namespace libConfig
{

class Bundle
{
private:
    Bundle();
    
    static Bundle *instance;
    
    std::string activeBundle;
    std::string activeBundlePath;
    std::vector<std::string> bundlePaths;

    std::vector<std::string> activeBundles;
    std::vector<std::string> activeBundlePaths;
    
    std::string logDir;
    std::string configDir;
    std::string dataDir;
    
    /**
     * This function creates a log directory 
     * under the activeBundlePath in the
     * subdirectory /log/'Year.Month.day-Hour.Minutes'.
     * If the directory for the current time already
     * exists, it will create a directory with a postfix
     * of .X
     * */
    bool createLogDirectory();

public:
    /**
     * @brief Creates singelton class instance
     * @return
     */
    static Bundle &getInstance();
    /**
     * @brief Delete the singleton class
     * @return
     */
    static Bundle &deleteInstance();

    const std::string &getActiveBundleName();
    
    /**
     * Returns the log directory path.
     * If the log directory has not been
     * created yet, it will get created
     * while calling this function.
     * */
    const std::string &getLogDirectory();
    
    /**
     * Returns the path to the directory 
     * (in the currently selected bundle)
     * containing the orogen config files.
     * */
    const std::string &getConfigurationDirectory();

    /**
     * Returns the full path to the configuration file
     * matching the given task model name.
     * It checks all the active bundles,
     * but only returns the first match. Throws
     * and exception if no match was found.
     * */
    std::string getConfigurationPath(const std::string &task);
    
    /**
     * Returns the path to the directory 
     * containing the data files in the active bundle.
     * */
    const std::string &getDataDirectory();

    /**
     * Returns the paths to all configuration files
     * matching the given task model name. It checks
     * all active bundles. The order of the files
     * is the same as the order of active bundles.
     * Throws an exception if no match is found.
     */
    std::vector<std::string> getConfigurationPaths(const std::string &task_model_name);
    
    /**
     * Checks in all active bundles for the relative file path
     * and returns the first match.
     */
    std::string findFile(const std::string &relativePath);

    /**
     * Checks in all active bundles for the relative file path
     * and returns all matches.
     */
    std::vector<std::string> findFiles(const std::string& relativePath);

    /**
     * Checks the bundle paths for the given bundle
     * and returns the full path if found, empty otherwise
     */
    std::string findBundle(const std::string &bundle_name);

    /**
     * Find all dependencies of the given bundle. Ignores cyclic dependencies.
     */
    void discoverDependencies(const std::string &bundle_name, std::vector<std::string> &dependencies);

    /**
     * Load the dependencies from the given bundle config file
     */
    std::vector<std::string> loadDependenciesFromYAML(const std::string &config_file);
};

}//end of namespace

#endif // BUNDLE_H
