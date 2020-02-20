fext
----

Fast CPython extensions to Python standard library with focus on performance.

This library provides CPython native extensions to mimic some of the well known
built-in types. The implementation relies on enforced protocol - all the
objects and abstract data types are implemented in C/C++ to provide highly
effective manipulation. This adds additional restrictions on the protocol
provided by these optimized data types, so use them on your own risk. Otherwise,
you can easily shoot yourself into your leg (multiple times).

Extended dict - fext.ExtDict
=============================

# TODO

Extended heapq - fext.ExtHeapQueue
==================================

# TODO

Using fext in a C++ project
===========================

The design of this library allows you to use sources in your C++ project as
well. The `eheapq.hpp` file defines the extended heap queue and `edict.hpp` the
extended dictionary. Python files then act as a bindings to their respective
Python interfaces. Mind the templating style used - use pointers as types to
avoid unnecessary/unwanted copy constructor calls in objects stored.

Original design
===============

This library was created for `project Thoth
<https://github.com/thoth-station>`_. The data structures present in this
library were designed for `Thoth's adviser
<https://github.com/thoth-station/adviser>`_. The extended heap queue is used
in the resolver to keep partially resolved states in a structure optimal for
lookup and state manipulation (last state, the highest scored state, optimal
removal of states, restrictions on number of internal states kept, ...). The
extended dictionary is used in the reinforcement learning part to limit memory
consumption and fast lookup for the policy learned during adviser's runs.
