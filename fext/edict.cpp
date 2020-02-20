/*
 * edict - An extended implementation of Python's dict.
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
/*
 * This module is based on the heapq implementation as present in the
 * Python standard library - git version used as a base for this
 * implementation: 1b55b65638254aa78b005fbf0b71fb02499f1852.
 *
 * This module adds an optimization for random item removal. Instead of
 * O(N) + O(log(N)) (item lookup and heap adjustment), the removal is
 * performed in O(log(N)). This speedup is significant for large Ns.
 */

#define PY_SSIZE_T_CLEAN

extern "C" {
#include <Python.h>
#include "structmember.h"
}

#include <iostream>
#include <limits>
#include <queue>
#include <unordered_map>
#include <queue>
#include <utility>
#include <vector>

const long unsigned int _DEFAULT_SIZE = std::numeric_limits<long unsigned int>::max();
const bool _DEFAULT_WEAKREF = false;

typedef std::pair<PyObject *, PyObject *> HeapQItem;
struct HeapCompare
{
  bool operator()(HeapQItem & left, HeapQItem & right) {
    Py_INCREF(left.first);
    Py_INCREF(right.first);
    int cmp = PyObject_RichCompareBool(left.first, right.first, Py_LT);
    Py_DECREF(left.first);
    Py_DECREF(right.first);

    // TODO: throw exception
    return cmp == 1;
  }
};

typedef std::priority_queue<HeapQItem, std::vector<HeapQItem>, HeapCompare> HeapQ;
typedef struct {
  PyObject_HEAD
  std::unordered_map<PyObject *, PyObject *> * value_map;
  HeapQ * heap;
  long unsigned int size;
  bool weakref;
} ExtDict;

static inline void clear_heap(ExtDict * self) {
  while (! self->heap->empty())
      self->heap->pop();
}

static int ExtDict_traverse(ExtDict *self, visitproc visit, void *arg) {
  if (self->weakref)
    return 0;

  for (auto it = self->value_map->begin(); it != self->value_map->end(); it++) {
    Py_VISIT(it->second);
  }

  return 0;
}

static int ExtDict_clear(ExtDict *self) {
  if (self->weakref)
    return 0;

  for (auto it = self->value_map->begin(); it != self->value_map->end(); it++) {
    Py_CLEAR(it->second);
  }

  return 0;
}

static void ExtDict_dealloc(ExtDict *self) {
  PyObject_GC_UnTrack(self);
  ExtDict_clear(self);

  self->value_map->clear();
  delete self->value_map;
  self->value_map = NULL;

  clear_heap(self);
  delete self->heap;
  self->heap = NULL;

  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *ExtDict_new(PyTypeObject *type, PyObject *args,
                             PyObject *kwds) {
  ExtDict *self;
  self = (ExtDict *)type->tp_alloc(type, 0);
  self->value_map = new std::unordered_map<PyObject *, PyObject *>;
  self->heap = new HeapQ;
  return (PyObject *)self;
}

static int ExtDict_init(ExtDict *self, PyObject *args, PyObject *kwds) {
  static char *kwlist[] = {"weakref", "size", NULL};

  self->size = _DEFAULT_SIZE;
  self->weakref = _DEFAULT_WEAKREF;

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|pk", kwlist, &self->weakref,
                                   &self->size))
    return -1;

  return 0;
}

int ExtDict_setitem(ExtDict *self, PyObject *key, PyObject *value) {
  if (!self->weakref)
    Py_INCREF(value);

  if (value == Py_None) {
    PyErr_SetString(PyExc_NotImplementedError, "deletion based value not supported");
    return -1;
  }

  if (self->heap->size() >= self->size) {
    auto to_remove = self->heap->top();

    Py_INCREF(value);
    Py_INCREF(to_remove.first);
    int cmp = PyObject_RichCompareBool(to_remove.first, value, Py_LT);
    Py_DECREF(value);
    Py_DECREF(to_remove.first);

    if (cmp == -1)
      return -1;

    if (cmp == 0) {
      return 0;
    }

    self->heap->pop();
    self->value_map->erase(to_remove.second);
  }

  Py_INCREF(key);
  Py_INCREF(value);

  if (self->value_map->find(key)) {
    PyErr_SetString(PyExc_KeyError, "the given key is already present in the ");
    return -1;
  }

  HeapQItem item = {key, value};
  self->value_map->insert(item);
  self->heap->push(item);
  return 0;
}

PyObject *ExtDict_getitem(ExtDict *self, PyObject *key) {
  auto value = self->value_map->find(key);

  if (value == self->value_map->end()) {
    PyErr_SetString(PyExc_KeyError, "the key is not present");
    return NULL;
  }

  Py_INCREF(value->second);
  return value->second;
}

