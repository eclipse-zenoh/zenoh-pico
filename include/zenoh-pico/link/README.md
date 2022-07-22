# Link abstraction documentation
The core of zenoh-pico is abstracted from the underlying transport link,
handling them in a uniform and generic way.
For example, zenoh-pico, currently, see transport link in terms of which
properties they support such as:
 - if it implements a streamed protocol or not;
 - if it implements a reliable link;
 - if it implements a unicast or multicast link;

Other properties are planned in the future, such as if the transport link can
support multicast communications or not.

## How a transport link is selected by zenoh-pico?
zenoh-pico implements a link manager that handles the creation and removal
of transport links based on the provided locator.

## How to implement the support to a new transport link?
New transport links must implement the following structure:
```
typedef struct {
    _zn_endpoint_t endpoint;

    union
    {
        _zn_tcp_socket_t tcp;
        _zn_udp_socket_t udp;
    } socket;

    _zn_f_link_open open_f;
    _zn_f_link_listen listen_f;
    _zn_f_link_close close_f;
    _zn_f_link_write write_f;
    _zn_f_link_write_all write_all_f;
    _zn_f_link_read read_f;
    _zn_f_link_read_exact read_exact_f;
    _zn_f_link_free free_f;

    uint16_t mtu;
    uint8_t is_reliable;
    uint8_t is_streamed;
    uint8_t is_multicast;
} _zn_link_t;
```

It includes a set of function pointers that implement the high-level behavior
of the transport link. All these must be implemented:
```
typedef _zn_socket_result_t (*_zn_f_link_open)(void *arg, unsigned long tout);
typedef _zn_socket_result_t (*_zn_f_link_listen)(void *arg, unsigned long tout);
typedef void (*_zn_f_link_close)(void *arg);
typedef size_t (*_zn_f_link_write)(const void *arg, const uint8_t *ptr, size_t len);
typedef size_t (*_zn_f_link_write_all)(const void *arg, const uint8_t *ptr, size_t len);
typedef size_t (*_zn_f_link_read)(const void *arg, uint8_t *ptr, size_t len, z_bytes_t *addr);
typedef size_t (*_zn_f_link_read_exact)(const void *arg, uint8_t *ptr, size_t len, z_bytes_t *addr);
typedef void (*_zn_f_link_free)(void *arg);
```

(see ```udp.c``` and ```tcp.c``` as examples).

Note that, platform specific code must be implemented under the ```system```
abstraction already implemented in zenoh-pico.

Finally, the corresponding entry must be added in the link manager, namely
in the ```_zn_open_link``` function, in order to map the new link protocol
scheme.
