#include "Configuration.hpp"
#include "YAMLConfiguration.hpp"
#include <iostream>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace libConfig {

YAML::Emitter& operator << (YAML::Emitter& out, const std::shared_ptr<ConfigValue>& v)
{
    std::shared_ptr<ComplexConfigValue> cptr = std::dynamic_pointer_cast<ComplexConfigValue>(v);
    std::shared_ptr<SimpleConfigValue> sptr = std::dynamic_pointer_cast<SimpleConfigValue>(v);
    std::shared_ptr<ArrayConfigValue> aptr = std::dynamic_pointer_cast<ArrayConfigValue>(v);

    if(cptr){
        return out << *cptr;
    }else if (sptr) {
        return out << *sptr;
    }else if (aptr) {
        return out << *aptr;
    }else{
        std::runtime_error("Could not determine ConfigValue type");
    }
    return out; //Never executed, here to remove warning
}

ComplexConfigValue::ComplexConfigValue(): ConfigValue(COMPLEX)
{

}

ComplexConfigValue::~ComplexConfigValue()
{
}


void ComplexConfigValue::print(std::ostream& stream, int level) const
{
    for(int i = 0; i < level; i++)
        stream << "  ";
    
    stream << getName() << ":" << std::endl;
    for(const auto &it : values)
    {
        it.second->print(stream, level + 1);
    }
}

YAML::Emitter& operator << (YAML::Emitter& out, const ComplexConfigValue& v) {
    out << YAML::BeginMap;
    for(const std::pair<const std::string, std::shared_ptr<ConfigValue> >& it : v.values){
        out << YAML::Key << it.first;
        out << YAML::Value << it.second;
    }
    out << YAML::EndMap;
    return out;
}

bool ComplexConfigValue::merge(std::shared_ptr<ConfigValue> other)
{
    if(other->getType() != COMPLEX)
        return false;

    if(name != other->getName())
    {
        throw std::runtime_error("Internal Error, merge between mismatching value");
    }

    const ComplexConfigValue *cother = dynamic_cast<const ComplexConfigValue *>(other.get());
    
    for(const auto &it : cother->values)
    {
        std::map<std::string, std::shared_ptr<ConfigValue> >::iterator entry = values.find(it.first);
        if(entry != values.end())
        {
            if(!entry->second->merge(it.second))
                return false;
        }
        else
        {
            values.insert(std::make_pair(it.first, it.second->clone()));
        }
    }    
    return true;
}

std::shared_ptr<ConfigValue> ComplexConfigValue::clone()
{
    ComplexConfigValue* copy = new ComplexConfigValue();
    copy->type = type;
    copy->name = name;
    copy->cxxTypeName = cxxTypeName;
    for(const auto &it : values)
    {
        copy->values.insert(std::make_pair(it.first, it.second->clone()));
    }
    return std::shared_ptr<ConfigValue>(copy);
}

void ComplexConfigValue::addValue(const std::string &name, std::shared_ptr<ConfigValue> value)
{
    values.insert(std::make_pair(name, value));
}

const std::map< std::string, std::shared_ptr<ConfigValue> >& ComplexConfigValue::getValues() const
{
    return values;
}


ArrayConfigValue::ArrayConfigValue(): ConfigValue(ARRAY)
{

}

ArrayConfigValue::~ArrayConfigValue()
{
}

void ArrayConfigValue::addValue(std::shared_ptr<ConfigValue> value)
{
    values.push_back(value);
}

const std::vector<std::shared_ptr<ConfigValue> > ArrayConfigValue::getValues() const
{
    return values;
}

void ArrayConfigValue::print(std::ostream &stream, int level) const
{
    for(int i = 0; i < level; i++)
        stream << "  ";

    stream << name << ":" << std::endl;
    for(const std::shared_ptr<ConfigValue> &v : values)
    {
        v->print(stream, level + 1);
    }
}

YAML::Emitter &operator <<(YAML::Emitter &out, const ArrayConfigValue &v)
{
    out << YAML::BeginSeq;
    for(const std::shared_ptr<ConfigValue> &value : v.values){
        out << value;
    }
    out << YAML::EndSeq;
    return out;
}

