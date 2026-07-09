# Zenoh-Pico Generic Collections

This directory contains a set of **header-only, type-generic container templates**
written in portable C. They emulate C++-style generic containers (`std::vector`,
`std::unordered_map`, `std::deque`, `std::priority_queue`, `std::variant`) using the
C preprocessor.

The templates documented here are:

| Header                              | Container               | Storage            |
| ----------------------------------- | ----------------------- | ------------------ |
| `vector_template.h`                 | Dynamic array (vector)  | Heap (growable)    |
| `static_vector_template.h`          | Dynamic array (vector)  | Inline, fixed cap. |
| `static_bit_vector_template.h`      | Bit vector (0/1 bits)   | Inline, fixed cap. |
| `hashmap_template.h`                | Hash map                | Heap (growable)    |
| `hashset_template.h`                | Hash set                | Heap (growable)    |
| `static_hashmap_template.h`         | Hash map                | Inline, fixed cap. |
| `static_hashset_template.h`         | Hash set                | Inline, fixed cap. |
| `static_deque_template.h`           | Double-ended queue      | Inline, fixed cap. |
| `static_pqueue_template.h`          | Binary-heap priority q. | Inline, fixed cap. |
| `variant_template.h`                | Tagged union (variant)  | Inline             |

In addition, `algorithms_template.h` provides generic iteration / search / removal
helper macros that work on any of the above containers exposing an iterator interface
(vectors and hash maps).

---

## How the templates work

These headers are **not** regular headers with include guards. Instead, each one is
a *code generator*: you define a handful of configuration macros, then `#include` the
header. The preprocessor expands the template into a concrete set of `struct` and
`static inline` functions specialised for your element type. The header `#undef`s all
of its configuration macros at the end, so it can be included again with a different
configuration to generate another specialisation.

```c
// 1. Configure the template
#define _ZP_VECTOR_TEMPLATE_ELEM_TYPE int
#define _ZP_VECTOR_TEMPLATE_NAME      intvec
// 2. Instantiate it
#include "zenoh-pico/collections/vector_template.h"

// 3. Use the generated type `intvec_t` and functions `intvec_*`
intvec_t v = intvec_new();
int x = 42;
intvec_push_back(&v, &x);
intvec_destroy(&v);
```

### Naming

Every generated symbol is prefixed with the configured `..._NAME`. For a vector named
`intvec`, you get the type `intvec_t` and functions such as `intvec_new`,
`intvec_push_back`, `intvec_destroy`, etc. This prefixing lets you instantiate many
specialisations in the same translation unit without name collisions.

### Move and destroy semantics

All containers operate with **move semantics** to support element types that own
resources (heap buffers, reference counts, etc.):

* **DESTROY_FN** — called to release an element. Default: no-op (trivial type).
* **MOVE_FN** — called to relocate an element from `src` to `dst`. The default copies
  the bytes (`*dst = *src`) **without** destroying the source; a custom move should
  transfer ownership and leave the source in a safe (destroyable) state.

Functions that *insert* an element (e.g. `push_back`, `insert`, `push`) take a
**pointer** to the source element and *move it in* — ownership is transferred to the
container. Functions that *remove* an element (e.g. `pop_back`, `remove`, `pop`)
typically take an `out` pointer:

* if `out != NULL`, the element is **moved out** into `*out` (caller now owns it);
* if `out == NULL`, the element is **destroyed** in place.

When a container exposes `destroy`, it destroys every contained element and releases
any owned storage. It does **not** free the container struct itself (which usually
lives on the stack or is embedded in another struct).

### Custom allocators (heap containers only)

The heap-backed containers (`vector_template.h`, `hashmap_template.h`, `hashset_template.h`) accept optional
`ALLOC_FN` / `FREE_FN` (and an optional `REALLOC_FN`) so they can be wired to a custom
allocator. They default to `malloc` / `free`.

---

## `vector_template.h` — heap-allocated vector

A growable contiguous array (like `std::vector`). The buffer is heap-allocated and
grows automatically (doubling) as elements are added.

### Configuration macros

| Macro                                    | Required | Default                   | Purpose                                                                                      |
| ---------------------------------------- | :------: | ------------------------- | -------------------------------------------------------------------------------------------- |
| `_ZP_VECTOR_TEMPLATE_ELEM_TYPE`          |    ✅    | —                         | Element type.                                                                                |
| `_ZP_VECTOR_TEMPLATE_NAME`               |    ❌    | derived from element type | Base name (without `_t`) for generated symbols.                                              |
| `_ZP_VECTOR_TEMPLATE_ELEM_DESTROY_FN(x)` |    ❌    | no-op                     | Destroy one element.                                                                         |
| `_ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN(d,s)`  |    ❌    | `*d = *s`                 | Move one element.                                                                            |
| `_ZP_VECTOR_TEMPLATE_ALLOC_FN(bytes)`    |    ❌    | `malloc`                  | Allocate memory.                                                                             |
| `_ZP_VECTOR_TEMPLATE_FREE_FN(ptr)`       |    ❌    | `free`                    | Free memory.                                                                                 |
| `_ZP_VECTOR_TEMPLATE_REALLOC_FN(p,size)` |    ❌    | (unused)                  | Optional in-place realloc; used only for trivially movable elements as a faster growth path. |

### Generated type

```c
typedef struct NAME_t {
    ELEM_TYPE *_buffer;
    size_t     _size;
    size_t     _capacity;
} NAME_t;

typedef ELEM_TYPE NAME_elem_t;  // element alias
typedef size_t    NAME_iter_t;  // iterator (a plain index)
```

### API (`NAME` = configured name)

| Function                                                                           | Description                                                                                                                                                                                                                                                                                     |
| ---------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `void NAME_init(NAME_t *v)`                                                        | Initialise an empty vector in place (no allocation).                                                                                                                                                                                                                                            |
| `NAME_t NAME_new(void)`                                                            | Return a new empty vector (no allocation).                                                                                                                                                                                                                                                      |
| `bool NAME_init_with_capacity(NAME_t *v, size_t cap)`                              | Initialise empty, pre-reserving `cap` slots.                                                                                                                                                                                                                                                    |
| `size_t NAME_size(const NAME_t *v)`                                                | Number of stored elements.                                                                                                                                                                                                                                                                      |
| `size_t NAME_capacity(const NAME_t *v)`                                            | Current allocated capacity.                                                                                                                                                                                                                                                                     |
| `bool NAME_is_empty(const NAME_t *v)`                                              | `true` if empty.                                                                                                                                                                                                                                                                                |
| `bool NAME_reserve(NAME_t *v, size_t new_cap)`                                     | Grow capacity to at least `new_cap`. `false` on allocation failure.                                                                                                                                                                                                                             |
| `bool NAME_push_back(NAME_t *v, ELEM_TYPE *e)`                                     | Move `*e` to the back, growing if needed. `false` on allocation failure.                                                                                                                                                                                                                        |
| `bool NAME_append(NAME_t *v, ELEM_TYPE *elems, size_t len)`                        | Move `len` elements to the back. Atomic: on failure nothing is moved.                                                                                                                                                                                                                           |
| `bool NAME_insert(NAME_t *v, size_t i, ELEM_TYPE *e)`                              | Insert at index `i`, shifting the tail right (growing if needed). `false` if `i > size` or a reallocation failed.                                                                                                                                                                               |
| `bool NAME_pop_back(NAME_t *v, ELEM_TYPE *out)`                                    | Remove the last element (move to `out`, or destroy if `out == NULL`). `false` if empty.                                                                                                                                                                                                         |
| `bool NAME_remove(NAME_t *v, size_t i, ELEM_TYPE *out)`                            | Remove at index `i`, shifting the tail left (move to `out`, or destroy if `NULL`). `false` if `i >= size`.                                                                                                                                                                                      |
| `void NAME_remove_at(NAME_t *v, NAME_iter_t i, ELEM_TYPE *out, NAME_iter_t *next)` | `remove`-based wrapper matching the hash-map signature so the vector works with `_ZP_REMOVE`. Removes index `i` (move to `out`, or destroy if `NULL`); if `next != NULL`, sets `*next` to the following iterator — the same index, or `end()` when `i` was the last element. UB if `i >= size`. |
| `bool NAME_swap_remove(NAME_t *v, size_t i, ELEM_TYPE *out)`                       | O(1) removal that does **not** preserve order: removes index `i` (move to `out`, or destroy if `NULL`) and moves the last element into the vacated slot (unless `i` was the last). `false` if `i >= size`.                                                                                      |
| `ELEM_TYPE *NAME_get(NAME_t *v, size_t i)`                                         | Pointer to element `i`, or `NULL` if out of bounds.                                                                                                                                                                                                                                             |
| `const NAME_elem_t *NAME_const_get(const NAME_t *v, size_t i)`                     | Const variant of `get`.                                                                                                                                                                                                                                                                         |
| `ELEM_TYPE *NAME_at(NAME_t *v, size_t i)`                                          | Pointer to element `i`, **no bounds check**.                                                                                                                                                                                                                                                    |
| `const NAME_elem_t *NAME_const_at(const NAME_t *v, size_t i)`                      | Const variant of `at`.                                                                                                                                                                                                                                                                          |
| `ELEM_TYPE *NAME_front(NAME_t *v)`                                                 | Pointer to first element, or `NULL`.                                                                                                                                                                                                                                                            |
| `ELEM_TYPE *NAME_back(NAME_t *v)`                                                  | Pointer to last element, or `NULL`.                                                                                                                                                                                                                                                             |
| `ELEM_TYPE *NAME_data(NAME_t *v)`                                                  | Pointer to the raw buffer.                                                                                                                                                                                                                                                                      |
| `const NAME_elem_t *NAME_const_data(const NAME_t *v)`                              | Const variant of `data`.                                                                                                                                                                                                                                                                        |
| `void NAME_destroy(NAME_t *v)`                                                     | Destroy all elements, free the buffer, reset to empty.                                                                                                                                                                                                                                          |
| `NAME_iter_t NAME_begin(const NAME_t *v)`                                          | First index (always `0`).                                                                                                                                                                                                                                                                       |
| `NAME_iter_t NAME_end(const NAME_t *v)`                                            | One-past-last index (equal to size).                                                                                                                                                                                                                                                            |
| `NAME_iter_t NAME_iter_next(const NAME_t *v, NAME_iter_t i)`                       | Advance the iterator.                                                                                                                                                                                                                                                                           |

