#include <boost/python.hpp>
#include "Bundle.hpp"
#include "Configuration.hpp"
#include <vector>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
using namespace boost;
namespace py = boost::python;
using namespace libConfig;


template<class T>
py::list std_vector_to_py_list(const std::vector<T>& v)
{
    py::object get_iter = py::iterator<std::vector<T> >();
    py::object iter = get_iter(v);
    py::list l(iter);
    return l;
}

BOOST_PYTHON_MODULE(bundle)
{
    py::class_<std::vector<std::string> >("string_vector")
            .def(py::vector_indexing_suite<std::vector<std::string> >());
    py::class_<std::vector<SingleBundle> >("single_bundle_vector")
            .def(py::vector_indexing_suite<std::vector<SingleBundle> >());

    py::class_<SingleBundle>("SingleBundle")
            .def_readwrite("name", &SingleBundle::name)
            .def_readwrite("path", &SingleBundle::path)
            .def_readwrite("logBaseDir", &SingleBundle::logBaseDir)
            .def_readwrite("dataDir", &SingleBundle::dataDir)
            .def_readwrite("configDir", &SingleBundle::configDir)
            .def_readwrite("orogenConfigDir", &SingleBundle::orogenConfigDir);
    py::class_<Bundle>("Bundle")
            .def("getSelectedBundle", &Bundle::getSelectedBundleName)
            .def("isBundleSelected", &Bundle::isBundleSelected)
            .def("setSelectedBundle", &Bundle::setSelectedBundle)
            .def("unselectBundle", &Bundle::unselectBundle)
            .def("getAvailableBundleNames", &Bundle::getAvailableBundleNames)
            .def("getAvailableBundles", &Bundle::getAvailableBundles)
            .def("findBundle", &Bundle::findBundle)
            .def("bundleSearchPaths", &Bundle::bundleSearchPaths)
            .def("initialize", &Bundle::initialize)
            .def("initialized", &Bundle::initialized)
            .def("getActiveBundleName", &Bundle::getActiveBundleName,
                 py::return_value_policy<py::copy_const_reference>())
            .def("getActiveBundles", &Bundle::getActiveBundles,
                py::return_value_policy<py::reference_existing_object>())
            .def("getActiveBundleNames", &Bundle::getActiveBundleNames)
            .def("selectedBundle", &Bundle::selectedBundle,
                 py::return_value_policy<py::reference_existing_object>())
            .def("getLogDirectory", &Bundle::getLogDirectory)
            .def("getConfigurationPathsForTaskModel",
				 &Bundle::getConfigurationPathsForTaskModel)
            .def("findFileByName", &Bundle::findFileByName)
            .def("findFilesByName", &Bundle::findFilesByName)
            .def("findFilesByExtension", &Bundle::findFilesByExtension)
			.def_readonly("taskConfigurations", &Bundle::taskConfigurations);

    /*py::class_<ConfigValue>("ConfigValue")
        .def("merge", &ConfigValue::merge)
        .def("clone", &ConfigValue::clone)
        .def("getName", &ConfigValue::getName)
        .def("getType", &ConfigValue::getType)
        .def("setName", &ConfigValue::setName)
        .def("setCxxTypeName", &ConfigValue::setCxxTypeName)
        .def("getCxxTypeName", &ConfigValue::getCxxTypeName);*/
    py::class_<SimpleConfigValue>("SimpleConfigValue")
        .def("merge", &SimpleConfigValue::merge)
        .def("clone", &SimpleConfigValue::clone)
        .def("getValue", &SimpleConfigValue::getValue,
            py::return_value_policy<py::copy_const_reference>());
        /*.def("getName", &SimpleConfigValue::getName)
        .def("getType", &SimpleConfigValue::getType)
        .def("setName", &SimpleConfigValue::setName)
        .def("setCxxTypeName", &SimpleConfigValue::setCxxTypeName)
        .def("getCxxTypeName", &ComplexConfigValue::getCxxTypeName)*/
    py::class_<ComplexConfigValue>("ComplexConfigValue")
        .def("merge", &ComplexConfigValue::merge)
        .def("clone", &ComplexConfigValue::clone)
        .def("getValues", &ComplexConfigValue::getValues,
            py::return_value_policy<py::copy_const_reference>())
        .def("addValue", &ComplexConfigValue::addValue);
        /*.def("getName", &SimpleConfigValue::getName)
        .def("getType", &SimpleConfigValue::getType)
        .def("setName", &SimpleConfigValue::setName)
        .def("setCxxTypeName", &SimpleConfigValue::setCxxTypeName)
        .def("getCxxTypeName", &SimpleConfigValue::getCxxTypeName)*/
    py::class_<ArrayConfigValue>("ArrayConfigValue")
        .def("merge", &ArrayConfigValue::merge)
        .def("clone", &ArrayConfigValue::clone)
		.def("getValues", &ArrayConfigValue::getValues,
			 py::return_value_policy<py::copy_const_reference>())
        .def("addValue", &ArrayConfigValue::addValue);
        /*.def("getName", &SimpleConfigValue::getName)
        .def("getType", &SimpleConfigValue::getType)
        .def("setName", &SimpleConfigValue::setName)
        .def("setCxxTypeName", &SimpleConfigValue::setCxxTypeName)
        .def("getCxxTypeName", &ComplexConfigValue::getCxxTypeName)*/

    py::class_<Configuration>("Configuration")
        .def("fillFromYaml", &Configuration::fillFromYaml)
        .def("toYaml", &Configuration::toYaml)
        .def("merge", &Configuration::merge)
		.def("getName", &Configuration::getName,
			 py::return_value_policy<py::copy_const_reference>())
		.def("getValues", &Configuration::getValues,
			 py::return_value_policy<py::copy_const_reference>());

    py::class_<std::map<std::string, Configuration> >("ConfigurationMap")
        .def(py::map_indexing_suite<std::map<std::wstring, Configuration> >() );

    py::class_<MultiSectionConfiguration>("MultiSectionConfiguration")
        .def("load", &MultiSectionConfiguration::load)
        .def("loadFromBundle", &MultiSectionConfiguration::loadFromBundle)
        .def("loadNoBundle", &MultiSectionConfiguration::loadNoBundle)
        .def("getConfig", &MultiSectionConfiguration::getConfig)
		.def("mergeConfigFile", &MultiSectionConfiguration::mergeConfigFile)
		.def("getSubsections", &MultiSectionConfiguration::getSubsections,
			py::return_value_policy<py::copy_const_reference>());
    
    py::class_<std::map<std::string, MultiSectionConfiguration> >("TaskConfigurationMap")
        .def(py::map_indexing_suite<std::map<std::wstring, MultiSectionConfiguration> >() );
    
    py::class_<TaskConfigurations>("TaskConfigurations")
        .def("initialize", &TaskConfigurations::initialize)
        .def("getConfig", &TaskConfigurations::getConfig)
        .def("getMultiConfig", &TaskConfigurations::getMultiConfig,
            py::return_value_policy<py::copy_const_reference>());
}
