/*
 * eheapq - An extended implementation of Python's heapq.
 * Copyright(C) 2020 Fridolin Pokorny
 *
 * This program is free software: you can redistribute it and / or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <unordered_map>
#include <vector>
#include <functional>
#include <exception>

const long unsigned int EHEAPQ_DEFAULT_SIZE = std::numeric_limits<long unsigned int>::max();

class EHeapQException: public std:: exception {
};

class EHeapQEmpty: public EHeapQException {
  public:
    virtual const char* what() const throw() {
      return "the heap is empty";
    }
} EHeapQEmptyExc;

class EHeapQNotFound: public EHeapQException {
  public:
    virtual const char* what() const throw() {
      return "the given item was not found in the heap";
    }
} EHeapQNotFoundExc;

class EHeapQAlreadyPresent: public EHeapQException {
  public:
    virtual const char* what() const throw() {
      return "the given item is already present in the heap";
    }
} EHeapQAlreadyPresentExc;

class EHeapQNoLast: public EHeapQException {
  public:
    virtual const char* what() const throw() {
      return "no record for the last item";
    }
} EHeapQNoLastExc;

template <class T, class Compare = std::less<T>>
class EHeapQ {
  public:
    EHeapQ(size_t size = EHEAPQ_DEFAULT_SIZE);
    ~EHeapQ();

    T get_top() const { this->throw_on_empty(); return this->heap->at(0); }
    T get_last() const {
        if (this->heap->size() == 0) {
           throw EHeapQEmptyExc;
        }

        if (!this->last_item_set) {
           throw EHeapQNoLastExc;
        }

        return this->last_item;
    }
    void set_size(size_t size);
    size_t get_size() const noexcept { return this->size; }
    size_t get_length() const noexcept { return this->heap->size(); }
    const std::vector<T> * get_items() const { return this->heap; }

    T get_max(void);
    void push(T item);
    T pushpop(T);
    T pop(void);
    T replace(T item);
    void remove(T item);

  private:
    std::vector<T> * heap;

    long unsigned int size;
    Compare comp;

    T last_item;
    bool last_item_set;

    T max_item;
    bool max_item_set;

    void throw_on_empty() const {
      if (this->heap->size() == 0)
        throw EHeapQEmptyExc;
    }

    std::unordered_map<T, size_t> * index_map;

    void siftdown(size_t start_pos, size_t pos);
    void siftup(size_t pod);

    void set_last_item(T item) noexcept { this->last_item = item; this->last_item_set = true; }
    void set_max_item(T item) noexcept { this->max_item = item; this->max_item_set = true; }

    void maybe_del_last_item(T item) noexcept { if (this->last_item_set && this->last_item == item) { this->last_item_set = false; }}
    void maybe_del_max_item(T item) noexcept { if (this->max_item_set && this->max_item == item) { this->max_item_set = false; }}
    void maybe_adjust_max(T item) noexcept {
        if (! this->max_item_set)
            return;

        if (this->comp(this->max_item, item))
            this->max_item = item;
    }
};

template <class T, class Compare>
EHeapQ<T, Compare>::EHeapQ(size_t size) {
    this->size = size;

    this->index_map = new std::unordered_map<T, size_t>;
    this->heap = new std::vector<T>;

    this->last_item_set = false;
    this->max_item_set = false;

    this->comp = Compare();
}

template <class T, class Compare>
EHeapQ<T, Compare>::~EHeapQ() {
    delete this->index_map;
    delete this->heap;
}

template <class T, class Compare>
T EHeapQ<T, Compare>::get_max(void) {
  this->throw_on_empty();

  if (this->max_item_set)
    return this->max_item;

  T result = this->heap->data()[this->heap->size() / 2];
  for (auto i = (this->heap->size() / 2) + 1; i < this->heap->size(); i++) {
    if (this->comp(result, this->heap->data()[i]))
      result = this->heap->data()[i];
  }

  this->max_item = result;
  return result;
}

template <class T, class Compare>
void EHeapQ<T, Compare>::siftdown(size_t startpos, size_t pos) {
  T newitem, parent, *arr;
  size_t parentpos;

  auto size = this->heap->size();
  if (size == 0)
    return;   // nothing to do..

  // Follow the path to the root, moving parents down until finding a place
  // newitem fits.
  arr = this->heap->data();
  newitem = arr[pos];
  while (pos > startpos) {
    parentpos = (pos - 1) >> 1;
    parent = arr[parentpos];

    if (! this->comp(newitem, parent))
      break;

    arr = this->heap->data();
    parent = arr[parentpos];
    newitem = arr[pos];
    arr[parentpos] = newitem;
    arr[pos] = parent;
    this->index_map->at(newitem) = parentpos;
    this->index_map->at(parent) = pos;
    pos = parentpos;
  }
}

template <class T, class Compare>
void EHeapQ<T, Compare>::siftup(size_t pos) {
  size_t startpos, endpos, childpos, limit;
  T tmp1;
  T tmp2;
  T * arr;
  int cmp;

  endpos = this->heap->size();
  startpos = pos;

  /* Bubble up the smaller child until hitting a leaf. */
  arr = this->heap->data();
  limit = endpos >> 1; /* smallest pos that has no child */
  while (pos < limit) {
    /* Set childpos to index of smaller child.   */
    childpos = (pos << 1) + 1; /* leftmost child position  */
    if (childpos + 1 < endpos) {
      cmp = int(this->comp(arr[childpos], arr[childpos + 1]));
      childpos += ((unsigned)cmp ^ 1); /* increment when cmp==0 */
      arr = this->heap->data();         /* arr may have changed */
    }
    /* Move the smaller child up. */
    tmp1 = arr[childpos];
    tmp2 = arr[pos];
    arr[childpos] = tmp2;
    arr[pos] = tmp1;
    this->index_map->at(tmp2) = childpos;
    this->index_map->at(tmp1) = pos;
    pos = childpos;
  }

  /* Bubble it up to its final resting place (by sifting its parents down). */
  this->siftdown(startpos, pos);
}

