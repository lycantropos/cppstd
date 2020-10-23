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
#define CONST_ITERATOR_NAME "const_iterator"
#define CONST_REVERSE_ITERATOR_NAME "const_reverse_iterator"
#define ITERATOR_NAME "iterator"
#define REVERSE_ITERATOR_NAME "reverse_iterator"
#define MAP_NAME "map"
#define MAP_PYTHON_ITERATOR_NAME "map_iterator"
#define MAP_PYTHON_REVERSE_ITERATOR_NAME "map_reverse_iterator"
#define SET_NAME "set"
#define SET_PYTHON_ITERATOR_NAME "set_iterator"
#define SET_PYTHON_REVERSE_ITERATOR_NAME "set_reverse_iterator"
#define VECTOR_NAME "vector"
#define VECTOR_PYTHON_ITERATOR_NAME "vector_iterator"
#define VECTOR_PYTHON_REVERSE_ITERATOR_NAME "vector_reverse_iterator"
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

template <class T>
T&& identity(T&& value) {
  return std::forward<T>(value);
}

namespace pybind11 {
static std::ostream& operator<<(std::ostream& stream, const Object& object) {
  return stream << std::string(py::repr(object));
}

static bool operator==(const Object& left, const Object& right) {
  return left.equal(right);
}
}  // namespace pybind11

class SharedToken {
 public:
  SharedToken(std::shared_ptr<bool> ptr)
      : _checking_ptr(ptr), _ptr(std::move(ptr)) {}

  bool expired() const { return _checking_ptr.expired(); }

 private:
  std::weak_ptr<bool> _checking_ptr;
  std::shared_ptr<bool> _ptr;
};

class WeakToken {
 public:
  WeakToken(std::weak_ptr<bool> ptr) : _ptr(std::move(ptr)) {}

  bool expired() const { return _ptr.expired(); }

 private:
  std::weak_ptr<bool> _ptr;
};

class Tokenizer {
 public:
  Tokenizer() : _ptr(std::make_shared<bool>(false)) {}

  void reset() { _ptr.reset(new bool(false)); }

  SharedToken create_shared() const { return {_ptr}; }

  WeakToken create_weak() const { return {_ptr}; }

 private:
  std::shared_ptr<bool> _ptr;
};

template <class T>
static bool are_addresses_equal(const T& left, const T& right) {
  return std::addressof(left) == std::addressof(right);
}

template <class T>
std::string to_repr(const T& value) {
  std::ostringstream stream;
  stream.precision(std::numeric_limits<double>::digits10 + 2);
  stream << value;
  return stream.str();
}

template <class Collection>
struct ToBegin {
  typename Collection::const_iterator operator()(
      const Collection& collection) const {
    return collection.cbegin();
  }
};

template <class Collection>
struct ToEnd {
  typename Collection::const_iterator operator()(
      const Collection& collection) const {
    return collection.cend();
  }
};

template <class Collection>
struct ToReverseBegin {
  typename Collection::const_reverse_iterator operator()(
      const Collection& collection) const {
    return collection.crbegin();
  }
};

template <class Collection>
struct ToReverseEnd {
  typename Collection::const_reverse_iterator operator()(
      const Collection& collection) const {
    return collection.crend();
  }
};

template <class Collection, bool constant, bool reversed>
class BaseIterator {
 public:
  using ConstPosition =
      std::conditional_t<reversed, typename Collection::const_reverse_iterator,
                         typename Collection::const_iterator>;
  using Position = std::conditional_t<
      constant, ConstPosition,
      std::conditional_t<reversed, typename Collection::reverse_iterator,
                         typename Collection::iterator>>;
  using ConstValueRef =
      const typename std::iterator_traits<Position>::value_type&;
  using ValueRef =
      std::conditional_t<constant, ConstValueRef,
                         typename std::iterator_traits<Position>::value_type&>;

  BaseIterator(const std::weak_ptr<Collection>& collection_ptr,
               Position position, const WeakToken& token)
      : _collection_ptr(collection_ptr),
        _position(std::move(position)),
        _token(token){};

  ConstValueRef operator*() const { return *_position; }

  ValueRef operator*() { return *_position; }

  ConstPosition to_begin() const {
    static const Replenisher replenish;
    return replenish(to_collection());
  }

