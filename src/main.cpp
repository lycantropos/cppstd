#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

namespace py = pybind11;

#define MODULE_NAME _cppstd
#define C_STR_HELPER(a) #a
#define C_STR(a) C_STR_HELPER(a)
#define ITERATOR_NAME "iterator"
#define MAP_NAME "map"
#define REVERSE_ITERATOR_NAME "reverse_iterator"
#define SET_NAME "set"
#define VECTOR_NAME "vector"
#ifndef VERSION_INFO
#define VERSION_INFO "dev"
#endif

using Index = Py_ssize_t;
using Object = py::object;
using RawMap = std::map<Object, Object>;
using RawMapItem = RawMap::value_type;
using RawSet = std::set<Object>;
using RawVector = std::vector<Object>;
using IterableState = py::list;

namespace pybind11 {
static std::ostream& operator<<(std::ostream& stream, const Object& object) {
  return stream << std::string(py::repr(object));
}

static bool operator==(const Object& left, const Object& right) {
  return left.equal(right);
}
}  // namespace pybind11

class Token {
 public:
  Token(std::weak_ptr<bool> ptr) : _ptr(ptr) {}

  bool expired() const { return _ptr.expired(); }

 private:
  std::weak_ptr<bool> _ptr;
};

class Tokenizer {
 public:
  Tokenizer() : _ptr(std::make_shared<bool>(false)) {}

  void reset() { _ptr.reset(new bool(false)); }

  Token create() const { return {std::weak_ptr<bool>{_ptr}}; }

 private:
  std::shared_ptr<bool> _ptr;
};

template <class T>
static bool are_addresses_equal(const T& left, const T& right) {
  return std::addressof(left) == std::addressof(right);
}

template <class Type>
std::string to_repr(const Type& value) {
  std::ostringstream stream;
  stream.precision(std::numeric_limits<double>::digits10 + 2);
  stream << value;
  return stream.str();
}

template <class RawCollection>
struct ToBegin {
  typename RawCollection::const_iterator operator()(
      const RawCollection& collection) const {
    return std::cbegin(collection);
  }
};

template <class RawCollection>
struct ToEnd {
  typename RawCollection::const_iterator operator()(
      const RawCollection& collection) const {
    return std::cend(collection);
  }
};

template <class RawCollection>
struct ToReverseBegin {
  typename RawCollection::const_reverse_iterator operator()(
      const RawCollection& collection) const {
    return std::crbegin(collection);
  }
};

template <class RawCollection>
struct ToReverseEnd {
  typename RawCollection::const_reverse_iterator operator()(
      const RawCollection& collection) const {
    return std::crend(collection);
  }
};

template <class RawCollection, bool reversed>
class Iterator {
 public:
  using Position =
      std::conditional_t<reversed,
                         typename RawCollection::const_reverse_iterator,
                         typename RawCollection::const_iterator>;

  Iterator(std::weak_ptr<RawCollection> raw_collection_ptr, Position position,
           const Token& token)
      : _raw_collection_ptr(raw_collection_ptr),
        position(position),
        _token(token){};

  typename std::iterator_traits<Position>::value_type operator*() const {
    return *position;
  }

  Position& to_position() {
    validate();
    return position;
  }

  const Position& to_position() const {
    validate();
    return position;
  }

  bool has_same_collection_with(
      const Iterator<RawCollection, reversed>& other) const {
    return are_addresses_equal(to_raw_collection(), other.to_raw_collection());
  }

  Position to_begin() const {
    static const Replenisher replenish;
    return replenish(to_raw_collection());
  }

  Position to_end() const {
    static const Exhauster exhaust;
    return exhaust(to_raw_collection());
  }

  Iterator<RawCollection, reversed> with_position(Position position) const {
    return Iterator<RawCollection, reversed>{_raw_collection_ptr, position,
                                             _token};
  }

 private:
  using Replenisher =
      std::conditional_t<reversed, ToReverseBegin<RawCollection>,
                         ToBegin<RawCollection>>;
  using Exhauster = std::conditional_t<reversed, ToReverseEnd<RawCollection>,
                                       ToEnd<RawCollection>>;