template <class T, class Compare>
T EHeapQ<T, Compare>::pushpop(T item) {
    if (this->index_map->find(item) != this->index_map->end())
      throw EHeapQAlreadyPresentExc;

    T to_return = item;
    if (this->heap->size() > 0 && this->comp(this->heap->at(0), item)) {
        T to_return = this->heap->data()[0];
        this->heap->data()[0] = item;
        this->index_map->insert({item, 0});
        this->index_map->erase(to_return);
        this->siftup(0);

        this->set_last_item(item);
        this->maybe_del_max_item(item);
        this->maybe_adjust_max(item);
        return to_return;
    }

    return to_return;
}

template <class T, class Compare>
void EHeapQ<T, Compare>::push(T item) {
  if (this->index_map->find(item) != this->index_map->end())
    throw EHeapQAlreadyPresentExc;

  if (this->heap->size() == this->size) {
    this->pushpop(item);
    return;
  }

  this->index_map->insert({item, this->heap->size()});
  this->heap->push_back(item);

  try {
      siftdown(0, this->heap->size() - 1);
  } catch (...) {
    this->index_map->erase(item);
    this->heap->pop_back();
    throw;
  }

  this->set_last_item(item);

  if (this->heap->size() == 1)
      this->set_max_item(item);
  else
      maybe_adjust_max(item);
}

template <class T, class Compare>
T EHeapQ<T, Compare>::pop(void) {
  this->throw_on_empty();

  T result = this->heap->data()[0];

  if (this->heap->size() > 1) {
    this->heap->data()[0] = this->heap->back();
    this->index_map->at(this->heap->data()[0]) = 0;
  }

  this->heap->pop_back();
  this->index_map->erase(result);

  siftup(0);

  this->maybe_del_last_item(result);
  this->maybe_del_max_item(result);

  return result;
}

template <class T, class Compare>
void EHeapQ<T, Compare>::set_size(size_t size) {
  this->size = size;

  while (this->heap->size() > this->size)
     this->pop();
}

template <class T, class Compare>
T EHeapQ<T, Compare>::replace(T item) {
  this->throw_on_empty();

  if (this->index_map->find(item) != this->index_map->end())
    throw EHeapQAlreadyPresentExc;

  T result = this->heap->data()[0];

  this->index_map->erase(result);
  this->heap->data()[0] = item;
  this->index_map->insert({item, 0});

  siftup(0);

  this->set_last_item(result);
  this->maybe_del_max_item(result);
  this->maybe_adjust_max(result);

  return result;
}

template <class T, class Compare>
void EHeapQ<T, Compare>::remove(T item) {
  auto size = this->heap->size();
  auto arr = this->heap->data();
  unsigned long idx;

  auto idx_value = this->index_map->find(item);
  if (idx_value == this->index_map->end())
    throw EHeapQNotFoundExc;

  if (size > 0 && item == arr[size - 1]) {
    this->heap->pop_back();
    this->index_map->erase(item);
    goto end;
  }

  idx = idx_value->second;
  this->heap->data()[idx] = this->heap->data()[this->heap->size() - 1];
  this->heap->pop_back();
  this->index_map->erase(item);

  if (idx < this->heap->size()) {
    siftup(idx);
    siftdown(0, idx);
  }

end:
  this->maybe_del_max_item(item);
  this->maybe_del_last_item(item);
}
