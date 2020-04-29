#include "YAMLConfiguration.hpp"
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <iostream>
#include "Bundle.hpp"

using namespace libConfig;

void YAMLConfigParser::printNode(const YAML::Node &node, int level)
{
    for(int i = 0; i < level; i++)
        std::cout << "  ";

    switch(node.Type())
    {
        case YAML::NodeType::Undefined:
            std::cout << "a Undefined Node" << std::endl;
            break;
        case YAML::NodeType::Scalar:
        {
            std::cout << "a Scalar: "<< node.as<std::string>() << std::endl;
        }
            break;
        case YAML::NodeType::Sequence:
            std::cout << "a Sequence: " << node.Tag() << std::endl;
            break;
        case YAML::NodeType::Map:
            std::cout << "a Map: " << node.Tag() << std::endl;
            displayMap(node, level + 1);
            break;
        case YAML::NodeType::Null:
            break;
    }
}

bool YAMLConfigParser::insetMapIntoArray(const YAML::Node& map, ComplexConfigValue& array)
{
    for(const auto &it : map)
    {
        std::string memberName = it.first.as<std::string>();
//         std::cout << "Name : " << memberName << std::endl;
        std::shared_ptr<ConfigValue> val = getConfigValue(it.second);
        
        if(!val)
        {
            std::cout << "Warning, could not get config value for " << memberName << std::endl;
            continue;
        }
        
        val->setName(memberName);

        array.addValue(memberName, val);
    }
    return !array.getValues().empty();
}


bool YAMLConfigParser::insetMapIntoArray(const YAML::Node &map, ArrayConfigValue &array)
{
    for(const auto &it : map)
    {
        std::string memberName = it.first.as<std::string>();
//         std::cout << "Name : " << memberName << std::endl;
        std::shared_ptr<ConfigValue> val = getConfigValue(it.second);
        
        if(!val)
        {
            std::cout << "Warning, could not get config value for " << memberName << std::endl;
            continue;
        }
        
        val->setName(memberName);

        array.addValue(val);
    }
    return !array.getValues().empty();
}

bool YAMLConfigParser::insetMapIntoArray(const YAML::Node& map, Configuration& conf)
{
    for(const auto &it : map)
    {
        std::string memberName = it.first.as<std::string>();
//         std::cout << "Name : " << memberName << std::endl;
        std::shared_ptr<ConfigValue> val = getConfigValue(it.second);
        
        if(!val)
        {
            std::cout << "Warning, could not get config value for " << memberName << std::endl;
            continue;
        }
        
        val->setName(memberName);

        conf.addValue(memberName, val);
    }
    return !conf.getValues().empty();

}       

std::shared_ptr<ConfigValue> YAMLConfigParser::getConfigValue(const YAML::Node &node)
{
    switch(node.Type())
    {
        case YAML::NodeType::Scalar:
        {
            std::shared_ptr<ConfigValue> conf;
    	    std::string s =node.as<std::string>();
	        if(s.compare(".nan")==0) {
	            s="nan";
	        }
            conf.reset(new SimpleConfigValue(s));
            return conf;
        }
            break;
        case YAML::NodeType::Sequence:
//             std::cout << "a Sequence: " << node.Tag() << std::endl;
            {
                std::shared_ptr<ArrayConfigValue> values = std::make_shared<ArrayConfigValue>();
                for(const auto it : node)
                {
                    std::shared_ptr<ConfigValue> curConf = getConfigValue(it);
                    
                    values->addValue(curConf);
                }
                return values;
            }
            break;
        case YAML::NodeType::Map:
        {
//             std::cout << "a Map: " << node.Tag() << std::endl;
            std::shared_ptr<ComplexConfigValue> mapValue = std::make_shared<ComplexConfigValue>();
            if(!insetMapIntoArray(node, *mapValue))
            {
                return std::shared_ptr<ConfigValue>();
            }
            return mapValue;
            break;
        }
        case YAML::NodeType::Undefined:
            std::cout << "Undefined" << std::endl;
            break;
        case YAML::NodeType::Null:
            std::cout << "NULL" << std::endl;
            break;
    }
    return nullptr;
}

std::shared_ptr< ConfigValue > YAMLConfigParser::getConfigValue(const std::string& ymlString)
{
    YAML::Node doc = YAML::Load(ymlString);

    return getConfigValue(doc);
}

std::shared_ptr<ComplexConfigValue> YAMLConfigParser::getMap(const YAML::Node &map)
{
    std::shared_ptr<ComplexConfigValue> mapValue = std::make_shared<ComplexConfigValue>();
    if(!insetMapIntoArray(map, *mapValue))
    {
        return nullptr;
    }

    return mapValue;
}

void YAMLConfigParser::displayMap(const YAML::Node &map, int level)
{
    for(const auto &it : map)
    {
        for(int i = 0; i < level; i++)
            std::cout << "  ";
        
        std::string value = it.first.as<std::string>();
        std::cout << "Value of first " << value << std::endl;
        printNode(it.second, level + 1);
    }
    
}

bool YAMLConfigParser::parseAndInsert(const std::string& configName, const std::string& ymlString, std::map< std::string, Configuration >& subConfigs)
{
    std::string afterInsertion = applyStringVariableInsertions(ymlString);

    Configuration config(configName);
    
    try {
        if(!parseYAML(config, afterInsertion))
            return false;
    } catch (std::runtime_error &e)
    {
        std::cout << "Error loading sub config << '" << configName << std::endl << "    " << e.what() << std::endl;
        std::cout << "YML of subconfig was :"  << std::endl << afterInsertion << std::endl;
        return false;
    }
    subConfigs.insert(std::make_pair(config.getName(), config));
    
    return true;
}

