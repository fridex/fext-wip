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
#include <unordered_map>
#include <vector>

#include "eheapq.hpp"

const bool _DEFAULT_WEAKREF = false;

class ObjCmpErr: public std::exception {
  public:
    virtual const char* what() const throw() {
      return "failed to compare Python objects";
    }
} ObjCmpErrExc;

struct PyObjectRichCmp {
    bool operator()(PyObject * a, PyObject * b) {
        Py_INCREF(a);
        Py_INCREF(b);
        auto cmp = PyObject_RichCompareBool(a, b, Py_LT);
        Py_DECREF(a);
        Py_DECREF(b);

        if (cmp < 0)
          throw ObjCmpErrExc;

        return bool(cmp);
    }
};

typedef struct {
  PyObject_HEAD
  EHeapQ<PyObject *, PyObjectRichCmp> * heap;
} ExtHeapQueue;

static int ExtHeapQueue_traverse(ExtHeapQueue *self, visitproc visit, void *arg) {
  for (auto i : *(self->heap->get_items()))
    Py_VISIT(i);

  return 0;
}

static int ExtHeapQueue_clear(ExtHeapQueue *self) {
  for (auto i : *(self->heap->get_items()))
    Py_CLEAR(i);

  return 0;
}

static void ExtHeapQueue_dealloc(ExtHeapQueue *self) {
  PyObject_GC_UnTrack(self);
  ExtHeapQueue_clear(self);
  delete self->heap;
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject * ExtHeapQueue_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  ExtHeapQueue *self;
  self = (ExtHeapQueue *)type->tp_alloc(type, 0);
  self->heap = new EHeapQ<PyObject *, PyObjectRichCmp>;
  return (PyObject *)self;
}

static int ExtHeapQueue_init(ExtHeapQueue *self, PyObject *args, PyObject *kwds) {
  static char *kwlist[] = {"size", NULL};

  size_t size = self->heap->get_size();

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|k", kwlist, &size))
    return -1;

  self->heap->set_size(size);
  return 0;
}

static PyObject * ExtHeapQueue_top(ExtHeapQueue *self) {
    PyObject * item;

    try {
      item = self->heap->get_top();
    } catch (EHeapQEmpty & exc) {
        PyErr_SetString(PyExc_KeyError, exc.what());
        return NULL;
    }

    Py_INCREF(item);
    return item;
}

static PyObject * ExtHeapQueue_last(ExtHeapQueue *self) {
  PyObject * item;

  try {
      item = self->heap->get_last();
  } catch (EHeapQNoLast & exc) {
      Py_RETURN_NONE;
  } catch (EHeapQEmpty & exc) {
      PyErr_SetString(PyExc_KeyError, exc.what());
      return NULL;
  }

  Py_INCREF(item);
  return item;
}

static PyObject * ExtHeapQueue_pushpop(ExtHeapQueue *self, PyObject *args) {
  PyObject * item, * to_return;

  if (!PyArg_ParseTuple(args, "O", &item))
    return NULL;

  try {
     to_return = self->heap->pushpop(item);
  } catch (ObjCmpErr & exc) {
      PyErr_SetString(PyExc_ValueError, exc.what());
      return NULL;
  } catch (EHeapQAlreadyPresent & exc) {
      PyErr_SetString(PyExc_ValueError, exc.what());
      return NULL;
  }

  Py_INCREF(item);
  Py_INCREF(to_return);
  return to_return;
}

static PyObject * ExtHeapQueue_push(ExtHeapQueue *self, PyObject *args) {
  PyObject *item;

  if (!PyArg_ParseTuple(args, "O", &item))
    return NULL;

  try {
      self->heap->push(item);
  } catch (ObjCmpErr & exc) {
      PyErr_SetString(PyExc_ValueError, exc.what());
      return NULL;
  } catch (EHeapQAlreadyPresent & exc) {
      PyErr_SetString(PyExc_ValueError, exc.what());
      return NULL;
  }

  Py_INCREF(item);
  Py_RETURN_NONE;
}

static PyObject * ExtHeapQueue_pop(ExtHeapQueue *self) {
  PyObject * item;

  try {
      item = self->heap->pop();
  } catch (EHeapQEmpty & exc) {
      PyErr_SetString(PyExc_KeyError, exc.what());
      return NULL;
  }

  return item;
}

static PyObject *ExtHeapQueue_remove(ExtHeapQueue *self, PyObject *args) {
  PyObject *item;
  if (!PyArg_ParseTuple(args, "O", &item))
    return NULL;

  try {
      self->heap->remove(item);
  } catch (EHeapQEmpty & exc) {
      PyErr_SetString(PyExc_KeyError, exc.what());
      return NULL;
  } catch (EHeapQNotFound & exc) {
      PyErr_SetString(PyExc_ValueError, exc.what());
      return NULL;
  }

  Py_DECREF(item);
  Py_RETURN_NONE;
}

