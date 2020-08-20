#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <yaml-cpp/yaml.h>

namespace libConfig
{

class ConfigValue
{
public:
    enum Type {
        SIMPLE,
        COMPLEX,
        ARRAY,
    };
    
    //Priority of other value is higher. i.e. values specified in other
    //replace values specified in this
    virtual bool merge(std::shared_ptr<ConfigValue> other) = 0;
    
    // returns a deep copy of the object
    virtual std::shared_ptr<ConfigValue> clone() = 0;

    const std::string &getName() const;
    const Type &getType() const;
    
    void setName(const std::string &name);
    void setCxxTypeName(const std::string &name);
    const std::string &getCxxTypeName();
    
    virtual void print(std::ostream &stream, int level = 0) const = 0;

    virtual ~ConfigValue();    
protected:
    enum Type type;
    std::string name;
    //C++ type representation name
    std::string cxxTypeName;
    ConfigValue(enum Type);
    friend YAML::Emitter& operator << (YAML::Emitter& out, const std::shared_ptr<ConfigValue>& v);
};
YAML::Emitter& operator << (YAML::Emitter& out, const std::shared_ptr<ConfigValue>& v);

class SimpleConfigValue : public ConfigValue
{
public:
    SimpleConfigValue(const std::string &value);
    SimpleConfigValue();
    virtual ~SimpleConfigValue();
    virtual void print(std::ostream &stream, int level = 0) const;
    virtual bool merge(std::shared_ptr<ConfigValue> other);
    virtual std::shared_ptr<ConfigValue> clone();
    
    const std::string &getValue() const;
private:
    std::string value;
    friend YAML::Emitter& operator << (YAML::Emitter& out, const SimpleConfigValue& v);
};
YAML::Emitter& operator << (YAML::Emitter& out, const SimpleConfigValue& v);

class ComplexConfigValue : public ConfigValue
{
public:
    ComplexConfigValue();
    virtual ~ComplexConfigValue();
    virtual void print(std::ostream &stream, int level = 0) const;
    virtual bool merge(std::shared_ptr<ConfigValue> other);
    virtual std::shared_ptr<ConfigValue> clone();
    const std::map<std::string, std::shared_ptr<ConfigValue>> &getValues() const;
    void addValue(const std::string &name, std::shared_ptr<ConfigValue> value);
private:
    friend YAML::Emitter& operator << (YAML::Emitter& out, const ComplexConfigValue& v);
    std::map<std::string, std::shared_ptr<ConfigValue>> values;
};


class ArrayConfigValue : public ConfigValue
{
public:
    ArrayConfigValue();
    virtual ~ArrayConfigValue();
    virtual void print(std::ostream &stream, int level = 0) const;
    virtual bool merge(std::shared_ptr<ConfigValue> other);
    virtual std::shared_ptr<ConfigValue> clone();
    const std::vector<std::shared_ptr<ConfigValue> > getValues() const;
    void addValue(std::shared_ptr<ConfigValue> value);
private:
    std::vector<std::shared_ptr<ConfigValue> > values;
    friend YAML::Emitter& operator << (YAML::Emitter& out, const ArrayConfigValue& v);
};
YAML::Emitter& operator << (YAML::Emitter& out, const ArrayConfigValue& v);

class Configuration
{
public:
    Configuration(const std::string &name);
    Configuration();
    ~Configuration();
    
    void print(std::ostream &stream = std::cout) const;

    bool fillFromYaml(const std::string &yml);
    std::string toYaml();
    //Other configuration has higher priority. I.e. values in other replace
    //values in this.
    bool merge(const Configuration &other);
    
    const std::string &getName() const;
    const std::map<std::string, std::shared_ptr<ConfigValue> > &getValues() const;
    void addValue(const std::string &name, std::shared_ptr<ConfigValue> value);    
private:
    std::string name;
    std::map<std::string, std::shared_ptr<ConfigValue> > values;
    friend YAML::Emitter& operator << (YAML::Emitter& out, const Configuration& v);
};
YAML::Emitter& operator << (YAML::Emitter& out, const Configuration& v);
std::ostream& operator << (std::ostream& stream, const Configuration &conf);

class MultiSectionConfiguration
{
public:
    MultiSectionConfiguration();
    //This funciton is DEPRECATED. Don't use it anymore. Use loadFromBundle or
    //loadNoBundle instead
    bool load(std::string filepath);
    //Task Model is extracted from file name pattern used in bundle
    //example: camera_usb::Task.yml
    bool loadFromBundle(std::string filepath);
    bool loadNoBundle(std::string filepath, std::string taskModelName="");
    //Sections should be sorted with increasing priority.
    //e.g. [default,specific,more_specific]
    //here default has lowest and more_specific highest priority
    Configuration getConfig(const std::vector<std::string> &sections) const;
    bool mergeConfigFile(const MultiSectionConfiguration& lowerPriorityFile);
    std::string taskModelName;
    const std::map<std::string, Configuration>& getSubsections();
protected:
    //Maps a configuration subsection name to Configuration
    std::map<std::string, Configuration> subsections;
    //std::string filepath;
};


}