  ConstPosition to_end() const {
    static const Exhauster exhaust;
    return exhaust(to_collection());
  }

  Position& to_position() {
    validate();
    return _position;
  }

  const Position& to_position() const {
    validate();
    return _position;
  }

  void validate_comparison_with(
      const BaseIterator<Collection, constant, reversed>& other) const {
    if (!are_addresses_equal(to_collection(), other.to_collection()))
      throw std::runtime_error(
          "Comparing iterators from different collections is undefined.");
  }

  BaseIterator<Collection, constant, reversed> with_position(
      Position position) const {
    return {_collection_ptr, position, _token};
  }

 private:
  using Replenisher = std::conditional_t<reversed, ToReverseBegin<Collection>,
                                         ToBegin<Collection>>;
  using Exhauster =
      std::conditional_t<reversed, ToReverseEnd<Collection>, ToEnd<Collection>>;

  std::weak_ptr<Collection> _collection_ptr;
  Position _position;
  WeakToken _token;

  const Collection& to_collection() const {
    validate();
    if (const auto* ptr = _collection_ptr.lock().get())
      return *ptr;
    else
      throw std::runtime_error("Iterator is invalidated.");
  }

  void validate() const {
    if (_token.expired()) throw std::runtime_error("Iterator is invalidated.");
  }
};

template <class Collection>
class PythonIterator {
 public:
  using Position = typename Collection::const_iterator;

  PythonIterator(Position position,
                 const std::shared_ptr<Collection>& collection_ptr,
                 const SharedToken& token)
      : _collection_ptr(collection_ptr),
        _position(std::move(position)),
        _token(token){};

  const typename Collection::value_type& next() {
    if (_token.expired()) throw std::runtime_error("Iterator is invalidated.");
    if (_position == _collection_ptr->end()) throw py::stop_iteration();
    return *_position++;
  }

 private:
  std::shared_ptr<Collection> _collection_ptr;
  Position _position;
  SharedToken _token;
};

template <class RawCollection, bool constant, bool reversed>
bool operator!=(const BaseIterator<RawCollection, constant, reversed>& left,
                const BaseIterator<RawCollection, constant, reversed>& right) {
  left.validate_comparison_with(right);
  return left.to_position() != right.to_position();
}

template <class RawCollection, bool constant, bool reversed>
bool operator==(const BaseIterator<RawCollection, constant, reversed>& left,
                const BaseIterator<RawCollection, constant, reversed>& right) {
  left.validate_comparison_with(right);
  return left.to_position() == right.to_position();
}

template <class RawCollection, bool constant, bool reversed>
static bool operator<=(
    const BaseIterator<RawCollection, constant, reversed>& left,
    const BaseIterator<RawCollection, constant, reversed>& right) {
  left.validate_comparison_with(right);
  return left.to_position() <= right.to_position();
}

template <class RawCollection, bool constant, bool reversed>
static bool operator<(
    const BaseIterator<RawCollection, constant, reversed>& left,
    const BaseIterator<RawCollection, constant, reversed>& right) {
  left.validate_comparison_with(right);
  return left.to_position() < right.to_position();
}

template <class It>
static typename It::Position to_advanced_position(const It& iterator,
                                                  Index offset) {
  const auto& position = iterator.to_position();
  const typename It::ConstPosition& const_position = position;
  const auto begin = iterator.to_begin();
  const auto end = iterator.to_end();
  Index min_offset = -std::distance(begin, const_position);
  Index max_offset = std::distance(const_position, end);
  if (offset < min_offset || offset > max_offset) {
    throw std::runtime_error(
        position == end
            ? std::string("Advancing of stop iterators is undefined.")
            : (std::string("Advancing of iterators out-of-bound is undefined: "
                           "offset should be in range(") +
               std::to_string(min_offset) + ", " +
               std::to_string(max_offset + 1) + "), but found " +
               std::to_string(offset) + "."));
  }
  return position + offset;
}

template <class RawCollection, bool constant, bool reversed>
BaseIterator<RawCollection, constant, reversed> operator+(
    const BaseIterator<RawCollection, constant, reversed>& iterator,
    Index offset) {
  return iterator.with_position(to_advanced_position(iterator, offset));
}

template <class RawCollection, bool constant, bool reversed>
BaseIterator<RawCollection, constant, reversed> operator-(
    const BaseIterator<RawCollection, constant, reversed>& iterator,
    Index offset) {
  return iterator + (-offset);
}