static PyObject *ExtHeapQueue_replace(ExtHeapQueue *self, PyObject *args) {
  PyObject *item;
  PyObject * result;

  if (!PyArg_ParseTuple(args, "O", &item))
    return NULL;

  Py_INCREF(item);

  try {
      result = self->heap->replace(item);
  } catch (ObjCmpErr & exc) {
      PyErr_SetString(PyExc_ValueError, exc.what());
      return NULL;
  } catch (EHeapQEmpty & exc) {
      PyErr_SetString(PyExc_KeyError, exc.what());
      return NULL;
  } catch (EHeapQNotFound & exc) {
      PyErr_SetString(PyExc_ValueError, exc.what());
      return NULL;
  } catch (EHeapQAlreadyPresent & exc) {
      PyErr_SetString(PyExc_ValueError, exc.what());
      return NULL;
  }

  return result;
}

static PyObject *ExtHeapQueue_max(ExtHeapQueue *self) {
    PyObject * item;

    try {
        item = self->heap->get_max();
    } catch (EHeapQEmpty & exc) {
        PyErr_SetString(PyExc_KeyError, exc.what());
        return NULL;
    }

    Py_INCREF(item);
    return item;
}

static PyObject *ExtHeapQueue_getsize(ExtHeapQueue *self) {
  return PyLong_FromUnsignedLong(self->heap->get_size());
}

static long int ExtHeapQueue_len(PyObject *self) {
  return ((ExtHeapQueue *)self)->heap->get_length();
}

static PySequenceMethods ExtHeapQueue_sequence_methods[] = {
    ExtHeapQueue_len, // sq_length
    {NULL}
};

static PyMethodDef ExtHeapQueue_methods[] = {
    {"push", (PyCFunction)ExtHeapQueue_push, METH_VARARGS, "Push item onto heap, maintaining the heap invariant."},
    {"pushpop", (PyCFunction)ExtHeapQueue_pushpop, METH_VARARGS,
     "Push item on the heap, then pop and return the smallest item from the "
     "heap. The combined action runs more efficiently than heappush() followed "
     "by a separate call tprint(a.get_size())o heappop()."},
    {"pop", (PyCFunction)ExtHeapQueue_pop, METH_NOARGS, "Pops top item from the heap."},
    {"replace", (PyCFunction)ExtHeapQueue_replace, METH_VARARGS, "Pops top item, and adds new item; the heap size is unchanged."},
    {"get_top", (PyCFunction)ExtHeapQueue_top, METH_NOARGS, "Gets top item from the heap, the heap is untouched."},
    {"get_last", (PyCFunction)ExtHeapQueue_last, METH_NOARGS, "Get last item added, if the item is still present in the heap."},
    {"get_max", (PyCFunction)ExtHeapQueue_max, METH_NOARGS, "Retrieve maximum stored in the min-heapq, in O(N/2)."},
    {"remove", (PyCFunction)ExtHeapQueue_remove, METH_VARARGS, "Remove the given item, in O(log(N))."},
    {NULL}
};

static PyGetSetDef ExtHeapQueue_getsetters[] = {
    {"size", (getter)ExtHeapQueue_getsize, NULL, "Max size of the heap.", NULL},
    {NULL} /* Sentinel */
};

PyMODINIT_FUNC PyInit_eheapq(void) {
  static PyTypeObject ExtMinHeapQueueType = {PyVarObject_HEAD_INIT(NULL, 0)};
  ExtMinHeapQueueType.tp_name = "eheapq.ExtHeapQueue";
  ExtMinHeapQueueType.tp_doc = "Extended heap queue algorithm.";
  ExtMinHeapQueueType.tp_basicsize = sizeof(ExtHeapQueue);
  ExtMinHeapQueueType.tp_itemsize = 0;
  ExtMinHeapQueueType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC;
  ExtMinHeapQueueType.tp_new = ExtHeapQueue_new;
  ExtMinHeapQueueType.tp_as_sequence = ExtHeapQueue_sequence_methods;
  ExtMinHeapQueueType.tp_init = (initproc)ExtHeapQueue_init;
  ExtMinHeapQueueType.tp_dealloc = (destructor)ExtHeapQueue_dealloc;
  ExtMinHeapQueueType.tp_traverse = (traverseproc)ExtHeapQueue_traverse;
  ExtMinHeapQueueType.tp_clear = (inquiry)ExtHeapQueue_clear;
  ExtMinHeapQueueType.tp_methods = ExtHeapQueue_methods;
  ExtMinHeapQueueType.tp_getset = ExtHeapQueue_getsetters;

  static PyModuleDef eheapq = {PyModuleDef_HEAD_INIT};
  eheapq.m_name = "eheapq";
  eheapq.m_doc = "Implementation of extended heap queues.";
  eheapq.m_size = -1;

  PyObject *m;
  if (PyType_Ready(&ExtMinHeapQueueType) < 0)
    return NULL;

  m = PyModule_Create(&eheapq);
  if (!m)
    return NULL;

  Py_INCREF(&ExtMinHeapQueueType);
  if (PyModule_AddObject(m, "ExtHeapQueue", (PyObject *)&ExtMinHeapQueueType) <
      0) {
    Py_DECREF(&ExtMinHeapQueueType);
    Py_DECREF(m);
    return NULL;
  }

  return m;
}
