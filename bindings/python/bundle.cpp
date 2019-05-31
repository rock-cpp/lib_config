#include <boost/python.hpp>
#include "Bundle.hpp"
#include <vector>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
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
            .def(py::vector_indexing_suite<std::vector<std::string> >())
        ;

    py::class_<SingleBundle>("SingleBundle");
    py::class_<Bundle>("Bundle")
            .def("initialize", &Bundle::initialize)
            .def("getActiveBundleName", &Bundle::getActiveBundleName,
                py::return_value_policy<py::reference_existing_object>())
            .def("selectedBundle", &Bundle::selectedBundle,
                 py::return_value_policy<py::reference_existing_object>())
            .def("findFileByName", &Bundle::findFileByName)
            .def("findFilesByName", &Bundle::findFilesByName)
            .def("findFilesByExtension", &Bundle::findFilesByExtension)
            .def("getConfigurationPathsForTaskModel",
                 &Bundle::getConfigurationPathsForTaskModel);
}
