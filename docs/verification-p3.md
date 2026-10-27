# Build, verification, and submission

The project uses the Conversation and SentinelScanner implementation copied
from the supplied Project 2 repository:
`https://github.com/Danialkhan15/ece309-project2-starter`, commit
`fc26d7ce3cd86d2c8450af449ec6cd94c90f48a3`. The four required P2 files are now in
`include/core/` and `src/core/`. P3's provided `message.h` is retained.

## Required Linux check

Run these commands from the project directory on a department Linux machine,
WSL2, or a Linux VM:

```sh
cmake -S . -B build-linux -DUSE_P2_REFERENCE=OFF -DMH_WERROR=ON -DMH_SANITIZE=ON
cmake --build build-linux -j 4
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-linux --output-on-failure
./build-linux/test_p3
```

Linux verification has not been performed in this workspace: the available host
is macOS and no Linux container runtime was found.

## Completed local verification

On October 26, 2026, Apple Clang 17 built with `-Wall -Wextra -Wpedantic -Werror`,
AddressSanitizer, and UndefinedBehaviorSanitizer. All four CTest suites passed:

| Suite | Cases passed |
|---|---:|
| P2 regression | 14 |
| P3 implementation tests | 29 |
| Provided tool-loop tests | 9 |
| Golden CLI sessions | 7 |

The P3 suite includes all specification path and calculator examples, duplicate
ownership, collisions, traversal, hash diversity, 10,000 registered tools,
20,000 arithmetic terms, and 5,000 nested parentheses. The real longest chain
was 6; the weak-hash comparison was 9,000, with 16,384 buckets and load factor
0.6103515625.

This Mac's SDK declares `mkdtemp` in `<unistd.h>`. The local build includes that
header through a compiler flag, leaving the provided source files unchanged:

```sh
cmake -S . -B build -DUSE_P2_REFERENCE=OFF -DMH_WERROR=ON -DMH_SANITIZE=ON \
  '-DCMAKE_CXX_FLAGS=-include unistd.h'
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

macOS sanitizer results do not establish that Linux leak detection passes.

## Remaining submission details

1. Add your name and review/personalize `docs/design-log-p3.md`, including the AI
   assistance disclosure. Be ready to explain the implementation.
2. The private Project 3 repository is
   `https://github.com/Danialkhan15/ece309-project3`, also recorded in `github.txt`.
   Give the course staff access as required by the submission instructions.
3. Complete the Linux check above.
4. After any edits, recreate `github.zip` from the project source. Exclude build
   directories and the previous ZIP itself. The supplied ZIP contains the source,
   tests, reference implementation, scripts, workspace fixtures, and docs; it
   omits generated binaries and CMake caches.
