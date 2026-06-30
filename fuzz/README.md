# Fuzzing

This folder contains `libFuzzer` targets for `zenoh-pico`.
The following targets will be tested:

- `z_fuzz_endpoint_str`: feeds arbitrary bytes into `_z_endpoint_from_string()`
  and, on successful parses, round-trips them through `_z_endpoint_to_string()`
- `z_fuzz_scouting_message_decode`: feeds arbitrary bytes into `_z_scouting_message_decode()`
- `z_fuzz_transport_message_decode`: feeds arbitrary bytes into `_z_transport_message_decode()`

## Requirements

- `clang`
- CMake
- `llvm-profdata`
- `llvm-cov`

`BUILD_FUZZERS=ON` currently requires `clang` because the fuzz targets use
LLVM `libFuzzer`.

## Configure

Create a dedicated build directory for fuzzing:

```bash
CC=clang cmake -B build-fuzz \
  -DBUILD_FUZZERS=ON \
  -DBUILD_TESTING=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DCMAKE_C_FLAGS="-fprofile-instr-generate -fcoverage-mapping" \
  -DCMAKE_EXE_LINKER_FLAGS="-fprofile-instr-generate"
```

This keeps the normal build separate from the fuzzing build while enabling
LLVM coverage data collection.

## Build

Build the fuzz targets:

```bash
cmake --build build-fuzz --target \
  z_fuzz_scouting_message_decode \
  z_fuzz_transport_message_decode \
  z_fuzz_endpoint_str
```

The executables will be generated at:

```bash
build-fuzz/fuzz/z_fuzz_scouting_message_decode
build-fuzz/fuzz/z_fuzz_transport_message_decode
build-fuzz/fuzz/z_fuzz_endpoint_str
```

## Seed policy

Committed inputs under `fuzz/seeds/` are a small, curated seed corpus.
Those seeds come from the Zenoh project.
They are intended as stable starting points for mutation.

When running the fuzz tests, we should copy them to `fuzz/corpus` for the mutation later.

```bash
mkdir -p fuzz/corpus/scouting_message_decode
mkdir -p fuzz/corpus/transport_message_decode
mkdir -p fuzz/corpus/endpoint_str
cp -r fuzz/seeds/scouting_message/. fuzz/corpus/scouting_message_decode/
cp -r fuzz/seeds/transport_message/. fuzz/corpus/transport_message_decode/
cp -r fuzz/seeds/endpoint_from_str/. fuzz/corpus/endpoint_str/
```

## Usage

- Run continuous fuzzing

You can let `libFuzzer` keep mutating inputs:

```bash
./build-fuzz/fuzz/z_fuzz_scouting_message_decode \
  -timeout=5 \
  fuzz/corpus/scouting_message_decode
./build-fuzz/fuzz/z_fuzz_transport_message_decode \
  -timeout=5 \
  fuzz/corpus/transport_message_decode
./build-fuzz/fuzz/z_fuzz_endpoint_str \
  -timeout=5 \
  fuzz/corpus/endpoint_str
```

- Replay a saved crashing input:

```bash
./build-fuzz/fuzz/z_fuzz_scouting_message_decode path/to/crash-input
./build-fuzz/fuzz/z_fuzz_transport_message_decode path/to/crash-input
./build-fuzz/fuzz/z_fuzz_endpoint_str path/to/crash-input
```

## Coverage report

Run each fuzz target with its own profile output file:

```bash
LLVM_PROFILE_FILE=scouting_message_decode.profraw \
./build-fuzz/fuzz/z_fuzz_scouting_message_decode \
  -timeout=5 \
  -runs=10000 \
  fuzz/corpus/scouting_message_decode

LLVM_PROFILE_FILE=transport_message_decode.profraw \
./build-fuzz/fuzz/z_fuzz_transport_message_decode \
  -timeout=5 \
  -runs=10000 \
  fuzz/corpus/transport_message_decode

LLVM_PROFILE_FILE=endpoint_str.profraw \
./build-fuzz/fuzz/z_fuzz_endpoint_str \
  -timeout=5 \
  -runs=10000 \
  fuzz/corpus/endpoint_str
```

Merge the raw profile data for each target:

```bash
llvm-profdata merge -sparse scouting_message_decode.profraw \
  -o scouting_message_decode.profdata
llvm-profdata merge -sparse transport_message_decode.profraw \
  -o transport_message_decode.profdata
llvm-profdata merge -sparse endpoint_str.profraw \
  -o endpoint_str.profdata
```

Generate a filtered summary report for the important files of each target:

`llvm-cov` needs both the fuzz target executable and `libzenohpico.so`.
This build links the harness against the shared library, so passing only the
executable mostly shows coverage from the harness itself and inline functions
defined in headers.
Adding `-object=./build-fuzz/lib/libzenohpico.so` lets `llvm-cov` report
coverage for the library implementation files under `src/`.
We then use `grep -E` to keep the report focused on the main parser and codec
files exercised by each fuzz target.

```bash
llvm-cov report ./build-fuzz/fuzz/z_fuzz_scouting_message_decode \
  -object=./build-fuzz/lib/libzenohpico.so \
  -instr-profile=scouting_message_decode.profdata
llvm-cov report ./build-fuzz/fuzz/z_fuzz_transport_message_decode \
  -object=./build-fuzz/lib/libzenohpico.so \
  -instr-profile=transport_message_decode.profdata
llvm-cov report ./build-fuzz/fuzz/z_fuzz_endpoint_str \
  -object=./build-fuzz/lib/libzenohpico.so \
  -instr-profile=endpoint_str.profdata
```

Inspect annotated source coverage for each target:

```bash
llvm-cov show ./build-fuzz/fuzz/z_fuzz_scouting_message_decode \
  -object=./build-fuzz/lib/libzenohpico.so \
  -instr-profile=scouting_message_decode.profdata \
  src/protocol/codec/message.c

llvm-cov show ./build-fuzz/fuzz/z_fuzz_transport_message_decode \
  -object=./build-fuzz/lib/libzenohpico.so \
  -instr-profile=transport_message_decode.profdata \
  src/protocol/codec/transport.c

llvm-cov show ./build-fuzz/fuzz/z_fuzz_endpoint_str \
  -object=./build-fuzz/lib/libzenohpico.so \
  -instr-profile=endpoint_str.profdata \
  src/link/endpoint.c
```

## Clean up

Remove the fuzzing build directory when you want a clean rebuild:

```bash
rm -rf build-fuzz
```
