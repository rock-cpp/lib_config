#include "TypelibConfiguration.hpp"
#include <typelib/typevisitor.hh>
#include <boost/lexical_cast.hpp>

namespace libConfig {

std::shared_ptr< ConfigValue > TypelibConfiguration::getFromValue(Typelib::Value& value)
{
    const Typelib::Type &type(value.getType());
    
    switch(type.getCategory())
    {
        case Typelib::Type::Array:
            return getFromArray(value);
            break;
        case Typelib::Type::Compound:
            return getFromCompound(value);
            break;
        case Typelib::Type::Container:
            return getFromContainer(value);
            break;
        case Typelib::Type::Enum:
            return getFromEnum(value);
            break;
        case Typelib::Type::NullType:
            break;
        case Typelib::Type::Numeric:
            return getFromNumeric(value);
            break;
        case Typelib::Type::Opaque:
            break;
        case Typelib::Type::Pointer:
            break;
        default:
            throw std::runtime_error("Got Unsupported Category");

    }
    
    std::shared_ptr<SimpleConfigValue> config(new SimpleConfigValue("Nothing"));
    config->setName("Unsupported");
    
    return config;    
}

template <class T>
std::string getValue(const Typelib::Value& value)
{
    T *val = static_cast<T *>(value.getData());
    return  boost::lexical_cast<std::string>(*val);
}

std::shared_ptr< ConfigValue > TypelibConfiguration::getFromNumeric(Typelib::Value& value)
{
    const Typelib::Numeric &numeric(static_cast<const Typelib::Numeric &>(value.getType()));
    
    std::string valueS;
    
    switch(numeric.getNumericCategory())
    {
        case Typelib::Numeric::Float:
            if(numeric.getSize() == sizeof(float))
            {
                valueS = getValue<float>(value);               
            }
            else
            {
                valueS = getValue<double>(value);               
            }
            break;
        case Typelib::Numeric::SInt:
            switch(numeric.getSize())
            {
                case sizeof(int8_t):
                    valueS = getValue<int8_t>(value);               
                    break;
                case sizeof(int16_t):
                    valueS = getValue<int16_t>(value);               
                    break;
                case sizeof(int32_t):
                    valueS = getValue<int32_t>(value);               
                    break;
                case sizeof(int64_t):
                    valueS = getValue<int64_t>(value);               
                    break;
                default:
                    std::cout << "Error, got integer of unexpected size " << numeric.getSize() << std::endl;
                    throw std::runtime_error("got integer of unexpected size " + numeric.getSize());
                    break;
            }
            break;
        case Typelib::Numeric::UInt:
        {
            switch(numeric.getSize())
            {
                case sizeof(uint8_t):
                    valueS = getValue<uint8_t>(value);               
                    break;
                case sizeof(uint16_t):
                    valueS = getValue<uint16_t>(value);               
                    break;
                case sizeof(uint32_t):
                    valueS = getValue<uint32_t>(value);               
                    break;
                case sizeof(uint64_t):
                    valueS = getValue<uint64_t>(value);               
                    break;
                default:
                    std::cout << "Error, got integer of unexpected size " << numeric.getSize() << std::endl;
                    throw std::runtime_error("got integer of unexpected size " + numeric.getSize());
                    break;
            }
        }
            break;
        case Typelib::Numeric::NumberOfValidCategories:
            throw std::runtime_error("Internal Error: Got invalid Category");
            break;
    }
    
    std::shared_ptr<SimpleConfigValue> config(new SimpleConfigValue(valueS));
    
    config->setCxxTypeName(numeric.getName());

    return config;
}

std::shared_ptr< ConfigValue > TypelibConfiguration::getFromContainer(Typelib::Value& value)
{
    const Typelib::Container &cont = static_cast<const Typelib::Container &>(value.getType());
    const size_t size = cont.getElementCount(value.getData());
    
    if(cont.kind() == "/std/string")
    {
        const std::string *content = static_cast<const std::string *>(value.getData());

        std::shared_ptr<SimpleConfigValue> config(new SimpleConfigValue(*content));
        config->setCxxTypeName(value.getType().getName());
        return config;
    }
    
    //std::vector
    std::shared_ptr<ArrayConfigValue> array(new ArrayConfigValue());
    array->setCxxTypeName(value.getType().getName());
    for(size_t i = 0; i < size; i++)
    {
        Typelib::Value elem = cont.getElement(value.getData(), i);
        array->addValue(getFromValue(elem));
    }

    return array;
}

std::shared_ptr< ConfigValue > TypelibConfiguration::getFromCompound(Typelib::Value& value)
{
    std::shared_ptr<ComplexConfigValue> config(new ComplexConfigValue());
    
    config->setCxxTypeName(value.getType().getName());
    
    const Typelib::Compound &comp = static_cast<const Typelib::Compound &>(value.getType());
     
    uint8_t *data = static_cast<uint8_t *>( value.getData());
    
    for(const Typelib::Field &field: comp.getFields())
    {
        Typelib::Value fieldV(data + field.getOffset(), field.getType());
        std::shared_ptr< ConfigValue > convV = getFromValue(fieldV);
        convV->setName(field.getName());
        config->addValue(field.getName(), convV);
    }
    
    return config;
}

std::shared_ptr< ConfigValue > TypelibConfiguration::getFromArray(Typelib::Value& value)
{
    std::shared_ptr<ArrayConfigValue> config(new ArrayConfigValue());

    const Typelib::Array &array = static_cast<const Typelib::Array &>(value.getType());
    config->setCxxTypeName(array.getName());
    
    void *data = value.getData();
    
    const Typelib::Type &indirect(array.getIndirection());
    
    for(size_t i = 0; i < array.getDimension(); i++)
    {
        Typelib::Value arrayV(static_cast<uint8_t *>(data) + i * indirect.getSize(), indirect);
        config->addValue(getFromValue(arrayV));
    }
    
    return config;
}

std::shared_ptr< ConfigValue > TypelibConfiguration::getFromEnum(Typelib::Value& value)
{
    const Typelib::Enum &enumT = static_cast<const Typelib::Enum &>(value.getType());
    
    Typelib::Enum::integral_type *intVal = (static_cast<Typelib::Enum::integral_type *>(value.getData()));
    
    std::shared_ptr<SimpleConfigValue> config(new SimpleConfigValue(enumT.get(*intVal)));
    config->setCxxTypeName(enumT.getName());
    
    return config;
}
}