template <class RawCollection, bool constant, bool reversed>
BaseIterator<RawCollection, constant, reversed>& operator+=(
    BaseIterator<RawCollection, constant, reversed>& iterator, Index offset) {
  iterator.to_position() = to_advanced_position(iterator, offset);
  return iterator;
}

template <class RawCollection, bool constant, bool reversed>
BaseIterator<RawCollection, constant, reversed>& operator-=(
    BaseIterator<RawCollection, constant, reversed>& iterator, Index offset) {
  return (iterator += (-offset));
}

template <class RawCollection, bool constant, bool reversed>
BaseIterator<RawCollection, constant, reversed> operator++(
    BaseIterator<RawCollection, constant, reversed>& iterator) {
  auto& position = iterator.to_position();
  if (position == iterator.to_end())
    throw std::runtime_error(
        "Post-incrementing of stop iterators is undefined.");
  return iterator.with_position(position++);
}

template <class RawCollection, bool constant, bool reversed>
BaseIterator<RawCollection, constant, reversed> operator--(
    BaseIterator<RawCollection, constant, reversed>& iterator) {
  auto& position = iterator.to_position();
  if (position == iterator.to_begin())
    throw std::runtime_error(
        "Post-decrementing of start iterators is undefined.");
  return iterator.with_position(position--);
}

template <class RawCollection, bool constant, bool reversed>
BaseIterator<RawCollection, constant, reversed>& operator++(
    BaseIterator<RawCollection, constant, reversed>& iterator, int) {
  auto& position = iterator.to_position();
  if (position == iterator.to_end())
    throw std::runtime_error(
        "Pre-incrementing of stop iterators is undefined.");
  ++position;
  return iterator;
}

template <class RawCollection, bool constant, bool reversed>
BaseIterator<RawCollection, constant, reversed>& operator--(
    BaseIterator<RawCollection, constant, reversed>& iterator, int) {
  auto& position = iterator.to_position();
  if (position == iterator.to_begin())
    throw std::runtime_error(
        "Pre-decrementing of start iterators is undefined.");
  --position;
  return iterator;
}

template <class It>
typename It::ConstValueRef get_iterator_value(const It& iterator) {
  if (iterator.to_position() == iterator.to_end())
    throw std::runtime_error("Getting value of stop iterators is undefined.");
  return *iterator;
}

template <class It>
void set_iterator_value(It& iterator, typename It::ConstValueRef value) {
  if (iterator.to_position() == iterator.to_end())
    throw std::runtime_error("Setting value of stop iterators is undefined.");
  *iterator = value;
}

template <class Collection>
using ConstIterator = BaseIterator<Collection, true, false>;
template <class Collection>
using ConstReverseIterator = BaseIterator<Collection, true, true>;
template <class Collection>
using Iterator = BaseIterator<Collection, false, false>;
template <class Collection>
using ReverseIterator = BaseIterator<Collection, false, true>;

using MapConstIterator = ConstIterator<RawMap>;
using MapConstReverseIterator = ConstReverseIterator<RawMap>;
using MapPythonIterator = PythonIterator<RawMap>;
using SetConstIterator = ConstIterator<RawSet>;
using SetConstReverseIterator = ConstReverseIterator<RawSet>;
using SetPythonIterator = PythonIterator<RawSet>;
using VectorConstIterator = ConstIterator<RawVector>;
using VectorConstReverseIterator = ConstReverseIterator<RawVector>;
using VectorIterator = Iterator<RawVector>;
using VectorReverseIterator = ReverseIterator<RawVector>;
using VectorPythonIterator = PythonIterator<RawVector>;

