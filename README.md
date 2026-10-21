# logscan: C++ optimization exercise

A small C++17 log-loading and searching utility. The baseline is intentionally
inefficient; optimize it without changing its observable results.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Or compile directly:

```sh
c++ -O2 -std=c++17 -Iinclude src/main.cpp src/logscan.cpp -o logscan
```

## Generate deterministic input

```sh
./build/logscan --generate logs.txt 1000000
```

Then run a search:

```sh
./build/logscan logs.txt ERROR timeout
./build/logscan logs.txt ALL request
./build/logscan logs.txt WARNING ""
```

The program reports loading and searching times separately. It also reports
counts and byte totals; these are correctness checks and must not change after
optimization.

## Your task

1. Build a Release version and generate 100,000 records, then 1,000,000.
2. Run the same searches several times. Record baseline timings.
3. Profile before editing. On Linux, try `perf stat` or `perf record`; on macOS,
   try Instruments or `sample`. You can also use Callgrind where available.
4. Identify one bottleneck and make a small change.
5. Re-run identical workloads and compare the result counts and timings.
6. Explain why the optimization helps and where it might not help.

## Constraints

- Preserve output counts and case-insensitive matching behavior.
- Preserve support for severity `ALL` and an empty keyword.
- Do not change the input format or simply remove work from the program.
- Measure loading and searching separately.
- Make one optimization at a time and keep a record of each result.

## Hints (not solutions)

- Look for work repeated for every record.
- Check whether every intermediate string needs its own allocation.
- Consider whether all fields need to be copied during parsing.
- Compare CPU time, elapsed time, and memory use rather than relying on one number.
