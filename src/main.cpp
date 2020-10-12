#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace py = pybind11;

#define MODULE_NAME _cppstd
#define C_STR_HELPER(a) #a
#define C_STR(a) C_STR_HELPER(a)
#define VECTOR_BACKWARD_ITERATOR_NAME "VectorBackwardIterator"
#define VECTOR_FORWARD_ITERATOR_NAME "VectorForwardIterator"
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

template <class Collection>
static std::size_t to_size(Collection& collection) {
  return collection.size();
}

template <class Collection>
struct ToBegin {
  typename Collection::const_iterator operator()(
      const Collection& collection) const {
    return std::cbegin(collection);
  }
};

template <class Collection>
struct ToEnd {
  typename Collection::const_iterator operator()(
      const Collection& collection) const {
    return std::cend(collection);
  }
};

template <class Collection>
struct ToReversedBegin {
  typename Collection::const_reverse_iterator operator()(
      const Collection& collection) const {
    return std::crbegin(collection);
  }
};

template <class Collection>
struct ToReversedEnd {
  typename Collection::const_reverse_iterator operator()(
      const Collection& collection) const {
    return std::crend(collection);
  }
};

template <class Collection, bool reversed>
class Iterator {
 public:
  using Position =
      std::conditional_t<reversed, typename Collection::const_reverse_iterator,
                         typename Collection::const_iterator>;

  Iterator(const Collection& collection_)
      : collection(collection_),
        begin(to_actual_begin()),
        position(to_actual_begin()){};

  Iterator(Position&& position, const Collection& collection_)
      : collection(collection_), begin(to_actual_begin()), position(position){};

  Iterator operator+(std::int64_t offset) const { return to_advanced(offset); }

  Iterator operator-(std::int64_t offset) const { return to_advanced(-offset); }

  Iterator& operator+=(std::int64_t offset) {
    advance(offset);
    return *this;
  }

  Iterator& operator-=(std::int64_t offset) {
    advance(-offset);
    return *this;
  }

  bool operator==(const Iterator& other) const {
    return to_actual_position() == other.to_actual_position();
  }

  const typename Collection::value_type& next() {
    actualize();
    if (position == to_actual_end()) throw py::stop_iteration();
    auto current_position = position;
    position = std::next(position);
    return *current_position;
  }

  Position to_actual_position() const {
    const auto& actual_begin = to_actual_begin();
    return begin == actual_begin
               ? position
               : std::next(actual_begin, std::distance(begin, position));
  }

 private:
  using Replenisher = std::conditional_t<reversed, ToReversedBegin<Collection>,
                                         ToBegin<Collection>>;
  using Exhauster = std::conditional_t<reversed, ToReversedEnd<Collection>,
                                       ToEnd<Collection>>;
  const Collection& collection;
  Position begin;
  Position position;

  void actualize() {
    const auto& actual_begin = to_actual_begin();
    if (begin != actual_begin) {
      position = std::next(actual_begin, std::distance(begin, position));
      begin = actual_begin;
    }
  }

  void advance(std::int64_t offset) { position = to_advanced_position(offset); }

  Position to_actual_begin() const {
    static const Replenisher replenish;
    return replenish(collection);
  }

  Position to_actual_end() const {
    static const Exhauster exhaust;
    return exhaust(collection);
  }

  Iterator to_advanced(std::int64_t offset) const {
    return {to_advanced_position(offset), collection};
  }

  Position to_advanced_position(std::int64_t offset) const {
    auto actual_position = to_actual_position();
    std::int64_t min_offset = std::distance(actual_position, to_actual_begin());
    std::int64_t max_offset = std::distance(actual_position, to_actual_end());
    if (offset < min_offset || offset > max_offset) {
      std::int64_t size = to_size(collection);
      throw std::out_of_range(
          size ? (std::string("Offset should be in range(" +
                              std::to_string(min_offset) + ", ") +
                  std::to_string(max_offset + 1) + "), but found " +
                  std::to_string(offset) + ".")
               : std::string("Sequence is empty."));
    }
    return std::next(actual_position, offset);
  }
};

template <class Collection>
using BackwardIterator = Iterator<Collection, true>;

template <class Collection>
using ForwardIterator = Iterator<Collection, false>;

template <class Collection>
static ForwardIterator<Collection> to_forward_iterator(
    const Collection& collection) {
  return {collection};
}

template <class Collection>
static BackwardIterator<Collection> to_backward_iterator(
    const Collection& collection) {
  return {collection};
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

using VectorBackwardIterator = BackwardIterator<Vector>;
using VectorForwardIterator = ForwardIterator<Vector>;

static bool operator<(const VectorForwardIterator& self,
                      const VectorForwardIterator& other) {
  return self.to_actual_position() < other.to_actual_position();
}

static bool operator<(const VectorBackwardIterator& self,
                      const VectorBackwardIterator& other) {
  return self.to_actual_position() < other.to_actual_position();
}

static bool operator<=(const VectorForwardIterator& self,
                       const VectorForwardIterator& other) {
  return self.to_actual_position() <= other.to_actual_position();
}

static bool operator<=(const VectorBackwardIterator& self,
                       const VectorBackwardIterator& other) {
  return self.to_actual_position() <= other.to_actual_position();
}

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
      .def("__iter__", to_forward_iterator<Vector>)
      .def("__len__", to_size<Vector>)
      .def("__repr__", repr<Vector>)
      .def("__reversed__", to_backward_iterator<Vector>)
      .def("begin",
           [](const Vector& self) {
             return VectorForwardIterator(self.begin(), self);
           })
      .def("end",
           [](const Vector& self) {
             return VectorForwardIterator(self.end(), self);
           })
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

  py::class_<VectorForwardIterator>(m, VECTOR_FORWARD_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self + std::int64_t{})
      .def(py::self - std::int64_t{})
      .def(py::self += std::int64_t{})
      .def(py::self -= std::int64_t{})
      .def("__iter__",
           [](VectorForwardIterator& self) -> VectorForwardIterator& {
             return self;
           })
      .def("__next__", &VectorForwardIterator::next);

  py::class_<VectorBackwardIterator>(m, VECTOR_BACKWARD_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self + std::int64_t{})
      .def(py::self - std::int64_t{})
      .def(py::self += std::int64_t{})
      .def(py::self -= std::int64_t{})
      .def("__iter__",
           [](VectorBackwardIterator& self) -> VectorBackwardIterator& {
             return self;
           })
      .def("__next__", &VectorBackwardIterator::next);
}