### Example

```c
#define _ZP_VECTOR_TEMPLATE_ELEM_TYPE int
#define _ZP_VECTOR_TEMPLATE_NAME      intvec
#include "zenoh-pico/collections/vector_template.h"

intvec_t v = intvec_new();
for (int i = 0; i < 5; i++) {
    intvec_push_back(&v, &i);
}
for (intvec_iter_t i = intvec_begin(&v); i != intvec_end(&v); i = intvec_iter_next(&v, i)) {
    printf("%d\n", *intvec_at(&v, i));
}
intvec_destroy(&v);
```

For an element type that owns memory, supply destroy/move callbacks:

```c
typedef struct { int *ptr; } box_t;
static void box_destroy(box_t *b) { free(b->ptr); b->ptr = NULL; }
static void box_move(box_t *d, box_t *s) { *d = *s; s->ptr = NULL; }

#define _ZP_VECTOR_TEMPLATE_ELEM_TYPE        box_t
#define _ZP_VECTOR_TEMPLATE_NAME             boxvec
#define _ZP_VECTOR_TEMPLATE_ELEM_DESTROY_FN(x)   box_destroy(x)
#define _ZP_VECTOR_TEMPLATE_ELEM_MOVE_FN(d, s)   box_move(d, s)
#include "zenoh-pico/collections/vector_template.h"
```

> **Note on iterators:** indices are stable across growth, so an index obtained
> before a `push_back` remains valid afterwards.

---

## `static_vector_template.h` — fixed-capacity vector

Same interface idea as the heap vector, but the buffer is embedded **inline** in the
struct with a compile-time maximum capacity — **no heap allocation**. Ideal for
constrained / no-malloc environments.

### Configuration macros

| Macro                                           | Required | Default                    | Purpose                               |
| ----------------------------------------------- | :------: | -------------------------- | ------------------------------------- |
| `_ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE`          |    ✅    | —                          | Element type.                         |
| `_ZP_STATIC_VECTOR_TEMPLATE_SIZE`               |    ❌    | `16`                       | Maximum capacity (inline array size). |
| `_ZP_STATIC_VECTOR_TEMPLATE_NAME`               |    ❌    | derived from type and size | Base name for generated symbols.      |
| `_ZP_STATIC_VECTOR_TEMPLATE_ELEM_DESTROY_FN(x)` |    ❌    | no-op                      | Destroy one element.                  |
| `_ZP_STATIC_VECTOR_TEMPLATE_ELEM_MOVE_FN(d,s)`  |    ❌    | `*d = *s`                  | Move one element.                     |

### Generated type

```c
typedef struct NAME_t {
    ELEM_TYPE _buffer[SIZE];
    size_t    _size;
} NAME_t;
```

### API

The static vector shares the same accessors as the heap vector
(`size`, `is_empty`, `get`/`const_get`, `at`/`const_at`, `front`, `back`, `data`/`const_data`,
`push_back`, `append`, `insert`, `pop_back`, `remove`, `remove_at`, `swap_remove`, `destroy`, and
`begin`/`end`/`iter_next`), with these differences:

| Function                                                    | Description                                                                      |
| ----------------------------------------------------------- | -------------------------------------------------------------------------------- |
| `void NAME_init(NAME_t *v)`                                 | Zero-initialise an empty vector in place.                                        |
| `NAME_t NAME_new(void)`                                     | Return a new (zero-initialised) empty vector.                                    |
| `size_t NAME_capacity(const NAME_t *v)`                     | Returns the compile-time `SIZE`.                                                 |
| `bool NAME_push_back(NAME_t *v, ELEM_TYPE *e)`              | Move `*e` to the back. `false` if **full**.                                      |
| `bool NAME_append(NAME_t *v, ELEM_TYPE *elems, size_t len)` | `false` if it does not fit.                                                      |
| `bool NAME_insert(NAME_t *v, size_t i, ELEM_TYPE *e)`       | Insert at index `i`, shifting the tail right. `false` if **full** or `i > size`. |

There is no `reserve`, `init_with_capacity`, or allocator configuration (capacity is fixed).
`NAME_remove`, `NAME_remove_at` and `NAME_swap_remove` behave exactly as in the heap vector.

### Example

```c
#define _ZP_STATIC_VECTOR_TEMPLATE_ELEM_TYPE int
#define _ZP_STATIC_VECTOR_TEMPLATE_NAME      ivec
#define _ZP_STATIC_VECTOR_TEMPLATE_SIZE      8
#include "zenoh-pico/collections/static_vector_template.h"

ivec_t v = ivec_new();
int a = 1, b = 2, c = 3;
ivec_push_back(&v, &a);
ivec_push_back(&v, &c);
ivec_insert(&v, 1, &b);     // v == [1, 2, 3]
ivec_destroy(&v);
```

---

## `static_bit_vector_template.h` — fixed-capacity bit vector

A fixed-capacity sequence of single **bits** (0/1 values) packed into an inline array
of unsigned-integer **blocks** — **no heap allocation**. It mirrors
`static_vector_template.h`, but because an individual bit is not addressable, there are no mutable accessors,
setters such as `set` and `set_at` should be used instead to set individual bits. Pointers returned by const
accessors can only be dereferenced to acquire corresponding bit values, but can not be used to get the
individual bit address in the vector.

Alternatively a bitset can be generated - an always full vector with size equal to its capacity.

### Configuration macros

| Macro                                        | Required | Default                               | Purpose                                                                |
| -------------------------------------------- | :------: | ------------------------------------- | ---------------------------------------------------------------------- |
| `_ZP_STATIC_BIT_VECTOR_TEMPLATE_IS_SET`      |    ❌    | `0`                                   | Generate bit vector if `0`, bitset (always full bit vector) otherwise. |
| `_ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE`        |    ❌    | `16`                                  | Maximum capacity in bits.                                              |
| `_ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME`        |    ❌    | derived from the size and is_set flag | Base name for generated symbols.                                       |
| `_ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE`  |    ❌    | `size_t`                              | Unsigned integer type backing the storage.                             |

There is no element type, destroy, or move configuration: the element type is always
`bool` and bits own no resources. The block type must be an **unsigned** integer type;
a smaller type (e.g. `uint8_t`) minimises the round-up overhead of the inline array,
while a wider type can reduce instruction count for bulk operations.

### Generated type

```c
// BITS = sizeof(BLOCK_TYPE) * CHAR_BIT
typedef struct NAME_t {
    BLOCK_TYPE _blocks[(SIZE + BITS - 1) / BITS];  // packed bits, LSB-first within each block
    size_t     _size; // only for bit vector case
} NAME_t;

typedef BLOCK_TYPE NAME_block_t;  // backing storage word
typedef bool       NAME_elem_t;   // element alias
typedef size_t     NAME_iter_t;   // iterator (a plain index)
```

### API (`NAME` = configured name)

