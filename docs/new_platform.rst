..
.. Copyright (c) 2025 ZettaScale Technology
..
.. This program and the accompanying materials are made available under the
.. terms of the Eclipse Public License 2.0 which is available at
.. http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
.. which is available at https://www.apache.org/licenses/LICENSE-2.0.
..
.. SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
..
.. Contributors:
..   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
..

********
Add a new platform
********

Zenoh-Pico's platform layer allow us to abstract the differences between operating systems or HALs when it comes to handling timers, memory allocations and network operations.  
Adding a new platform then is a matter of correctly registering its specific abstraction in our system. Although each platform is unique, we strongly suggest not starting from scratch and basing from another platform implementation, e.g: RPI_PICO.

Implementation include
====================

The platform include files are situated at `include/zenoh-pico/system/platform/`, if you require multiple include files for your platform, we suggest putting them in a sub-directory. 

The include files contain the platform-dependent definitions of our wrapper types:

* z_clock_t: A time type geared towards instants.
* z_time_t: A time type geared towards days.
* _z_sys_net_socket_t: A network type to store socket information.
* _z_sys_net_endpoint_t: A network type to store endpoint information (e.g: ip address, port).

Additionally, if your platform supports multi thread you'll have to define:

* _z_task_t: A multi-thread type to store thread/task information.
* z_task_attr_t: A multi-thread type to store thread/task attributes information.
* _z_mutex_t:  A multi-thread type to store mutex information.
* _z_mutex_rec_t: A multi-thread type to store recursive mutex information.
* _z_condvar_t: (optional) A multi-thread type to store conditional variable information.

Implementation source
====================

The first thing is to create a folder for your platform in `src/system`. Then we suggest to create at least 2 files, one for systems functions and one for network but you can subdivide even more.

Here a are the system functions you must implement:

Random
-----------

* `z_random_u8(void)`: Generates a random unsigned 8bits integer.
* `z_random_u16(void)`: Generates a random unsigned 16bits integer.
* `z_random_u32(void)`: Generates a random unsigned 32bits integer.
* `z_random_u64(void)`: Generates a random unsigned 64bits integer.
* `z_random_fill(void *buf, size_t len)`: Fills buffer with random data.

Memory
-----------

* `z_malloc(size_t size)`: Allocates memory of the specified size.
* `z_realloc(void *ptr, size_t size)`: Reallocates the given memory block to a new size.
* `z_free(void *ptr)`: Frees the memory previously allocated by z_malloc or z_realloc.


Tasks
-----------

* `_z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg)`:  Initializes a new task.
* `_z_task_join(_z_task_t *task)`: Joins the task and releases all allocated resources.
* `_z_task_detach(_z_task_t *task)`: Detaches the task and releases all allocated resources.
* `_z_task_cancel(_z_task_t *task)`: Drops the task. Same as :c:func:`z_task_detach`. Use :c:func:`z_task_join` to wait for the task completion.
* `_z_task_exit(void)`: Terminate the calling task.
* `_z_task_free(_z_task_t **task)`: Free the task container.

Mutex
-----------

* `_z_mutex_init(_z_mutex_t *m)`: Initializes a mutex.
* `_z_mutex_drop(_z_mutex_t *m)`: Destroys a mutex and releases its resources.
* `_z_mutex_lock(_z_mutex_t *m)`: Locks a mutex. If the mutex is already locked, blocks the thread until it acquires the lock.
* `_z_mutex_try_lock(_z_mutex_t *m)`: Tries to lock a mutex. If the mutex is already locked, the function returns immediately.
* `_z_mutex_unlock(_z_mutex_t *m)`: Unlocks a previously locked mutex. If the mutex was not locked by the current thread, the behavior is undefined.
* `_z_mutex_rec_init(_z_mutex_rec_t *m)`: Initializes a recursive mutex.
* `_z_mutex_rec_drop(_z_mutex_rec_t *m)`: Destroys a recursive mutex and releases its resources.
* `_z_mutex_rec_lock(_z_mutex_rec_t *m)`: Locks a recursive mutex. If the mutex is already locked by another thread, blocks the thread until it acquires the lock.
* `_z_mutex_rec_try_lock(_z_mutex_rec_t *m)`: Tries to lock a recursive mutex. If the mutex is already locked, the function returns immediately.
* `_z_mutex_rec_unlock(_z_mutex_rec_t *m)`: Unlocks a previously locked recursive mutex. If the mutex was not locked by the current thread, the behavior is undefined.

Condvar
-----------

