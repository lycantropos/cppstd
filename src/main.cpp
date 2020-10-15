#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include <algorithm>
#include <limits>
#include <memory>
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
using Index = Py_ssize_t;

template <class T>
static bool are_addresses_equal(const T& left, const T& right) {
  return std::addressof(left) == std::addressof(right);
}

template <class Type>
std::string repr(const Type& value) {
  std::ostringstream stream;
  stream.precision(std::numeric_limits<double>::digits10 + 2);
  stream << value;
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

  Iterator operator+(Index offset) const { return to_advanced(offset); }

  Iterator operator-(Index offset) const { return to_advanced(-offset); }

  Iterator& operator+=(Index offset) {
    advance(offset);
    return *this;
  }

  Iterator& operator-=(Index offset) {
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

  bool has_same_collection_with(
      const Iterator<Collection, reversed>& other) const {
    return are_addresses_equal(collection, other.collection);
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

  void advance(Index offset) { position = to_advanced_position(offset); }

  Position to_actual_begin() const {
    static const Replenisher replenish;
    return replenish(collection);
  }

  Position to_actual_end() const {
    static const Exhauster exhaust;
    return exhaust(collection);
  }

  Iterator to_advanced(Index offset) const {
    return {to_advanced_position(offset), collection};
  }

  Position to_advanced_position(Index offset) const {
    auto actual_position = to_actual_position();
    Index min_offset = std::distance(actual_position, to_actual_begin());
    Index max_offset = std::distance(actual_position, to_actual_end());
    if (offset < min_offset || offset > max_offset) {
      Index size = to_size(collection);
      throw std::out_of_range(
          size ? (std::string("Offset should be in range(") +
                  std::to_string(min_offset) + ", " +
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

template <class Sequence>
static void extend_sequence(Sequence& sequence, py::iterable iterable) {
  auto iterator = py::iter(iterable);
  while (iterator != py::iterator::sentinel())
    sequence.emplace_back(*(iterator++), true);
}

template <class Collection>
static void delete_item(Collection& collection, Index index) {
  Index size = to_size(collection);
  Index normalized_index = index >= 0 ? index : index + size;
  if (normalized_index < 0 || normalized_index >= size)
    throw std::out_of_range(size ? (std::string("Index should be in range(" +
                                                std::to_string(-size) + ", ") +
                                    std::to_string(size) + "), but found " +
                                    std::to_string(index) + ".")
                                 : std::string("Sequence is empty."));
  collection.erase(std::next(std::begin(collection), normalized_index));
}

template <class Sequence>
static const typename Sequence::value_type& to_item(const Sequence& sequence,
                                                    Index index) {
  Index size = to_size(sequence);
  Index normalized_index = index >= 0 ? index : index + size;
  if (normalized_index < 0 || normalized_index >= size)
    throw std::out_of_range(size ? (std::string("Index should be in range(" +
                                                std::to_string(-size) + ", ") +
                                    std::to_string(size) + "), but found " +
                                    std::to_string(index) + ".")
                                 : std::string("Sequence is empty."));
  return sequence[normalized_index];
}

template <class Sequence>
static const typename Sequence::const_iterator to_position(
    const Sequence& sequence, const typename Sequence::value_type& value) {
  const auto& end = std::end(sequence);
  const auto& position = std::find(sequence.begin(), end, value);
  if (position == end) {
    throw std::invalid_argument(repr(value) + " is not found.");
  }
  return position;
}

namespace pybind11 {
static std::ostream& operator<<(std::ostream& stream, const Object& object) {
  return stream << std::string(py::repr(object));
}

static bool operator==(const Object& left, const Object& right) {
  return left.equal(right);
}
}  // namespace pybind11

namespace std {
static std::ostream& operator<<(std::ostream& stream, const Vector& vector) {
  stream << C_STR(MODULE_NAME) "." VECTOR_NAME "(";
  auto object = py::cast(vector);
  if (Py_ReprEnter(object.ptr()) == 0) {
    if (!vector.empty()) {
      stream << vector[0];
      for (std::size_t index = 1; index < vector.size(); ++index)
        stream << ", " << vector[index];
    }
    Py_ReprLeave(object.ptr());
  } else {
    stream << "...";
  }
  return stream << ")";
}
}  // namespace std

using VectorBackwardIterator = BackwardIterator<Vector>;
using VectorForwardIterator = ForwardIterator<Vector>;

static bool operator<(const VectorBackwardIterator& self,
                      const VectorBackwardIterator& other) {
  return self.has_same_collection_with(other) &&
         self.to_actual_position() < other.to_actual_position();
}

static bool operator<(const VectorForwardIterator& self,
                      const VectorForwardIterator& other) {
  return self.has_same_collection_with(other) &&
         self.to_actual_position() < other.to_actual_position();
}

static bool operator<=(const VectorBackwardIterator& self,
                       const VectorBackwardIterator& other) {
  return self.has_same_collection_with(other) &&
         self.to_actual_position() <= other.to_actual_position();
}

static bool operator<=(const VectorForwardIterator& self,
                       const VectorForwardIterator& other) {
  return self.has_same_collection_with(other) &&
         self.to_actual_position() <= other.to_actual_position();
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
      .def(py::self == py::self)
      .def("__bool__", [](const Vector& self) { return !self.empty(); })
      .def(
          "__contains__",
          [](const Vector& self, Object value) {
            return std::find(self.begin(), self.end(), value) != self.end();
          },
          py::arg("value"), py::is_operator{})
      .def("__delitem__", delete_item<Vector>, py::arg("index"))
      .def(
          "__delitem__",
          [](Vector& self, py::slice slice) {
            auto size = to_size(self);
            std::size_t raw_start, raw_stop, raw_step, slice_length;
            if (!slice.compute(size, &raw_start, &raw_stop, &raw_step,
                               &slice_length))
              throw py::error_already_set();
            auto start = static_cast<Index>(raw_start);
            auto stop = static_cast<Index>(raw_stop);
            auto step = static_cast<Index>(raw_step);
            if (step > 0 ? start >= stop : start <= stop)
              return;
            else if (step == 1)
              self.erase(std::next(self.begin(), start),
                         std::next(self.begin(), stop));
            else if (step == -1)
              self.erase(std::next(self.begin(), stop + 1),
                         std::next(self.begin(), start + 1));
            else if (step > 0) {
              const auto& begin = self.begin();
              Vector result{begin, std::next(begin, start)};
              result.reserve(size - slice_length);
              for (; step < stop - start; start += step)
                result.insert(result.end(), std::next(begin, start + 1),
                              std::next(begin, start + step));
              result.insert(result.end(), std::next(begin, start + 1),
                            self.end());
              self.assign(result.begin(), result.end());
            } else {
              start = size - start - 1;
              stop = size - stop - 1;
              step = -step;
              const auto& rbegin = self.rbegin();
              Vector result{rbegin, std::next(rbegin, start)};
              result.reserve(size - slice_length);
              for (; step < stop - start; start += step)
                result.insert(result.end(), std::next(rbegin, start + 1),
                              std::next(rbegin, start + step));
              result.insert(result.end(), std::next(rbegin, start + 1),
                            self.rend());
              self.assign(result.rbegin(), result.rend());
            }
          },
          py::arg("slice"))
      .def("__getitem__", to_item<Vector>, py::arg("index"))
      .def(
          "__getitem__",
          [](const Vector& self, py::slice slice) {
            std::size_t raw_start, raw_stop, raw_step, slice_length;
            if (!slice.compute(self.size(), &raw_start, &raw_stop, &raw_step,
                               &slice_length))
              throw py::error_already_set();
            auto start = static_cast<Index>(raw_start);
            auto stop = static_cast<Index>(raw_stop);
            auto step = static_cast<Index>(raw_step);
            Vector result;
            result.reserve(slice_length);
            if (step < 0)
              for (; start > stop; start += step) result.push_back(self[start]);
            else
              for (; start < stop; start += step) result.push_back(self[start]);
            return result;
          },
          py::arg("slice"))
      .def(
          "__iadd__",
          [](Vector& self, py::iterable iterable) {
            extend_sequence(self, iterable);
            return self;
          },
          py::is_operator{}, py::arg("values"))
      .def("__iter__", to_forward_iterator<Vector>)
      .def("__len__", to_size<Vector>)
      .def("__repr__", repr<Vector>)
      .def("__reversed__", to_backward_iterator<Vector>)
      .def(
          "__setitem__",
          [](Vector& self, Index index, Object value) {
            Index size = to_size(self);
            Index normalized_index = index >= 0 ? index : index + size;
            if (normalized_index < 0 || normalized_index >= size)
              throw std::out_of_range(
                  size ? (std::string("Index should be in range(" +
                                      std::to_string(-size) + ", ") +
                          std::to_string(size) + "), but found " +
                          std::to_string(index) + ".")
                       : std::string("Sequence is empty."));
            self[normalized_index] = value;
          },
          py::arg("index"), py::arg("value"))
      .def(
          "__setitem__",
          [](Vector& self, py::slice slice, py::iterable iterable) {
            auto size = self.size();
            std::size_t raw_start, raw_stop, raw_step, slice_length;
            if (!slice.compute(size, &raw_start, &raw_stop, &raw_step,
                               &slice_length))
              throw py::error_already_set();
            auto start = static_cast<Index>(raw_start);
            auto stop = static_cast<Index>(raw_stop);
            auto step = static_cast<Index>(raw_step);
            Vector values;
            auto iterator = py::iter(iterable);
            while (iterator != py::iterator::sentinel())
              values.emplace_back(*(iterator++), true);
            auto values_count = values.size();
            if (step == 1) {
              auto new_size = size - slice_length + values_count;
              if (new_size > size) {
                self.resize(new_size, py::none{});
                const auto& last_replaced = std::next(self.begin(), stop - 1);
                for (auto source = std::next(self.begin(), size - 1),
                          destination = std::next(self.begin(), new_size - 1);
                     source != last_replaced; --source, --destination)
                  std::iter_swap(source, destination);
              } else if (new_size < size) {
                const auto& old_end = self.end();
                for (auto source = std::next(self.begin(), stop),
                          destination =
                              std::next(self.begin(), start + values_count);
                     source != old_end; ++source, ++destination)
                  std::iter_swap(source, destination);
                self.erase(std::next(self.begin(), new_size), old_end);
              }
              std::copy(values.begin(), values.end(),
                        std::next(self.begin(), start));
              return;
            }
            if (slice_length != values_count)
              throw std::range_error(
                  std::string("Attempt to assign iterable with capacity ") +
                  std::to_string(values_count) + " to slice with size " +
                  std::to_string(slice_length) + ".");
            auto position = values.begin();
            if (step < 0)
              for (; start > stop; start += step) self[start] = *(position++);
            else
              for (; start < stop; start += step) self[start] = *(position++);
          },
          py::arg("slice"), py::arg("values"))
      .def(
          "append", [](Vector& self, Object value) { self.push_back(value); },
          py::arg("value"))
      .def("begin",
           [](const Vector& self) {
             return VectorForwardIterator(self.begin(), self);
           })
      .def("clear", &Vector::clear)
      .def(
          "count",
          [](const Vector& self, Object value) {
            return std::count(self.begin(), self.end(), value);
          },
          py::arg("value"))
      .def("end",
           [](const Vector& self) {
             return VectorForwardIterator(self.end(), self);
           })
      .def("extend", extend_sequence<Vector>, py::arg("values"))
      .def(
          "index",
          [](const Vector& self, Object value, Index start, Index stop) {
            Index size = self.size();
            std::size_t normalized_start =
                std::max(std::min(start >= 0 ? start : start + size, size),
                         static_cast<Index>(0));
            std::size_t normalized_stop =
                std::max(std::min(stop >= 0 ? stop : stop + size, size),
                         static_cast<Index>(0));
            if (normalized_start >= normalized_stop)
              throw std::invalid_argument(
                  repr(value) + " is not found in slice(" +
                  std::to_string(normalized_start) + ", " +
                  std::to_string(normalized_stop) + ").");
            const auto& end = std::next(self.begin(), normalized_stop);
            const auto& position = std::find(
                std::next(self.begin(), normalized_start), end, value);
            if (position == end)
              throw std::invalid_argument(
                  repr(value) + " is not found in slice(" +
                  std::to_string(normalized_start) + ", " +
                  std::to_string(normalized_stop) + ").");
            return std::distance(self.begin(), position);
          },
          py::arg("value"), py::arg("start") = 0,
          py::arg("stop") = std::numeric_limits<Index>::max())
      .def(
          "insert",
          [](Vector& self, Index index, Object value) {
            Index size = self.size();
            std::size_t normalized_index =
                std::max(std::min(index >= 0 ? index : index + size, size),
                         static_cast<Index>(0));
            self.insert(std::next(self.begin(), normalized_index), value);
          },
          py::arg("index"), py::arg("value"))
      .def(
          "pop",
          [](Vector& self, Index index) {
            Index size = self.size();
            Index normalized_index = index >= 0 ? index : index + size;
            if (normalized_index < 0 || normalized_index >= size)
              throw std::out_of_range(
                  size ? (std::string("Index should be in range(" +
                                      std::to_string(-size) + ", ") +
                          std::to_string(size) + "), but found " +
                          std::to_string(index) + ".")
                       : std::string("Vector is empty."));
            if (normalized_index == size - 1) {
              auto result = self.back();
              self.pop_back();
              return result;
            }
            auto result = self[normalized_index];
            self.erase(std::next(self.begin(), normalized_index));
            return result;
          },
          py::arg("index") = -1)
      .def("pop_back",
           [](Vector& self) {
             if (self.empty()) throw std::out_of_range("Vector is empty.");
             self.pop_back();
           })
      .def(
          "push_back",
          [](Vector& self, Object value) { self.push_back(value); },
          py::arg("value"))
      .def("rbegin",
           [](const Vector& self) {
             return VectorBackwardIterator(self.rbegin(), self);
           })
      .def(
          "remove",
          [](Vector& self, Object value) {
            self.erase(to_position(self, value));
          },
          py::arg("value"))
      .def("rend",
           [](const Vector& self) {
             return VectorBackwardIterator(self.rend(), self);
           })
      .def("reserve", &Vector::reserve, py::arg("capacity"))
      .def(
          "resize",
          [](Vector& self, std::size_t size, Object value) {
            self.resize(size, value);
          },
          py::arg("size"), py::arg("value") = py::none())
      .def("reverse",
           [](Vector& self) { std::reverse(self.begin(), self.end()); });

  py::class_<VectorBackwardIterator>(m, VECTOR_BACKWARD_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self + Index{})
      .def(py::self - Index{})
      .def(py::self += Index{})
      .def(py::self -= Index{})
      .def("__iter__",
           [](VectorBackwardIterator& self) -> VectorBackwardIterator& {
             return self;
           })
      .def("__next__", &VectorBackwardIterator::next);

  py::class_<VectorForwardIterator>(m, VECTOR_FORWARD_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self + Index{})
      .def(py::self - Index{})
      .def(py::self += Index{})
      .def(py::self -= Index{})
      .def("__iter__",
           [](VectorForwardIterator& self) -> VectorForwardIterator& {
             return self;
           })
      .def("__next__", &VectorForwardIterator::next);
}
