#include <iostream>
#include "YAMLConfiguration.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        std::cout << "Usage : cmd <configFile>" << std::endl;
        return 0;
    }
    
    libConfig::YAMLConfigParser parser;
    
    std::map<std::string, libConfig::Configuration> config;
    
    parser.loadConfigFile(argv[1], config);

    std::cout << "File " << argv[1] << " contains this configurations " << std::endl;

    for(const auto &it : config)
    {
        it.second.print();
    }
    
    return 0;
}
