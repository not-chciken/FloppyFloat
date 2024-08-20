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
The following tables shows FloppyFloat functions and their corresponding ISA instructions.


| FloatFunction      | RISC-V    | x86 SSE     | ARM64  |
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
| I32ToF16           | FCVT.H.W  | -           | SCVTF  |
| I32ToF32           | FCVT.S.W  | CVTSI2SS    | SCVTF  |
| I32ToF64           | FCVT.D.W  | CVTSI2SD    | SCVTF  |
| U32ToF16           | FCVT.H.WU | -           | UCVTF  |
| U32ToF32           | FCVT.S.WU | -           | UCVTF  |
| U32ToF64           | FCVT.D.WU | -           | UCVTF  |
| Eq<f32>            | FEQ.S     | (2)         | (3)    |
| Lt<f32>            | FLT.S     | (2)         | (3)    |
| Le<f32>            | FLE.S     | (2)         | (3)    |
| MaximumNumber<f32> | FMAX.S    |             | (4)    |
| MinimumNumber<f32> | FMIN.S    |             | (4)    |
| MaxX86<f32>        |           | MAXSS       |        |
| MinX86<f32>        |           | MINSS       |        |
| Add<f64>           | FADD.D    | ADDSD       | FADD   |
| Sub<f64>           | FSUB.D    | SUBSD       | FSUB   |
| Mul<f64>           | FMUL.D    | MULSD       | FMUL   |
| Div<f64>           | FDIV.D    | DIVSD       | FDIV   |
| Sqrt<f64>          | FSQRT.D   | SQRTSD      | FSQRT  |
| Fma<f64>           | FMADD.D   | VFMADDxxxSD | FMADD  |
| F64ToI32           | FCVT.W.D  | CVTSD2SI    | FCVTxS |
| F64ToI64           | FCVT.L.D  | CVTSD2SI    | FCVTxS |
| F64ToU32           | FCVT.WU.D | (5)         | FCVTxU |
| F64ToU64           | FCVT.LU.D | (5)         | FCVTxU |
| F64ToF16           | FCVT.H.D  | -           | FCVT   |
| F64ToF32           | FCVT.S.D  | CVTSS2SD    | FCVT   |
| Eq<f64>            | FEQ.D     | (2)         | (3)    |
| Lt<f64>            | FLT.D     | (2)         | (3)    |
| Le<f64>            | FLE.D     | (2)         | (3)    |
| MaximumNumber<f64> | FMAX.D    |             | (4)    |
| MinimumNumber<f64> | FMIN.D    |             | (4)    |
| MaxX86<f64>        |           | MAXSD       |        |
| MinX86<f64>        |           | MINSD       |        |
| I64ToF16           | FCVT.H.L  | -           | SCVTF  |
| I64ToF32           | FCVT.S.L  | CVTSI2SS    | SCVTF  |
| I64ToF64           | FCVT.D.L  | CVTSI2SD    | SCVTF  |
| U64ToF16           | FCVT.H.LU | -           | UCVTF  |
| U64ToF32           | FCVT.S.LU | -           | UCVTF  |
| U64ToF64           | FCVT.D.LU | -           | UCVTF  |

(1): Compiled code for x86 SSE resorts to CVTSS2SI for F32ToUxx.
(2): x86 SSE uses UCOMISS to achieve a the same functionality.
(3): ARM64 uses FCMP and FCMPE.
(4): ARM64 provides FMAXNM/FMINNM and FMAX/FMIN
(5): Compiled code for x86 SSE resorts to CVTSD2SI for F64ToUxx.

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

## Things You Need To Take Care Of
- RISC-V NaN Boxing

## How does it work?
Coding, algorithms, and a bit of math.
For a detailed explanation see [this blog post](https://www.chciken.com/simulation/2023/11/12/fast-floating-point-simulation.html).

## Issues
- Overflow exception for `Add`, `Sub`, `Mul`, `Div`, `Sqrt`, `Fma` may not trigger in some cases
- Underflow exception for `Mul`, `Div`, `Sqrt`, `Fma` may not trigger in some cases
- ARM's FPCR.DN = 0 needs to be implemented.
- `Fma` and `Sqrt` for f16 inherits the incorrect rounding problem of glibc (calculates the result as f32 and then casts to f16). Currently, this is fixed by soft float fall back,

For instance,
fma<f16>(a=-854, b=-28, c=1.6689e-05)
standard library result: 23904
correct result: 23920
a*b = 23912
a*b+c = 23912.000016689
