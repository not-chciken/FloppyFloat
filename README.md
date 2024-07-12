# FloppyFloat

A faster than soft float floating point library.

## What can it be used for?
FloppyFloat is primarly designed as a faster alternative to soft float libraries in simulator environments.
Soft float libraries, such as [Berkeley SoftFloat](https://github.com/ucb-bar/berkeley-softfloat-3) or [SoftFP](https://bellard.org/softfp/),
compute floating point instruction purely by integer arithmetic, which is a slow and painful process.
As most computers come with an FPU, computing results and exception flags by floating point arithmetic is way faster.
This is why FloppyFloat uses the host's FPU most of the time, and only resorts to software rectifications in some corner cases.

FloppyFloat only relies on correct IEEE 754 floating point results in round to nearest mode.
It's not relying on floating point exceptions flags, particular NaN values, rounding modes, and so forth.
Hence, you can even use it on systems like the [XuanTie C910](https://www.riscfive.com/2023/03/09/t-head-xuantie-c910-risc-v/),
which break IEEE 754 compliance by not setting floating point exception flags.

## Build
FloppyFloat follows a simple CMake build process:
``
mkdir build
cd build
cmake ..
cmake --build . --target floppy_float
``
There are no third-party dependencies.
Your compiler needs to support at least C++23.

## Usage
Despite using the host's FPU, FloppyFloat can be configured to a certain extent.
For example, canonical qNaNs can bet set:
```
SetQnan32() // TODO:
SetQnan64() // TODO:
```

## How does it work?
Coding, algorithms, and a bit of math.
For a detailed explanation see [this blog post](https://www.chciken.com/simulation/2023/11/12/fast-floating-point-simulation.html).
