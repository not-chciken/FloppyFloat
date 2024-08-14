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

Currently, FloppyFloat supports x86_64, ARM64, and RISC-V, both as host and targets.
Opposed to many soft float libraries, FloppyFloat does not rely on global state/variables.
Hence, you can also use it to easily model heterogeneous systems.

## Features
At the moment, the following functions and corresponding rounding modes are implemented:

| Function           | RNE | RTZ | RDN | RUP | RMM |
|--------------------|-----|-----|-----|-----|-----|
| Add<f16,f32,f64>   | X   | X   | X   | X   | X   |
| Sub<f16,f32,f64>   | X   | X   | X   | X   | X   |
| Mul<f16,f32,f64>   | X   | X   | X   | X   | O   |
| Div<f16,f32,f64>   | X   | X   | X   | X   | O   |
| Sqrt<f16,f32,f64>  | X   | X   | X   | X   | O   |
| Fma<f16,f32,f64>   | X   | X   | X   | X   | O   |
| F32ToI32           | X   | X   | X   | X   | X   |
| F32ToI64           | X   | X   | X   | X   | X   |
| F32ToU32           | X   | X   | X   | X   | X   |
| F32ToU64           | X   | X   | X   | X   | X   |
| F32ToF64           | X   | X   | X   | X   | X   |
| F32ToF16           | X   | X   | X   | X   | O   |
| F64ToI32           | X   | X   | X   | X   | X   |
| F64ToI64           | X   | X   | X   | X   | X   |
| Eq<f16,f32,f64>    | X   | X   | X   | X   | X   |
| Lt<f16,f32,f64>    | X   | X   | X   | X   | X   |
| Le<f16,f32,f64>    | X   | X   | X   | X   | X   |

| Function           | RISC-V    | x86 SSE     | ARM64  |
|--------------------|-----------|-------------|--------|
| Add<f16>           | FADD.H    | -           | FADD   |
| Sub<f16>           | FSUB.H    | -           | FSUB   |
| Mul<f16>           | FMUL.H    | -           | FMUL   |
| Div<f16>           | FDIV.H    | -           | FDIV   |
| Sqrt<f16>          | FSQRT.H   | -           | FSQRT  |
| Fma<f16>           | FMADD.H   | -           | FMADD  |
| Add<f32>           | FADD.S    | ADDSS       | FADD   |
| Sub<f32>           | FSUB.S    | SUBSS       | FSUB   |
| Mul<f32>           | FMUL.S    | MULSS       | FMUL   |
| Div<f32>           | FDIV.S    | DIVSS       | FDIV   |
| Sqrt<f32>          | FSQRT.S   | SQRTSS      | FSQRT  |
| Fma<f32>           | FMADD.S   | VFMADDxxxSS | FMADD  |
| F32ToI32           | FCVT.W.S  | CVTSS2SI    | FCVTxS |
| F32ToI64           | FCVT.L.S  | CVTSS2SI    | FCVTxS |
| F32ToU32           | FCVT.WU.S | (1)         | FCVTxU |
| F32ToU64           | FCVT.LU.S | (1)         | FCVTxU |
| F32ToF16           | FCVT.H.S  | -           | FCVT   |
| F32ToF64           | FCVT.D.S  | CVTSS2SD    | FCVT   |
| Eq<f32>            | FEQ.S     | (2)         | (3)    |
| Lt<f32>            | FLT.S     | (2)         | (3)    |
| Le<f32>            | FLE.S     | (2)         | (3)    |
| MaximumNumber<f32> | FMAX.S    | MAXSS       | (4)    |
| MinimumNumber<f32> | FMIN.S    | MINSS       | (4)    |
| F64ToI32           | X         |             |        |
| F64ToI64           | X         |             |        |

(1): Compiled code fpr x86 SSE resorts to CVTSS2SI for F32ToUxx.
(2): x86 SSE uses UCOMISS to achieve a the same functionality.
(3): ARM64 uses FCMP and FCMPE.
(4): ARM64 provides FMAXNM/FMINNM and FMAX/FMIN

## Build
FloppyFloat follows a CMake build process:
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
Here's an example:
```c++
FloppyFloat ff;
ff.nan_propagation_scheme = FloppyFloat::NanPropX86sse;
ff.SetQnan<f32>(0xffc00000);

f32 result = ff.Mul<f32, FloppyFloat::kRoundTowardZero>(a, b);
```

## How does it work?
Coding, algorithms, and a bit of math.
For a detailed explanation see [this blog post](https://www.chciken.com/simulation/2023/11/12/fast-floating-point-simulation.html).

## Issues
- `Fma` and `Sqrt` for f16 inherits the incorrect rounding problem of glibc (calculates the result as f32 and then casts to f16).
- Overflow exception for `Add`, `Sub`, `Mul`, `Div`, `Sqrt`, `Fma` may not trigger in some cases
- Underflow exception for `Mul`, `Div`, `Sqrt`, `Fma` may not trigger in some cases