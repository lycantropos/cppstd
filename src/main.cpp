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

template <class RawCollection, bool constant, bool reversed>
class BaseIterator {
 public:
  using ConstPosition =
      std::conditional_t<reversed,
                         typename RawCollection::const_reverse_iterator,
                         typename RawCollection::const_iterator>;
  using Position = std::conditional_t<
      constant, ConstPosition,
      std::conditional_t<reversed, typename RawCollection::reverse_iterator,
                         typename RawCollection::iterator>>;
  using ConstValueRef =
      const typename std::iterator_traits<Position>::value_type&;
  using ValueRef =
      std::conditional_t<constant, ConstValueRef,
                         typename std::iterator_traits<Position>::value_type&>;

  BaseIterator(std::weak_ptr<RawCollection> raw_collection_ptr,
               Position position, const Token& token)
      : _raw_collection_ptr(raw_collection_ptr),
        position(position),
        _token(token){};

  const typename std::iterator_traits<Position>::value_type& operator*() const {
    return *position;
  }

  ValueRef operator*() { return *position; }

  Position& to_position() {
    validate();
    return position;
  }

  const Position& to_position() const {
    validate();
    return position;
  }

  void validate_comparison_with(
      const BaseIterator<RawCollection, constant, reversed>& other) const {
    if (!are_addresses_equal(to_raw_collection(), other.to_raw_collection()))
      throw std::runtime_error(
          "Comparing iterators from different collections is undefined.");
  }

  ConstPosition to_begin() const {
    static const Replenisher replenish;
    return replenish(to_raw_collection());
  }

  ConstPosition to_end() const {
    static const Exhauster exhaust;
    return exhaust(to_raw_collection());
  }

  BaseIterator<RawCollection, constant, reversed> with_position(
      Position position) const {
    return BaseIterator<RawCollection, constant, reversed>{_raw_collection_ptr,
                                                           position, _token};
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
    if (auto* ptr = _raw_collection_ptr.lock().get())
      return *ptr;
    else
      throw py::value_error("Iterator is invalidated.");
  }

  void validate() const {
    if (_token.expired()) throw py::value_error("Iterator is invalidated.");
  }
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

template <class RawCollection, bool constant, bool reversed>
static typename BaseIterator<RawCollection, constant, reversed>::Position
to_advanced_position(
    const BaseIterator<RawCollection, constant, reversed>& iterator,
    Index offset) {
  const auto& position = iterator.to_position();
  const typename BaseIterator<RawCollection, constant, reversed>::ConstPosition&
      const_position = position;
  const auto begin = iterator.to_begin();
  const auto end = iterator.to_end();
  Index min_offset = -std::distance(begin, const_position);
  Index max_offset = std::distance(const_position, end);
  if (offset < min_offset || offset > max_offset) {
    throw std::runtime_error(
        position == end
            ? std::string("Advancing of placeholder iterators is undefined.")
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
        "Post-incrementing of placeholder iterators is undefined.");
  return iterator.with_position(position++);
}

template <class RawCollection, bool constant, bool reversed>
BaseIterator<RawCollection, constant, reversed>& operator++(
    BaseIterator<RawCollection, constant, reversed>& iterator, int) {
  auto& position = iterator.to_position();
  if (position == iterator.to_end())
    throw std::runtime_error(
        "Pre-incrementing of placeholder iterators is undefined.");
  ++position;
  return iterator;
}

template <class It>
typename It::ConstValueRef get_iterator_value(const It& iterator) {
  if (iterator.to_position() == iterator.to_end())
    throw std::runtime_error(
        "Reading value of placeholder iterators is undefined.");
  return *iterator;
}

template <class It>
void set_iterator_value(It& iterator, typename It::ConstValueRef value) {
  if (iterator.to_position() == iterator.to_end())
    throw std::runtime_error(
        "Setting value of placeholder iterators is undefined.");
  *iterator = value;
}

template <class RawCollection>
using ConstIterator = BaseIterator<RawCollection, true, false>;
template <class RawCollection>
using ConstReverseIterator = BaseIterator<RawCollection, true, true>;
template <class RawCollection>
using Iterator = BaseIterator<RawCollection, false, false>;
template <class RawCollection>
using ReverseIterator = BaseIterator<RawCollection, false, true>;

using MapConstIterator = ConstIterator<RawMap>;
using MapConstReverseIterator = ConstReverseIterator<RawMap>;
using SetConstIterator = ConstIterator<RawSet>;
using SetConstReverseIterator = ConstReverseIterator<RawSet>;
using VectorIterator = Iterator<RawVector>;
using VectorReverseIterator = ReverseIterator<RawVector>;
using VectorConstIterator = ConstIterator<RawVector>;
using VectorConstReverseIterator = ConstReverseIterator<RawVector>;

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
    return {_raw, _raw->begin(), _tokenizer.create()};
  }

  MapConstIterator cbegin() const {
    return {_raw, _raw->cbegin(), _tokenizer.create()};
  }

  MapConstIterator cend() const {
    return {_raw, _raw->cend(), _tokenizer.create()};
  }

  void clear() {
    _tokenizer.reset();
    return _raw->clear();
  }

  MapConstReverseIterator crbegin() const {
    return {_raw, _raw->crbegin(), _tokenizer.create()};
  }

  MapConstReverseIterator crend() const {
    return {_raw, _raw->crend(), _tokenizer.create()};
  }

  bool empty() const { return _raw->empty(); }

  MapConstIterator end() const {
    return {_raw, _raw->end(), _tokenizer.create()};
  }

  MapConstReverseIterator rbegin() const {
    return {_raw, _raw->rbegin(), _tokenizer.create()};
  }

  MapConstReverseIterator rend() const {
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

  SetConstIterator begin() const {
    return {_raw, _raw->begin(), _tokenizer.create()};
  }

  SetConstIterator cbegin() const {
    return {_raw, _raw->cbegin(), _tokenizer.create()};
  }

  SetConstIterator cend() const {
    return {_raw, _raw->cend(), _tokenizer.create()};
  }

  void clear() {
    _tokenizer.reset();
    return _raw->clear();
  }

  SetConstReverseIterator crbegin() const {
    return {_raw, _raw->crbegin(), _tokenizer.create()};
  }

  SetConstReverseIterator crend() const {
    return {_raw, _raw->crend(), _tokenizer.create()};
  }

  SetConstIterator end() const {
    return {_raw, _raw->end(), _tokenizer.create()};
  }

  SetConstReverseIterator rbegin() const {
    return {_raw, _raw->rbegin(), _tokenizer.create()};
  }

  SetConstReverseIterator rend() const {
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

  VectorIterator begin() { return {_raw, _raw->begin(), _tokenizer.create()}; }

  VectorConstIterator cbegin() const {
    return {_raw, _raw->cbegin(), _tokenizer.create()};
  }

  VectorConstIterator cend() const {
    return {_raw, _raw->cend(), _tokenizer.create()};
  }

  VectorConstReverseIterator crbegin() const {
    return {_raw, _raw->crbegin(), _tokenizer.create()};
  }

  VectorConstReverseIterator crend() const {
    return {_raw, _raw->crend(), _tokenizer.create()};
  }

  void clear() {
    _tokenizer.reset();
    _raw->clear();
  }

  bool empty() const { return _raw->empty(); }

  VectorIterator end() { return {_raw, _raw->end(), _tokenizer.create()}; }

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
    return {_raw, _raw->rbegin(), _tokenizer.create()};
  }

  VectorReverseIterator rend() {
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
}
