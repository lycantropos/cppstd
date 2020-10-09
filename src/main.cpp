#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include <sstream>
#include <stdexcept>
#include <vector>

namespace py = pybind11;

#define MODULE_NAME _cppstd
#define C_STR_HELPER(a) #a
#define C_STR(a) C_STR_HELPER(a)
#define VECTOR_ITERATOR_NAME "VectorIterator"
#define VECTOR_NAME "Vector"
#ifndef VERSION_INFO
#define VERSION_INFO "dev"
#endif

using Object = py::object;
using Vector = std::vector<py::object>;
using VectorIterator = Vector::iterator;

template <class Object>
std::string repr(const Object& object) {
  std::ostringstream stream;
  stream.precision(std::numeric_limits<double>::digits10 + 2);
  stream << object;
  return stream.str();
}

namespace std {
static std::ostream& operator<<(std::ostream& stream, const Vector& vector) {
  stream << C_STR(MODULE_NAME) "." VECTOR_NAME "(";
  if (!vector.empty()) {
    stream << std::string(py::repr(vector[0]));
    for (std::size_t index = 1; index < vector.size(); ++index)
      stream << ", " << std::string(py::repr(vector[index]));
  }
  return stream << ")";
}
}  // namespace std

PYBIND11_MAKE_OPAQUE(Vector);

PYBIND11_MODULE(MODULE_NAME, m) {
  m.doc() = R"pbdoc(Partial binding of C++ standard library.)pbdoc";
  m.attr("__version__") = VERSION_INFO;

  py::class_<Vector>(m, VECTOR_NAME)
      .def(py::init([](py::args args) {
        Vector result;
        for (auto& element : args)
          result.push_back(py::reinterpret_borrow<Object>(element));
        return result;
      }))
      .def("__repr__", repr<Vector>)
      .def("begin", [](Vector& self) { return self.begin(); })
      .def("end", [](Vector& self) { return self.end(); })
      .def("push_back",
           [](Vector& self, Object object) { self.push_back(object); })
      .def("reserve", &Vector::reserve), py::arg("capacity");

  py::class_<VectorIterator>(m, VECTOR_ITERATOR_NAME)
      .def(py::self + std::size_t())
      .def(py::self - std::size_t())
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def(py::self < py::self)
      .def("value", &VectorIterator::operator*);
}