bool ArrayConfigValue::merge(std::shared_ptr< ConfigValue > other)
{
    if(other->getType() != ARRAY)
        return false;

    if(name != other->getName())
    {
        throw std::runtime_error("Internal Error, merge between mismatching value");
    }
    
    const ArrayConfigValue *aother = dynamic_cast<const ArrayConfigValue *>(other.get());
    
    //we only support direct overwrite by index
    for(size_t i = 0; i < aother->values.size(); i++)
    {
        if(i < values.size())
        {
            values[i]->merge(aother->values[i]->clone());
        }
        else
        {
            values.push_back(aother->values[i]->clone());
        }
    }
    
    return true;
}

std::shared_ptr<ConfigValue> ArrayConfigValue::clone()
{
    ArrayConfigValue* copy = new ArrayConfigValue();
    copy->type = type;
    copy->name = name;
    copy->cxxTypeName = cxxTypeName;
    for(const auto& entry : values)
    {
        copy->values.push_back(entry->clone());
    }
    return std::shared_ptr<ConfigValue>(copy);
}


SimpleConfigValue::SimpleConfigValue(const std::string &v): ConfigValue(SIMPLE), value(v)
{

}

SimpleConfigValue::SimpleConfigValue(): ConfigValue(SIMPLE), value("")
{

}

SimpleConfigValue::~SimpleConfigValue()
{

}

const std::string& SimpleConfigValue::getValue() const
{
    return value;
}

void SimpleConfigValue::print(std::ostream &stream, int level) const
{
    for(int i = 0; i < level; i++)
        stream << "  ";
    stream << name << " : " << value << std::endl;
}

YAML::Emitter& operator << (YAML::Emitter& out, const SimpleConfigValue& v) {
    out << v.getValue();
    return out;
}

bool SimpleConfigValue::merge(const std::shared_ptr< ConfigValue > other)
{
    if(other->getType() != SIMPLE)
        return false;
    
    if(name != other->getName())
    {
        throw std::runtime_error("Internal Error, merge between mismatching value");
    }
    
    const SimpleConfigValue *sother = dynamic_cast<const SimpleConfigValue *>(other.get());
    value = sother->value;
    
    return true;
}

std::shared_ptr<ConfigValue> SimpleConfigValue::clone()
{
    SimpleConfigValue* copy = new SimpleConfigValue(value);
    copy->type = type;
    copy->name = name;
    copy->cxxTypeName = cxxTypeName;
    return std::shared_ptr<ConfigValue>(copy);
}

ConfigValue::ConfigValue(Type t) : type(t)
{
    
}

ConfigValue::~ConfigValue()
{

}

const ConfigValue::Type& ConfigValue::getType() const
{
    return type;
}

const std::string& ConfigValue::getName() const
{
    return name;
}

void ConfigValue::setName(const std::string& newName)
{
    name = newName;
}

void ConfigValue::setCxxTypeName(const std::string& name)
{
    cxxTypeName = name;
}

const std::string& ConfigValue::getCxxTypeName()
{
    return cxxTypeName;
}

std::ostream& operator<<(std::ostream& stream, const Configuration& conf)
{
    conf.print(stream);
    
    return stream;
}

void Configuration::print(std::ostream &stream) const
{
    stream << "Configuration name is : " << name << std::endl;
    for(std::map<std::string, std::shared_ptr<ConfigValue> >::const_iterator it = values.begin(); it != values.end(); it++)
    {
        it->second->print(stream, 1);
    }
}

YAML::Emitter& operator << (YAML::Emitter& out, const Configuration& v) {
    out << YAML::BeginMap;
    for(const std::pair<const std::string, std::shared_ptr<ConfigValue>> it : v.values){
        out << YAML::Key << it.first;
        out << YAML::Value << it.second;
    }
    out << YAML::EndMap;
    return out;
}

Configuration::Configuration(const std::string& name) : name(name)
{

}

Configuration::Configuration()
{

}

Configuration::~Configuration()
{
}