* `z_condvar_init(_z_condvar_t *cv)`: Initializes a condition variable.
* `z_condvar_drop(_z_condvar_t *cv)`: Destroys a condition variable and releases its resources.
* `z_condvar_signal(_z_condvar_t *cv)`: Signals (wakes up) one thread waiting on the condition variable.
* `z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m)`: Waits for a signal on the condition variable while holding a mutex.
* `z_condvar_wait_until(_z_condvar_t *cv, _z_mutex_t *m, const _z_clock_t *abstime)`: Waits for a signal on the condition variable while holding a mutex until a specified time.

Sleep
-----------

* `z_sleep_us(size_t time)`: Suspends execution for a specified amount of time in microseconds.
* `z_sleep_ms(size_t time)`: Suspends execution for a specified amount of time in milliseconds. 
* `z_sleep_s(size_t time)`: Suspends execution for a specified amount of time in seconds.

Clock
-----------

* `z_clock_now(void)`: Returns monotonic clock time point corresponding to the current time instant.
* `z_clock_elapsed_us(z_clock_t *time)`: Returns the elapsed time in microseconds since a given clock time.
* `z_clock_elapsed_ms(z_clock_t *time)`: Returns the elapsed time in milliseconds since a given clock time.
* `z_clock_elapsed_s(z_clock_t *time)`: Returns the elapsed time in seconds since a given clock time.
* `z_clock_advance_us(z_clock_t *clock, unsigned long duration)`: Offsets the clock by a specified duration in microseconds.
* `z_clock_advance_ms(z_clock_t *clock, unsigned long duration)`: Offsets the clock by a specified duration in milliseconds.
* `z_clock_advance_s(z_clock_t *clock, unsigned long duration)`: Offsets the clock by a specified duration in seconds.

Time
-----------

* `z_time_now(void)`: Returns system clock time point corresponding to the current time instant.
* `z_time_now_as_str(char *const buf, unsigned long buflen)`: Gets the current time as a string.
* `z_time_elapsed_us(z_time_t *time)`: Returns the elapsed time in microseconds since a given time.
* `z_time_elapsed_ms(z_time_t *time)`: Returns the elapsed time in milliseconds since a given time.
* `z_time_elapsed_s(z_time_t *time)`: Returns the elapsed time in seconds since a given time.
* `_z_get_time_since_epoch(_z_time_since_epoch *t)`: Return the elapsed time since the epoch.

Network
-----------

Here are the functions needed for peer-to-peer unicast:

* `_z_socket_set_non_blocking(const _z_sys_net_socket_t *sock)`: Convert a regular socket to a non-blocking one.
* `_z_socket_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out)`: Accept an incoming connection to a listen socket.
* `_z_socket_wait_event(void *v_peers, _z_mutex_rec_t *mutex)`: Use io multiplexing to wait an event on a list of sockets.

Then, you must decide which links you want to add support for on your platform, with the possible list being:

* TCP unicast
* UDP unicast
* UDP multicast
* Serial
* Bluetooth
* WebSocket
* Raw Ethernet

For each of these links you'll have to implement the functions that are defined in `include/zenoh-pico/system/link/`, for example for TCP:

* `_z_create_endpoint_tcp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port)`: Create the network endpoint structure necessary to open a TCP connection.
* `_z_free_endpoint_tcp(_z_sys_net_endpoint_t *ep)`: Free the network endpoint.
* `_z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout)`: Open a TCP connection based on endpoint information.
* `_z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep)`: Listen on a TCP socket based on endpoint information (requires peer-to-peer unicast functions).
* `_z_close_tcp(_z_sys_net_socket_t *sock)`: Close a TCP connection/socket.
* `_z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len)`: Read available bytes on a TCP socket.
* `_z_read_exact_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len)`: Read an exact amount of bytes on a TCP socket
* `_z_send_tcp(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len)`: Send bytes on a TCP socket.

Build system registration
====================

Once the implementation is done, you need to register it in our build system. For that you'll need to define a platform specific symbol in our main `CMakeLists.txt`.
For example, for the RPI_PICO platform the symbol is `ZENOH_RPI_PICO`. You can then use this symbol to determine which implementation to include in `include/zenoh-pico/system/common/platform.h`.

If you need to introduce foreign sources to build for your platform, you might have to add more platform specifics in the main `CMakeLists.txt`.

Examples
====================

The final step, once your implementation is done and registered is to create platform specific examples in our `examples/` folder. Ideally the 4 most important examples to include are:

* z_pub
* z_sub
* z_get
* z_queryable

You can of course base off other platform examples to facilitate the work. 

Once this is done you simply need to build the examples and test them on your platform to see if they work correctly.

