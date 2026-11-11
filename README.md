# CS598APE HW3

This repository contains code for homework 3 of CS598APE. It is relatively
simple in comparison to HW1 and HW2 to ensure you have enough time to work on
the course project. 

`sandpile.c` contains the implementation of finding the identity element of a 2D
abelian sandpile.


## Build with Docker

Run the following command. Make sure you have Docker installed on your machine.

It builds the required compiler toolchain into a Docker image.

```bash
docker build -t "598ape-shengyi7-mp3" -f docker/Dockerfile .
```

## Run with Docker

```bash
docker run -it -v ".:/workspace" -t 598ape-shengyi7-mp3 bash
```

After this, the project root will be in `/workspace`.

## Checkout and Compile Programs

We develop our code on the `main` branch. However, it contains all of the CI code and Makefile changes in the middle of the commit history. For better testing, we reorder the commits and create a new branch called `final`, so that all miscellaneous changes are at the bottom of the tree.

First, checkout the `final` branch.

```bash
git checkout final
```

If Git is unhappy about modifying the Git repository inside the container, run the following command:

```bash
git config --global --add safe.directory /workspace
```

## Test a Specific Optimization

To test the speedup of a specific optimization, we first test the commit where the optimization is introduced and then test `HEAD^`.

The commit hash can be found using `git log`.

The possible hashes for each optimization in corresponding with the paper:

1.1: f0b9741
1.2: 5369256
1.3: 8898ee8
1.4: 5ba98b7
2.1: 070798d
2.2-1: faa3bcd
2.2-2: 22a3b16

The testing procedure is:

```bash
git checkout <HASH>
make clean
make
# Run your tests.

git checkout HEAD^
make clean
make
# Run your tests.
```

Here is the possible test test. We 

```bash
./sandpile.exe 1001
```

Alternatively, you can run `./scripts/performance-test.sh` directly.

This script runs the previous test. Each scene is run three times, and the average value is reported.
