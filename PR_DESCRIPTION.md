# Add RT-Thread Platform Support

## Summary

This PR adds comprehensive support for the RT-Thread real-time operating system to zenoh-pico, enabling zenoh communication capabilities on RT-Thread-based embedded devices.

## Changes Overview

### 🎯 **Platform Abstraction Layer**
- **Platform Header**: `include/zenoh-pico/system/platform/rtthread.h`
  - Defines RT-Thread specific types for tasks, mutexes, condition variables, and network sockets
  - Compatible with RT-Thread's threading and synchronization primitives

- **System Implementation**: `src/system/rtthread/system.c`
  - Random number generation using `rt_tick_get()`
  - Memory management with RT-Thread's `rt_malloc/rt_realloc/rt_free`
  - Thread management using RT-Thread threads and events
  - Mutex and recursive mutex support
  - Condition variables implemented with RT-Thread semaphores and mutexes
  - Sleep and timing functions

- **Network Implementation**: `src/system/rtthread/network.c`
  - TCP socket operations (client/server, non-blocking I/O)
  - UDP unicast and multicast support
  - IPv4/IPv6 compatibility
  - Standard POSIX socket API integration with lwIP

### ⚙️ **Configuration System**
- **RT-Thread Config**: `include/zenoh-pico/config_rtthread.h`
  - Platform-specific feature flags and buffer sizes
  - Optimized defaults for embedded environments
  - Configurable thread priorities and stack sizes

- **Build Integration**: 
  - `SConscript` - SCons build script for RT-Thread projects
  - `Kconfig` - Configuration options for RT-Thread menuconfig
  - `package.json` - RT-Thread package descriptor

- **Platform Detection**: Updated `include/zenoh-pico/config.h` and `include/zenoh-pico/system/platform.h`

### 📚 **Examples and Documentation**
- **Example Programs**:
  - `examples/rtthread/z_pub.c` - Publisher example
  - `examples/rtthread/z_sub.c` - Subscriber example  
  - `examples/rtthread/z_get.c` - GET query example
  - `examples/rtthread/z_put.c` - PUT example
  - `examples/rtthread/z_test.c` - Basic functionality test

- **Documentation**:
  - `README_RTTHREAD.md` - Comprehensive usage guide
  - Integration instructions and troubleshooting

## Features Implemented

### ✅ **Core Functionality**
- Multi-threading support with RT-Thread primitives
- Memory management using RT-Thread allocators
- Network communication (TCP/UDP unicast/multicast)
- Time management and sleep functions
- Synchronization primitives (mutexes, condition variables)
- Random number generation

### ✅ **RT-Thread Integration**
- MSH command support for examples
- Kconfig-based configuration
- SCons build system integration
- Standard RT-Thread package format

### ✅ **Network Protocols**
- TCP transport with connection management
- UDP unicast for point-to-point communication
- UDP multicast for group communication
- IPv4 and IPv6 support
- Socket timeout and non-blocking I/O

## Testing

### **Basic Functionality Tests**
- ✅ Random number generation
- ✅ Memory allocation/deallocation
- ✅ Mutex operations (lock/unlock/try_lock)
- ✅ Thread creation and synchronization
- ✅ Clock and timing functions

### **Network Tests**
- ✅ TCP connection establishment
- ✅ UDP socket operations
- ✅ Multicast group management
- ✅ Socket option configuration

### **Zenoh Protocol Tests**
- ✅ Session establishment
- ✅ Publisher/Subscriber messaging
- ✅ Query/Reply interactions
- ✅ Configuration management

## Compatibility

### **RT-Thread Versions**
- RT-Thread 4.0+
- Standard RT-Thread API compatibility
- Multiple hardware platform support

### **Network Stack**
- lwIP 2.0+
- Standard POSIX socket API
- IPv4/IPv6 dual stack

### **Memory Requirements**
- Basic configuration: ~16KB RAM
- Full configuration: ~32KB RAM
- Configurable feature set for memory optimization

## Performance Characteristics

- **Memory Efficient**: Optimized for resource-constrained devices
- **Low Latency**: Event-driven architecture with minimal overhead
- **Scalable**: Configurable buffer sizes and feature set
- **Real-time**: Compatible with RT-Thread's real-time guarantees

## Usage Example

```c
#include "zenoh-pico.h"

// Publisher example
z_owned_config_t config;
z_config_default(&config);

z_owned_session_t session;
z_open(&session, z_move(config), NULL);

z_owned_publisher_t pub;
z_view_keyexpr_t ke;
z_view_keyexpr_from_str_unchecked(&ke, "demo/example");
z_declare_publisher(&pub, z_loan(session), z_loan(ke), NULL);

// Publish data
z_owned_bytes_t payload;
z_bytes_from_str(&payload, "Hello from RT-Thread!");
z_publisher_put(z_loan(pub), z_move(payload), NULL);
```

## Integration Instructions

1. **Add to RT-Thread Project**:
   ```bash
   # Copy to RT-Thread packages directory
   cp -r zenoh-pico /path/to/rtthread/packages/iot/
   ```

2. **Configure Features**:
   ```bash
   scons --menuconfig
   # Navigate to: RT-Thread online packages -> IoT -> zenoh-pico
   ```

3. **Build and Run**:
   ```bash
   scons
   # In RT-Thread MSH: zenoh_test, zenoh_pub, zenoh_sub
   ```

## Breaking Changes

None. This is a new platform addition that doesn't affect existing platforms.

## Checklist

- [x] Platform abstraction layer implemented
- [x] Network layer implemented  
- [x] Configuration system integrated
- [x] Example programs created
- [x] Documentation written
- [x] Basic functionality tested
- [x] Network functionality tested
- [x] Memory usage optimized
- [x] RT-Thread coding standards followed
- [x] No breaking changes to existing code

## Related Issues

This PR addresses the need for RT-Thread support in zenoh-pico, enabling zenoh communication on one of the most popular Chinese RTOS platforms used in IoT and embedded applications.

## Future Enhancements

- Serial transport implementation
- Advanced security features
- Performance optimizations for specific RT-Thread hardware platforms
- Integration with RT-Thread's device driver framework