  std::weak_ptr<RawCollection> _raw_collection_ptr;
  Position position;
  Token _token;

  const RawCollection& to_raw_collection() const {
    validate();
    if (const auto* ptr = _raw_collection_ptr.lock().get())
      return *ptr;
    else
      throw py::value_error("Iterator is invalidated.");
  }

  void validate() const {
    if (_token.expired()) throw py::value_error("Iterator is invalidated.");
  }
};

template <class RawCollection, bool reversed>
bool operator!=(const Iterator<RawCollection, reversed>& left,
                const Iterator<RawCollection, reversed>& right) {
  return left.to_position() != right.to_position();
}

template <class RawCollection, bool reversed>
bool operator==(const Iterator<RawCollection, reversed>& left,
                const Iterator<RawCollection, reversed>& right) {
  return left.to_position() == right.to_position();
}

template <class RawCollection, bool reversed>
static bool operator<=(const Iterator<RawCollection, reversed>& self,
                       const Iterator<RawCollection, reversed>& other) {
  if (!self.has_same_collection_with(other))
    throw py::value_error(
        "Comparing iterators of different collections is prohibited.");
  return self.to_position() <= other.to_position();
}

template <class RawCollection, bool reversed>
static bool operator<(const Iterator<RawCollection, reversed>& self,
                      const Iterator<RawCollection, reversed>& other) {
  if (!self.has_same_collection_with(other))
    throw py::value_error(
        "Comparing iterators of different collections is prohibited.");
  return self.to_position() < other.to_position();
}

template <class RawCollection, bool reversed>
static typename Iterator<RawCollection, reversed>::Position
to_advanced_position(const Iterator<RawCollection, reversed>& iterator,
                     Index offset) {
  const auto& position = iterator.to_position();
  const auto begin = iterator.to_begin();
  const auto end = iterator.to_end();
  Index min_offset = -std::distance(begin, position);
  Index max_offset = std::distance(position, end);
  if (offset < min_offset || offset > max_offset) {
    throw py::value_error(
        begin == end ? std::string("Collection is empty.")
                     : (std::string("Offset should be in range(") +
                        std::to_string(min_offset) + ", " +
                        std::to_string(max_offset + 1) + "), but found " +
                        std::to_string(offset) + "."));
  }
  return position + offset;
}

template <class RawCollection, bool reversed>
Iterator<RawCollection, reversed> operator+(
    const Iterator<RawCollection, reversed>& iterator, Index offset) {
  return iterator.with_position(to_advanced_position(iterator, offset));
}

template <class RawCollection, bool reversed>
Iterator<RawCollection, reversed> operator-(
    const Iterator<RawCollection, reversed>& iterator, Index offset) {
  return iterator + (-offset);
}

template <class RawCollection, bool reversed>
Iterator<RawCollection, reversed>& operator+=(
    Iterator<RawCollection, reversed>& iterator, Index offset) {
  iterator.to_position() = to_advanced_position(iterator, offset);
  return iterator;
}

template <class RawCollection, bool reversed>
Iterator<RawCollection, reversed>& operator-=(
    Iterator<RawCollection, reversed>& iterator, Index offset) {
  return (iterator += (-offset));
}

template <class RawCollection, bool reversed>
Iterator<RawCollection, reversed> operator++(
    Iterator<RawCollection, reversed>& iterator) {
  auto& position = iterator.to_position();
  if (position == iterator.to_end()) throw py::stop_iteration();
  return iterator.with_position(position++);
}

template <class RawCollection, bool reversed>
Iterator<RawCollection, reversed>& operator++(
    Iterator<RawCollection, reversed>& iterator, int) {
  auto& position = iterator.to_position();
  if (position == iterator.to_end()) throw py::stop_iteration();
  ++position;
  return iterator;
}

template <class RawCollection>
using BackwardIterator = Iterator<RawCollection, true>;
template <class RawCollection>
using ForwardIterator = Iterator<RawCollection, false>;

