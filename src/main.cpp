#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace py = pybind11;

#define MODULE_NAME _cppstd
#define C_STR_HELPER(a) #a
#define C_STR(a) C_STR_HELPER(a)
#define SET_BACKWARD_ITERATOR_NAME "SetBackwardIterator"
#define SET_FORWARD_ITERATOR_NAME "SetForwardIterator"
#define SET_NAME "Set"
#define VECTOR_BACKWARD_ITERATOR_NAME "VectorBackwardIterator"
#define VECTOR_FORWARD_ITERATOR_NAME "VectorForwardIterator"
#define VECTOR_NAME "Vector"
#ifndef VERSION_INFO
#define VERSION_INFO "dev"
#endif

using Index = Py_ssize_t;
using Object = py::object;
using RawSet = std::set<Object>;
using RawVector = std::vector<Object>;
using Tokenizer = std::shared_ptr<bool>;
using Token = std::weak_ptr<bool>;

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

  Iterator(std::weak_ptr<RawCollection> raw_collection_ptr, Position&& position,
           Token token)
      : _raw_collection_ptr(raw_collection_ptr),
        position(position),
        _token(token){};

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
    return to_position() == other.to_position();
  }

  const typename RawCollection::value_type& next() {
    validate();
    if (position == to_end()) throw py::stop_iteration();
    auto current_position = position;
    position = std::next(position);
    return *current_position;
  }

  Position to_position() const {
    validate();
    return position;
  }

  bool has_same_collection_with(
      const Iterator<RawCollection, reversed>& other) const {
    return are_addresses_equal(to_raw_collection(), other.to_raw_collection());
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

  void advance(Index offset) { position = to_advanced_position(offset); }

  Position to_begin() const {
    static const Replenisher replenish;
    return replenish(to_raw_collection());
  }

  Position to_end() const {
    static const Exhauster exhaust;
    return exhaust(to_raw_collection());
  }

  Iterator to_advanced(Index offset) const {
    return {_raw_collection_ptr, to_advanced_position(offset), _token};
  }

  Position to_advanced_position(Index offset) const {
    Index min_offset = std::distance(position, to_begin());
    Index max_offset = std::distance(position, to_end());
    if (offset < min_offset || offset > max_offset) {
      throw py::value_error(to_raw_collection().empty()
                                ? std::string("Sequence is empty.")
                                : (std::string("Offset should be in range(") +
                                   std::to_string(min_offset) + ", " +
                                   std::to_string(max_offset + 1) +
                                   "), but found " + std::to_string(offset) +
                                   "."));
    }
    return std::next(position, offset);
  }

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

template <class RawCollection>
using BackwardIterator = Iterator<RawCollection, true>;
template <class RawCollection>
using ForwardIterator = Iterator<RawCollection, false>;

using SetBackwardIterator = BackwardIterator<RawSet>;
using SetForwardIterator = ForwardIterator<RawSet>;
using VectorBackwardIterator = BackwardIterator<RawVector>;
using VectorForwardIterator = ForwardIterator<RawVector>;

namespace pybind11 {
static std::ostream& operator<<(std::ostream& stream, const Object& object) {
  return stream << std::string(py::repr(object));
}

static bool operator==(const Object& left, const Object& right) {
  return left.equal(right);
}
}  // namespace pybind11

class Set {
 public:
  Set(const RawSet& raw)
      : _raw(std::make_shared<RawSet>(raw)),
        _tokenizer(std::make_shared<bool>(false)) {}

  ~Set() { reset_tokenizer(); }

  operator bool() const { return !_raw->empty(); }

  bool operator==(const Set& other) const { return *_raw == *other._raw; }

  SetForwardIterator begin() const { return {_raw, _raw->begin(), _tokenizer}; }

  SetBackwardIterator rbegin() const {
    return {_raw, _raw->rbegin(), _tokenizer};
  }

  std::size_t size() const { return _raw->size(); }

  const RawSet& to_raw() const { return *_raw; }

  void remove(Object value) {
    auto position = _raw->find(value);
    if (position == _raw->end())
      throw py::value_error(repr(value) + " is not found.");
    reset_tokenizer();
    _raw->erase(position);
  }

  using State = py::list;

  State to_state() const {
    State result;
    for (const auto& element : *_raw) result.append(element);
    return result;
  }

  static Set from_state(State state) {
    RawSet raw;
    for (auto& element : state)
      raw.insert(py::reinterpret_borrow<Object>(element));
    return {raw};
  }

 private:
  std::shared_ptr<RawSet> _raw;
  Tokenizer _tokenizer;

  void reset_tokenizer() { _tokenizer.reset(new bool(false)); }
};

static std::ostream& operator<<(std::ostream& stream, const Set& set) {
  stream << C_STR(MODULE_NAME) "." SET_NAME "(";
  auto object = py::cast(set);
  if (Py_ReprEnter(object.ptr()) == 0) {
    const auto& raw = set.to_raw();
    if (!raw.empty()) {
      auto position = raw.cbegin();
      stream << *(position++);
      for (; position != raw.end(); ++position) stream << ", " << *position;
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
      : _raw(std::make_shared<RawVector>(raw)),
        _tokenizer(std::make_shared<bool>(false)) {}

  ~Vector() { reset_tokenizer(); }

  bool operator==(const Vector& other) const { return *_raw == *other._raw; }

  bool operator<(const Vector& other) const {
    return std::lexicographical_compare(_raw->begin(), _raw->end(),
                                        other._raw->begin(), other._raw->end());
  }

  bool operator<=(const Vector& other) const {
    return *this < other || *this == other;
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
      if (values_count || slice_length) reset_tokenizer();
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
    if (values_count || slice_length) reset_tokenizer();
    auto position = values.begin();
    if (step < 0)
      for (; start > stop; start += step) (*_raw)[start] = *(position++);
    else
      for (; start < stop; start += step) (*_raw)[start] = *(position++);
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

  operator bool() const { return !_raw->empty(); }

  VectorForwardIterator begin() const {
    return {_raw, _raw->begin(), _tokenizer};
  }

  bool contains(Object value) const {
    return std::find(_raw->begin(), _raw->end(), value) != _raw->end();
  }

  VectorForwardIterator end() const { return {_raw, _raw->end(), _tokenizer}; }

  VectorBackwardIterator rbegin() const {
    return {_raw, _raw->rbegin(), _tokenizer};
  }

  VectorBackwardIterator rend() const {
    return {_raw, _raw->rend(), _tokenizer};
  }

  std::size_t size() const { return _raw->size(); }

  void extend(py::iterable iterable) {
    auto iterator = py::iter(iterable);
    if (iterator == py::iterator::sentinel()) return;
    reset_tokenizer();
    while (iterator != py::iterator::sentinel())
      _raw->emplace_back(*(iterator++), true);
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
    reset_tokenizer();
    _raw->erase(_raw->begin() + normalized_index);
  }

  void remove(Object value) {
    const auto& end = _raw->end();
    const auto& position = std::find(_raw->begin(), end, value);
    if (position == end) {
      throw py::value_error(repr(value) + " is not found.");
    }
    reset_tokenizer();
    _raw->erase(position);
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
    if (slice_length) reset_tokenizer();
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

  using State = py::list;

  void clear() {
    reset_tokenizer();
    _raw->clear();
  }

  std::size_t count(Object value) const {
    return std::count(_raw->begin(), _raw->end(), value);
  }

  void pop_back() {
    if (_raw->empty()) throw py::index_error("Vector is empty.");
    reset_tokenizer();
    _raw->pop_back();
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
    throw py::value_error(repr(value) + " is not found in slice(" +
                          std::to_string(normalized_start) + ", " +
                          std::to_string(normalized_stop) + ").");
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
    reset_tokenizer();
    if (normalized_index == size - 1) {
      auto result = _raw->back();
      _raw->pop_back();
      return result;
    }
    auto result = (*_raw)[normalized_index];
    _raw->erase(std::next(_raw->begin(), normalized_index));
    return result;
  }

  void push_back(Object value) {
    reset_tokenizer();
    _raw->push_back(value);
  }

  void insert(Index index, Object value) {
    reset_tokenizer();
    Index size = _raw->size();
    std::size_t normalized_index =
        std::max(std::min(index >= 0 ? index : index + size, size),
                 static_cast<Index>(0));
    _raw->insert(_raw->begin() + normalized_index, value);
  }

  State to_state() const {
    State result;
    for (const auto& element : *_raw) result.append(element);
    return result;
  }

  static Vector from_state(State state) {
    RawVector raw;
    raw.reserve(state.size());
    for (auto& element : state)
      raw.push_back(py::reinterpret_borrow<Object>(element));
    return {raw};
  }

  const RawVector& to_raw() const { return *_raw; }

  void reverse() {
    reset_tokenizer();
    std::reverse(_raw->begin(), _raw->end());
  }

  void reserve(std::size_t capacity) { _raw->reserve(capacity); }

  void resize(Index size, Object value) {
    if (size < 0)
      throw py::value_error(std::string("Size should be positive, but found ") +
                            std::to_string(size) + ".");
    reset_tokenizer();
    _raw->resize(size, value);
  }

 private:
  std::shared_ptr<RawVector> _raw;
  Tokenizer _tokenizer;

  void reset_tokenizer() { _tokenizer.reset(new bool(false)); }
};

static std::ostream& operator<<(std::ostream& stream, const Vector& vector) {
  stream << C_STR(MODULE_NAME) "." VECTOR_NAME "(";
  auto object = py::cast(vector);
  if (Py_ReprEnter(object.ptr()) == 0) {
    if (vector) {
      const auto& raw = vector.to_raw();
      stream << raw[0];
      for (std::size_t index = 1; index < raw.size(); ++index)
        stream << ", " << raw[index];
    }
    Py_ReprLeave(object.ptr());
  } else {
    stream << "...";
  }
  return stream << ")";
}

static bool operator<(const VectorBackwardIterator& self,
                      const VectorBackwardIterator& other) {
  return self.has_same_collection_with(other) &&
         self.to_position() < other.to_position();
}

static bool operator<(const VectorForwardIterator& self,
                      const VectorForwardIterator& other) {
  return self.has_same_collection_with(other) &&
         self.to_position() < other.to_position();
}

static bool operator<=(const VectorBackwardIterator& self,
                       const VectorBackwardIterator& other) {
  return self.has_same_collection_with(other) &&
         self.to_position() <= other.to_position();
}

static bool operator<=(const VectorForwardIterator& self,
                       const VectorForwardIterator& other) {
  return self.has_same_collection_with(other) &&
         self.to_position() <= other.to_position();
}

PYBIND11_MODULE(MODULE_NAME, m) {
  m.doc() = R"pbdoc(Partial binding of C++ standard library.)pbdoc";
  m.attr("__version__") = VERSION_INFO;

  py::class_<Set>(m, SET_NAME)
      .def(py::init([](py::args args) {
        RawSet raw;
        for (auto& element : args)
          raw.insert(py::reinterpret_borrow<Object>(element));
        return Set{raw};
      }))
      .def(py::self == py::self)
      .def(py::pickle([](const Set& self) { return self.to_state(); },
                      &Set::from_state))
      .def("__bool__", &Set::operator bool)
      .def("__iter__", &Set::begin)
      .def("__len__", &Set::size)
      .def("__repr__", repr<Set>)
      .def("__reversed__", &Set::rbegin)
      .def("remove", &Set::remove, py::arg("value"));

  py::class_<SetBackwardIterator>(m, SET_BACKWARD_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self + Index{})
      .def(py::self - Index{})
      .def(py::self += Index{})
      .def(py::self -= Index{})
      .def("__iter__",
           [](SetBackwardIterator& self) -> SetBackwardIterator& {
             return self;
           })
      .def("__next__", &SetBackwardIterator::next);

  py::class_<SetForwardIterator>(m, SET_FORWARD_ITERATOR_NAME)
      .def(py::self == py::self)
      .def(py::self + Index{})
      .def(py::self - Index{})
      .def(py::self += Index{})
      .def(py::self -= Index{})
      .def("__iter__",
           [](SetForwardIterator& self) -> SetForwardIterator& { return self; })
      .def("__next__", &SetForwardIterator::next);

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
      .def(py::pickle([](const Vector& self) { return self.to_state(); },
                      &Vector::from_state))
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
      .def("__iter__", &Vector::begin)
      .def("__len__", &Vector::size)
      .def("__repr__", repr<Vector>)
      .def("__reversed__", &Vector::rbegin)
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

  py::module collections_abc = py::module::import("collections.abc");
  collections_abc.attr("MutableSequence").attr("register")(PyVector);

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
