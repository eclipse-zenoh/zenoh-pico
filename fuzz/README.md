# Fuzzing

This folder contains `libFuzzer` targets for `zenoh-pico`.
The following targets will be tested:

- `z_fuzz_scouting_message_decode`: feeds arbitrary bytes into `_z_scouting_message_decode()`

## Requirements

- `clang`
- CMake

`BUILD_FUZZERS=ON` currently requires `clang` because the fuzz targets use
LLVM `libFuzzer`.

## Configure

Create a dedicated build directory for fuzzing:

```bash
CC=clang cmake -B build-fuzz \
  -DBUILD_FUZZERS=ON \
  -DBUILD_TESTING=OFF \
  -DBUILD_EXAMPLES=OFF
```

This keeps the normal build separate from the fuzzing build.

## Build

Build the first fuzz target:

```bash
cmake --build build-fuzz --target z_fuzz_scouting_message_decode
```

The executable will be generated at:

```bash
build-fuzz/fuzz/z_fuzz_scouting_message_decode
```

## Run continuous fuzzing

You can let `libFuzzer` keep mutating inputs:

```bash
mkdir -p fuzz/corpus/scouting_message_decode
./build-fuzz/fuzz/z_fuzz_scouting_message_decode \
  -timeout=5 \
  fuzz/corpus/scouting_message_decode
```

## Useful options

Run for a fixed number of executions:

```bash
./build-fuzz/fuzz/z_fuzz_scouting_message_decode \
  -timeout=5 \
  -runs=10000 \
  fuzz/corpus/scouting_message_decode
```

Limit generated input size:

```bash
./build-fuzz/fuzz/z_fuzz_scouting_message_decode \
  -timeout=5 \
  -max_len=256 \
  fuzz/corpus/scouting_message_decode
```

Replay a saved crashing input:

```bash
./build-fuzz/fuzz/z_fuzz_scouting_message_decode path/to/crash-input
```

## Clean up

Remove the fuzzing build directory when you want a clean rebuild:

```bash
rm -rf build-fuzz
```
