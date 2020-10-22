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
#define REVERSED_ITERATOR_NAME "reversed_iterator"
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
struct ToReversedBegin {
  typename RawCollection::const_reverse_iterator operator()(
      const RawCollection& collection) const {
    return std::crbegin(collection);
  }
};

template <class RawCollection>
struct ToReversedEnd {
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
      std::conditional_t<reversed, ToReversedBegin<RawCollection>,
                         ToBegin<RawCollection>>;
  using Exhauster = std::conditional_t<reversed, ToReversedEnd<RawCollection>,
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

  operator bool() const { return !_raw->empty(); }

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

  bool contains(Object key) const { return _raw->find(key) != _raw->end(); }

  void delete_item(Object key) {
    auto position = _raw->find(key);
    if (position == _raw->end())
      throw py::value_error(to_repr(key) + " is not found.");
    _tokenizer.reset();
    _raw->erase(position);
  }

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
    if (map) {
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

  Set operator&(const Set& other) const {
    RawSet raw;
    std::set_intersection(_raw->begin(), _raw->end(), other._raw->begin(),
                          other._raw->end(), std::inserter(raw, raw.end()));
    return {raw};
  }

  Set& operator&=(const Set& other) {
    RawSet raw;
    std::set_intersection(_raw->begin(), _raw->end(), other._raw->begin(),
                          other._raw->end(), std::inserter(raw, raw.end()));
    if (raw.size() != _raw->size()) {
      _tokenizer.reset();
      *_raw = raw;
    }
    return *this;
  }

  Set operator-(const Set& other) const {
    RawSet raw;
    std::set_difference(_raw->begin(), _raw->end(), other._raw->begin(),
                        other._raw->end(), std::inserter(raw, raw.end()));
    return {raw};
  }

  Set& operator-=(const Set& other) {
    RawSet common_values;
    std::set_intersection(other._raw->begin(), other._raw->end(), _raw->begin(),
                          _raw->end(),
                          std::inserter(common_values, common_values.end()));
    if (!common_values.empty()) {
      _tokenizer.reset();
      RawSet* result = new RawSet{};
      std::set_difference(_raw->begin(), _raw->end(), common_values.begin(),
                          common_values.end(),
                          std::inserter(*result, result->end()));
      _raw.reset(result);
    }
    return *this;
  }

  bool operator<(const Set& other) const {
    const auto& raw = *_raw;
    const auto& other_raw = *other._raw;
    const auto& other_end = other_raw.cend();
    if (raw.size() >= other_raw.size()) return false;
    for (const auto& element : raw)
      if (other_raw.find(element) == other_end) return false;
    return true;
  }

  bool operator<=(const Set& other) const {
    const auto& raw = *_raw;
    const auto& other_raw = *other._raw;
    const auto& other_end = other_raw.cend();
    if (raw.size() > other_raw.size()) return false;
    for (const auto& element : raw)
      if (other_raw.find(element) == other_end) return false;
    return true;
  }

  bool operator==(const Set& other) const { return *_raw == *other._raw; }

  Set operator^(const Set& other) const {
    RawSet raw;
    std::set_symmetric_difference(_raw->begin(), _raw->end(),
                                  other._raw->begin(), other._raw->end(),
                                  std::inserter(raw, raw.end()));
    return {raw};
  }

  Set& operator^=(const Set& other) {
    if (other) {
      _tokenizer.reset();
      RawSet* result = new RawSet{};
      std::set_symmetric_difference(_raw->begin(), _raw->end(),
                                    other._raw->begin(), other._raw->end(),
                                    std::inserter(*result, result->end()));
      _raw.reset(result);
    }
    return *this;
  }

  Set operator|(const Set& other) const {
    RawSet raw;
    std::set_union(other._raw->begin(), other._raw->end(), _raw->begin(),
                   _raw->end(), std::inserter(raw, raw.end()));
    return {raw};
  }

  Set& operator|=(const Set& other) {
    RawSet extra_values;
    std::set_difference(other._raw->begin(), other._raw->end(), _raw->begin(),
                        _raw->end(),
                        std::inserter(extra_values, extra_values.end()));
    if (!extra_values.empty()) {
      _tokenizer.reset();
      _raw->insert(extra_values.begin(), extra_values.end());
    }
    return *this;
  }

  operator bool() const { return !_raw->empty(); }

  static Set from_state(IterableState state) {
    RawSet raw;
    for (auto& element : state)
      raw.insert(py::reinterpret_borrow<Object>(element));
    return {raw};
  }

  void add(Object value) {
    auto position = _raw->find(value);
    if (position != _raw->end()) return;
    _tokenizer.reset();
    _raw->insert(value);
  }

  SetForwardIterator begin() const {
    return {_raw, _raw->begin(), _tokenizer.create()};
  }

  void clear() {
    _tokenizer.reset();
    return _raw->clear();
  }

  bool contains(Object value) const { return _raw->find(value) != _raw->end(); }

  void discard(Object value) {
    auto position = _raw->find(value);
    if (position == _raw->end()) return;
    _tokenizer.reset();
    _raw->erase(position);
  }

  SetForwardIterator end() const {
    return {_raw, _raw->end(), _tokenizer.create()};
  }

  bool isdisjoint(const Set& other) const {
    const auto& raw = *_raw;
    const auto& other_raw = *other._raw;
    if (raw.size() < other_raw.size()) {
      const auto& other_end = other_raw.cend();
      for (const auto& element : raw)
        if (other_raw.find(element) != other_end) return false;
    } else {
      const auto& end = raw.cend();
      for (const auto& element : other_raw)
        if (raw.find(element) != end) return false;
    }
    return true;
  }

  Object max() const {
    if (_raw->empty()) throw py::value_error("Set is empty.");
    return *_raw->rbegin();
  }

  Object min() const {
    if (_raw->empty()) throw py::value_error("Set is empty.");
    return *_raw->begin();
  }

  Object pop() {
    if (_raw->empty()) throw py::value_error("Set is empty.");
    _tokenizer.reset();
    const auto position = _raw->cbegin();
    _raw->erase(position);
    return *position;
  }

  SetBackwardIterator rbegin() const {
    return {_raw, _raw->rbegin(), _tokenizer.create()};
  }

  void remove(Object value) {
    auto position = _raw->find(value);
    if (position == _raw->end())
      throw py::value_error(to_repr(value) + " is not found.");
    _tokenizer.reset();
    _raw->erase(position);
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
    if (set) {
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
  Vector(const RawVector& raw)
      : _raw(std::make_shared<RawVector>(raw)), _tokenizer() {}

  bool operator==(const Vector& other) const { return *_raw == *other._raw; }

  bool operator<(const Vector& other) const {
    return std::lexicographical_compare(_raw->begin(), _raw->end(),
                                        other._raw->begin(), other._raw->end());
  }

  bool operator<=(const Vector& other) const {
    return *this < other || *this == other;
  }

  Vector operator+(const Vector& other) const {
    RawVector raw{*_raw};
    raw.insert(raw.end(), other._raw->begin(), other._raw->end());
    return {raw};
  }

  operator bool() const { return !_raw->empty(); }

  static Vector from_state(IterableState state) {
    RawVector raw;
    raw.reserve(state.size());
    for (auto& element : state)
      raw.push_back(py::reinterpret_borrow<Object>(element));
    return {raw};
  }

  VectorForwardIterator begin() const {
    return {_raw, _raw->begin(), _tokenizer.create()};
  }

  void clear() {
    _tokenizer.reset();
    _raw->clear();
  }

  bool contains(Object value) const {
    return std::find(_raw->begin(), _raw->end(), value) != _raw->end();
  }

  std::size_t count(Object value) const {
    return std::count(_raw->begin(), _raw->end(), value);
  }

  void delete_item(Index index) {
    Index size = _raw->size();
    Index normalized_index = index >= 0 ? index : index + size;
    if (normalized_index < 0 || normalized_index >= size)
      throw py::index_error(size ? (std::string("Index should be in range(" +
                                                std::to_string(-size) + ", ") +
                                    std::to_string(size) + "), but found " +
                                    std::to_string(index) + ".")
                                 : std::string("Sequence is empty."));
    _tokenizer.reset();
    _raw->erase(_raw->begin() + normalized_index);
  }

  void delete_items(py::slice slice) {
    auto size = _raw->size();
    std::size_t raw_start, raw_stop, raw_step, slice_length;
    if (!slice.compute(size, &raw_start, &raw_stop, &raw_step, &slice_length))
      throw py::error_already_set();
    auto start = static_cast<Index>(raw_start);
    auto stop = static_cast<Index>(raw_stop);
    auto step = static_cast<Index>(raw_step);
    if (step > 0 ? start >= stop : start <= stop) return;
    if (slice_length) _tokenizer.reset();
    if (step == 1)
      _raw->erase(std::next(_raw->begin(), start),
                  std::next(_raw->begin(), stop));
    else if (step == -1)
      _raw->erase(std::next(_raw->begin(), stop + 1),
                  std::next(_raw->begin(), start + 1));
    else if (step > 0) {
      const auto& begin = _raw->begin();
      RawVector raw{begin, std::next(begin, start)};
      raw.reserve(size - slice_length);
      for (; step < stop - start; start += step)
        raw.insert(raw.end(), std::next(begin, start + 1),
                   std::next(begin, start + step));
      raw.insert(raw.end(), std::next(begin, start + 1), _raw->end());
      _raw->assign(raw.begin(), raw.end());
    } else {
      start = size - start - 1;
      stop = size - stop - 1;
      step = -step;
      const auto& rbegin = _raw->rbegin();
      RawVector raw{rbegin, std::next(rbegin, start)};
      raw.reserve(size - slice_length);
      for (; step < stop - start; start += step)
        raw.insert(raw.end(), std::next(rbegin, start + 1),
                   std::next(rbegin, start + step));
      raw.insert(raw.end(), std::next(rbegin, start + 1), _raw->rend());
      _raw->assign(raw.rbegin(), raw.rend());
    }
  }

  VectorForwardIterator end() const {
    return {_raw, _raw->end(), _tokenizer.create()};
  }

  void extend(py::iterable iterable) {
    auto iterator = py::iter(iterable);
    if (iterator == py::iterator::sentinel()) return;
    _tokenizer.reset();
    while (iterator != py::iterator::sentinel())
      _raw->emplace_back(*(iterator++), true);
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

  Vector get_items(py::slice slice) const {
    std::size_t raw_start, raw_stop, raw_step, slice_length;
    if (!slice.compute(_raw->size(), &raw_start, &raw_stop, &raw_step,
                       &slice_length))
      throw py::error_already_set();
    auto start = static_cast<Index>(raw_start);
    auto stop = static_cast<Index>(raw_stop);
    auto step = static_cast<Index>(raw_step);
    RawVector raw;
    raw.reserve(slice_length);
    if (step < 0)
      for (; start > stop; start += step) raw.push_back((*_raw)[start]);
    else
      for (; start < stop; start += step) raw.push_back((*_raw)[start]);
    return Vector{raw};
  }

  Index index(Object value, Index start, Index stop) const {
    Index size = _raw->size();
    auto normalized_start =
        std::max(std::min(start >= 0 ? start : start + size, size),
                 static_cast<Index>(0));
    auto normalized_stop = std::max(
        std::min(stop >= 0 ? stop : stop + size, size), static_cast<Index>(0));
    for (Index index = normalized_start; index < normalized_stop; ++index)
      if ((*_raw)[index] == value) return index;
    throw py::value_error(to_repr(value) + " is not found in slice(" +
                          std::to_string(normalized_start) + ", " +
                          std::to_string(normalized_stop) + ").");
  }

  void insert(Index index, Object value) {
    _tokenizer.reset();
    Index size = _raw->size();
    std::size_t normalized_index =
        std::max(std::min(index >= 0 ? index : index + size, size),
                 static_cast<Index>(0));
    _raw->insert(_raw->begin() + normalized_index, value);
  }

  Object pop(Index index) {
    Index size = _raw->size();
    Index normalized_index = index >= 0 ? index : index + size;
    if (normalized_index < 0 || normalized_index >= size)
      throw py::index_error(size ? (std::string("Index should be in range(" +
                                                std::to_string(-size) + ", ") +
                                    std::to_string(size) + "), but found " +
                                    std::to_string(index) + ".")
                                 : std::string("Vector is empty."));
    _tokenizer.reset();
    if (normalized_index == size - 1) {
      auto result = _raw->back();
      _raw->pop_back();
      return result;
    }
    auto result = (*_raw)[normalized_index];
    _raw->erase(std::next(_raw->begin(), normalized_index));
    return result;
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

  void remove(Object value) {
    const auto& end = _raw->end();
    const auto& position = std::find(_raw->begin(), end, value);
    if (position == end)
      throw py::value_error(to_repr(value) + " is not found.");
    _tokenizer.reset();
    _raw->erase(position);
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

  void reverse() {
    _tokenizer.reset();
    std::reverse(_raw->begin(), _raw->end());
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

  void set_items(py::slice slice, py::iterable iterable) {
    auto size = _raw->size();
    std::size_t raw_start, raw_stop, raw_step, slice_length;
    if (!slice.compute(size, &raw_start, &raw_stop, &raw_step, &slice_length))
      throw py::error_already_set();
    auto start = static_cast<Index>(raw_start);
    auto stop = static_cast<Index>(raw_stop);
    auto step = static_cast<Index>(raw_step);
    RawVector values;
    auto iterator = py::iter(iterable);
    while (iterator != py::iterator::sentinel())
      values.emplace_back(*(iterator++), true);
    auto values_count = values.size();
    if (step == 1) {
      auto new_size = size - slice_length + values_count;
      if (values_count || slice_length) _tokenizer.reset();
      if (new_size > size) {
        _raw->resize(new_size, py::none{});
        const auto& last_replaced =
            std::next(_raw->begin(), std::max(start, stop) - 1);
        for (auto source = std::next(_raw->begin(), size - 1),
                  destination = std::next(_raw->begin(), new_size - 1);
             source != last_replaced; --source, --destination)
          std::iter_swap(source, destination);
      } else if (new_size < size) {
        const auto& old_end = _raw->end();
        for (auto source = std::next(_raw->begin(), stop),
                  destination = std::next(_raw->begin(), start + values_count);
             source != old_end; ++source, ++destination)
          std::iter_swap(source, destination);
        _raw->erase(std::next(_raw->begin(), new_size), old_end);
      }
      std::copy(values.begin(), values.end(), std::next(_raw->begin(), start));
      return;
    }
    if (slice_length != values_count)
      throw py::value_error(
          std::string("Attempt to assign iterable with capacity") +
          std::to_string(values_count) + " to slice with size " +
          std::to_string(slice_length) + ".");
    if (values_count || slice_length) _tokenizer.reset();
    auto position = values.begin();
    if (step < 0)
      for (; start > stop; start += step) (*_raw)[start] = *(position++);
    else
      for (; start < stop; start += step) (*_raw)[start] = *(position++);
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
    if (vector) {
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
      .def("__bool__", &Map::operator bool)
      .def("__contains__", &Map::contains, py::arg("key"))
      .def("__delitem__", &Map::delete_item, py::arg("key"))
      .def("__len__", &Map::size)
      .def("__repr__", to_repr<Map>)
      .def("__setitem__", &Map::set_item, py::arg("key"), py::arg("value"))
      .def("begin", &Map::begin)
      .def("clear", &Map::clear)
      .def("end", &Map::end)
      .def("rbegin", &Map::rbegin)
      .def("rend", &Map::rend);

  py::class_<MapBackwardIterator>(PyMap, REVERSED_ITERATOR_NAME)
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
      .def(py::self & py::self)
      .def(py::self &= py::self)
      .def(py::self - py::self)
      .def(py::self -= py::self)
      .def(py::self < py::self)
      .def(py::self <= py::self)
      .def(py::self == py::self)
      .def(py::self ^ py::self)
      .def(py::self ^= py::self)
      .def(py::self | py::self)
      .def(py::self |= py::self)
      .def(py::pickle(&iterable_to_state<Set>, &Set::from_state))
      .def("__bool__", &Set::operator bool)
      .def("__contains__", &Set::contains)
      .def("__len__", &Set::size)
      .def("__repr__", to_repr<Set>)
      .def("add", &Set::add)
      .def("begin", &Set::begin)
      .def("clear", &Set::clear)
      .def("discard", &Set::discard)
      .def("isdisjoint", &Set::isdisjoint)
      .def("end", &Set::end)
      .def("max", &Set::max)
      .def("min", &Set::min)
      .def("pop", &Set::pop)
      .def("rbegin", &Set::rbegin)
      .def("remove", &Set::remove, py::arg("value"))
      .def("rend", &Set::rend);

  py::class_<SetBackwardIterator>(PySet, REVERSED_ITERATOR_NAME)
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
      .def(py::self + py::self)
      .def(py::pickle(&iterable_to_state<Vector>, &Vector::from_state))
      .def("__bool__", &Vector::operator bool)
      .def("__contains__", &Vector::contains, py::arg("value"))
      .def("__delitem__", &Vector::delete_item, py::arg("index"))
      .def("__delitem__", &Vector::delete_items, py::arg("slice"))
      .def("__getitem__", &Vector::get_item, py::arg("index"))
      .def("__getitem__", &Vector::get_items, py::arg("slice"))
      .def(
          "__iadd__",
          [](Vector& self, py::iterable iterable) {
            self.extend(iterable);
            return self;
          },
          py::arg("values"), py::is_operator{})
      .def("__len__", &Vector::size)
      .def("__repr__", to_repr<Vector>)
      .def("__setitem__", &Vector::set_item, py::arg("index"), py::arg("value"))
      .def("__setitem__", &Vector::set_items, py::arg("slice"),
           py::arg("values"))
      .def("append", &Vector::push_back, py::arg("value"))
      .def("begin", &Vector::begin)
      .def("clear", &Vector::clear)
      .def("count", &Vector::count, py::arg("value"))
      .def("end", &Vector::end)
      .def("extend", &Vector::extend, py::arg("values"))
      .def("index", &Vector::index, py::arg("value"), py::arg("start") = 0,
           py::arg("stop") = std::numeric_limits<Index>::max())
      .def("insert", &Vector::insert, py::arg("index"), py::arg("value"))
      .def("pop", &Vector::pop, py::arg("index") = -1)
      .def("pop_back", &Vector::pop_back)
      .def("push_back", &Vector::push_back, py::arg("value"))
      .def("rbegin", &Vector::rbegin)
      .def("remove", &Vector::remove, py::arg("value"))
      .def("rend", &Vector::rend)
      .def("reserve", &Vector::reserve, py::arg("capacity"))
      .def("resize", &Vector::resize, py::arg("size"),
           py::arg("value") = py::none())
      .def("reverse", &Vector::reverse);

  py::class_<VectorBackwardIterator>(PyVector, REVERSED_ITERATOR_NAME)
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
