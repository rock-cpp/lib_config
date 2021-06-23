#include <iostream>
#include "Bundle.hpp"
#include <iostream>

void usage()
{
    std::cout <<
    R"USAGE(
rock-bundle is a tool to support working with Rock Bundles and to access data inside of them.

USAGE:
    rock-bundle [MODE] ([ARG1] ...)

MODES:
    info                   : Print info about the currently selected bundle
    selected               : Print the currently selected bundles
    selectedpath           : Print directory of currently selected bundle
    active                 : Print the curently active bundles
    find [FILENAME]        : find all files that match the given name pattern
                             FILENAME within all active bundles
    find1 [FILENAME]       : find single file that match the given name pattern
                             within all active bundles. If multiple occurances
                             of the file are found, the one from the highest
                             level bundle is returned.
    findext (SUBDIR) [EXT] : Find all files woth the extension EXT in all
                             selected bundles. By passing SUBDIR search is
                             limited to the given sub-folder within the bundles

A Bundles is selected by setting the ROCK_BUNDLE environment variable to the name of the bundle.
The name of a bundle is defioned by its folder name. The folder needs to be placed within on of possibly multipe paths that can be defined in the environment varibale ROCK_BUNDLE_PATH.
)USAGE";
}

template <typename T>
std::string str(const std::vector<T> & vec, std::string sep=", ")
{
    std::stringstream ss;
    bool first = true;
    for(auto elem : vec)
    {
        if(first){
            ss << elem;
            first = false;
        }else{
            ss << sep << elem;
        }
    }
    return ss.str();
}

int handle_request(int argc, char** argv)
{
    std::string mode=argv[1];
    if(mode == "info"){
        libConfig::Bundle b;
        b.initialize(false);
        std::cout << "Active bundles: " << str(b.getActiveBundleNames()) << std::endl;
        std::cout << "Available bundles: " << std::endl;
        for(const libConfig::SingleBundle& sb : b.getAvailableBundles())
        {
            std::cout << "  " << sb.name << " (" << sb.path << ")" << std::endl;
        }
        std::cout << "Selected bundle: " << b.getActiveBundleName() << " (" <<  b.getActiveBundles()[0].path << ")" << std::endl;
        return EXIT_SUCCESS;
    }
    else if(mode == "selected")
    {
        libConfig::Bundle b;
        b.initialize(false);
        std::cout << b.getActiveBundles()[0].name << std::endl;
        return EXIT_SUCCESS;
    }
    else if(mode == "selectedpath")
    {
        libConfig::Bundle b;
        b.initialize(false);
        std::cout << b.getActiveBundles()[0].path << std::endl;
        return EXIT_SUCCESS;
    }
    else if(mode == "active")
    {
        libConfig::Bundle b;
        b.initialize(false);
        for (const libConfig::SingleBundle& sb : b.getActiveBundles()){
            std::cout << sb.name << std::endl;
        }
        return EXIT_SUCCESS;
    }
    else if(mode == "find")
    {
        if(argc < 3){
            std::cerr << "No filename to search for was given" << std::endl;
            return EXIT_FAILURE;
        }
        libConfig::Bundle b;
        b.initialize(false);
        std::vector<std::string> res = b.findFilesByName(argv[2]);
        for(const std::string& p : res){
            std::cout << p << std::endl;
        }
    }
    else if(mode == "find1")
    {
        if(argc < 3){
            std::cerr << "No filename to search for was given" << std::endl;
            return EXIT_FAILURE;
        }
        libConfig::Bundle b;
        b.initialize(false);
        std::cout << b.findFileByName(argv[2]) << std::endl;
    }
    else if(mode == "findext")
    {
        std::string ext = "";
        std::string rel_path = "";
        if(argc == 3){
            ext = argv[2];
        }
        else if(argc == 4){
            rel_path = argv[2];
            ext = argv[3];
        }
        else{
            std::cerr << "Wrong number of arguments for 'findext'" << std::endl;
            std::clog << "findext expects one or two arguments. Either \n"
                      << "    findext [EXT]\n"
                      << "or \n"
                      << "    findext [BUNDLE_RELATIVE_SUBFOLDER] [EXT]\n"
                      << "Note that EXT must be given with trailing dot (e.g. "
                      << "'.txt')" << std::endl;
            return EXIT_FAILURE;
        }
        libConfig::Bundle b;
        b.initialize(false);
        std::vector<std::string> res = b.findFilesByExtension(rel_path, ext);
        for(const std::string& p : res){
            std::cout << p << std::endl;
        }
    }
    else
    {
        std::cerr << "Unknown mode '" << mode << "'" <<std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


int main(int argc, char** argv)
{
    if(argc < 2){
        std::cerr << "Mode must be specified as first argument" << std::endl;
        usage();
        return EXIT_FAILURE;
    }

    try{
        return handle_request(argc, argv);
    }
    catch(std::exception& ex){
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