const std::map< std::string, std::shared_ptr<ConfigValue> >& Configuration::getValues() const
{
    return values;
}

void Configuration::addValue(const std::string& name, std::shared_ptr<ConfigValue> value)
{
    values.insert(std::make_pair(name, value));
}

const std::string& Configuration::getName() const
{
    return name;
}

bool Configuration::fillFromYaml(const std::string& yml)
{
    values.clear();
    YAMLConfigParser parser;
    std::string afterInsertion = parser.applyStringVariableInsertions(yml);
    bool ret = false;
    
    try {
    ret = parser.parseYAML(*this, afterInsertion);
    }
    catch (...)
    {}
    
    if(!ret)
        std::cout << "Modified YML is " << std::endl << afterInsertion << std::endl;
    return ret;
}

std::string Configuration::toYaml()
{
    YAML::Emitter emitter;
    emitter << *this;
    return emitter.c_str();
}

bool Configuration::merge(const Configuration& other)
{
    std::map<std::string, std::shared_ptr<ConfigValue> >::const_iterator it;
    for(it = other.values.begin(); it != other.values.end(); it++)
    {
        std::map<std::string, std::shared_ptr<ConfigValue> >::iterator entry = values.find(it->first);
        if(entry != values.end())
        {
            if(!entry->second->merge(it->second)){
                std::clog << "Error merging property " << it->first << std::endl;
                return false;
            }
        }
        else
        {
            values.insert(std::make_pair(it->first, it->second->clone()));
        }
    }
    
    return true;
}

MultiSectionConfiguration::MultiSectionConfiguration()
{

}

bool MultiSectionConfiguration::load(std::string filepath)
{
    return loadFromBundle(filepath);
}

bool MultiSectionConfiguration::loadFromBundle(std::string filepath)
{
    taskModelName = fs::path(filepath).stem().string();
    if(taskModelName.find("::") == std::string::npos){
        std::clog << "File " << taskModelName << " does not appear to be a oroGen " <<
                     "configuration file." << std::endl;
        taskModelName = "";
        return false;
    }

    return loadNoBundle(filepath, taskModelName);
}

bool MultiSectionConfiguration::loadNoBundle(std::string filepath, std::string taskModelName)
{
    this->taskModelName = taskModelName;
    libConfig::YAMLConfigParser parser;
    try{
        parser.loadConfigFile(filepath, this->subsections);
    }catch(std::runtime_error &e){
        std::cerr << "Error loading configuration file " << filepath <<
                     std::endl;
        throw e;
    }
    return true;
}

Configuration MultiSectionConfiguration::getConfig(
        const std::vector<std::string>& sections) const
{
    std::string mergedConfigName;
    bool first = true;
    for (const auto &piece : sections){
        if(!first){
            mergedConfigName += ",";
        }
        mergedConfigName += piece;
        first = false;
    };

    Configuration result(mergedConfigName);
    for(const std::string &conf: sections){
        try{
            result.merge(this->subsections.at(conf));
        }catch(std::out_of_range &e){
            throw std::runtime_error(
                        "No configuration section names '" + conf + "' for " +
                        "Task '" + taskModelName + "'");
        }
    }
    return result;
}

bool MultiSectionConfiguration::mergeConfigFile(
        const MultiSectionConfiguration &lowerPriorityFile)
{
    for(const std::pair<std::string, Configuration>& other : lowerPriorityFile.subsections)
    {
        const std::string& sectionName = other.first;
        const Configuration& otherCfg = other.second;
        if(subsections.find(sectionName) == subsections.end()){
            //lowerPrioFile defines a subsection that was not defined before
            subsections.insert(std::make_pair(sectionName, otherCfg));
        }else{
            //lowerPrioFile defines a subsection that was already defined before
            Configuration higher = subsections.at(sectionName);
            Configuration merged(sectionName);
            merged.merge(otherCfg);
            merged.merge(higher);
            subsections[sectionName] = merged;
        }
    }
    return true;
}

const std::map<std::string, Configuration> &MultiSectionConfiguration::getSubsections()
{
    return subsections;
}
}

