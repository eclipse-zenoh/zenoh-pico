# Link abstraction documentation
The core of zenoh-pico is abstracted from the underlying transport link,
handling them in a uniform and generic way.
For example, zenoh-pico, currently, see transport link in terms of which
properties they support such as:
 - if it implements a streamed protocol or not;
 - if it implements a reliable link;

Other properties are planned in the future, such as if the transport link can
support multicast communications or not.

## How a transport link is selected by zenoh-pico?
zenoh-pico implements a link manager that handles the creation and removal
of transport links based on the provided locator.

## How to implement the support to a new transport link?
New transport links must implement the following structure:
```
typedef struct {
    _zn_socket_t sock;

    uint8_t is_reliable;
    uint8_t is_streamed;

    void* endpoint;
    uint16_t mtu;

    // Function pointers
    _zn_f_link_open open_f;
    _zn_f_link_close close_f;
    _zn_f_link_release release_f;
    _zn_f_link_write write_f;
    _zn_f_link_write_all write_all_f;
    _zn_f_link_read read_f;
    _zn_f_link_read_exact read_exact_f;
} _zn_link_t;
```

It includes a set of function pointers that implement the high-level behavior
of the transport link. All these must be implemented:
```
typedef _zn_socket_result_t (*_zn_f_link_open)(void *arg, clock_t tout);
typedef int (*_zn_f_link_close)(void *arg);
typedef void (*_zn_f_link_release)(void *arg);
typedef size_t (*_zn_f_link_write)(void *arg, const uint8_t *ptr, size_t len);
typedef size_t (*_zn_f_link_write_all)(void *arg, const uint8_t *ptr, size_t len);
typedef size_t (*_zn_f_link_read)(void *arg, uint8_t *ptr, size_t len);
typedef size_t (*_zn_f_link_read_exact)(void *arg, uint8_t *ptr, size_t len);
```

(see ```udp.c``` and ```tcp.c``` as examples).

Note that, platform specific code must be implemented under the ```system```
abstraction already implemented in zenoh-pico.

Finally, the corresponding entry must be added in the link manager, namely
in the ```_zn_open_link``` function, in order to map the new link protocol
scheme.

