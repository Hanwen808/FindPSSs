# C++ implementation

This is a C++ implementation for PSS-Sketch and related solutions.

## Project Structure

```
CPU/
├── main2016.cpp
├── main2019.cpp
├── main2024.cpp
├── main2025.cpp
├── params.cpp
├── threshold_card.cpp
├── threshold_k.cpp
├── Headers/
│   ├── MurmurHash3.h
│   ├── BloomFilter.h
│   ├── FreeRS.h
│   ├── HLLsampler.h
│   ├── KTSketch.h
│   ├── PSS_BM.h
│   ├── PSS_HLL.h
│   ├── Sketch.h
│   ├── RGS.h
│   ├── SpreadSketch.h
│   ├── StreamSummary.h
│   └── bitmap.h
├── Sources/
│   ├── FreeRS.cpp
│   ├── HLLsampler.cpp
│   ├── MurmurHash3.cpp
│   ├── KTSketch.cpp
│   ├── PSS_BM.cpp
│   ├── PSS_HLL.cpp
│   ├── RGS.cpp
│   ├── SpreadSketch.cpp
│   └── bitmap.cpp
```

## Usage

### Compilation
Before compiling, make sure you are in the directory /CPU/. And create the directories: ./records_CAIDA16/, ./records_CAIDA19/, ./records_MAWI24/, ./records_MAWI25/ to save the running results of different algorithms under four real-world datasets.
```bash
$ mkdir ./records_CAIDA16/
$ mkdir ./records_CAIDA19/
$ mkdir ./records_MAWI24/
$ mkdir ./records_MAWI25/
```

If you want to compile PSS-Sketch and other algorithms using CMake, the following commands should be executed.
```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
$ cd ..
```

And then, please running the scripts provided in the directory ./scripts/. The following commands should be executed.
```shell
$ chmod u+x ./scripts/params.sh
$ ./scripts/params.sh  # for testing the parameter tunning experiments
$ chmod u+x ./scripts/run_caida16.sh
$ ./scripts/run_caida16.sh   # for testing the accuracy experiments under CAIDA-16
$ chmod u+x ./scripts/run_caida19.sh
$ ./scripts/run_caida19.sh   # for testing the accuracy experiments under CAIDA-19
$ chmod u+x ./scripts/run_mawi24.sh
$ ./scripts/run_mawi24.sh    # for testing the accuracy experiments under MAWI-24
$ chmod u+x ./scripts/run_mawi25.sh
$ ./scripts/run_mawi25.sh    # for testing the accuracy experiments under MAWI-25
$ chmod u+x ./scripts/run_threshold_card.sh
$ ./scripts/run_threshold_card.sh # for testing the experiments under different cardinality thresholds
$ chmod u+x ./scripts/run_threshold_k.sh
$ ./scripts/run_threshold_k.sh # for testing the experiments under different persistence thresholds
```

## Requirements

- CMake 3.10 or above
- g++ 7.5.0 or above
- Compiler with support for C++17 standard
