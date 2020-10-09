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

template <class Object>
std::string repr(const Object& object) {
  std::ostringstream stream;
  stream.precision(std::numeric_limits<double>::digits10 + 2);
  stream << object;
  return stream.str();
}

template <class Sequence>
static std::size_t to_size(Sequence& sequence) {
  return sequence.size();
}

template <class Collection>
class Iterator {
 public:
  Iterator(const Collection& collection_)
      : position(std::begin(collection_)), collection(collection_){};

  Iterator(typename Collection::const_iterator&& position,
           const Collection& collection_)
      : position(position), collection(collection_){};

  const typename Collection::value_type& next() {
    if (position == std::end(collection)) throw py::stop_iteration();
    return *position++;
  }

  Iterator operator+(std::int64_t offset) const {
    return Iterator(
        position + (offset < 0
                        ? std::max(static_cast<std::int64_t>(std::distance(
                                       position, std::begin(collection))),
                                   offset)
                        : std::min(static_cast<std::int64_t>(std::distance(
                                       position, std::end(collection))),
                                   offset)),
        collection);
  }

  Iterator operator-(std::int64_t offset) const {
    return Iterator(
        position - (offset < 0
                        ? std::max(static_cast<std::int64_t>(std::distance(
                                       std::end(collection), position)),
                                   offset)
                        : std::min(static_cast<std::int64_t>(std::distance(
                                       std::begin(collection), position)),
                                   offset)),
        collection);
  }

  Iterator& operator+=(std::int64_t offset) {
    position += (offset < 0 ? std::max(static_cast<std::int64_t>(std::distance(
                                           position, std::begin(collection))),
                                       offset)
                            : std::min(static_cast<std::int64_t>(std::distance(
                                           position, std::end(collection))),
                                       offset));
    return *this;
  }

  Iterator& operator-=(std::int64_t offset) {
    position -= (offset < 0 ? std::max(static_cast<std::int64_t>(std::distance(
                                           std::end(collection), position)),
                                       offset)
                            : std::min(static_cast<std::int64_t>(std::distance(
                                           std::begin(collection), position)),
                                       offset));
    return *this;
  }

  bool operator==(const Iterator& other) const {
    return position == other.position;
  }

  bool operator<(const Iterator& other) const {
    return position < other.position;
  }

  bool operator<=(const Iterator& other) const {
    return position <= other.position;
  }

 private:
  typename Collection::const_iterator position;
  const Collection& collection;
};

template <class Collection>
static Iterator<Collection> to_iterator(const Collection& collection) {
  return Iterator(collection);
}

template <class Sequence, class Index = std::int64_t>
static const typename Sequence::value_type& to_item(const Sequence& sequence,
                                                    Index index) {
  std::int64_t size = to_size(sequence);
  std::int64_t normalized_index = index >= 0 ? index : index + size;
  if (normalized_index < 0 || normalized_index >= size)
    throw std::out_of_range(size ? (std::string("Index should be in range(" +
                                                std::to_string(-size) + ", ") +
                                    std::to_string(size) + "), but found " +
                                    std::to_string(index) + ".")
                                 : std::string("Sequence is empty."));
  return sequence[normalized_index];
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

using VectorIterator = Iterator<Vector>;

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
      .def("__getitem__", to_item<Vector>, py::arg("index"))
      .def(
          "__getitem__",
          [](Vector& self, py::slice slice) {
            std::size_t raw_start, raw_stop, raw_step, slice_length;
            if (!slice.compute(self.size(), &raw_start, &raw_stop, &raw_step,
                               &slice_length))
              throw py::error_already_set();
            auto start = static_cast<std::int64_t>(raw_start);
            auto stop = static_cast<std::int64_t>(raw_stop);
            auto step = static_cast<std::int64_t>(raw_step);
            Vector result;
            result.reserve(slice_length);
            if (step < 0)
              for (; start > stop; start += step) result.push_back(self[start]);
            else
              for (; start < stop; start += step) result.push_back(self[start]);
            return result;
          },
          py::arg("slice"))
      .def("__iter__", to_iterator<Vector>)
      .def("__len__", to_size<Vector>)
      .def("__repr__", repr<Vector>)
      .def(
          "begin",
          [](const Vector& self) { return VectorIterator(self.begin(), self); })
      .def("end",
           [](const Vector& self) { return VectorIterator(self.end(), self); })
      .def("pop_back",
           [](Vector& self) {
             if (self.empty()) throw std::out_of_range("Vector is empty.");
             self.pop_back();
           })
      .def(
          "push_back",
          [](Vector& self, Object object) { self.push_back(object); },
          py::arg("value"))
      .def("reserve", &Vector::reserve, py::arg("capacity"))
      .def(
          "resize",
          [](Vector& self, std::size_t size, Object value) {
            self.resize(size, value);
          },
          py::arg("size"), py::arg("value") = py::none());

  py::class_<VectorIterator>(m, VECTOR_ITERATOR_NAME)
      .def(py::self + std::int64_t())
      .def(py::self - std::int64_t())
      .def(py::self += std::int64_t())
      .def(py::self -= std::int64_t())
      .def(py::self == py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def("__iter__", [](const VectorIterator& self) { return self; })
      .def("__next__", &VectorIterator::next);
}