PyObject *ExtDict_dict_clear(ExtDict *self) {
    if (! self->weakref) {
        for (auto it = self->value_map->begin(); it != self->value_map->end(); it++) {
            Py_DECREF(it->second);
        }
    }
    self->value_map->clear();
    clear_heap(self);
    Py_RETURN_NONE;
}

PyObject *ExtDict_get(ExtDict *self, PyObject * key) {
    auto value = self->value_map->find(key);

    if (value == self->value_map->end())
        Py_RETURN_NONE;

    Py_INCREF(value->second);
    return value->second;
}

PyObject *ExtDict_items(ExtDict *self) {
    // TODO: implement
    Py_RETURN_NONE;
}

PyObject *ExtDict_keys(ExtDict *self) {
    // TODO: implement
    Py_RETURN_NONE;
}

PyObject *ExtDict_setdefault(ExtDict *self) {
    // TODO: implement
    Py_RETURN_NONE;
}

PyObject *ExtDict_values(ExtDict *self) {
    // TODO: implement
    Py_RETURN_NONE;
}

static PyObject *ExtDict_getweakref(ExtDict *self) {
  return PyBool_FromLong(long(self->weakref));
}

static PyObject *ExtDict_getsize(ExtDict *self) {
  return PyLong_FromUnsignedLong(self->size);
}

static long int ExtDict_len(PyObject *self) {
  return ((ExtDict *)self)->value_map->size();
}

static PyMethodDef ExtDict_methods[] = {
    {"clear", (PyCFunction)ExtDict_dict_clear, METH_VARARGS, "TODO."},
    {"get", (PyCFunction)ExtDict_get, METH_VARARGS, "TODO."},
    {"items", (PyCFunction)ExtDict_items, METH_VARARGS, "TODO."},
    {"keys", (PyCFunction)ExtDict_keys, METH_VARARGS, "TODO."},
    {"setdefault", (PyCFunction)ExtDict_setdefault, METH_VARARGS, "TODO."},
    {"values", (PyCFunction)ExtDict_values, METH_VARARGS, "TODO."},
    {NULL}};

static PyMappingMethods ExtDict_mapping_methods[] = {
    ExtDict_len,                    // mp_length
    (binaryfunc)ExtDict_getitem,    // mp_subscript
    (objobjargproc)ExtDict_setitem, // mp_ass_subscript
    {NULL}};

static PyGetSetDef ExtDict_getsetters[] = {
    {"weakref", (getter)ExtDict_getweakref, NULL,
     "Flag for weak referencing objects.", NULL},
    {"size", (getter)ExtDict_getsize, NULL, "Max size of the dictionary.",
     NULL},
    {NULL} /* Sentinel */
};

PyMODINIT_FUNC PyInit_edict(void) {
  static PyTypeObject ExtDict = {PyVarObject_HEAD_INIT(NULL, 0)};
  ExtDict.tp_name = "eheapq.ExtDict";
  ExtDict.tp_doc = "Extended heap queue algorithm.";
  ExtDict.tp_basicsize = sizeof(ExtDict);
  ExtDict.tp_itemsize = 0;
  ExtDict.tp_flags =
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC;
  ExtDict.tp_new = ExtDict_new;
  ExtDict.tp_init = (initproc)ExtDict_init;
  ExtDict.tp_dealloc = (destructor)ExtDict_dealloc;
  ExtDict.tp_traverse = (traverseproc)ExtDict_traverse;
  ExtDict.tp_clear = (inquiry)ExtDict_clear;
  ExtDict.tp_methods = ExtDict_methods;
  ExtDict.tp_getset = ExtDict_getsetters;
  ExtDict.tp_as_mapping = ExtDict_mapping_methods;

  static PyModuleDef eheapq = {PyModuleDef_HEAD_INIT};
  eheapq.m_name = "edict";
  eheapq.m_doc = "Implementation of extended dictionary.";
  eheapq.m_size = -1;

  PyObject *m;
  if (PyType_Ready(&ExtDict) < 0)
    return NULL;

  m = PyModule_Create(&eheapq);
  if (!m)
    return NULL;

  Py_INCREF(&ExtDict);
  if (PyModule_AddObject(m, "ExtDict", (PyObject *)&ExtDict) < 0) {
    Py_DECREF(&ExtDict);
    Py_DECREF(m);
    return NULL;
  }

  return m;
}

/*
 * TODO: respect size
 * TODO: testsuite
 * TODO: implement methods
 */