| Function                                                            | Description                                                                                 |
| ------------------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| `void NAME_init(NAME_t *v)`                                         | Zero-initialise an empty bit vector in place.                                               |
| `NAME_t NAME_new(void)`                                             | Return a new (zero-initialised) empty bit vector.                                           |
| `size_t NAME_size(const NAME_t *v)`                                 | Number of bits stored.                                                                      |
| `size_t NAME_capacity(const NAME_t *v)`                             | Returns the compile-time `SIZE`.                                                            |
| `bool NAME_is_empty(const NAME_t *v)`                               | `true` if no bits are stored. Bit vector only.                                              |
| `void NAME_destroy(NAME_t *v)`                                      | Reset to empty (clears storage). Provided for parity; frees nothing. (Bit vector only.)     |
| `const bool *NAME_const_at(const NAME_t *v, size_t i)`              | Bit at `i`, **no** bounds check. UB if  `i >= size`.                                        |
| `const bool *NAME_const_get(const NAME_t *v, size_t i)`             | Bounds-checked read; return `NULL` if `i >= size`.                                          |
| `void NAME_set_at(NAME_t *v, size_t i, bool x)`                     | Set bit `i`, **no** bounds check. UB if `i >= size`.                                        |
| `bool NAME_set(NAME_t *v, size_t i, bool x)`                        | Bounds-checked set. `false` if `i >= size`.                                                 |
| `void NAME_flip_at(NAME_t *v, size_t i)`                            | Toggle bit `i`, **no** bounds check.                                                        |
| `bool NAME_flip(NAME_t *v, size_t i)`                               | Bounds-checked toggle. `false` if `i >= size`.                                              |
| `bool NAME_push_back(NAME_t *v, bool x)`                            | Append a bit. `false` if **full**. Bit vector only.                                         |
| `bool NAME_append(NAME_t *v, const bool *xs, size_t len)`           | Append `len` bits. `false` if it does not fit (unchanged). Bit vector only.                 |
| `bool NAME_pop_back(NAME_t *v, bool *out)`                          | Remove the last bit (optionally into `*out`). `false` if empty. Bit vector only.            |
| `const bool *NAME_const_front(const NAME_t *v)`                     | Peek the first bit. `NULL` if empty. Bit vector only.                                       |
| `const bool *NAME_const_back(const NAME_t *v)`                      | Peek the last bit. `NULL` if empty. Bit vector only.                                        |
| `bool NAME_insert(NAME_t *v, size_t i, bool x)`                     | Insert at `i`, shifting the tail right. `false` if **full** or `i > size`. Bit vector only. |
| `bool NAME_remove(NAME_t *v, size_t i, bool *out)`                  | Remove at `i`, shifting the tail left. `false` if `i >= size`. Bit vector only.             |
| `void NAME_remove_at(NAME_t *v, size_t i, bool *out, size_t *next)` | Iterator-friendly `remove` (see the vector). UB if `i >= size`. Bit vector only.            |
| `bool NAME_swap_remove(NAME_t *v, size_t i, bool *out)`             | O(1) remove (does not preserve order). `false` if `i >= size`. Bit vector only.             |
| `void NAME_set_all(NAME_t *v, bool x)`                              | Set every stored bit to `x`.                                                                |
| `void NAME_flip_all(NAME_t *v)`                                     | Toggle all bits.                                                                            |
| `size_t NAME_count(const NAME_t *v)`                                | Number of bits set to 1.                                                                    |
| `NAME_block_t *NAME_blocks(NAME_t *v)`                              | Raw packed storage (LSB-first within each block).                                           |
| `const NAME_block_t *NAME_const_blocks(const NAME_t *v)`            | Const raw packed storage.                                                                   |
| `size_t NAME_block_count(const NAME_t *v)`                          | Number of storage blocks, `ceil(SIZE / block_bits)`.                                        |
| `size_t NAME_block_bits(const NAME_t *v)`                           | Number of bits per storage block (`sizeof(BLOCK_TYPE) * CHAR_BIT`).                         |
| `NAME_iter_t NAME_begin/end/iter_next(...)`                         | Index-based iteration (dereference with `const_at`).                                        |
| `NAME_iter_t NAME_begin_true/end/iter_next_true(...)`               | Index-based iteration over set bits (dereference with `const_at`).                          |
| `bool NAME_eq(const NAME_t *left, const NAME_t *right)`             | Check if two bit vectors are equal (i.e. have the same size, and same bits).                |
| `void NAME_copy(NAME_t *dst, const NAME_t *src)`                    | Copies `src` into `dst`.                                                                    |
| `bool NAME_all(const NAME_t *v)`                                    | Returns true if all bits of vector are set, false otherwise.                                |
| `bool NAME_any(const NAME_t *v)`                                    | Returns true if at least one bit of vector is set, false otherwise.                         |

### Example

```c
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME       bits
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE       32
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE uint8_t  // optional; defaults to size_t
#include "zenoh-pico/collections/static_bit_vector_template.h"

bits_t v = bits_new();
bits_push_back(&v, true);
bits_push_back(&v, false);
bits_insert(&v, 1, true);   // v == [1, 1, 0]
bits_flip(&v, 2);           // v == [1, 1, 1]
size_t ones = bits_count(&v);  // 3
bits_destroy(&v);
```

---

## `hashmap_template.h` — heap-allocated hash map

A growable hash map using **separate chaining** over a single contiguous node pool.
It is the dynamically-growable counterpart of `static_hashmap_template.h` and exposes
the same interface plus growth/allocation helpers.