bool YAMLConfigParser::loadConfigFile(const std::string& pathStr, std::map<std::string, Configuration> &subConfigs)
{
    using namespace boost::filesystem;
    path path(pathStr);
    if(!exists(path))
    {
        throw std::runtime_error(std::string("Error, could not find config file ") + path.c_str());
    }

    std::ifstream fin(path.c_str());
    bool st = loadConfig(fin, subConfigs);
    fin.close();
    return st;
}

bool libConfig::YAMLConfigParser::loadConfigString(const std::string &yamlstring, std::map<std::string, libConfig::Configuration> &subConfigs)
{
    std::istringstream ss;
    ss.str(yamlstring);
    return loadConfig(ss, subConfigs);
}
template <typename T>
bool libConfig::YAMLConfigParser::loadConfig(T &stream, std::map<std::string, libConfig::Configuration> &subConfigs)
{
    //as this is non standard yml, we need to load and parse the config file first
    std::string line;

    std::string buffer;

    std::string configName;

    while(std::getline(stream, line))
    {
        if(line.size() >= 3 && line.at(0) == '-'  && line.at(1) == '-'  && line.at(2) == '-' )
        {
            //found new subsection
//             std::cout << "found subsection " << line << std::endl;

            std::string searched("--- name:");
            if(!line.compare(0, searched.size(), searched))
            {
                if(!configName.empty())
                {
                    if(!parseAndInsert(configName, buffer, subConfigs))
                        return false;
                    buffer.clear();
                }

                configName = line.substr(searched.size(), line.size());

//                 std::cout << "Found new configuration " << curConfig.name << std::endl;
            } else
            {
                std::cout << "Error, sections must begin with '--- name:<SectionName>'" << std::endl;
                return false;
            }

        } else
        {
            //line belongs to the last detected section, add it to the buffer
            buffer.append(line);
            buffer.append("\n");
        }
    }

    if(!configName.empty())
    {
        if(!parseAndInsert(configName, buffer, subConfigs))
            return false;
    }

//     for(std::map<std::string, Configuration>::const_iterator it = subConfigs.begin(); it != subConfigs.end(); it++)
//     {
//         std::cout << "Cur conf \"" << it->first << "\"" << std::endl;
//         displayConfiguration(it->second);
//     }

    return true;
}

bool YAMLConfigParser::parseYAML(Configuration& curConfig, const std::string& yamlBuffer)
{
    std::vector<YAML::Node> docs = YAML::LoadAll(yamlBuffer);

    for(const YAML::Node &doc : docs)
    {
        if(doc.Type() != YAML::NodeType::Map)
        {
            std::cout << "Error, configurations section should only contain yml maps" << std::endl;
            return false;
        }
        
        if(doc.Type() == YAML::NodeType::Map)
        {
            if(!insetMapIntoArray(doc, curConfig))
            {
                std::cout << "Warning, could not parse config" << std::endl;
            }
        }
    }
    
    return true;
}

std::string YAMLConfigParser::applyStringVariableInsertions(const std::string& val)
{
    std::string retVal;
    std::vector<std::string> items;
    
    boost::regex outerRegex("<%=?\\s*(.*?)\\s*%?>");
    
    //Note, this needs to be defined outside of the lambdas for regex_replace
    //if this is not the case a reference to a stack varaiable is returned, 
    //resulting in a segfault
    std::string ret;
    
    auto innerReplace = [&](const boost::smatch &innerMatch) {
        if(!innerMatch[3].str().empty() || !innerMatch[4].str().empty() )
        {
            std::string var;
            if(innerMatch[3].str().empty())
                var = innerMatch[4];
            else
                var = innerMatch[3];
            
            char *envVal = std::getenv(var.c_str());
            if(!envVal)
                throw std::runtime_error("YAML Parser: Error, could not resolve environment variable " + var + " (from " + innerMatch[0] + ")");
            //add variable to output string:
            ret = envVal;
            return ret;
        }
        
        if(!innerMatch[6].str().empty() || !innerMatch[7].str().empty() )
        {
            std::string var;
            if(innerMatch[6].str().empty())
                var = innerMatch[7];
            else
                var = innerMatch[6];
            
            std::string file;
            try{
                file = Bundle::getInstance().findFileByName(var);
            }
            catch (...)
            {
            }
            
            if(file.empty())
                throw std::runtime_error("YAML Parser: Error, could not find file " + var + " (from " + innerMatch[0] + ")");
            //add variable to output string:
            ret = file;
            return ret;
        }
        
        throw std::runtime_error("YAML Parser: Error, could not evaluate statement: " + innerMatch[0]);
    };
    
    retVal = boost::regex_replace(val, outerRegex, [&](const boost::smatch &match) {
        if(match.size() <= 1)
            throw std::runtime_error("YAMLConfigParser::Error empty <% > sequence");
        
        std::string in = boost::regex_replace(match[1].str(), boost::regex("#\\{(.*)?\\}"), "$1");
        
        std::string innerMatcher("(\\[\\s*(?:\"|\')?(.*?)(?:\"|\')?\\s*\\]|\\(\\s*(?:\"|\')?(.*?)(?:\"|\')?\\s*\\))");
        ret = boost::regex_replace(in, boost::regex("(ENV" + innerMatcher + "|(?:BUNDLES|Bundles.find_file|Bundles.find_dir)" + innerMatcher + ")"), innerReplace);
        return ret;
    }
    );

    return retVal;
}