using MapBackwardIterator = BackwardIterator<RawMap>;
using MapForwardIterator = ForwardIterator<RawMap>;
using SetBackwardIterator = BackwardIterator<RawSet>;
using SetForwardIterator = ForwardIterator<RawSet>;
using VectorBackwardIterator = BackwardIterator<RawVector>;
using VectorForwardIterator = ForwardIterator<RawVector>;

template <class Iterable>
IterableState iterable_to_state(const Iterable& self) {
  IterableState result;
  for (const auto& element : self) result.append(element);
  return result;
}

class Map {
 public:
  Map(const RawMap& raw) : _raw(std::make_shared<RawMap>(raw)), _tokenizer() {}

  bool operator==(const Map& other) const { return *_raw == *other._raw; }

  static Map from_state(IterableState state) {
    RawMap raw;
    for (auto& element : state) {
      auto item = element.cast<py::tuple>();
      raw[item[0]] = item[1];
    }
    return {raw};
  }

  MapForwardIterator begin() const {
    return {_raw, _raw->begin(), _tokenizer.create()};
  }

  void clear() {
    _tokenizer.reset();
    return _raw->clear();
  }

  bool empty() const { return _raw->empty(); }

  MapForwardIterator end() const {
    return {_raw, _raw->end(), _tokenizer.create()};
  }

  MapBackwardIterator rbegin() const {
    return {_raw, _raw->rbegin(), _tokenizer.create()};
  }

  MapBackwardIterator rend() const {
    return {_raw, _raw->rend(), _tokenizer.create()};
  }

  void set_item(Object key, Object value) {
    auto& place = (*_raw)[key];
    _tokenizer.reset();
    place = value;
  }

  std::size_t size() const { return _raw->size(); }

 private:
  std::shared_ptr<RawMap> _raw;
  Tokenizer _tokenizer;
};

namespace std {
static std::ostream& operator<<(std::ostream& stream, const RawMapItem& item) {
  stream << "(";
  auto object = item.first;
  if (Py_ReprEnter(object.ptr()) == 0) {
    stream << object;
    Py_ReprLeave(object.ptr());
  } else
    stream << "...";
  stream << ", ";
  object = item.second;
  if (Py_ReprEnter(object.ptr()) == 0) {
    stream << object;
    Py_ReprLeave(object.ptr());
  } else
    stream << "...";
  return stream << ")";
}
}  // namespace std

static std::ostream& operator<<(std::ostream& stream, const Map& map) {
  stream << C_STR(MODULE_NAME) "." MAP_NAME "(";
  auto object = py::cast(map);
  if (Py_ReprEnter(object.ptr()) == 0) {
    if (!map.empty()) {
      auto position = map.begin();
      stream << *position;
      for (++position; position != map.end(); ++position)
        stream << ", " << *position;
    }
    Py_ReprLeave(object.ptr());
  } else {
    stream << "...";
  }
  return stream << ")";
}

class Set {
 public:
  Set(const RawSet& raw) : _raw(std::make_shared<RawSet>(raw)), _tokenizer() {}

  bool operator<(const Set& other) const { return *_raw < *other._raw; }

  bool operator<=(const Set& other) const { return *_raw <= *other._raw; }

  bool operator==(const Set& other) const { return *_raw == *other._raw; }

  bool empty() const { return _raw->empty(); }

  static Set from_state(IterableState state) {
    RawSet raw;
    for (auto& element : state)
      raw.insert(py::reinterpret_borrow<Object>(element));
    return {raw};
  }

  SetForwardIterator begin() const {
    return {_raw, _raw->begin(), _tokenizer.create()};
  }

  void clear() {
    _tokenizer.reset();
    return _raw->clear();
  }

  SetForwardIterator end() const {
    return {_raw, _raw->end(), _tokenizer.create()};
  }

  SetBackwardIterator rbegin() const {
    return {_raw, _raw->rbegin(), _tokenizer.create()};
  }

  SetBackwardIterator rend() const {
    return {_raw, _raw->rend(), _tokenizer.create()};
  }