For a hash set (unique keys without values), see [`hashset_template.h`](#hashset_templateh--heap-allocated-hash-set).

Key design points:

* Nodes live in one flat pool (one allocation) for good cache locality; bucket heads
  are merged into the same array.
* **Iterators are pool indices and remain stable** across rehashing, growth, and the
  removal of *other* keys. An iterator only becomes invalid when *its own* entry is
  removed.
* The index/iterator width is configurable (`uint8_t` / `uint16_t` / `uint32_t`),
  trading addressable capacity for per-entry memory overhead.

### Configuration macros

| Macro                                      | Required | Default                    | Purpose                                                                                         |
| ------------------------------------------ | :------: | -------------------------- | ----------------------------------------------------------------------------------------------- |
| `_ZP_HASHMAP_TEMPLATE_KEY_TYPE`            |    ✅    | —                          | Key type.                                                                                       |
| `_ZP_HASHMAP_TEMPLATE_KEY_HASH_FN(k)`      |    ✅    | —                          | Hash of `*k` → `size_t`.                                                                        |
| `_ZP_HASHMAP_TEMPLATE_VAL_TYPE`            |    ✅    | —                          | Value type.                                                                                     |
| `_ZP_HASHMAP_TEMPLATE_KEY_EQ_FN(a,b)`      |    ❌    | `*a == *b`                 | Key equality.                                                                                   |
| `_ZP_HASHMAP_TEMPLATE_NAME`                |    ❌    | derived from key/val types | Base name for generated symbols.                                                                |
| `_ZP_HASHMAP_TEMPLATE_INDEX_TYPE`          |    ❌    | `uint32_t`                 | Unsigned index/iterator type; its max value is reserved as a sentinel, so capacity ≤ `max - 1`. |
| `_ZP_HASHMAP_TEMPLATE_INITIAL_CAPACITY`    |    ❌    | `16`                       | Entries/buckets reserved on first insert.                                                       |
| `_ZP_HASHMAP_TEMPLATE_KEY_DESTROY_FN(k)`   |    ❌    | no-op                      | Destroy a key.                                                                                  |
| `_ZP_HASHMAP_TEMPLATE_VAL_DESTROY_FN(v)`   |    ❌    | no-op                      | Destroy a value. (Ignored in hash-set mode.)                                                    |
| `_ZP_HASHMAP_TEMPLATE_KEY_MOVE_FN(d,s)`    |    ❌    | `*d = *s`                  | Move a key.                                                                                     |
| `_ZP_HASHMAP_TEMPLATE_VAL_MOVE_FN(d,s)`    |    ❌    | `*d = *s`                  | Move a value. (Ignored in hash-set mode.)                                                       |
| `_ZP_HASHMAP_TEMPLATE_ALLOC_FN(bytes)`     |    ❌    | `malloc`                   | Allocate memory.                                                                                |
| `_ZP_HASHMAP_TEMPLATE_FREE_FN(ptr)`        |    ❌    | `free`                     | Free memory.                                                                                    |
| `_ZP_HASHMAP_TEMPLATE_REALLOC_FN(p,bytes)` |    ❌    | (unused)                   | In-place pool growth when key+value are trivially movable (key only in hash-set mode).          |

### Generated types

```c
typedef struct NAME_elem_t {     // a key/value node
    KEY_TYPE key;
    VAL_TYPE val;
} NAME_elem_t;

typedef KEY_TYPE   NAME_key_t;
typedef VAL_TYPE   NAME_val_t;
typedef INDEX_TYPE NAME_iter_t;  // pool index / iterator

typedef struct NAME_t { /* opaque pool + bookkeeping */ } NAME_t;
```

An invalid iterator is the all-ones value of the index type; compare against
`NAME_end(&map)` rather than relying on a literal.

### API

| Function                                                                             | Description                                                                                                                                                                                                                                                                                    |
| ------------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `void NAME_init(NAME_t *m)`                                                          | Initialise empty (no allocation until first insert).                                                                                                                                                                                                                                           |
| `NAME_t NAME_new(void)`                                                              | Return a new empty map.                                                                                                                                                                                                                                                                        |
| `bool NAME_reserve(NAME_t *m, size_t min_cap)`                                       | Ensure capacity for `min_cap` entries. `false` on allocation failure.                                                                                                                                                                                                                          |
| `size_t NAME_size(const NAME_t *m)`                                                  | Number of live entries.                                                                                                                                                                                                                                                                        |
| `size_t NAME_capacity(const NAME_t *m)`                                              | Pool capacity.                                                                                                                                                                                                                                                                                 |
| `bool NAME_is_empty(const NAME_t *m)`                                                | `true` if empty.                                                                                                                                                                                                                                                                               |
| `NAME_iter_t NAME_insert(NAME_t *m, KEY_TYPE *k, VAL_TYPE *v)`                       | Move `*k`/`*v` in. If the key exists, the old value is destroyed and replaced and the *incoming key* is destroyed. `v` may be `NULL` to create an uninitialised value (fill it via `NAME_at`). Returns the entry iterator, or an invalid iterator on allocation failure / capacity exhaustion. |
| `VAL_TYPE *NAME_get(NAME_t *m, const NAME_key_t *k)`                                 | Pointer to the value for `k`, or `NULL`.                                                                                                                                                                                                                                                       |
| `const NAME_val_t *NAME_const_get(const NAME_t *m, const NAME_key_t *k)`             | Const variant of `get`.                                                                                                                                                                                                                                                                        |
| `bool NAME_contains(const NAME_t *m, const NAME_key_t *k)`                           | `true` if `k` is present.                                                                                                                                                                                                                                                                      |
| `NAME_iter_t NAME_get_iter(const NAME_t *m, const NAME_key_t *k)`                    | Iterator to the entry for `k`, or an invalid iterator.                                                                                                                                                                                                                                         |
| `NAME_elem_t *NAME_at(NAME_t *m, NAME_iter_t i)`                                     | Node at iterator `i` (UB if `i` is invalid).                                                                                                                                                                                                                                                   |
| `const NAME_elem_t *NAME_const_at(const NAME_t *m, NAME_iter_t i)`                   | Const variant of `at`.                                                                                                                                                                                                                                                                         |
| `bool NAME_remove(NAME_t *m, const NAME_key_t *k, VAL_TYPE *out)`                    | Remove entry for `k` (move value to `out`, else destroy). `true` if found.                                                                                                                                                                                                                     |
| `void NAME_remove_at(NAME_t *m, NAME_iter_t i, NAME_elem_t *out, NAME_iter_t *next)` | Remove the entry at `i` (move node to `out`, else destroy); if `next != NULL`, set `*next` to the following iterator.                                                                                                                                                                          |
| `void NAME_clear(NAME_t *m)`                                                         | Destroy all entries but keep the backing store.                                                                                                                                                                                                                                                |
| `void NAME_destroy(NAME_t *m)`                                                       | Destroy all entries and free the backing store.                                                                                                                                                                                                                                                |
| `NAME_iter_t NAME_begin(const NAME_t *m)`                                            | Iterator to the first entry, or `end` if empty.                                                                                                                                                                                                                                                |
| `NAME_iter_t NAME_end(const NAME_t *m)`                                              | Invalid post-end iterator.                                                                                                                                                                                                                                                                     |
| `NAME_iter_t NAME_iter_next(const NAME_t *m, NAME_iter_t i)`                         | Iterator to the next entry.                                                                                                                                                                                                                                                                    |

### Iteration

```c
for (NAME_iter_t i = NAME_begin(&m); i != NAME_end(&m); i = NAME_iter_next(&m, i)) {
    NAME_elem_t *e = NAME_at(&m, i);
    // use e->key, e->val
}
```

### Example

```c
static inline size_t int_hash(const int *k) { return (size_t)*k; }

#define _ZP_HASHMAP_TEMPLATE_KEY_TYPE     int
#define _ZP_HASHMAP_TEMPLATE_VAL_TYPE     int
#define _ZP_HASHMAP_TEMPLATE_NAME         i2i
#define _ZP_HASHMAP_TEMPLATE_KEY_HASH_FN  int_hash
#include "zenoh-pico/collections/hashmap_template.h"

i2i_t m = i2i_new();
int k = 1, v = 100;
i2i_insert(&m, &k, &v);
int *got = i2i_get(&m, &k);   // *got == 100
i2i_destroy(&m);
```

---

## `hashset_template.h` — heap-allocated hash set

A growable hash set — a heap-backed collection of unique keys with no associated
values.  Shares the same internal implementation as `hashmap_template.h` but generates
only the set-oriented API: no `get`/`const_get`, and `insert`/`remove` take no value argument.

### Configuration macros

| Macro                                      | Required | Default                    | Purpose                                                                                         |
| ------------------------------------------ | :------: | -------------------------- | ----------------------------------------------------------------------------------------------- |
| `_ZP_HASHSET_TEMPLATE_KEY_TYPE`            |    ✅    | —                          | Key type (the set element).                                                                     |
| `_ZP_HASHSET_TEMPLATE_KEY_HASH_FN(k)`      |    ✅    | —                          | Hash of `*k` → `size_t`.                                                                        |
| `_ZP_HASHSET_TEMPLATE_KEY_EQ_FN(a,b)`      |    ❌    | `*a == *b`                 | Key equality.                                                                                   |
| `_ZP_HASHSET_TEMPLATE_NAME`                |    ❌    | `<key_type>hset`           | Base name for generated symbols.                                                                |
| `_ZP_HASHSET_TEMPLATE_INDEX_TYPE`          |    ❌    | `uint32_t`                 | Unsigned index/iterator type; its max value is reserved as a sentinel, so capacity ≤ `max - 1`. |
| `_ZP_HASHSET_TEMPLATE_INITIAL_CAPACITY`    |    ❌    | `16`                       | Entries/buckets reserved on first insert.                                                       |
| `_ZP_HASHSET_TEMPLATE_KEY_DESTROY_FN(k)`   |    ❌    | no-op                      | Destroy a key.                                                                                  |
| `_ZP_HASHSET_TEMPLATE_KEY_MOVE_FN(d,s)`    |    ❌    | `*d = *s`                  | Move a key.                                                                                     |
| `_ZP_HASHSET_TEMPLATE_ALLOC_FN(bytes)`     |    ❌    | `malloc`                   | Allocate memory.                                                                                |
| `_ZP_HASHSET_TEMPLATE_FREE_FN(ptr)`        |    ❌    | `free`                     | Free memory.                                                                                    |
| `_ZP_HASHSET_TEMPLATE_REALLOC_FN(p,bytes)` |    ❌    | (unused)                   | In-place pool growth when key is trivially movable.                                             |

### Generated types

```c
typedef KEY_TYPE   NAME_key_t;
typedef KEY_TYPE   NAME_elem_t;    // the set element (alias for key)
typedef INDEX_TYPE NAME_iter_t;    // pool index / iterator

typedef struct NAME_t { /* opaque pool + bookkeeping */ } NAME_t;
```

An invalid iterator is the all-ones value of the index type; compare against
`NAME_end(&set)` rather than relying on a literal.

### API

| Function                                                                             | Description                                                                                                                                                                                                                                     |
| ------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `void NAME_init(NAME_t *s)`                                                          | Initialise empty (no allocation until first insert).                                                                                                                                                                                            |
| `NAME_t NAME_new(void)`                                                              | Return a new empty set.                                                                                                                                                                                                                         |
| `bool NAME_reserve(NAME_t *s, size_t min_cap)`                                       | Ensure capacity for `min_cap` entries. `false` on allocation failure.                                                                                                                                                                           |
| `size_t NAME_size(const NAME_t *s)`                                                  | Number of live entries.                                                                                                                                                                                                                         |
| `size_t NAME_capacity(const NAME_t *s)`                                              | Pool capacity.                                                                                                                                                                                                                                  |
| `bool NAME_is_empty(const NAME_t *s)`                                                | `true` if empty.                                                                                                                                                                                                                                |
| `NAME_iter_t NAME_insert(NAME_t *s, KEY_TYPE *k)`                                    | Move `*k` in. If the key already exists, the *incoming key* is destroyed and the entry is left unchanged. Returns the entry iterator, or an invalid iterator on allocation failure / capacity exhaustion.                                       |
| `bool NAME_contains(const NAME_t *s, const NAME_key_t *k)`                           | `true` if `k` is present.                                                                                                                                                                                                                       |
| `NAME_iter_t NAME_get_iter(const NAME_t *s, const NAME_key_t *k)`                    | Iterator to the entry for `k`, or an invalid iterator.                                                                                                                                                                                          |
| `NAME_elem_t *NAME_at(NAME_t *s, NAME_iter_t i)`                                     | Element at iterator `i` (UB if `i` is invalid).                                                                                                                                                                                                 |
| `const NAME_elem_t *NAME_const_at(const NAME_t *s, NAME_iter_t i)`                   | Const variant of `at`.                                                                                                                                                                                                                          |
| `bool NAME_remove(NAME_t *s, const NAME_key_t *k)`                                   | Remove entry for `k`. `true` if found.                                                                                                                                                                                                          |
| `void NAME_remove_at(NAME_t *s, NAME_iter_t i, NAME_elem_t *out, NAME_iter_t *next)` | Remove the entry at `i` (move element to `out`, else destroy); if `next != NULL`, set `*next` to the following iterator.                                                                                                                        |
| `void NAME_clear(NAME_t *s)`                                                         | Destroy all entries but keep the backing store.                                                                                                                                                                                                 |
| `void NAME_destroy(NAME_t *s)`                                                       | Destroy all entries and free the backing store.                                                                                                                                                                                                 |
| `NAME_iter_t NAME_begin(const NAME_t *s)`                                            | Iterator to the first entry, or `end` if empty.                                                                                                                                                                                                 |
| `NAME_iter_t NAME_end(const NAME_t *s)`                                              | Invalid post-end iterator.                                                                                                                                                                                                                      |
| `NAME_iter_t NAME_iter_next(const NAME_t *s, NAME_iter_t i)`                         | Iterator to the next entry.                                                                                                                                                                                                                     |

### Iteration

```c
for (NAME_iter_t i = NAME_begin(&s); i != NAME_end(&s); i = NAME_iter_next(&s, i)) {
    NAME_elem_t *e = NAME_at(&s, i);
    // use *e (the set element)
}
```

### Example

```c
static inline size_t id_hash(const uint32_t *k) { return *k; }

#define _ZP_HASHSET_TEMPLATE_KEY_TYPE     uint32_t
#define _ZP_HASHSET_TEMPLATE_NAME         idset
#define _ZP_HASHSET_TEMPLATE_KEY_HASH_FN  id_hash
#include "zenoh-pico/collections/hashset_template.h"

idset_t s = idset_new();
uint32_t a = 7;
idset_insert(&s, &a);
bool has = idset_contains(&s, &a); // true
idset_destroy(&s);
```

---

## `static_hashmap_template.h` — fixed-capacity hash map

Separate-chaining hash map backed by a **fixed-size, inline node pool** — no heap
allocation. The capacity is also used as the number of buckets, and the index type is
chosen automatically to be the smallest that fits the capacity.

For a hash set (unique keys without values), see [`static_hashset_template.h`](#static_hashset_templateh--fixed-capacity-hash-set).

### Configuration macros

| Macro                                           | Required | Default                    | Purpose                                                          |
| ----------------------------------------------- | :------: | -------------------------- | ---------------------------------------------------------------- |
| `_ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE`          |    ✅    | —                          | Key type.                                                        |
| `_ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN(k)`    |    ✅    | —                          | Hash of `*k` → `size_t`.                                         |
| `_ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE`          |    ✅    | —                          | Value type.                                                      |
| `_ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN(a,b)`    |    ❌    | `*a == *b`                 | Key equality.                                                    |
| `_ZP_STATIC_HASHMAP_TEMPLATE_NAME`              |    ❌    | derived from key/val types | Base name for generated symbols.                                 |
| `_ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY`          |    ❌    | `16`                       | Max entries (also the bucket count).                             |
| `_ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN(k)` |    ❌    | no-op                      | Destroy a key.                                                   |
| `_ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN(v)` |    ❌    | no-op                      | Destroy a value.                                                 |
| `_ZP_STATIC_HASHMAP_TEMPLATE_KEY_MOVE_FN(d,s)`  |    ❌    | `*d = *s`                  | Move a key.                                                      |
| `_ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN(d,s)`  |    ❌    | `*d = *s`                  | Move a value.                                                    |

### Generated types

```c
typedef struct NAME_elem_t {     // a key/value node
    KEY_TYPE key;
    VAL_TYPE val;
} NAME_elem_t;

typedef KEY_TYPE   NAME_key_t;
typedef VAL_TYPE   NAME_val_t;
typedef ITER_TYPE  NAME_iter_t;  // pool index / iterator (auto-selected from capacity)

typedef struct NAME_t { /* inline pool + bookkeeping */ } NAME_t;
```

An invalid iterator is the all-ones value of the index type; compare against
`NAME_end(&map)` rather than relying on a literal.

### API

| Function                                                                             | Description                                                                                                                                                                                                                                           |
| ------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `void NAME_init(NAME_t *m)`                                                          | Zero-initialise empty in place.                                                                                                                                                                                                                       |
| `NAME_t NAME_new(void)`                                                              | Return a new (zero-initialised) empty map.                                                                                                                                                                                                            |
| `size_t NAME_size(const NAME_t *m)`                                                  | Number of live entries.                                                                                                                                                                                                                               |
| `bool NAME_is_empty(const NAME_t *m)`                                                | `true` if empty.                                                                                                                                                                                                                                      |
| `NAME_iter_t NAME_insert(NAME_t *m, KEY_TYPE *k, VAL_TYPE *v)`                       | Move `*k`/`*v` in. If the key exists, the old value is destroyed and replaced and the *incoming key* is destroyed. `v` may be `NULL` to create an uninitialised value. Returns the entry iterator, or an invalid iterator when the pool is exhausted. |
| `VAL_TYPE *NAME_get(NAME_t *m, const NAME_key_t *k)`                                 | Pointer to the value for `k`, or `NULL`.                                                                                                                                                                                                              |
| `const NAME_val_t *NAME_const_get(const NAME_t *m, const NAME_key_t *k)`             | Const variant of `get`.                                                                                                                                                                                                                               |
| `bool NAME_contains(const NAME_t *m, const NAME_key_t *k)`                           | `true` if `k` is present.                                                                                                                                                                                                                             |
| `NAME_iter_t NAME_get_iter(const NAME_t *m, const NAME_key_t *k)`                    | Iterator to the entry for `k`, or an invalid iterator.                                                                                                                                                                                                |
| `NAME_elem_t *NAME_at(NAME_t *m, NAME_iter_t i)`                                     | Node at iterator `i` (UB if `i` is invalid).                                                                                                                                                                                                          |
| `const NAME_elem_t *NAME_const_at(const NAME_t *m, NAME_iter_t i)`                   | Const variant of `at`.                                                                                                                                                                                                                                |
| `bool NAME_remove(NAME_t *m, const NAME_key_t *k, VAL_TYPE *out)`                    | Remove entry for `k` (move value to `out`, else destroy). `true` if found.                                                                                                                                                                            |
| `void NAME_remove_at(NAME_t *m, NAME_iter_t i, NAME_elem_t *out, NAME_iter_t *next)` | Remove the entry at `i` (move node to `out`, else destroy); if `next != NULL`, set `*next` to the following iterator.                                                                                                                                 |
| `void NAME_clear(NAME_t *m)`                                                         | Destroy all entries but keep the backing store for reuse.                                                                                                                                                                                             |
| `void NAME_destroy(NAME_t *m)`                                                       | Destroy all entries and reset the map for reuse (frees nothing since storage is inline).                                                                                                                                                              |
| `NAME_iter_t NAME_begin(const NAME_t *m)`                                            | Iterator to the first entry, or `end` if empty.                                                                                                                                                                                                       |
| `NAME_iter_t NAME_end(const NAME_t *m)`                                              | Invalid post-end iterator.                                                                                                                                                                                                                            |
| `NAME_iter_t NAME_iter_next(const NAME_t *m, NAME_iter_t i)`                         | Iterator to the next entry.                                                                                                                                                                                                                           |

Differences from the heap map (`hashmap_template.h`):

* **No** `capacity`, `reserve`, or allocator configuration — capacity is the fixed compile-time `CAPACITY`.
* Index type is auto-selected from `CAPACITY` (uint8_t / uint16_t / uint32_t).
* `NAME_destroy` resets the map for reuse rather than freeing memory.

### Iteration

```c
for (NAME_iter_t i = NAME_begin(&m); i != NAME_end(&m); i = NAME_iter_next(&m, i)) {
    NAME_elem_t *e = NAME_at(&m, i);
    // use e->key, e->val
}
```

### Example

```c
static inline size_t key_hash(const size_t *k) { return *k; }

#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE     size_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE     int
#define _ZP_STATIC_HASHMAP_TEMPLATE_NAME         tasks
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN  key_hash
#define _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY     32
#include "zenoh-pico/collections/static_hashmap_template.h"

tasks_t m = tasks_new();
size_t k = 7; int v = 1;
tasks_insert(&m, &k, &v);
tasks_destroy(&m);
```

---

## `static_hashset_template.h` — fixed-capacity hash set

A fixed-capacity hash set — a compile-time-sized collection of unique keys with no
associated values.  Shares the same internal implementation as `static_hashmap_template.h`
but generates only the set-oriented API: no `get`/`const_get`, and `insert`/`remove` take no value argument.

### Configuration macros

| Macro                                           | Required | Default                    | Purpose                                                          |
| ----------------------------------------------- | :------: | -------------------------- | ---------------------------------------------------------------- |
| `_ZP_STATIC_HASHSET_TEMPLATE_KEY_TYPE`          |    ✅    | —                          | Key type (the set element).                                      |
| `_ZP_STATIC_HASHSET_TEMPLATE_KEY_HASH_FN(k)`    |    ✅    | —                          | Hash of `*k` → `size_t`.                                         |
| `_ZP_STATIC_HASHSET_TEMPLATE_KEY_EQ_FN(a,b)`    |    ❌    | `*a == *b`                 | Key equality.                                                    |
| `_ZP_STATIC_HASHSET_TEMPLATE_NAME`              |    ❌    | `<key_type>hset`           | Base name for generated symbols.                                 |
| `_ZP_STATIC_HASHSET_TEMPLATE_CAPACITY`          |    ❌    | `16`                       | Max entries (also the bucket count).                             |
| `_ZP_STATIC_HASHSET_TEMPLATE_KEY_DESTROY_FN(k)` |    ❌    | no-op                      | Destroy a key.                                                   |
| `_ZP_STATIC_HASHSET_TEMPLATE_KEY_MOVE_FN(d,s)`  |    ❌    | `*d = *s`                  | Move a key.                                                      |

### Generated types

```c
typedef KEY_TYPE   NAME_key_t;
typedef KEY_TYPE   NAME_elem_t;    // the set element (alias for key)
typedef ITER_TYPE  NAME_iter_t;    // pool index / iterator (auto-selected from capacity)

typedef struct NAME_t { /* inline pool + bookkeeping */ } NAME_t;
```

An invalid iterator is the all-ones value of the index type; compare against
`NAME_end(&set)` rather than relying on a literal.

### API

| Function                                                                             | Description                                                                                                                                                                                                                                     |
| ------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `void NAME_init(NAME_t *s)`                                                          | Zero-initialise empty in place.                                                                                                                                                                                                                 |
| `NAME_t NAME_new(void)`                                                              | Return a new (zero-initialised) empty set.                                                                                                                                                                                                      |
| `size_t NAME_size(const NAME_t *s)`                                                  | Number of live entries.                                                                                                                                                                                                                         |
| `bool NAME_is_empty(const NAME_t *s)`                                                | `true` if empty.                                                                                                                                                                                                                                |
| `NAME_iter_t NAME_insert(NAME_t *s, KEY_TYPE *k)`                                    | Move `*k` in. If the key already exists, the *incoming key* is destroyed and the entry is left unchanged. Returns the entry iterator, or an invalid iterator when the pool is exhausted and the key is not present.                             |
| `bool NAME_contains(const NAME_t *s, const NAME_key_t *k)`                           | `true` if `k` is present.                                                                                                                                                                                                                       |
| `NAME_iter_t NAME_get_iter(const NAME_t *s, const NAME_key_t *k)`                    | Iterator to the entry for `k`, or an invalid iterator.                                                                                                                                                                                          |
| `NAME_elem_t *NAME_at(NAME_t *s, NAME_iter_t i)`                                     | Element at iterator `i` (UB if `i` is invalid).                                                                                                                                                                                                 |
| `const NAME_elem_t *NAME_const_at(const NAME_t *s, NAME_iter_t i)`                   | Const variant of `at`.                                                                                                                                                                                                                          |
| `bool NAME_remove(NAME_t *s, const NAME_key_t *k)`                                   | Remove entry for `k`. `true` if found.                                                                                                                                                                                                          |
| `void NAME_remove_at(NAME_t *s, NAME_iter_t i, NAME_elem_t *out, NAME_iter_t *next)` | Remove the entry at `i` (move element to `out`, else destroy); if `next != NULL`, set `*next` to the following iterator.                                                                                                                        |
| `void NAME_clear(NAME_t *s)`                                                         | Destroy all entries but keep the backing store for reuse.                                                                                                                                                                                       |
| `void NAME_destroy(NAME_t *s)`                                                       | Reset the set for reuse (destroys all entries; frees nothing since storage is inline).                                                                                                                                                          |
| `NAME_iter_t NAME_begin(const NAME_t *s)`                                            | Iterator to the first entry, or `end` if empty.                                                                                                                                                                                                 |
| `NAME_iter_t NAME_end(const NAME_t *s)`                                              | Invalid post-end iterator.                                                                                                                                                                                                                      |
| `NAME_iter_t NAME_iter_next(const NAME_t *s, NAME_iter_t i)`                         | Iterator to the next entry.                                                                                                                                                                                                                     |

Differences from the heap set (`hashset_template.h`):

* **No** `capacity`, `reserve`, or allocator configuration — capacity is the fixed compile-time `CAPACITY`.
* Index type is auto-selected from `CAPACITY` (uint8_t / uint16_t / uint32_t).
* `NAME_destroy` resets the set for reuse rather than freeing memory.

### Iteration

```c
for (NAME_iter_t i = NAME_begin(&s); i != NAME_end(&s); i = NAME_iter_next(&s, i)) {
    NAME_elem_t *e = NAME_at(&s, i);
    // use *e (the set element)
}
```

### Example

```c
static inline size_t id_hash(const uint32_t *k) { return *k; }

#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_TYPE     uint32_t
#define _ZP_STATIC_HASHSET_TEMPLATE_NAME         idset
#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_HASH_FN  id_hash
#define _ZP_STATIC_HASHSET_TEMPLATE_CAPACITY     32
#include "zenoh-pico/collections/static_hashset_template.h"

idset_t s = idset_new();
uint32_t a = 7;
idset_insert(&s, &a);
bool has = idset_contains(&s, &a); // true
idset_destroy(&s);
```

---

## `static_deque_template.h` — fixed-capacity double-ended queue

A double-ended queue implemented as an inline **circular buffer** with a compile-time
maximum size — no heap allocation. Supports O(1) insertion and removal at both ends.

### Configuration macros

| Macro                                          | Required | Default                    | Purpose                             |
| ---------------------------------------------- | :------: | -------------------------- | ----------------------------------- |
| `_ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE`          |    ✅    | —                          | Element type.                       |
| `_ZP_STATIC_DEQUE_TEMPLATE_SIZE`               |    ❌    | `16`                       | Maximum capacity (circular buffer). |
| `_ZP_STATIC_DEQUE_TEMPLATE_NAME`               |    ❌    | derived from type and size | Base name for generated symbols.    |
| `_ZP_STATIC_DEQUE_TEMPLATE_ELEM_DESTROY_FN(x)` |    ❌    | no-op                      | Destroy one element.                |
| `_ZP_STATIC_DEQUE_TEMPLATE_ELEM_MOVE_FN(d,s)`  |    ❌    | `*d = *s`                  | Move one element.                   |

### API

| Function                                         | Description                                                            |
| ------------------------------------------------ | ---------------------------------------------------------------------- |
| `NAME_t NAME_new(void)`                          | Return a new (zero-initialised) empty deque.                           |
| `size_t NAME_size(const NAME_t *d)`              | Number of stored elements.                                             |
| `bool NAME_is_empty(const NAME_t *d)`            | `true` if empty.                                                       |
| `bool NAME_push_back(NAME_t *d, ELEM_TYPE *e)`   | Move `*e` to the back. `false` if full.                                |
| `bool NAME_push_front(NAME_t *d, ELEM_TYPE *e)`  | Move `*e` to the front. `false` if full.                               |
| `bool NAME_pop_back(NAME_t *d, ELEM_TYPE *out)`  | Remove from the back (move to `out`, else destroy). `false` if empty.  |
| `bool NAME_pop_front(NAME_t *d, ELEM_TYPE *out)` | Remove from the front (move to `out`, else destroy). `false` if empty. |
| `ELEM_TYPE *NAME_back(NAME_t *d)`                | Pointer to the back element, or `NULL`.                                |
| `ELEM_TYPE *NAME_front(NAME_t *d)`               | Pointer to the front element, or `NULL`.                               |
| `void NAME_destroy(NAME_t *d)`                   | Destroy all elements and reset to empty.                               |

Use it as a **FIFO queue** (`push_back` + `pop_front`) or a **LIFO stack**
(`push_back` + `pop_back`).

### Example

```c
#define _ZP_STATIC_DEQUE_TEMPLATE_ELEM_TYPE int
#define _ZP_STATIC_DEQUE_TEMPLATE_NAME      idq
#define _ZP_STATIC_DEQUE_TEMPLATE_SIZE      8
#include "zenoh-pico/collections/static_deque_template.h"

idq_t d = idq_new();
int a = 1, b = 2;
idq_push_back(&d, &a);   // [1]
idq_push_front(&d, &b);  // [2, 1]
int out;
idq_pop_front(&d, &out); // out == 2, d == [1]
idq_destroy(&d);
```

---

## `static_pqueue_template.h` — fixed-capacity priority queue

A binary-heap priority queue stored in an inline array with a compile-time maximum
size — no heap allocation. By default it is a **min-priority queue** (the smallest
element, per the comparator, is at the top).

### Configuration macros

| Macro                                           | Required | Default                    | Purpose                                                                                                                               |
| ----------------------------------------------- | :------: | -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| `_ZP_STATIC_PQUEUE_TEMPLATE_ELEM_TYPE`          |    ✅    | —                          | Element type.                                                                                                                         |
| `_ZP_STATIC_PQUEUE_TEMPLATE_ELEM_CMP_FN(a,b)`   |    ✅    | —                          | Compare two elements: `<0` if `a` has higher priority than `b`, `0` if equal, `>0` otherwise.                                         |
| `_ZP_STATIC_PQUEUE_TEMPLATE_NAME`               |    ❌    | derived from type and size | Base name for generated symbols.                                                                                                      |
| `_ZP_STATIC_PQUEUE_TEMPLATE_SIZE`               |    ❌    | `16`                       | Maximum capacity.                                                                                                                     |
| `_ZP_STATIC_PQUEUE_TEMPLATE_ELEM_DESTROY_FN(x)` |    ❌    | no-op                      | Destroy one element.                                                                                                                  |
| `_ZP_STATIC_PQUEUE_TEMPLATE_ELEM_MOVE_FN(d,s)`  |    ❌    | `*d = *s`                  | Move one element.                                                                                                                     |
| `_ZP_STATIC_PQUEUE_TEMPLATE_CMP_CTX_TYPE`       |    ❌    | (none)                     | Optional context type; when defined, the comparator signature becomes `(a, b, ctx_ptr)` and a context pointer is stored in the queue. |

### API

| Function                                      | Description                                                          |
| --------------------------------------------- | -------------------------------------------------------------------- |
| `NAME_t NAME_new(void)`                       | Return a new (zero-initialised) empty queue.                         |
| `NAME_t NAME_new_with_ctx(CTX_TYPE *ctx)`     | *(context builds only)* New queue carrying comparator context `ctx`. |
| `void NAME_set_ctx(NAME_t *q, CTX_TYPE *ctx)` | *(context builds only)* Replace the stored context pointer.          |
| `size_t NAME_size(const NAME_t *q)`           | Number of stored elements.                                           |
| `bool NAME_is_empty(const NAME_t *q)`         | `true` if empty.                                                     |
| `ELEM_TYPE *NAME_peek(NAME_t *q)`             | Pointer to the top (highest-priority) element, or `NULL`.            |
| `bool NAME_push(NAME_t *q, ELEM_TYPE *e)`     | Move `*e` in and sift up. `false` if full.                           |
| `bool NAME_pop(NAME_t *q, ELEM_TYPE *out)`    | Move the top element into `*out` and re-heapify. `false` if empty.   |
| `void NAME_destroy(NAME_t *q)`                | Destroy all elements and reset to empty.                             |

> `NAME_sift_up` / `NAME_sift_down` are generated as internal heap helpers; prefer
> `push` / `pop`.

### Example — min-heap

```c
static inline int int_cmp(const int *a, const int *b) { return (*a > *b) - (*a < *b); }

#define _ZP_STATIC_PQUEUE_TEMPLATE_ELEM_TYPE     int
#define _ZP_STATIC_PQUEUE_TEMPLATE_NAME          intpq
#define _ZP_STATIC_PQUEUE_TEMPLATE_SIZE          8
#define _ZP_STATIC_PQUEUE_TEMPLATE_ELEM_CMP_FN   int_cmp
#include "zenoh-pico/collections/static_pqueue_template.h"

intpq_t pq = intpq_new();
int vals[] = {5, 1, 3};
for (int i = 0; i < 3; i++) intpq_push(&pq, &vals[i]);
int out;
intpq_pop(&pq, &out);   // out == 1 (smallest first)
intpq_destroy(&pq);
```

### Example — comparator with context (min/max selectable at runtime)

```c
typedef struct { int multiplier; } pq_ctx_t;  // +1 → min-heap, -1 → max-heap
static inline int ctx_cmp(const int *a, const int *b, const pq_ctx_t *c) {
    return c->multiplier * ((*a > *b) - (*a < *b));
}

#define _ZP_STATIC_PQUEUE_TEMPLATE_ELEM_TYPE     int
#define _ZP_STATIC_PQUEUE_TEMPLATE_NAME          ctxpq
#define _ZP_STATIC_PQUEUE_TEMPLATE_SIZE          8
#define _ZP_STATIC_PQUEUE_TEMPLATE_CMP_CTX_TYPE  pq_ctx_t
#define _ZP_STATIC_PQUEUE_TEMPLATE_ELEM_CMP_FN   ctx_cmp
#include "zenoh-pico/collections/static_pqueue_template.h"

pq_ctx_t ctx = { .multiplier = -1 };       // max-heap
ctxpq_t pq = ctxpq_new_with_ctx(&ctx);
```

> When storing a context-carrying queue inside a movable, self-referential struct,
> call `NAME_set_ctx` after the move so the stored pointer stays valid.

---

## `variant_template.h` — tagged union (variant)

A tagged union holding **one of up to eight** user-supplied alternative types at a
time, plus an empty/`NONE` state (similar to `std::variant`). Storage is an anonymous
union with no heap allocation; the active alternative is tracked by a tag.

Alternatives are **independent and may be sparse**: you may enable any subset of slots
`1..8` (e.g. slot 4 without slot 3), as long as at least one is defined.

### Configuration macros

| Macro                                    | Required | Default          | Purpose                                                   |
| ---------------------------------------- | :------: | ---------------- | --------------------------------------------------------- |
| `_ZP_VARIANT_TEMPLATE_NAME`              |    ✅    | —                | Base name for all generated symbols.                      |
| `_ZP_VARIANT_TEMPLATE_<N>_TYPE`          |    ⚠️    | —                | Type of alternative `N` (1..8). At least one is required. |
| `_ZP_VARIANT_TEMPLATE_<N>_NAME`          |    ❌    | `N` (the digit)  | Identifier suffix used in that alternative's symbols.     |
| `_ZP_VARIANT_TEMPLATE_<N>_DESTROY_FN(p)` |    ❌    | no-op            | Destroy alternative `N`.                                  |
| `_ZP_VARIANT_TEMPLATE_<N>_MOVE_FN(d,s)`  |    ❌    | `*d = *s`        | Move alternative `N`.                                     |
| `_ZP_VARIANT_TEMPLATE_NO_MOVE_FN`        |    ❌    | (move generated) | When defined, suppress generation of `NAME_move`.         |

Using a custom `<N>_NAME` (e.g. `ok` / `err`) changes the generated suffixes:
`NAME_from_ok`, `NAME_is_ok`, `NAME_get_ok`, `NAME_take_ok`, and the tag `NAME_tagok`.
Below, `XN` denotes an alternative's name (the digit by default).

### Generated types

```c
typedef struct NAME_t   { /* tag + anonymous union */ } NAME_t;
typedef enum   NAME_tag_t { NAME_tag_none = 0, NAME_tag1, NAME_tag2, ... } NAME_tag_t;
typedef void   NAME_none_t;          // the empty state holds no value
typedef <N>_TYPE NAME_XN_t;          // alias per enabled alternative
```

### API (`NAME` = configured name, `XN` = alternative name)

#### Construction / destruction

| Function                                   | Description                                                                                                              |
| ------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------ |
| `NAME_t NAME_none(void)`                   | Construct the empty (`NONE`) variant.                                                                                    |
| `NAME_t NAME_from_XN(<N>_TYPE *val)`       | Construct holding alternative `XN`, moving `*val` in.                                                                    |
| `void NAME_destroy(NAME_t *v)`             | Destroy the active alternative and reset to `NONE`.                                                                      |
| `void NAME_move(NAME_t *dst, NAME_t *src)` | Move `*src` into `*dst`; `*dst` must be uninitialized or safe to overwrite.                                              |
|                                            | Previous `*dst` contents are not destroyed; `*src` becomes `NONE`. Omitted if `..._NO_MOVE_FN` is defined.               |

#### Inspection

| Function                               | Description                          |
| -------------------------------------- | ------------------------------------ |
| `NAME_tag_t NAME_tag(const NAME_t *v)` | Current tag.                         |
| `bool NAME_is_none(const NAME_t *v)`   | `true` if empty.                     |
| `bool NAME_is_XN(const NAME_t *v)`     | `true` if it holds alternative `XN`. |

#### Access

| Function                                             | Description                                                                                                    |
| ---------------------------------------------------- | -------------------------------------------------------------------------------------------------------------- |
| `<N>_TYPE *NAME_get_XN(NAME_t *v)`                   | Pointer to the held value (UB if not alternative `XN`).                                                        |
| `const <N>_TYPE *NAME_const_get_XN(const NAME_t *v)` | Const variant of `get`.                                                                                        |
| `bool NAME_take_XN(NAME_t *v, <N>_TYPE *out)`        | Move the held value into `*out` and reset to `NONE`. `false` (leaving `*v` untouched) if not alternative `XN`. |

#### Visitation (macros, not functions)

| Macro                                                | Description                                                                                                                                                                                                                                                                                                                                                                     |
| ---------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `_ZP_VARIANT_VISIT(NAME, v, (tag, expr), ...)`       | Dispatch on the active alternative of `v`. Provide one `(tag, expr)` arm per alternative — where `tag` is the alternative's name (a custom `<N>_NAME` such as `ok`, or the default digit `1`) — plus a `(none, )` arm for the empty state. Inside `expr`, `_` is a pointer to the active member. Covering every tag (including `none`) keeps the generated `switch` exhaustive. |
| `_ZP_VARIANT_CONST_VISIT(NAME, v, (tag, expr), ...)` | Read-only counterpart that binds `_` to a `const` pointer (use on a `const NAME_t *`).                                                                                                                                                                                                                                                                                          |

### Example — default names

```c
#define _ZP_VARIANT_TEMPLATE_NAME    num_or_str
#define _ZP_VARIANT_TEMPLATE_1_TYPE  int
#define _ZP_VARIANT_TEMPLATE_2_TYPE  my_string_t
#define _ZP_VARIANT_TEMPLATE_2_DESTROY_FN(p)  my_string_destroy(p)
#define _ZP_VARIANT_TEMPLATE_2_MOVE_FN(d, s)  my_string_move(d, s)
#include "zenoh-pico/collections/variant_template.h"

int n = 7;
num_or_str_t v = num_or_str_from_1(&n);
if (num_or_str_is_1(&v)) {
    printf("int = %d\n", *num_or_str_get_1(&v));
}
num_or_str_destroy(&v);
```

### Example — custom alternative names (`ok` / `err`)

```c
#define _ZP_VARIANT_TEMPLATE_NAME    my_result
#define _ZP_VARIANT_TEMPLATE_1_TYPE  int
#define _ZP_VARIANT_TEMPLATE_1_NAME  ok
#define _ZP_VARIANT_TEMPLATE_2_TYPE  my_string_t
#define _ZP_VARIANT_TEMPLATE_2_NAME  err
#define _ZP_VARIANT_TEMPLATE_2_DESTROY_FN(p)  my_string_destroy(p)
#define _ZP_VARIANT_TEMPLATE_2_MOVE_FN(d, s)  my_string_move(d, s)
#include "zenoh-pico/collections/variant_template.h"

int code = 0;
my_result_t r = my_result_from_ok(&code);
my_result_destroy(&r);
```

### Example — visitation

```c
// A variant with named alternatives: `num` (int) and `text` (my_string_t).
#define _ZP_VARIANT_TEMPLATE_NAME    value
#define _ZP_VARIANT_TEMPLATE_1_TYPE  int
#define _ZP_VARIANT_TEMPLATE_1_NAME  num
#define _ZP_VARIANT_TEMPLATE_2_TYPE  my_string_t
#define _ZP_VARIANT_TEMPLATE_2_NAME  text
#define _ZP_VARIANT_TEMPLATE_2_DESTROY_FN(p)  my_string_destroy(p)
#define _ZP_VARIANT_TEMPLATE_2_MOVE_FN(d, s)  my_string_move(d, s)
#include "zenoh-pico/collections/variant_template.h"

static void on_num(int *p)          { printf("num: %d\n", *p); }
static void on_text(my_string_t *p) { printf("text\n"); (void)p; }

int n = 7;
value_t v = value_from_num(&n);

// Arms use the alternative names (`num`, `text`); always include the `none` arm.
_ZP_VARIANT_VISIT(value, &v,
    (num,  on_num(_)),
    (text, on_text(_)),
    (none, ),
);

value_destroy(&v);
```

---

## `algorithms_template.h` — generic iteration / search helpers

Unlike the other headers, `algorithms_template.h` is a **regular header** (with an
include guard) — you just `#include` it once, no configuration macros required. It
provides a family of **function-like macros** for iterating, searching and removing
elements that work against *any* container generated by the templates above which
exposes the standard iterator surface:

```text
NAME_iter_t  NAME_begin(const NAME_t *)
NAME_iter_t  NAME_end(const NAME_t *)
NAME_iter_t  NAME_iter_next(const NAME_t *, NAME_iter_t)
NAME_elem_t *NAME_at(NAME_t *, NAME_iter_t)            // + NAME_const_at
```

In practice this means the **vectors**, **bit vectors**, **hash maps** and **hash sets**.
The deque, priority queue and variant do not expose iterators and are not supported.

Every macro takes the container's base name (`collection_name`, e.g. `intvec`) as its
first argument and a pointer to the instance (`collection_ptr`) as its second, so it
can stitch together the correct `collection_name##_begin` / `_at` / `_iter_next`
calls. Include the header next to the container instantiation:

```c
#define _ZP_VECTOR_TEMPLATE_ELEM_TYPE int
#define _ZP_VECTOR_TEMPLATE_NAME      intvec
#include "zenoh-pico/collections/vector_template.h"

#include "zenoh-pico/collections/algorithms_template.h"
```

### Conventions

* **`var_name`** — a pointer variable you **declare yourself before** the macro. The
  iteration macros assign each element/value pointer to it; the find macros set it to
  the match or `NULL`.
* **`predicate`** — an expression evaluated per element in which the identifier `_`
  is bound to a pointer to the current element (`NAME_elem_t *`) or value
  (`NAME_val_t *`), depending on the macro. For a vector of `int`, `_` is `int *`, so
  the predicate reads e.g. `*_ == 7`; for a hash map, `_->key` / `_->val` are available.
* **element vs value** — for hash maps, `NAME_elem_t` is the `{ key, val }` node. The
  plain macros expose the element/node; the `_VAL` variants expose `&node->val`
  directly, which is handy when you only care about values.
* **`const` variants** — macros prefixed `_ZP_CONST_` use `NAME_const_at` and bind
  `_` / `var_name` as read-only pointers, so they work on a `const NAME_t *`.

### Macros

| Macro                                                 | Purpose                                                                                                                                                                                                |
| ----------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `_ZP_FOREACH(name, ptr, var)`                         | Loop over every element; `var` (an `elem_t *`) points to each in turn. Use a block body `{ ... }`.                                                                                                     |
| `_ZP_CONST_FOREACH(name, ptr, var)`                   | `const` counterpart of `_ZP_FOREACH` (`var` is `const elem_t *`).                                                                                                                                      |
| `_ZP_FOREACH_VAL(name, ptr, var)`                     | Loop over hash-map **values**; `var` (a `val_t *`) points to each value.                                                                                                                               |
| `_ZP_CONST_FOREACH_VAL(name, ptr, var)`               | `const` counterpart of `_ZP_FOREACH_VAL`.                                                                                                                                                              |
| `_ZP_FIND(name, ptr, var, predicate)`                 | Set `var` to the first element matching `predicate` (with `_` bound to the element), or `NULL` if none.                                                                                                |
| `_ZP_CONST_FIND(name, ptr, var, predicate)`           | `const` counterpart of `_ZP_FIND`.                                                                                                                                                                     |
| `_ZP_FIND_VAL(name, ptr, var, predicate)`             | Set `var` to the first **value** matching `predicate` (`_` bound to `val_t *`), or `NULL`.                                                                                                             |
| `_ZP_CONST_FIND_VAL(name, ptr, var, predicate)`       | `const` counterpart of `_ZP_FIND_VAL`.                                                                                                                                                                 |
| `_ZP_IT_FIND(name, ptr, begin, end, predicate)`       | Advance the iterator `begin` to the first element in the range `[begin, end)` matching `predicate` (`_` bound to the element); leaves `begin == end` if none. `begin`/`end` are `NAME_iter_t` lvalues. |
| `_ZP_CONST_IT_FIND(name, ptr, begin, end, predicate)` | `const` counterpart of `_ZP_IT_FIND`.                                                                                                                                                                  |
| `_ZP_REMOVE(name, ptr, predicate)`                    | Remove every element matching `predicate` (`_` bound to the element), destroying it in place. Requires the container to provide `NAME_remove_at` (hash maps, hash sets, vectors and bit vectors).      |

> **Notes**
>
> * The `_FOREACH*` macros are loop headers — follow them with a statement or block,
>   exactly like a `for` loop.
> * The `_FIND*` and `_REMOVE` macros expand to statements (they internally start by
>   assigning `var_name = NULL`); place them where a statement is expected.
> * The predicate in `_ZP_REMOVE` must not have side effects that modify the collection.
> * Because bit vector provides only const version of `at`, only the const iteration helpers
>   (`_ZP_CONST_FOREACH`, `_ZP_CONST_FIND`, `_ZP_CONST_IT_FIND`) are directly compatible
>   with bit vector.

### Examples

**Iterate a vector:**

```c
int *elem = NULL;
int sum = 0;
_ZP_FOREACH (intvec, &v, elem) {
    sum += *elem;
}
```

**Find an element in a vector (`_` is `int *`):**

```c
const int *found = NULL;
_ZP_CONST_FIND(intvec, &v, found, *_ == 7);
if (found != NULL) { /* *found == 7 */ }
```

**Bounded search with explicit iterators:**

```c
intvec_iter_t it  = intvec_begin(&v);
intvec_iter_t end = intvec_end(&v);
_ZP_IT_FIND(intvec, &v, it, end, *_ == 6);
if (it != end) { /* *intvec_at(&v, it) == 6 */ }
```

**Iterate hash-map entries and values (`_` exposes `->key` / `->val`):**

```c
u32map_elem_t *node = NULL;
_ZP_FOREACH (u32map, &m, node) {
    printf("%u -> %u\n", node->key, node->val);
}

uint32_t *val = NULL;
_ZP_FOREACH_VAL (u32map, &m, val) {
    *val += 1;  // mutate every value in place
}
```

**Find by value, then erase matching entries:**

```c
const uint32_t *fv = NULL;
_ZP_CONST_FIND_VAL(u32map, &m, fv, *_ == 10);   // first entry whose val == 10

_ZP_REMOVE(u32map, &m, _->key % 2 != 0);        // drop all odd keys
```

**Erase matching elements from a vector (`_` is `int *`):**

```c
_ZP_REMOVE(intvec, &v, *_ < 0);                 // drop all negative values
```

---

## Quick reference: which container to use

* **`vector` (heap)** — ordered list, unbounded, random access (incl. positional
  `insert`/`remove`); needs `malloc`.
* **`static_vector`** — ordered list with a known maximum capacity (incl. positional
  `insert`/`remove`); no `malloc`.
* **`static_bit_vector`** — ordered list of single bits (0/1) with a known maximum capacity;
  packed storage with a configurable block type, no `malloc`.
* **`hashmap` (heap)** — key→value lookup, unbounded, stable iterators; needs `malloc`.
* **`hashset` (heap)** — unique-key set, unbounded, stable iterators; needs `malloc`.
* **`static_hashmap`** — key→value lookup with a known maximum capacity; no `malloc`.
* **`static_hashset`** — unique-key set with a known maximum capacity; no `malloc`.
* **`static_deque`** — bounded FIFO/LIFO with O(1) push/pop at both ends; no `malloc`.
* **`static_pqueue`** — bounded priority queue (binary heap); no `malloc`.
* **`variant`** — one value out of several distinct types; no `malloc`.
* **`algorithms`** — generic `foreach` / `find` / `remove` macros over vectors and
  hash maps.

See the corresponding tests under `tests/` (e.g. `z_vector_template_test.c`,
`z_static_bit_vector_template_test.c`, `z_hashmap_template_test.c`,
`z_hashset_template_test.c`, `z_static_hashmap_template_test.c`,
`z_static_hashset_template_test.c`, `z_static_deque_test.c`,
`z_static_pqueue_test.c`, `z_variant_template_test.c`) for
complete, compilable usage examples. The vector and hash-map tests also exercise the
`algorithms_template.h` macros.
