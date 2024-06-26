#ifndef MEMHIVE_MAP_H
#define MEMHIVE_MAP_H

#include <stdint.h>
#include <stdarg.h>
#include "Python.h"
#include "module.h"
#include "proxy.h"

/*
HAMT tree is shaped by hashes of keys. Every group of 5 bits of a hash denotes
the exact position of the key in one level of the tree. Since we're using
32 bit hashes, we can have at most 7 such levels. Although if there are
two distinct keys with equal hashes, they will have to occupy the same
cell in the 7th level of the tree -- so we'd put them in a "collision" node.
Which brings the total possible tree depth to 8. Read more about the actual
layout of the HAMT tree in `_map.c`.

This constant is used to define a datastucture for storing iteration state.
*/
#define _Py_HAMT_MAX_TREE_DEPTH 8


#define Map_Check(state, o) (Py_IS_TYPE(o, state->MapType))
#define MapMutation_Check(state, o) (Py_IS_TYPE(o, state->MapMutationType))


#define _InterpreterFields      \
    int64_t interpreter_id;


/* Abstract tree node. */
struct MapNode;


#ifdef Py_TPFLAGS_MANAGED_WEAKREF
#define _MapCommonFields(pref)          \
    ProxyableObject __proxyable;        \
    _InterpreterFields                  \
    struct MapNode *pref##_root;               \
    Py_ssize_t pref##_count;
#else
#define _MapCommonFields(pref)          \
    ProxyableObject __proxyable;        \
    _InterpreterFields                  \
    struct MapNode *pref##_root;               \
    PyObject *pref##_weakreflist;       \
    Py_ssize_t pref##_count;
#endif

/* Base mapping struct; used in methods shared between
   MapObject and MapMutationObject types. */
typedef struct {
    _MapCommonFields(b)
} BaseMapObject;


/* An HAMT immutable mapping collection. */
typedef struct {
    _MapCommonFields(h)
    Py_hash_t h_hash;
} MapObject;


/* MapMutation object (returned from `map.mutate()`.) */
typedef struct {
    _MapCommonFields(m)
    uint64_t m_mutid;
} MapMutationObject;


typedef PyObject * (*iteryield)(int, module_state *, PyObject *, PyObject *);


/* A struct to hold the state of depth-first traverse of the tree.

   HAMT is an immutable collection.  Iterators will hold a strong reference
   to it, and every node in the HAMT has strong references to its children.

   So for iterators, we can implement zero allocations and zero reference
   inc/dec depth-first iteration.

   - i_nodes: an array of seven pointers to tree nodes
   - i_level: the current node in i_nodes
   - i_pos: an array of positions within nodes in i_nodes.
*/
typedef struct {
    struct MapNode *i_nodes[_Py_HAMT_MAX_TREE_DEPTH];
    Py_ssize_t i_pos[_Py_HAMT_MAX_TREE_DEPTH];
    int8_t i_level;
} MapIteratorState;


/* Base iterator object.

   Contains the iteration state, a pointer to the HAMT tree,
   and a pointer to the 'yield function'.  The latter is a simple
   function that returns a key/value tuple for the 'Items' iterator,
   just a key for the 'Keys' iterator, and a value for the 'Values'
   iterator.
*/

typedef struct {
    PyObject_HEAD
    _InterpreterFields
    MapObject *mv_obj;
    iteryield mv_yield;
    PyTypeObject *mv_itertype;
} MapView;

typedef struct {
    PyObject_HEAD
    _InterpreterFields
    MapObject *mi_obj;
    iteryield mi_yield;
    MapIteratorState mi_iter;
} MapIterator;


extern PyType_Spec MapItems_TypeSpec;
extern PyType_Spec MapItemsIter_TypeSpec;
extern PyType_Spec MapKeys_TypeSpec;
extern PyType_Spec MapKeysIter_TypeSpec;
extern PyType_Spec MapValues_TypeSpec;
extern PyType_Spec MapValuesIter_TypeSpec;
extern PyType_Spec Map_TypeSpec;
extern PyType_Spec MapMutation_TypeSpec;
extern PyType_Spec ArrayNode_TypeSpec;
extern PyType_Spec BitmapNode_TypeSpec;
extern PyType_Spec CollisionNode_TypeSpec;

PyObject * MemHive_NewMap(module_state *);
PyObject * MemHive_MapSetItem(module_state *state,
                              PyObject *self, PyObject *key, PyObject *val);
PyObject * MemHive_MapGetItem(module_state *state, PyObject *self,
                              PyObject *key, PyObject *def);
int MemHive_MapContains(module_state *state, PyObject *self, PyObject *key);
PyObject * MemHive_NewMapProxy(module_state *, PyObject *);
PyObject * MemHive_CopyMapProxy(module_state *, PyObject *);

#endif