  std::size_t size() const { return _raw->size(); }

 private:
  std::shared_ptr<RawSet> _raw;
  Tokenizer _tokenizer;
};

static std::ostream& operator<<(std::ostream& stream, const Set& set) {
  stream << C_STR(MODULE_NAME) "." SET_NAME "(";
  auto object = py::cast(set);
  if (Py_ReprEnter(object.ptr()) == 0) {
    if (!set.empty()) {
      auto position = set.begin();
      stream << *position;
      for (++position; position != set.end(); ++position)
        stream << ", " << *position;
    }
    Py_ReprLeave(object.ptr());
  } else {
    stream << "...";
  }
  return stream << ")";
}

class Vector {
 public:
  static Vector from_state(IterableState state) {
    RawVector raw;
    raw.reserve(state.size());
    for (auto& element : state)
      raw.push_back(py::reinterpret_borrow<Object>(element));
    return {raw};
  }

  Vector(const RawVector& raw)
      : _raw(std::make_shared<RawVector>(raw)), _tokenizer() {}

  bool operator==(const Vector& other) const { return *_raw == *other._raw; }

  bool operator<(const Vector& other) const {
    return *this->_raw < *other._raw;
  }

  bool operator<=(const Vector& other) const {
    return *this->_raw <= *other._raw;
  }

  VectorForwardIterator begin() const {
    return {_raw, _raw->begin(), _tokenizer.create()};
  }

  void clear() {
    _tokenizer.reset();
    _raw->clear();
  }

  bool empty() const { return _raw->empty(); }

  VectorForwardIterator end() const {
    return {_raw, _raw->end(), _tokenizer.create()};
  }

  Object get_item(Index index) const {
    Index size = _raw->size();
    Index normalized_index = index >= 0 ? index : index + size;
    if (normalized_index < 0 || normalized_index >= size)
      throw py::index_error(size ? (std::string("Index should be in range(" +
                                                std::to_string(-size) + ", ") +
                                    std::to_string(size) + "), but found " +
                                    std::to_string(index) + ".")
                                 : std::string("Sequence is empty."));
    return (*_raw)[normalized_index];
  }

  void pop_back() {
    if (_raw->empty()) throw py::index_error("Vector is empty.");
    _tokenizer.reset();
    _raw->pop_back();
  }

  void push_back(Object value) {
    _tokenizer.reset();
    _raw->push_back(value);
  }

  VectorBackwardIterator rbegin() const {
    return {_raw, _raw->rbegin(), _tokenizer.create()};
  }

  VectorBackwardIterator rend() const {
    return {_raw, _raw->rend(), _tokenizer.create()};
  }

  void reserve(std::size_t capacity) { _raw->reserve(capacity); }

  void resize(Index size, Object value) {
    if (size < 0)
      throw py::value_error(std::string("Size should be positive, but found ") +
                            std::to_string(size) + ".");
    _tokenizer.reset();
    _raw->resize(size, value);
  }

  void set_item(Index index, Object value) {
    Index size = _raw->size();
    Index normalized_index = index >= 0 ? index : index + size;
    if (normalized_index < 0 || normalized_index >= size)
      throw py::index_error(size ? (std::string("Index should be in range(" +
                                                std::to_string(-size) + ", ") +
                                    std::to_string(size) + "), but found " +
                                    std::to_string(index) + ".")
                                 : std::string("Sequence is empty."));
    (*_raw)[normalized_index] = value;
  }

  std::size_t size() const { return _raw->size(); }

 private:
  std::shared_ptr<RawVector> _raw;
  Tokenizer _tokenizer;
};

static std::ostream& operator<<(std::ostream& stream, const Vector& vector) {
  stream << C_STR(MODULE_NAME) "." VECTOR_NAME "(";
  auto object = py::cast(vector);
  if (Py_ReprEnter(object.ptr()) == 0) {
    if (!vector.empty()) {
      stream << vector.get_item(0);
      for (std::size_t index = 1; index < vector.size(); ++index)
        stream << ", " << vector.get_item(index);
    }
    Py_ReprLeave(object.ptr());
  } else {
    stream << "...";
  }
  return stream << ")";
}

