#pragma once

#include <typelib/value.hh>
#include "Configuration.hpp"

namespace libConfig {
class TypelibConfiguration
{
    std::shared_ptr< libConfig::ConfigValue > getFromArray(Typelib::Value& value);
    std::shared_ptr< libConfig::ConfigValue > getFromEnum(Typelib::Value& value);
    std::shared_ptr< libConfig::ConfigValue > getFromCompound(Typelib::Value& value);
    std::shared_ptr< libConfig::ConfigValue > getFromNumeric(Typelib::Value& value);

public:
    std::shared_ptr< libConfig::ConfigValue > getFromValue(Typelib::Value& value);
protected:
    void num();
};
}

