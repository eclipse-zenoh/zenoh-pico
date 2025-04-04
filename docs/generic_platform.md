# How to add a new platform to zenoh-pico 
- objective is to use the zenoh-pico tree as such and provide the platform specific implementation
- provide all config.h setting specifically for your platform
- tested for TI Tiva platform lm4f120h5qr libopencm3 single thread and stm32cube

## Flags for zenoh-pico compilation
- pass define ZENOH_GENERIC via compile flags
- the below example uses the source tree zenoh-pico from github directly
```ini
[env:tiva]
platform = titiva
board = lplm4f120h5qr
framework = libopencm3
extra_scripts =
    pre:set_library_vars.py  # Sets env variables

platform_packages =
  	toolchain-gccarmnoneeabi@~1.90201.0
lib_deps = 
	https://github.com/eclipse/zenoh-pico
	lib/titiva_libopencm3
	https://github.com/mpaland/printf

build_flags = 
	-D__GLIBCXX_ASSERTIONS
	-DZENOH_GENERIC=1
	-DZENOH_DEBUG=0
	-std=gnu++17
	-Wno-missing-braces
	-Wno-missing-field-initializers
	-Wno-missing-prototypes
	-Ilib/titiva_libopencm3
	-Ilib/zenoh-pico/zenoh-pico
	-Iprintf

build_unflags = 
	-std=gnu++14
	-Wredundant-decls
```

- pass environment variable ZENOH_GENERIC=1 via script
```python
Import("env")
env.Append(ZENOH_GENERIC="1")
```

## Create platform specific files in your own library 
- zenoh_generic_config.h
- zenoh_generic_platform.h
- network.c(pp)
- system.c(pp)

- these files should implement the API's needed for the specific transport and platform