PYBIND11_MODULE(MODULE_NAME, m) {
  m.doc() = R"pbdoc(Partial binding of C++ standard library.)pbdoc";
  m.attr("__version__") = VERSION_INFO;

  py::class_<Map> PyMap(m, MAP_NAME);
  PyMap
      .def(py::init([](py::args args) {
        RawMap raw;
        for (auto& element : args) {
          auto item = element.cast<py::tuple>();
          auto item_size = item.size();
          if (item_size != 2)
            throw py::type_error(
                std::string("Items should be iterables of size 2, but found ") +
                std::string(py::repr(element.get_type())) + " with " +
                std::to_string(item_size) + " elements.");
          else
            raw[item[0]] = item[1];
        }
        return Map{raw};
      }))
      .def(py::pickle(&iterable_to_state<Map>, &Map::from_state))
      .def(py::self == py::self)
      .def("__repr__", to_repr<Map>)
      .def("__setitem__", &Map::set_item, py::arg("key"), py::arg("value"))
      .def("begin", &Map::begin)
      .def("clear", &Map::clear)
      .def("empty", &Map::empty)
      .def("end", &Map::end)
      .def("rbegin", &Map::rbegin)
      .def("rend", &Map::rend)
      .def("size", &Map::size);

  py::class_<MapBackwardIterator>(PyMap, REVERSE_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self != py::self);

  py::class_<MapForwardIterator>(PyMap, ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self != py::self);

  py::class_<Set> PySet(m, SET_NAME);
  PySet
      .def(py::init([](py::args args) {
        RawSet raw;
        for (auto& element : args)
          raw.insert(py::reinterpret_borrow<Object>(element));
        return Set{raw};
      }))
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self == py::self)
      .def(py::pickle(&iterable_to_state<Set>, &Set::from_state))
      .def("__repr__", to_repr<Set>)
      .def("begin", &Set::begin)
      .def("clear", &Set::clear)
      .def("empty", &Set::empty)
      .def("end", &Set::end)
      .def("rbegin", &Set::rbegin)
      .def("rend", &Set::rend)
      .def("size", &Set::size);

  py::class_<SetBackwardIterator>(PySet, REVERSE_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self != py::self);

  py::class_<SetForwardIterator>(PySet, ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self != py::self);

  py::class_<Vector> PyVector(m, VECTOR_NAME);
  PyVector
      .def(py::init([](py::args args) {
        RawVector raw;
        raw.reserve(args.size());
        for (auto& element : args)
          raw.push_back(py::reinterpret_borrow<Object>(element));
        return Vector{raw};
      }))
      .def(py::self == py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::pickle(&iterable_to_state<Vector>, &Vector::from_state))
      .def("__getitem__", &Vector::get_item, py::arg("index"))
      .def("__repr__", to_repr<Vector>)
      .def("__setitem__", &Vector::set_item, py::arg("index"), py::arg("value"))
      .def("begin", &Vector::begin)
      .def("clear", &Vector::clear)
      .def("empty", &Vector::empty)
      .def("end", &Vector::end)
      .def("pop_back", &Vector::pop_back)
      .def("push_back", &Vector::push_back, py::arg("value"))
      .def("rbegin", &Vector::rbegin)
      .def("rend", &Vector::rend)
      .def("reserve", &Vector::reserve, py::arg("capacity"))
      .def("resize", &Vector::resize, py::arg("size"),
           py::arg("value") = py::none())
      .def("size", &Vector::size);

  py::class_<VectorBackwardIterator>(PyVector, REVERSE_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self + Index{})
      .def(py::self - Index{})
      .def(py::self += Index{})
      .def(py::self -= Index{});

  py::class_<VectorForwardIterator>(PyVector, ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self + Index{})
      .def(py::self - Index{})
      .def(py::self += Index{})
      .def(py::self -= Index{});
}
