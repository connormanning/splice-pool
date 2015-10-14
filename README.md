# splice-pool

### What is it?
A C++11 memory pool designed for multi-threaded applications.  Contention is
minimized by allowing multiple threads to perform useful work for
allocation/deallocation locally to each thread, and then using light-weight
"commit" steps to manage the memory within the pool.

Memory is allocated in chunks when the pool is empty, and singly-linked stacks
are used to keep track of available objects.  This way we can acquire or
release many objects at once, simply by twiddling a linked list pointer.

### When would I use it?
This pool would be a good candidate if you have multiple threads that need to
acquire many objects that may later be shared across threads, and your
application allows multiple objects to be released at once (possibly from
different threads than they were acquired).

### Any other cool features?
For managing pool-owned objects, unique-pointer-ish RAII semantics have been
implemented to avoid leaking objects from the pool (see samples in the unit
tests).

Perfect forwarding can be used to construct pooled objects, and they will be
destructed on release back into the pool.

In addition to an object pool, a buffer pool is also provided with the same
semantics.  If you need many raw buffers of a fixed and known-at-runtime size,
you can use the buffer pool to avoid making many tiny vectors.

### Usage
The project is header only, but uses CMake for unit testing.  Here's how to run
the tests on UNIX:

```
mkdir build && cd build
cmake -G "Unix Makefiles" ..
make test
```

Simply include `splice-pool.hpp` and make sure C++11 is enabled (`-std=c++11`).
All `ObjectPool`/`BufferPool` operations are thread-safe.  `Stack` types are
not thread-safe, and should be managed on a per-thread basis.

For now, see the unit tests for code samples.

### License
This software is licensed under the permissive
[MIT](http://opensource.org/licenses/MIT) license.