template <class Iterable>
IterableState iterable_to_state(const Iterable& self) {
  IterableState result;
  for (auto position = self.cbegin(); position != self.cend(); ++position)
    result.append(*position);
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

  MapConstIterator begin() const {
    return {_raw, _raw->begin(), _tokenizer.create_weak()};
  }

  MapConstIterator cbegin() const {
    return {_raw, _raw->cbegin(), _tokenizer.create_weak()};
  }

  MapConstIterator cend() const {
    return {_raw, _raw->cend(), _tokenizer.create_weak()};
  }

  void clear() {
    _tokenizer.reset();
    return _raw->clear();
  }

  MapConstReverseIterator crbegin() const {
    return {_raw, _raw->crbegin(), _tokenizer.create_weak()};
  }

  MapConstReverseIterator crend() const {
    return {_raw, _raw->crend(), _tokenizer.create_weak()};
  }

  bool empty() const { return _raw->empty(); }

  MapConstIterator end() const {
    return {_raw, _raw->end(), _tokenizer.create_weak()};
  }

  MapConstReverseIterator rbegin() const {
    return {_raw, _raw->rbegin(), _tokenizer.create_weak()};
  }

  MapConstReverseIterator rend() const {
    return {_raw, _raw->rend(), _tokenizer.create_weak()};
  }

  void set_item(Object key, Object value) {
    auto& place = (*_raw)[key];
    _tokenizer.reset();
    place = value;
  }

  std::size_t size() const { return _raw->size(); }

  MapPythonIterator to_python_iterator() const {
    return {_raw->cbegin(), _raw, _tokenizer.create_shared()};
  }

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

  SetConstIterator begin() const {
    return {_raw, _raw->begin(), _tokenizer.create_weak()};
  }

  SetConstIterator cbegin() const {
    return {_raw, _raw->cbegin(), _tokenizer.create_weak()};
  }

  SetConstIterator cend() const {
    return {_raw, _raw->cend(), _tokenizer.create_weak()};
  }

  void clear() {
    _tokenizer.reset();
    return _raw->clear();
  }

  SetConstReverseIterator crbegin() const {
    return {_raw, _raw->crbegin(), _tokenizer.create_weak()};
  }

  SetConstReverseIterator crend() const {
    return {_raw, _raw->crend(), _tokenizer.create_weak()};
  }

  SetConstIterator end() const {
    return {_raw, _raw->end(), _tokenizer.create_weak()};
  }

  SetConstReverseIterator rbegin() const {
    return {_raw, _raw->rbegin(), _tokenizer.create_weak()};
  }

  SetConstReverseIterator rend() const {
    return {_raw, _raw->rend(), _tokenizer.create_weak()};
  }

  std::size_t size() const { return _raw->size(); }

  SetPythonIterator to_python_iterator() const {
    return {_raw->cbegin(), _raw, _tokenizer.create_shared()};
  }

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

  VectorIterator begin() {
    return {_raw, _raw->begin(), _tokenizer.create_weak()};
  }

  VectorConstIterator cbegin() const {
    return {_raw, _raw->cbegin(), _tokenizer.create_weak()};
  }

  VectorConstIterator cend() const {
    return {_raw, _raw->cend(), _tokenizer.create_weak()};
  }

  VectorConstReverseIterator crbegin() const {
    return {_raw, _raw->crbegin(), _tokenizer.create_weak()};
  }

  VectorConstReverseIterator crend() const {
    return {_raw, _raw->crend(), _tokenizer.create_weak()};
  }

  void clear() {
    _tokenizer.reset();
    _raw->clear();
  }

  bool empty() const { return _raw->empty(); }

  VectorIterator end() { return {_raw, _raw->end(), _tokenizer.create_weak()}; }

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

  VectorReverseIterator rbegin() {
    return {_raw, _raw->rbegin(), _tokenizer.create_weak()};
  }

  VectorReverseIterator rend() {
    return {_raw, _raw->rend(), _tokenizer.create_weak()};
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

  VectorPythonIterator to_python_iterator() const {
    return {_raw->cbegin(), _raw, _tokenizer.create_shared()};
  }

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
      .def("__iter__", &Map::to_python_iterator)
      .def("__repr__", to_repr<Map>)
      .def("__setitem__", &Map::set_item, py::arg("key"), py::arg("value"))
      .def("begin", &Map::begin)
      .def("cbegin", &Map::cbegin)
      .def("cend", &Map::cend)
      .def("clear", &Map::clear)
      .def("crbegin", &Map::crbegin)
      .def("crend", &Map::crend)
      .def("empty", &Map::empty)
      .def("end", &Map::end)
      .def("rbegin", &Map::rbegin)
      .def("rend", &Map::rend)
      .def("size", &Map::size);

  py::class_<MapConstIterator> PyMapConstIterator(PyMap, CONST_ITERATOR_NAME);
  PyMapConstIterator.def(py::self == py::self)
      .def(py::self != py::self)
      .def_property_readonly("value", &get_iterator_value<MapConstIterator>);

  py::class_<MapConstReverseIterator> PyMapConstReverseIterator(
      PyMap, CONST_REVERSE_ITERATOR_NAME);
  PyMapConstReverseIterator.def(py::self == py::self)
      .def(py::self != py::self)
      .def_property_readonly("value",
                             &get_iterator_value<MapConstReverseIterator>);

  PyMap.attr(ITERATOR_NAME) = PyMapConstIterator;
  PyMap.attr(REVERSE_ITERATOR_NAME) = PyMapConstReverseIterator;

  py::class_<MapPythonIterator>(m, MAP_PYTHON_ITERATOR_NAME)
      .def("__iter__", &identity<const MapPythonIterator&>)
      .def("__next__", &MapPythonIterator::next);

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
      .def("__iter__", &Set::to_python_iterator)
      .def("__repr__", to_repr<Set>)
      .def("begin", &Set::begin)
      .def("cbegin", &Set::cbegin)
      .def("cend", &Set::cend)
      .def("clear", &Set::clear)
      .def("crbegin", &Set::crbegin)
      .def("crend", &Set::crend)
      .def("empty", &Set::empty)
      .def("end", &Set::end)
      .def("rbegin", &Set::rbegin)
      .def("rend", &Set::rend)
      .def("size", &Set::size);

  py::class_<SetConstIterator> PySetConstIterator(PySet, CONST_ITERATOR_NAME);
  PySetConstIterator.def(py::self == py::self)
      .def(py::self != py::self)
      .def_property_readonly("value", &get_iterator_value<SetConstIterator>);

  py::class_<SetConstReverseIterator> PySetConstReverseIterator(
      PySet, CONST_REVERSE_ITERATOR_NAME);
  PySetConstReverseIterator.def(py::self == py::self)
      .def(py::self != py::self)
      .def_property_readonly("value",
                             &get_iterator_value<SetConstReverseIterator>);

  PySet.attr(ITERATOR_NAME) = PySetConstIterator;
  PySet.attr(REVERSE_ITERATOR_NAME) = PySetConstReverseIterator;

  py::class_<SetPythonIterator>(m, SET_PYTHON_ITERATOR_NAME)
      .def("__iter__", &identity<const SetPythonIterator&>)
      .def("__next__", &SetPythonIterator::next);

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
      .def("__iter__", &Vector::to_python_iterator)
      .def("__repr__", to_repr<Vector>)
      .def("__setitem__", &Vector::set_item, py::arg("index"), py::arg("value"))
      .def("begin", &Vector::begin)
      .def("cbegin", &Vector::cbegin)
      .def("cend", &Vector::cend)
      .def("clear", &Vector::clear)
      .def("crbegin", &Vector::crbegin)
      .def("crend", &Vector::crend)
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

  py::class_<VectorConstIterator>(PyVector, CONST_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self + Index{})
      .def(py::self - Index{})
      .def(py::self += Index{})
      .def(py::self -= Index{})
      .def_property_readonly("value", &get_iterator_value<VectorConstIterator>);

  py::class_<VectorConstReverseIterator>(PyVector, CONST_REVERSE_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self + Index{})
      .def(py::self - Index{})
      .def(py::self += Index{})
      .def(py::self -= Index{})
      .def_property_readonly("value",
                             &get_iterator_value<VectorConstReverseIterator>);

  py::class_<VectorIterator>(PyVector, ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self + Index{})
      .def(py::self - Index{})
      .def(py::self += Index{})
      .def(py::self -= Index{})
      .def_property("value", &get_iterator_value<VectorIterator>,
                    &set_iterator_value<VectorIterator>);

  py::class_<VectorReverseIterator>(PyVector, REVERSE_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self + Index{})
      .def(py::self - Index{})
      .def(py::self += Index{})
      .def(py::self -= Index{})
      .def_property("value", &get_iterator_value<VectorReverseIterator>,
                    &set_iterator_value<VectorReverseIterator>);

  py::class_<VectorPythonIterator>(m, VECTOR_PYTHON_ITERATOR_NAME)
      .def("__iter__", &identity<const VectorPythonIterator&>)
      .def("__next__", &VectorPythonIterator::next);
}
