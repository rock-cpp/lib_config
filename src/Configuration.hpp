#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>

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
    
    virtual bool merge(std::shared_ptr<ConfigValue> other) = 0;
    
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
};

class SimpleConfigValue : public ConfigValue
{
public:
    SimpleConfigValue(const std::string &value);
    virtual ~SimpleConfigValue();
    virtual void print(std::ostream &stream, int level = 0) const;
    virtual bool merge(std::shared_ptr<ConfigValue> other);
    
    const std::string &getValue() const;
private:
    std::string value;
};

class ComplexConfigValue : public ConfigValue
{
public:
    ComplexConfigValue();
    virtual ~ComplexConfigValue();
    virtual void print(std::ostream &stream, int level = 0) const;
    virtual bool merge(std::shared_ptr<ConfigValue> other);
    const std::map<std::string, std::shared_ptr<ConfigValue>> &getValues() const;
    void addValue(const std::string &name, std::shared_ptr<ConfigValue> value);
private:
    std::map<std::string, std::shared_ptr<ConfigValue>> values;
};

class ArrayConfigValue : public ConfigValue
{
public:
    ArrayConfigValue();
    virtual ~ArrayConfigValue();
    virtual void print(std::ostream &stream, int level = 0) const;
    virtual bool merge(std::shared_ptr<ConfigValue> other);
    const std::vector<std::shared_ptr<ConfigValue> > getValues() const;
    void addValue(std::shared_ptr<ConfigValue> value);
private:
    std::vector<std::shared_ptr<ConfigValue> > values;
};

class Configuration
{
public:
    Configuration(const std::string &name);
    ~Configuration();
    
    void print(std::ostream &stream = std::cout) const;
    bool fillFromYaml(const std::string &yml);
    bool merge(const Configuration &other);
    
    const std::string &getName() const;
    const std::map<std::string, std::shared_ptr<ConfigValue> > &getValues() const;
    void addValue(const std::string &name, std::shared_ptr<ConfigValue> value);    
private:
    std::string name;
    std::map<std::string, std::shared_ptr<ConfigValue> > values;
};

std::ostream& operator <<(std::ostream& stream, const Configuration &conf);

}