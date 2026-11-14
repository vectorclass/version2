# Standard vector and matrix benchmarks - wide FP32 register allocation

**Scalar FP32/FP64 versus <span style="color: #bc4c00;">sse2neon</span> versus <span style="color: #6f42c1;">Direct&nbsp;Neon</span>**: ten operations, object
and batch layouts, 128/4096 objects and logical batch widths 128/256/512.
All **160 three-way rows** below were remeasured after the helper-interface change.

The grouped native partial-memory helpers reduce register pressure in wide
FP32 addition/cross while preserving the dot gains. All slower rows remain.
See [measurement and code-generation context](measurement-notes.md).
Before/after and full-suite runs use their own controls; do not mix their times.

[CSV](standard_math_results.csv) · [JSON](standard_math_results.json) ·
[audit](standard_math_validation.json)

In this run, <span style="color: #6f42c1;">Direct&nbsp;Neon</span> wins or is within the shared 5% band in **160/160 rows**.

<!-- standard-math-reading-guide -->
## How to read the tables

**The competing columns are <span style="color: #bc4c00;">sse2neon</span> and <span style="color: #6f42c1;">Direct&nbsp;Neon</span>.** Their adjacent timing
numbers are bold and colored. The **Winning backend** column names the faster backend,
or says **Within&nbsp;5%** inside the shared 5% throughput band.
The adjacent **Throughput gain (%)** column shows the winner's percentage lead.
**Scalar means no SIMD in this benchmark.** It uses ordinary float/double
arithmetic with the compiler's automatic SIMD vectorization disabled. The
scalar result is the baseline for measuring how much SIMD helps. Both
<span style="color: #bc4c00;">sse2neon</span> and <span style="color: #6f42c1;">Direct&nbsp;Neon</span> use SIMD instructions; they are two implementations
of the same vector/matrix work.

**nsec/op means nanoseconds per complete vector or matrix operation.**

**Lower times win; higher throughput ratios win.** Each timing is measured in
nanoseconds per complete vector or matrix operation, averaged over repeated
array processing. It is not the time for the whole array or one CPU instruction.
Compare implementations within the same row first.

### Memory layout: AoS and SoA

These names describe **how the components of many vectors or matrices are
arranged in memory**. Here is an example with four-component float vectors;
`0`, `1`, etc. identify different vectors.

**Array of Structures (AoS): keep each object's components together.**

```text
Vector 0          Vector 1          Vector 2
[x0, y0, z0, w0]  [x1, y1, z1, w1]  [x2, y2, z2, w2] ...
```

In the object-layout benchmark, a 128-bit Neon register holds one FP32
vector's four components:

```text
SIMD lanes: [x0, y0, z0, w0]
             one vector's components
```

**Structure of Arrays (SoA): keep matching components from different objects together.**

```text
x: [x0, x1, x2, x3, ...]
y: [y0, y1, y2, y3, ...]
z: [z0, z1, z2, z3, ...]
w: [w0, w1, w2, w3, ...]
```

In the batch-layout benchmark, a 128-bit Neon register holds the same FP32
component from four different vectors. Other registers hold their other components:

```text
SIMD lanes: [x0, x1, x2, x3]
             four vectors' x components
```

For matrices, the same principle applies: AoS keeps each matrix's entries
together; SoA groups the same entry from different matrices together.

**The calculation is the same; its storage and SIMD organization differ.**
AoS uses SIMD across one object's components here; SoA applies the same
calculation across independent objects. Both layouts report time per complete
vector or matrix operation, not per component. Compare backends within the
same layout first; neither layout is automatically faster.

### Columns and colors

| Column | Meaning |
| --- | --- |
| Operation | The calculation, such as vector dot product or matrix multiplication. |
| FP | 32 means single-precision `float`; 64 means double-precision `double`. |
| **<span style="color: #bc4c00;">sse2neon</span><br>(nsec/op)** | SSE operations translated to Neon through <span style="color: #bc4c00;">sse2neon</span>. Competes directly with the adjacent <span style="color: #6f42c1;">Direct&nbsp;Neon</span> timing. |
| **<span style="color: #6f42c1;">Direct&nbsp;Neon</span><br>(nsec/op)** | The native Neon backend. Competes directly with the adjacent <span style="color: #bc4c00;">sse2neon</span> timing. |
| **<span style="color: #6f42c1;">Direct&nbsp;Neon</span> /<br><span style="color: #bc4c00;">sse2neon</span>** | **<span style="color: #bc4c00;">sse2neon</span> time ÷ <span style="color: #6f42c1;">Direct&nbsp;Neon</span> time**. Above 1 favors <span style="color: #6f42c1;">Direct&nbsp;Neon</span>; below 1 favors <span style="color: #bc4c00;">sse2neon</span>. Its continuous color scale shows which backend leads and by how much. |
| **Winning<br>backend** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span>, <span style="color: #bc4c00;">sse2neon</span>, or Within&nbsp;5% inside the shared band. |
| **Throughput<br>gain&nbsp;(%)** | The named winner's percentage throughput lead. An em dash means the result is within the 5% band. |
| **Scalar<br>(nsec/op)** | Scalar float/double arithmetic with automatic SIMD vectorization disabled. This is a separate reference. |
| Scalar /<br><span style="color: #6f42c1;">Direct&nbsp;Neon</span> | Non-SIMD scalar reference time ÷ <span style="color: #6f42c1;">Direct&nbsp;Neon</span> time. For example, 2× means <span style="color: #6f42c1;">Direct&nbsp;Neon</span> delivers twice the scalar throughput. |
| Consistent<br>Timing | Consistency of the difference between <span style="color: #bc4c00;">sse2neon</span> and <span style="color: #6f42c1;">Direct&nbsp;Neon</span> across runs: **Yes** means their three-run median timing ranges do not overlap; **No** means they overlap. This does not measure each backend's stability alone, identify the winner, or establish statistical significance. |

**Throughput percentages describe higher throughput, not reduced elapsed time.**
For the displayed throughput ratio `r`, a <span style="color: #6f42c1;">Direct&nbsp;Neon</span> win shows
`100 × (r − 1)%`; an <span style="color: #bc4c00;">sse2neon</span> win shows `100 × (1/r − 1)%`.
For example, 1.25× gives Winning backend **<span style="color: #6f42c1;">Direct&nbsp;Neon</span>** and Throughput gain **+25.0%**;
0.8× gives Winning backend **<span style="color: #bc4c00;">sse2neon</span>** and Throughput gain **+25.0%**. Percentages use the
displayed ratio, rounded to one decimal place. A Within&nbsp;5% verdict shows
**—** in Throughput gain. The existing ratio column retains its continuous gradient.

**Backend names have fixed identity colors:** <span style="color: #6f42c1;">Direct&nbsp;Neon</span> is purple and
<span style="color: #bc4c00;">sse2neon</span> is orange. <span style="color: #6f42c1;">Direct&nbsp;Neon</span> and Within&nbsp;5% stay together on one line.
These label colors identify the implementation; the timing colors show the result.

**Both competing timing columns are colored relative to each other:**

- <span style="color: #1a7f37;">**Green:**</span> the winning backend's time.
- <span style="color: #cf222e;">**Red:**</span> the losing backend's time.
- <span style="color: #0969da;">**Blue:**</span> both times when approximately equal.

**The throughput ratio uses a continuous gradient:**

- **1.0× is white:** equal throughput.
- **Above 1.0×**, the number fades toward <span style="color: #6f42c1;">Direct&nbsp;Neon</span>'s purple, reaching full
  purple at **1.5×** (<span style="color: #6f42c1;">Direct&nbsp;Neon</span> has a 50% throughput lead).
- **Below 1.0×**, it fades toward <span style="color: #bc4c00;">sse2neon</span>'s orange using the reciprocal lead,
  reaching full orange at **2/3× ≈ 0.667×** (<span style="color: #bc4c00;">sse2neon</span> has a 50% throughput lead).
- Colors saturate beyond those endpoints. Equal winning throughput leads have
  equal fade strength: 1.25× and 0.8× each represent a 25% lead for the winner.

| Throughput ratio | Meaning |
| ---: | --- |
| <span style="color: #bc4c00;">0.667×</span> | <span style="color: #bc4c00;">sse2neon</span> leads by approximately 50% |
| <span style="color: #dea680;">0.800×</span> | <span style="color: #bc4c00;">sse2neon</span> leads by 25% |
| <span style="color: #ffffff;">1.000×</span> | Equal throughput |
| <span style="color: #b7a0e0;">1.250×</span> | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> leads by 25% |
| <span style="color: #6f42c1;">1.500×</span> | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> leads by 50% |

This gradient starts changing immediately on either side of 1.0×. The timing
colors and Winning backend column still use the shared 5% band below, so a slight
purple/orange tint can appear alongside a Within&nbsp;5% verdict.

The shared decision uses **<span style="color: #6f42c1;">Direct&nbsp;Neon</span> / <span style="color: #bc4c00;">sse2neon</span>**:

| Throughput ratio | Winning backend | <span style="color: #bc4c00;">sse2neon</span> time color | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> time color |
| --- | --- | --- | --- |
| Above 1.05× | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | Red | Green |
| 0.95–1.05× inclusive | Within&nbsp;5% | Blue | Blue |
| Below 0.95× | <span style="color: #bc4c00;">sse2neon</span> | Green | Red |

Within&nbsp;5% is a practical throughput band, not a claim that the times
are identical. A row with blue timing numbers can have separate median ranges: a small
difference can be consistent while remaining inside that band. Colors compare these two
backends only; they do not indicate which is faster than scalar.

The section headings describe the workload:

- **128 or 4,096 objects** is the array length, not the SIMD width. All timing
  columns still report time **per object**.
- **Array of Structures (AoS)** corresponds to `object_aos` in the data files;
  **Structure of Arrays (SoA)** corresponds to `batch_soa`.
- **Logical 128/256/512 bits** is the batch-vector width. Neon hardware vectors
  remain 128-bit; wider values combine registers. FP32 logical 256 processes
  eight independent objects at a time; FP64 logical 256 processes four.
  Wider logical vectors are not automatically faster. Scalar batch results
  are reused across widths because scalar has no SIMD width.

For a worked example, take **<span style="color: #bc4c00;">sse2neon</span> = 0.318 nsec/op** and
**<span style="color: #6f42c1;">Direct&nbsp;Neon</span> = 0.289 nsec/op**, with a throughput ratio of **1.099×**.
<span style="color: #6f42c1;">Direct&nbsp;Neon</span> delivers approximately **9.9% more throughput**, or about
**9% less elapsed time**, than <span style="color: #bc4c00;">sse2neon</span>. The <span style="color: #6f42c1;">Direct&nbsp;Neon</span> time is green, the
<span style="color: #bc4c00;">sse2neon</span> time is red, Winning backend says **<span style="color: #6f42c1;">Direct&nbsp;Neon</span>**, and Throughput gain shows **+9.9%**. Ratios use
unrounded measurements, so dividing the displayed times can differ slightly.

These results include loads/stores and loop/call overhead. They describe array
throughput on the measured host, not isolated instruction latency or whole-
application speedup. Times are medians of invocation medians; raw outliers are
retained. The separate before/after optimization reports use their own measurements and
controls: do not combine their times with this report's times.
<!-- /standard-math-reading-guide -->

Units are **ns/vector or ns/matrix**, lower is better. Ratios divide the named
baseline time by Neon time: above one means Neon is faster. The scalar batch
baseline is reused across widths. `object_aos` packs four coordinates per
vector/column; `batch_soa` assigns an independent object to each SIMD lane.
Hardware vectors remain 128-bit; wider logical values compose registers.
These are array throughput measurements, including loads/stores and loop/call
overhead, with data creation, references, validation and dumps outside timing.

TRS inverse requires nonzero orthogonal scaled affine columns (rotation/scale/
translation); general projective/sheared inverse is not covered. Scale/add is
`a * 0.625 + b`, without FMA; zero vectors normalize to zero. Scalar float/double
disable loop/SLP auto-vectorization, verified to contain no packed FP arithmetic.

Three invocations per backend/count, rotating order, nine warmed calibrated
trials per shape. Medians are taken across invocation medians. Raw scheduler
outliers are retained; range separation is descriptive, not a significance test.
**18 measured processes / 1,200 shapes / 10,800 trial batches**. All outputs
pass the unchanged independent-reference/canary/input gates before and after
timing; maximum reference error remains within the 64-epsilon gate. Reference
uses coordinate loops, hypot and pivoted Gauss-Jordan; host long double is binary64.

Apple M2/macOS 26.7, Clang 17/SDK 26.1, ARM64 C++20 Release `-O3 -mcpu=apple-m1
-ffp-contract=off`, no fast-math/LTO/sanitizer in timing, nearest/FZ off (FPCR 0).
Frequency/core placement is unpinned. Same guarded memory layout as baseline:
17-element batch-plane padding, data one scalar past allocation base, unaligned
loads/stores permitted. Only the grouped native FP32 partial-memory helper interfaces/attributes
changed; gfxmath/application sources and benchmark formulas/inputs are untouched.
No commit or install occurred.

[Implementation contract](../validation/regression-tests.md) ·
[Original operation/protocol definitions](measurement-notes.md)

## 128 objects — Array of Structures (AoS): SIMD across one object’s components

<!-- standard-math-table-notes -->
- **<span style="color: #bc4c00;">sse2neon</span>:** SSE operations translated to Neon SIMD.
- **<span style="color: #6f42c1;">Direct&nbsp;Neon</span>:** native Neon SIMD.
- **(nsec/op):** nanoseconds per complete vector/matrix operation; lower is better.
- **Scalar:** float/double arithmetic without SIMD; auto-vectorization disabled.
<!-- /standard-math-table-notes -->

| Operation | FP | **<span style="color: #bc4c00;">sse2neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span> /<br><span style="color: #bc4c00;">sse2neon</span>** | **Winning<br>backend** | **Throughput<br>gain&nbsp;(%)** | **Scalar<br>(nsec/op)** | Scalar /<br><span style="color: #6f42c1;">Direct&nbsp;Neon</span> | Consistent<br>Timing |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- |
| mat4_inverse_trs | 32 | **<span style="color: #0969da;">1.899</span>** | **<span style="color: #0969da;">1.899</span>** | **<span style="color: #ffffff;">1.000×</span>** | Within&nbsp;5% | — | 4.184 | 2.20× | no |
| mat4_inverse_trs | 64 | **<span style="color: #0969da;">3.369</span>** | **<span style="color: #0969da;">3.429</span>** | **<span style="color: #fdf9f6;">0.983×</span>** | Within&nbsp;5% | — | 4.426 | 1.29× | no |
| mat4_multiply | 32 | **<span style="color: #0969da;">2.120</span>** | **<span style="color: #0969da;">2.060</span>** | **<span style="color: #f7f4fb;">1.029×</span>** | Within&nbsp;5% | — | 10.212 | 4.96× | yes |
| mat4_multiply | 64 | **<span style="color: #0969da;">4.336</span>** | **<span style="color: #0969da;">4.409</span>** | **<span style="color: #fdf9f7;">0.984×</span>** | Within&nbsp;5% | — | 10.707 | 2.43× | no |
| mat4_transpose | 32 | **<span style="color: #0969da;">1.197</span>** | **<span style="color: #0969da;">1.185</span>** | **<span style="color: #fcfbfe;">1.010×</span>** | Within&nbsp;5% | — | 2.410 | 2.03× | no |
| mat4_transpose | 64 | **<span style="color: #0969da;">2.324</span>** | **<span style="color: #0969da;">2.390</span>** | **<span style="color: #fbf5f0;">0.972×</span>** | Within&nbsp;5% | — | 2.530 | 1.06× | no |
| mat4_vector | 32 | **<span style="color: #0969da;">0.596</span>** | **<span style="color: #0969da;">0.595</span>** | **<span style="color: #fefeff;">1.002×</span>** | Within&nbsp;5% | — | 2.484 | 4.18× | no |
| mat4_vector | 64 | **<span style="color: #0969da;">1.209</span>** | **<span style="color: #0969da;">1.199</span>** | **<span style="color: #fdfcfe;">1.008×</span>** | Within&nbsp;5% | — | 2.531 | 2.11× | no |
| vec3_cross | 32 | **<span style="color: #cf222e;">0.896</span>** | **<span style="color: #1a7f37;">0.616</span>** | **<span style="color: #7d54c7;">1.453×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +45.3% | 1.119 | 1.81× | yes |
| vec3_cross | 64 | **<span style="color: #0969da;">0.889</span>** | **<span style="color: #0969da;">0.853</span>** | **<span style="color: #f3effa;">1.042×</span>** | Within&nbsp;5% | — | 1.133 | 1.33× | yes |
| vec4_add | 32 | **<span style="color: #0969da;">0.359</span>** | **<span style="color: #0969da;">0.354</span>** | **<span style="color: #faf9fd;">1.016×</span>** | Within&nbsp;5% | — | 0.923 | 2.61× | no |
| vec4_add | 64 | **<span style="color: #0969da;">0.688</span>** | **<span style="color: #0969da;">0.668</span>** | **<span style="color: #f6f3fb;">1.031×</span>** | Within&nbsp;5% | — | 0.934 | 1.40× | no |
| vec4_dot | 32 | **<span style="color: #0969da;">0.392</span>** | **<span style="color: #0969da;">0.398</span>** | **<span style="color: #fdfaf8;">0.986×</span>** | Within&nbsp;5% | — | 0.702 | 1.77× | no |
| vec4_dot | 64 | **<span style="color: #0969da;">0.522</span>** | **<span style="color: #0969da;">0.515</span>** | **<span style="color: #fbfafd;">1.013×</span>** | Within&nbsp;5% | — | 0.687 | 1.33× | no |
| vec4_length | 32 | **<span style="color: #0969da;">0.583</span>** | **<span style="color: #0969da;">0.580</span>** | **<span style="color: #fefdff;">1.004×</span>** | Within&nbsp;5% | — | 0.679 | 1.17× | no |
| vec4_length | 64 | **<span style="color: #0969da;">0.578</span>** | **<span style="color: #0969da;">0.576</span>** | **<span style="color: #fefeff;">1.002×</span>** | Within&nbsp;5% | — | 0.614 | 1.06× | no |
| vec4_normalize | 32 | **<span style="color: #0969da;">1.028</span>** | **<span style="color: #0969da;">1.023</span>** | **<span style="color: #fefdfe;">1.005×</span>** | Within&nbsp;5% | — | 1.565 | 1.53× | no |
| vec4_normalize | 64 | **<span style="color: #0969da;">1.209</span>** | **<span style="color: #0969da;">1.207</span>** | **<span style="color: #fefeff;">1.002×</span>** | Within&nbsp;5% | — | 1.641 | 1.36× | no |
| vec4_scale_add | 32 | **<span style="color: #0969da;">0.361</span>** | **<span style="color: #0969da;">0.358</span>** | **<span style="color: #fcfcfe;">1.009×</span>** | Within&nbsp;5% | — | 0.953 | 2.66× | yes |
| vec4_scale_add | 64 | **<span style="color: #0969da;">0.615</span>** | **<span style="color: #0969da;">0.598</span>** | **<span style="color: #f7f4fb;">1.029×</span>** | Within&nbsp;5% | — | 0.954 | 1.60× | yes |

## 128 objects — Structure of Arrays (SoA): SIMD across objects / logical 128 bits

<!-- standard-math-table-notes -->
- **<span style="color: #bc4c00;">sse2neon</span>:** SSE operations translated to Neon SIMD.
- **<span style="color: #6f42c1;">Direct&nbsp;Neon</span>:** native Neon SIMD.
- **(nsec/op):** nanoseconds per complete vector/matrix operation; lower is better.
- **Scalar:** float/double arithmetic without SIMD; auto-vectorization disabled.
<!-- /standard-math-table-notes -->

| Operation | FP | **<span style="color: #bc4c00;">sse2neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span> /<br><span style="color: #bc4c00;">sse2neon</span>** | **Winning<br>backend** | **Throughput<br>gain&nbsp;(%)** | **Scalar<br>(nsec/op)** | Scalar /<br><span style="color: #6f42c1;">Direct&nbsp;Neon</span> | Consistent<br>Timing |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- |
| mat4_inverse_trs | 32 | **<span style="color: #cf222e;">1.700</span>** | **<span style="color: #1a7f37;">1.412</span>** | **<span style="color: #c4b2e6;">1.204×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +20.4% | 4.636 | 3.28× | yes |
| mat4_inverse_trs | 64 | **<span style="color: #0969da;">2.749</span>** | **<span style="color: #0969da;">2.733</span>** | **<span style="color: #fdfdfe;">1.006×</span>** | Within&nbsp;5% | — | 4.733 | 1.73× | yes |
| mat4_multiply | 32 | **<span style="color: #cf222e;">7.153</span>** | **<span style="color: #1a7f37;">3.306</span>** | **<span style="color: #6f42c1;">2.164×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +116.4% | 17.789 | 5.38× | yes |
| mat4_multiply | 64 | **<span style="color: #0969da;">6.795</span>** | **<span style="color: #0969da;">6.777</span>** | **<span style="color: #fefeff;">1.003×</span>** | Within&nbsp;5% | — | 18.520 | 2.73× | yes |
| mat4_transpose | 32 | **<span style="color: #0969da;">1.529</span>** | **<span style="color: #0969da;">1.499</span>** | **<span style="color: #f9f7fd;">1.020×</span>** | Within&nbsp;5% | — | 3.321 | 2.22× | yes |
| mat4_transpose | 64 | **<span style="color: #0969da;">2.594</span>** | **<span style="color: #0969da;">2.600</span>** | **<span style="color: #fffefe;">0.998×</span>** | Within&nbsp;5% | — | 3.319 | 1.28× | no |
| mat4_vector | 32 | **<span style="color: #cf222e;">3.080</span>** | **<span style="color: #1a7f37;">0.772</span>** | **<span style="color: #6f42c1;">3.989×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +298.9% | 2.988 | 3.87× | yes |
| mat4_vector | 64 | **<span style="color: #0969da;">1.581</span>** | **<span style="color: #0969da;">1.566</span>** | **<span style="color: #fcfbfe;">1.010×</span>** | Within&nbsp;5% | — | 2.994 | 1.91× | no |
| vec3_cross | 32 | **<span style="color: #cf222e;">0.451</span>** | **<span style="color: #1a7f37;">0.381</span>** | **<span style="color: #cab9e8;">1.185×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +18.5% | 0.841 | 2.21× | yes |
| vec3_cross | 64 | **<span style="color: #0969da;">0.603</span>** | **<span style="color: #0969da;">0.604</span>** | **<span style="color: #fffefd;">0.997×</span>** | Within&nbsp;5% | — | 0.847 | 1.40× | no |
| vec4_add | 32 | **<span style="color: #cf222e;">0.481</span>** | **<span style="color: #1a7f37;">0.383</span>** | **<span style="color: #b69fdf;">1.255×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +25.5% | 0.927 | 2.42× | yes |
| vec4_add | 64 | **<span style="color: #0969da;">0.720</span>** | **<span style="color: #0969da;">0.716</span>** | **<span style="color: #fefdfe;">1.005×</span>** | Within&nbsp;5% | — | 0.945 | 1.32× | no |
| vec4_dot | 32 | **<span style="color: #cf222e;">0.404</span>** | **<span style="color: #1a7f37;">0.274</span>** | **<span style="color: #764bc4;">1.477×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +47.7% | 0.843 | 3.08× | yes |
| vec4_dot | 64 | **<span style="color: #0969da;">0.550</span>** | **<span style="color: #0969da;">0.549</span>** | **<span style="color: #fefeff;">1.002×</span>** | Within&nbsp;5% | — | 0.864 | 1.57× | no |
| vec4_length | 32 | **<span style="color: #cf222e;">0.393</span>** | **<span style="color: #1a7f37;">0.238</span>** | **<span style="color: #6f42c1;">1.651×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +65.1% | 0.684 | 2.87× | yes |
| vec4_length | 64 | **<span style="color: #0969da;">0.477</span>** | **<span style="color: #0969da;">0.478</span>** | **<span style="color: #fffefd;">0.997×</span>** | Within&nbsp;5% | — | 0.648 | 1.36× | no |
| vec4_normalize | 32 | **<span style="color: #cf222e;">0.635</span>** | **<span style="color: #1a7f37;">0.372</span>** | **<span style="color: #6f42c1;">1.707×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +70.7% | 1.564 | 4.20× | yes |
| vec4_normalize | 64 | **<span style="color: #0969da;">0.788</span>** | **<span style="color: #0969da;">0.788</span>** | **<span style="color: #ffffff;">1.001×</span>** | Within&nbsp;5% | — | 1.618 | 2.05× | yes |
| vec4_scale_add | 32 | **<span style="color: #cf222e;">0.512</span>** | **<span style="color: #1a7f37;">0.384</span>** | **<span style="color: #9f81d6;">1.334×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +33.4% | 0.951 | 2.48× | yes |
| vec4_scale_add | 64 | **<span style="color: #0969da;">0.598</span>** | **<span style="color: #0969da;">0.603</span>** | **<span style="color: #fefcfa;">0.991×</span>** | Within&nbsp;5% | — | 0.937 | 1.55× | no |

## 128 objects — Structure of Arrays (SoA): SIMD across objects / logical 256 bits

<!-- standard-math-table-notes -->
- **<span style="color: #bc4c00;">sse2neon</span>:** SSE operations translated to Neon SIMD.
- **<span style="color: #6f42c1;">Direct&nbsp;Neon</span>:** native Neon SIMD.
- **(nsec/op):** nanoseconds per complete vector/matrix operation; lower is better.
- **Scalar:** float/double arithmetic without SIMD; auto-vectorization disabled.
<!-- /standard-math-table-notes -->

| Operation | FP | **<span style="color: #bc4c00;">sse2neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span> /<br><span style="color: #bc4c00;">sse2neon</span>** | **Winning<br>backend** | **Throughput<br>gain&nbsp;(%)** | **Scalar<br>(nsec/op)** | Scalar /<br><span style="color: #6f42c1;">Direct&nbsp;Neon</span> | Consistent<br>Timing |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- |
| mat4_inverse_trs | 32 | **<span style="color: #cf222e;">2.122</span>** | **<span style="color: #1a7f37;">1.491</span>** | **<span style="color: #855fca;">1.424×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +42.4% | 4.636 | 3.11× | yes |
| mat4_inverse_trs | 64 | **<span style="color: #0969da;">2.989</span>** | **<span style="color: #0969da;">3.002</span>** | **<span style="color: #fefefd;">0.996×</span>** | Within&nbsp;5% | — | 4.733 | 1.58× | no |
| mat4_multiply | 32 | **<span style="color: #cf222e;">5.100</span>** | **<span style="color: #1a7f37;">4.084</span>** | **<span style="color: #b7a1e0;">1.249×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +24.9% | 17.789 | 4.36× | yes |
| mat4_multiply | 64 | **<span style="color: #0969da;">7.717</span>** | **<span style="color: #0969da;">7.757</span>** | **<span style="color: #fefdfc;">0.995×</span>** | Within&nbsp;5% | — | 18.520 | 2.39× | no |
| mat4_transpose | 32 | **<span style="color: #cf222e;">2.152</span>** | **<span style="color: #1a7f37;">1.341</span>** | **<span style="color: #6f42c1;">1.605×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +60.5% | 3.321 | 2.48× | yes |
| mat4_transpose | 64 | **<span style="color: #0969da;">2.361</span>** | **<span style="color: #0969da;">2.350</span>** | **<span style="color: #fefdfe;">1.005×</span>** | Within&nbsp;5% | — | 3.319 | 1.41× | no |
| mat4_vector | 32 | **<span style="color: #cf222e;">1.507</span>** | **<span style="color: #1a7f37;">1.087</span>** | **<span style="color: #906dcf;">1.386×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +38.6% | 2.988 | 2.75× | yes |
| mat4_vector | 64 | **<span style="color: #0969da;">2.060</span>** | **<span style="color: #0969da;">2.040</span>** | **<span style="color: #fcfbfe;">1.010×</span>** | Within&nbsp;5% | — | 2.994 | 1.47× | no |
| vec3_cross | 32 | **<span style="color: #cf222e;">0.360</span>** | **<span style="color: #1a7f37;">0.329</span>** | **<span style="color: #e4dbf3;">1.094×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +9.4% | 0.841 | 2.56× | yes |
| vec3_cross | 64 | **<span style="color: #0969da;">0.589</span>** | **<span style="color: #0969da;">0.589</span>** | **<span style="color: #ffffff;">1.000×</span>** | Within&nbsp;5% | — | 0.847 | 1.44× | no |
| vec4_add | 32 | **<span style="color: #cf222e;">0.386</span>** | **<span style="color: #1a7f37;">0.346</span>** | **<span style="color: #ded3f1;">1.116×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +11.6% | 0.927 | 2.68× | yes |
| vec4_add | 64 | **<span style="color: #0969da;">0.567</span>** | **<span style="color: #0969da;">0.562</span>** | **<span style="color: #fdfcfe;">1.008×</span>** | Within&nbsp;5% | — | 0.945 | 1.68× | no |
| vec4_dot | 32 | **<span style="color: #cf222e;">0.292</span>** | **<span style="color: #1a7f37;">0.272</span>** | **<span style="color: #ebe4f6;">1.071×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +7.1% | 0.843 | 3.10× | yes |
| vec4_dot | 64 | **<span style="color: #0969da;">0.506</span>** | **<span style="color: #0969da;">0.512</span>** | **<span style="color: #fefbf9;">0.989×</span>** | Within&nbsp;5% | — | 0.864 | 1.69× | no |
| vec4_length | 32 | **<span style="color: #cf222e;">0.395</span>** | **<span style="color: #1a7f37;">0.220</span>** | **<span style="color: #6f42c1;">1.799×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +79.9% | 0.684 | 3.11× | yes |
| vec4_length | 64 | **<span style="color: #0969da;">0.414</span>** | **<span style="color: #0969da;">0.405</span>** | **<span style="color: #f8f6fc;">1.023×</span>** | Within&nbsp;5% | — | 0.648 | 1.60× | no |
| vec4_normalize | 32 | **<span style="color: #cf222e;">0.610</span>** | **<span style="color: #1a7f37;">0.402</span>** | **<span style="color: #6f42c1;">1.520×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +52.0% | 1.564 | 3.89× | yes |
| vec4_normalize | 64 | **<span style="color: #0969da;">0.780</span>** | **<span style="color: #0969da;">0.785</span>** | **<span style="color: #fefdfc;">0.995×</span>** | Within&nbsp;5% | — | 1.618 | 2.06× | no |
| vec4_scale_add | 32 | **<span style="color: #cf222e;">0.398</span>** | **<span style="color: #1a7f37;">0.347</span>** | **<span style="color: #d5c8ed;">1.146×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +14.6% | 0.951 | 2.74× | yes |
| vec4_scale_add | 64 | **<span style="color: #0969da;">0.613</span>** | **<span style="color: #0969da;">0.620</span>** | **<span style="color: #fefbf9;">0.989×</span>** | Within&nbsp;5% | — | 0.937 | 1.51× | no |

## 128 objects — Structure of Arrays (SoA): SIMD across objects / logical 512 bits

<!-- standard-math-table-notes -->
- **<span style="color: #bc4c00;">sse2neon</span>:** SSE operations translated to Neon SIMD.
- **<span style="color: #6f42c1;">Direct&nbsp;Neon</span>:** native Neon SIMD.
- **(nsec/op):** nanoseconds per complete vector/matrix operation; lower is better.
- **Scalar:** float/double arithmetic without SIMD; auto-vectorization disabled.
<!-- /standard-math-table-notes -->

| Operation | FP | **<span style="color: #bc4c00;">sse2neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span> /<br><span style="color: #bc4c00;">sse2neon</span>** | **Winning<br>backend** | **Throughput<br>gain&nbsp;(%)** | **Scalar<br>(nsec/op)** | Scalar /<br><span style="color: #6f42c1;">Direct&nbsp;Neon</span> | Consistent<br>Timing |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- |
| mat4_inverse_trs | 32 | **<span style="color: #cf222e;">2.718</span>** | **<span style="color: #1a7f37;">2.169</span>** | **<span style="color: #b69fe0;">1.253×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +25.3% | 4.636 | 2.14× | yes |
| mat4_inverse_trs | 64 | **<span style="color: #0969da;">4.742</span>** | **<span style="color: #0969da;">4.702</span>** | **<span style="color: #fdfcfe;">1.008×</span>** | Within&nbsp;5% | — | 4.733 | 1.01× | yes |
| mat4_multiply | 32 | **<span style="color: #cf222e;">5.452</span>** | **<span style="color: #1a7f37;">4.231</span>** | **<span style="color: #ac92db;">1.289×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +28.9% | 17.789 | 4.20× | yes |
| mat4_multiply | 64 | **<span style="color: #0969da;">10.407</span>** | **<span style="color: #0969da;">10.195</span>** | **<span style="color: #f9f7fc;">1.021×</span>** | Within&nbsp;5% | — | 18.520 | 1.82× | no |
| mat4_transpose | 32 | **<span style="color: #cf222e;">2.405</span>** | **<span style="color: #1a7f37;">1.629</span>** | **<span style="color: #764bc4;">1.477×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +47.7% | 3.321 | 2.04× | yes |
| mat4_transpose | 64 | **<span style="color: #0969da;">4.527</span>** | **<span style="color: #0969da;">4.507</span>** | **<span style="color: #fefdff;">1.004×</span>** | Within&nbsp;5% | — | 3.319 | 0.74× | no |
| mat4_vector | 32 | **<span style="color: #cf222e;">1.433</span>** | **<span style="color: #1a7f37;">1.217</span>** | **<span style="color: #ccbce9;">1.177×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +17.7% | 2.988 | 2.45× | yes |
| mat4_vector | 64 | **<span style="color: #0969da;">2.677</span>** | **<span style="color: #0969da;">2.670</span>** | **<span style="color: #fefeff;">1.003×</span>** | Within&nbsp;5% | — | 2.994 | 1.12× | no |
| vec3_cross | 32 | **<span style="color: #cf222e;">0.325</span>** | **<span style="color: #1a7f37;">0.308</span>** | **<span style="color: #efebf8;">1.054×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +5.4% | 0.841 | 2.73× | yes |
| vec3_cross | 64 | **<span style="color: #0969da;">0.580</span>** | **<span style="color: #0969da;">0.570</span>** | **<span style="color: #faf9fd;">1.017×</span>** | Within&nbsp;5% | — | 0.847 | 1.49× | yes |
| vec4_add | 32 | **<span style="color: #0969da;">0.338</span>** | **<span style="color: #0969da;">0.323</span>** | **<span style="color: #f2eef9;">1.046×</span>** | Within&nbsp;5% | — | 0.927 | 2.87× | yes |
| vec4_add | 64 | **<span style="color: #0969da;">0.563</span>** | **<span style="color: #0969da;">0.552</span>** | **<span style="color: #f9f7fc;">1.021×</span>** | Within&nbsp;5% | — | 0.945 | 1.71× | no |
| vec4_dot | 32 | **<span style="color: #0969da;">0.261</span>** | **<span style="color: #0969da;">0.253</span>** | **<span style="color: #f6f3fb;">1.032×</span>** | Within&nbsp;5% | — | 0.843 | 3.34× | yes |
| vec4_dot | 64 | **<span style="color: #0969da;">0.497</span>** | **<span style="color: #0969da;">0.491</span>** | **<span style="color: #fcfbfe;">1.011×</span>** | Within&nbsp;5% | — | 0.864 | 1.76× | no |
| vec4_length | 32 | **<span style="color: #cf222e;">0.396</span>** | **<span style="color: #1a7f37;">0.198</span>** | **<span style="color: #6f42c1;">1.993×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +99.3% | 0.684 | 3.45× | yes |
| vec4_length | 64 | **<span style="color: #0969da;">0.401</span>** | **<span style="color: #0969da;">0.397</span>** | **<span style="color: #fcfbfe;">1.010×</span>** | Within&nbsp;5% | — | 0.648 | 1.63× | no |
| vec4_normalize | 32 | **<span style="color: #cf222e;">0.629</span>** | **<span style="color: #1a7f37;">0.449</span>** | **<span style="color: #8b67cd;">1.402×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +40.2% | 1.564 | 3.48× | yes |
| vec4_normalize | 64 | **<span style="color: #0969da;">0.852</span>** | **<span style="color: #0969da;">0.853</span>** | **<span style="color: #fffffe;">0.999×</span>** | Within&nbsp;5% | — | 1.618 | 1.90× | no |
| vec4_scale_add | 32 | **<span style="color: #cf222e;">0.364</span>** | **<span style="color: #1a7f37;">0.339</span>** | **<span style="color: #eae3f6;">1.073×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +7.3% | 0.951 | 2.80× | yes |
| vec4_scale_add | 64 | **<span style="color: #0969da;">0.638</span>** | **<span style="color: #0969da;">0.654</span>** | **<span style="color: #fcf6f2;">0.975×</span>** | Within&nbsp;5% | — | 0.937 | 1.43× | no |

## 4,096 objects — Array of Structures (AoS): SIMD across one object’s components

<!-- standard-math-table-notes -->
- **<span style="color: #bc4c00;">sse2neon</span>:** SSE operations translated to Neon SIMD.
- **<span style="color: #6f42c1;">Direct&nbsp;Neon</span>:** native Neon SIMD.
- **(nsec/op):** nanoseconds per complete vector/matrix operation; lower is better.
- **Scalar:** float/double arithmetic without SIMD; auto-vectorization disabled.
<!-- /standard-math-table-notes -->

| Operation | FP | **<span style="color: #bc4c00;">sse2neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span> /<br><span style="color: #bc4c00;">sse2neon</span>** | **Winning<br>backend** | **Throughput<br>gain&nbsp;(%)** | **Scalar<br>(nsec/op)** | Scalar /<br><span style="color: #6f42c1;">Direct&nbsp;Neon</span> | Consistent<br>Timing |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- |
| mat4_inverse_trs | 32 | **<span style="color: #0969da;">1.951</span>** | **<span style="color: #0969da;">1.949</span>** | **<span style="color: #ffffff;">1.001×</span>** | Within&nbsp;5% | — | 4.208 | 2.16× | no |
| mat4_inverse_trs | 64 | **<span style="color: #0969da;">3.378</span>** | **<span style="color: #0969da;">3.436</span>** | **<span style="color: #fdf9f6;">0.983×</span>** | Within&nbsp;5% | — | 4.488 | 1.31× | yes |
| mat4_multiply | 32 | **<span style="color: #0969da;">2.157</span>** | **<span style="color: #0969da;">2.145</span>** | **<span style="color: #fefdfe;">1.005×</span>** | Within&nbsp;5% | — | 10.976 | 5.12× | no |
| mat4_multiply | 64 | **<span style="color: #0969da;">4.413</span>** | **<span style="color: #0969da;">4.370</span>** | **<span style="color: #fcfbfe;">1.010×</span>** | Within&nbsp;5% | — | 12.291 | 2.81× | no |
| mat4_transpose | 32 | **<span style="color: #0969da;">1.333</span>** | **<span style="color: #0969da;">1.330</span>** | **<span style="color: #fefeff;">1.002×</span>** | Within&nbsp;5% | — | 2.751 | 2.07× | no |
| mat4_transpose | 64 | **<span style="color: #0969da;">2.634</span>** | **<span style="color: #0969da;">2.650</span>** | **<span style="color: #fefdfc;">0.994×</span>** | Within&nbsp;5% | — | 3.372 | 1.27× | no |
| mat4_vector | 32 | **<span style="color: #0969da;">0.938</span>** | **<span style="color: #0969da;">0.937</span>** | **<span style="color: #fefeff;">1.002×</span>** | Within&nbsp;5% | — | 2.783 | 2.97× | no |
| mat4_vector | 64 | **<span style="color: #0969da;">1.860</span>** | **<span style="color: #0969da;">1.870</span>** | **<span style="color: #fefdfc;">0.995×</span>** | Within&nbsp;5% | — | 3.251 | 1.74× | no |
| vec3_cross | 32 | **<span style="color: #cf222e;">0.886</span>** | **<span style="color: #1a7f37;">0.608</span>** | **<span style="color: #7c53c6;">1.456×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +45.6% | 1.141 | 1.87× | yes |
| vec3_cross | 64 | **<span style="color: #0969da;">1.021</span>** | **<span style="color: #0969da;">1.012</span>** | **<span style="color: #fdfcfe;">1.008×</span>** | Within&nbsp;5% | — | 1.185 | 1.17× | no |
| vec4_add | 32 | **<span style="color: #0969da;">0.315</span>** | **<span style="color: #0969da;">0.316</span>** | **<span style="color: #fefdfc;">0.995×</span>** | Within&nbsp;5% | — | 0.922 | 2.91× | no |
| vec4_add | 64 | **<span style="color: #0969da;">0.987</span>** | **<span style="color: #0969da;">0.980</span>** | **<span style="color: #fdfcfe;">1.007×</span>** | Within&nbsp;5% | — | 1.119 | 1.14× | no |
| vec4_dot | 32 | **<span style="color: #0969da;">0.362</span>** | **<span style="color: #0969da;">0.368</span>** | **<span style="color: #fdf8f6;">0.982×</span>** | Within&nbsp;5% | — | 0.692 | 1.88× | no |
| vec4_dot | 64 | **<span style="color: #0969da;">0.727</span>** | **<span style="color: #0969da;">0.728</span>** | **<span style="color: #fffefe;">0.998×</span>** | Within&nbsp;5% | — | 0.769 | 1.06× | no |
| vec4_length | 32 | **<span style="color: #0969da;">0.582</span>** | **<span style="color: #0969da;">0.581</span>** | **<span style="color: #ffffff;">1.001×</span>** | Within&nbsp;5% | — | 0.674 | 1.16× | no |
| vec4_length | 64 | **<span style="color: #0969da;">0.582</span>** | **<span style="color: #0969da;">0.580</span>** | **<span style="color: #fefeff;">1.003×</span>** | Within&nbsp;5% | — | 0.624 | 1.08× | yes |
| vec4_normalize | 32 | **<span style="color: #0969da;">1.013</span>** | **<span style="color: #0969da;">1.010</span>** | **<span style="color: #fefeff;">1.003×</span>** | Within&nbsp;5% | — | 1.558 | 1.54× | yes |
| vec4_normalize | 64 | **<span style="color: #0969da;">1.210</span>** | **<span style="color: #0969da;">1.207</span>** | **<span style="color: #fefeff;">1.003×</span>** | Within&nbsp;5% | — | 1.647 | 1.36× | no |
| vec4_scale_add | 32 | **<span style="color: #0969da;">0.319</span>** | **<span style="color: #0969da;">0.324</span>** | **<span style="color: #fdfaf8;">0.987×</span>** | Within&nbsp;5% | — | 0.950 | 2.94× | no |
| vec4_scale_add | 64 | **<span style="color: #0969da;">0.981</span>** | **<span style="color: #0969da;">0.987</span>** | **<span style="color: #fefdfc;">0.994×</span>** | Within&nbsp;5% | — | 1.141 | 1.16× | yes |

## 4,096 objects — Structure of Arrays (SoA): SIMD across objects / logical 128 bits

<!-- standard-math-table-notes -->
- **<span style="color: #bc4c00;">sse2neon</span>:** SSE operations translated to Neon SIMD.
- **<span style="color: #6f42c1;">Direct&nbsp;Neon</span>:** native Neon SIMD.
- **(nsec/op):** nanoseconds per complete vector/matrix operation; lower is better.
- **Scalar:** float/double arithmetic without SIMD; auto-vectorization disabled.
<!-- /standard-math-table-notes -->

| Operation | FP | **<span style="color: #bc4c00;">sse2neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span> /<br><span style="color: #bc4c00;">sse2neon</span>** | **Winning<br>backend** | **Throughput<br>gain&nbsp;(%)** | **Scalar<br>(nsec/op)** | Scalar /<br><span style="color: #6f42c1;">Direct&nbsp;Neon</span> | Consistent<br>Timing |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- |
| mat4_inverse_trs | 32 | **<span style="color: #cf222e;">2.474</span>** | **<span style="color: #1a7f37;">2.344</span>** | **<span style="color: #efeaf8;">1.055×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +5.5% | 4.628 | 1.97× | yes |
| mat4_inverse_trs | 64 | **<span style="color: #0969da;">3.750</span>** | **<span style="color: #0969da;">3.745</span>** | **<span style="color: #ffffff;">1.001×</span>** | Within&nbsp;5% | — | 4.754 | 1.27× | no |
| mat4_multiply | 32 | **<span style="color: #cf222e;">7.185</span>** | **<span style="color: #1a7f37;">3.840</span>** | **<span style="color: #6f42c1;">1.871×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +87.1% | 18.633 | 4.85× | yes |
| mat4_multiply | 64 | **<span style="color: #0969da;">7.176</span>** | **<span style="color: #0969da;">7.154</span>** | **<span style="color: #fefeff;">1.003×</span>** | Within&nbsp;5% | — | 19.719 | 2.76× | no |
| mat4_transpose | 32 | **<span style="color: #0969da;">2.584</span>** | **<span style="color: #0969da;">2.485</span>** | **<span style="color: #f3f0fa;">1.040×</span>** | Within&nbsp;5% | — | 4.577 | 1.84× | yes |
| mat4_transpose | 64 | **<span style="color: #0969da;">4.042</span>** | **<span style="color: #0969da;">4.041</span>** | **<span style="color: #ffffff;">1.000×</span>** | Within&nbsp;5% | — | 4.587 | 1.13× | no |
| mat4_vector | 32 | **<span style="color: #cf222e;">3.192</span>** | **<span style="color: #1a7f37;">1.486</span>** | **<span style="color: #6f42c1;">2.148×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +114.8% | 3.122 | 2.10× | yes |
| mat4_vector | 64 | **<span style="color: #0969da;">2.245</span>** | **<span style="color: #0969da;">2.257</span>** | **<span style="color: #fefdfc;">0.995×</span>** | Within&nbsp;5% | — | 3.201 | 1.42× | no |
| vec3_cross | 32 | **<span style="color: #cf222e;">0.704</span>** | **<span style="color: #1a7f37;">0.613</span>** | **<span style="color: #d5c7ed;">1.147×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +14.7% | 1.147 | 1.87× | yes |
| vec3_cross | 64 | **<span style="color: #0969da;">1.391</span>** | **<span style="color: #0969da;">1.385</span>** | **<span style="color: #fefdfe;">1.005×</span>** | Within&nbsp;5% | — | 1.487 | 1.07× | no |
| vec4_add | 32 | **<span style="color: #cf222e;">0.977</span>** | **<span style="color: #1a7f37;">0.807</span>** | **<span style="color: #c3b0e5;">1.210×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +21.0% | 1.366 | 1.69× | yes |
| vec4_add | 64 | **<span style="color: #0969da;">1.685</span>** | **<span style="color: #0969da;">1.685</span>** | **<span style="color: #ffffff;">1.000×</span>** | Within&nbsp;5% | — | 1.819 | 1.08× | no |
| vec4_dot | 32 | **<span style="color: #cf222e;">0.416</span>** | **<span style="color: #1a7f37;">0.287</span>** | **<span style="color: #7e55c7;">1.449×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +44.9% | 0.850 | 2.96× | yes |
| vec4_dot | 64 | **<span style="color: #0969da;">0.774</span>** | **<span style="color: #0969da;">0.773</span>** | **<span style="color: #fefeff;">1.002×</span>** | Within&nbsp;5% | — | 1.171 | 1.52× | no |
| vec4_length | 32 | **<span style="color: #cf222e;">0.394</span>** | **<span style="color: #1a7f37;">0.238</span>** | **<span style="color: #6f42c1;">1.655×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +65.5% | 0.666 | 2.80× | yes |
| vec4_length | 64 | **<span style="color: #0969da;">0.474</span>** | **<span style="color: #0969da;">0.475</span>** | **<span style="color: #ffffff;">1.000×</span>** | Within&nbsp;5% | — | 0.617 | 1.30× | no |
| vec4_normalize | 32 | **<span style="color: #cf222e;">0.633</span>** | **<span style="color: #1a7f37;">0.394</span>** | **<span style="color: #6f42c1;">1.608×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +60.8% | 1.538 | 3.90× | yes |
| vec4_normalize | 64 | **<span style="color: #0969da;">0.948</span>** | **<span style="color: #0969da;">0.952</span>** | **<span style="color: #fefefd;">0.996×</span>** | Within&nbsp;5% | — | 1.676 | 1.76× | yes |
| vec4_scale_add | 32 | **<span style="color: #cf222e;">0.976</span>** | **<span style="color: #1a7f37;">0.803</span>** | **<span style="color: #c1aee4;">1.215×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +21.5% | 1.373 | 1.71× | yes |
| vec4_scale_add | 64 | **<span style="color: #0969da;">1.689</span>** | **<span style="color: #0969da;">1.685</span>** | **<span style="color: #fefeff;">1.002×</span>** | Within&nbsp;5% | — | 1.819 | 1.08× | no |

## 4,096 objects — Structure of Arrays (SoA): SIMD across objects / logical 256 bits

<!-- standard-math-table-notes -->
- **<span style="color: #bc4c00;">sse2neon</span>:** SSE operations translated to Neon SIMD.
- **<span style="color: #6f42c1;">Direct&nbsp;Neon</span>:** native Neon SIMD.
- **(nsec/op):** nanoseconds per complete vector/matrix operation; lower is better.
- **Scalar:** float/double arithmetic without SIMD; auto-vectorization disabled.
<!-- /standard-math-table-notes -->

| Operation | FP | **<span style="color: #bc4c00;">sse2neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span> /<br><span style="color: #bc4c00;">sse2neon</span>** | **Winning<br>backend** | **Throughput<br>gain&nbsp;(%)** | **Scalar<br>(nsec/op)** | Scalar /<br><span style="color: #6f42c1;">Direct&nbsp;Neon</span> | Consistent<br>Timing |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- |
| mat4_inverse_trs | 32 | **<span style="color: #cf222e;">3.339</span>** | **<span style="color: #1a7f37;">2.213</span>** | **<span style="color: #6f42c1;">1.509×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +50.9% | 4.628 | 2.09× | yes |
| mat4_inverse_trs | 64 | **<span style="color: #0969da;">3.530</span>** | **<span style="color: #0969da;">3.557</span>** | **<span style="color: #fefcfb;">0.992×</span>** | Within&nbsp;5% | — | 4.754 | 1.34× | no |
| mat4_multiply | 32 | **<span style="color: #cf222e;">7.271</span>** | **<span style="color: #1a7f37;">6.017</span>** | **<span style="color: #c3b0e5;">1.208×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +20.8% | 18.633 | 3.10× | yes |
| mat4_multiply | 64 | **<span style="color: #0969da;">10.367</span>** | **<span style="color: #0969da;">10.244</span>** | **<span style="color: #fcfafe;">1.012×</span>** | Within&nbsp;5% | — | 19.719 | 1.92× | no |
| mat4_transpose | 32 | **<span style="color: #cf222e;">3.331</span>** | **<span style="color: #1a7f37;">2.330</span>** | **<span style="color: #835dca;">1.429×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +42.9% | 4.577 | 1.96× | yes |
| mat4_transpose | 64 | **<span style="color: #0969da;">3.733</span>** | **<span style="color: #0969da;">3.737</span>** | **<span style="color: #fffffe;">0.999×</span>** | Within&nbsp;5% | — | 4.587 | 1.23× | no |
| mat4_vector | 32 | **<span style="color: #cf222e;">2.219</span>** | **<span style="color: #1a7f37;">1.579</span>** | **<span style="color: #8a66cd;">1.405×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +40.5% | 3.122 | 1.98× | yes |
| mat4_vector | 64 | **<span style="color: #0969da;">2.564</span>** | **<span style="color: #0969da;">2.537</span>** | **<span style="color: #fcfbfe;">1.011×</span>** | Within&nbsp;5% | — | 3.201 | 1.26× | no |
| vec3_cross | 32 | **<span style="color: #cf222e;">0.640</span>** | **<span style="color: #1a7f37;">0.567</span>** | **<span style="color: #daceef;">1.129×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +12.9% | 1.147 | 2.02× | yes |
| vec3_cross | 64 | **<span style="color: #0969da;">1.362</span>** | **<span style="color: #0969da;">1.362</span>** | **<span style="color: #ffffff;">1.000×</span>** | Within&nbsp;5% | — | 1.487 | 1.09× | no |
| vec4_add | 32 | **<span style="color: #cf222e;">0.886</span>** | **<span style="color: #1a7f37;">0.762</span>** | **<span style="color: #d0c2eb;">1.162×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +16.2% | 1.366 | 1.79× | yes |
| vec4_add | 64 | **<span style="color: #0969da;">1.705</span>** | **<span style="color: #0969da;">1.701</span>** | **<span style="color: #fefeff;">1.002×</span>** | Within&nbsp;5% | — | 1.819 | 1.07× | no |
| vec4_dot | 32 | **<span style="color: #cf222e;">0.313</span>** | **<span style="color: #1a7f37;">0.288</span>** | **<span style="color: #e6def4;">1.086×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +8.6% | 0.850 | 2.95× | yes |
| vec4_dot | 64 | **<span style="color: #0969da;">0.777</span>** | **<span style="color: #0969da;">0.775</span>** | **<span style="color: #fefeff;">1.003×</span>** | Within&nbsp;5% | — | 1.171 | 1.51× | no |
| vec4_length | 32 | **<span style="color: #cf222e;">0.392</span>** | **<span style="color: #1a7f37;">0.220</span>** | **<span style="color: #6f42c1;">1.786×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +78.6% | 0.666 | 3.03× | yes |
| vec4_length | 64 | **<span style="color: #0969da;">0.417</span>** | **<span style="color: #0969da;">0.412</span>** | **<span style="color: #fcfafe;">1.012×</span>** | Within&nbsp;5% | — | 0.617 | 1.50× | no |
| vec4_normalize | 32 | **<span style="color: #cf222e;">0.618</span>** | **<span style="color: #1a7f37;">0.408</span>** | **<span style="color: #6f42c1;">1.515×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +51.5% | 1.538 | 3.77× | yes |
| vec4_normalize | 64 | **<span style="color: #0969da;">0.917</span>** | **<span style="color: #0969da;">0.916</span>** | **<span style="color: #ffffff;">1.000×</span>** | Within&nbsp;5% | — | 1.676 | 1.83× | no |
| vec4_scale_add | 32 | **<span style="color: #cf222e;">0.888</span>** | **<span style="color: #1a7f37;">0.764</span>** | **<span style="color: #d0c2eb;">1.162×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +16.2% | 1.373 | 1.80× | yes |
| vec4_scale_add | 64 | **<span style="color: #0969da;">1.708</span>** | **<span style="color: #0969da;">1.702</span>** | **<span style="color: #fefdff;">1.004×</span>** | Within&nbsp;5% | — | 1.819 | 1.07× | yes |

## 4,096 objects — Structure of Arrays (SoA): SIMD across objects / logical 512 bits

<!-- standard-math-table-notes -->
- **<span style="color: #bc4c00;">sse2neon</span>:** SSE operations translated to Neon SIMD.
- **<span style="color: #6f42c1;">Direct&nbsp;Neon</span>:** native Neon SIMD.
- **(nsec/op):** nanoseconds per complete vector/matrix operation; lower is better.
- **Scalar:** float/double arithmetic without SIMD; auto-vectorization disabled.
<!-- /standard-math-table-notes -->

| Operation | FP | **<span style="color: #bc4c00;">sse2neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span><br>(nsec/op)** | **<span style="color: #6f42c1;">Direct&nbsp;Neon</span> /<br><span style="color: #bc4c00;">sse2neon</span>** | **Winning<br>backend** | **Throughput<br>gain&nbsp;(%)** | **Scalar<br>(nsec/op)** | Scalar /<br><span style="color: #6f42c1;">Direct&nbsp;Neon</span> | Consistent<br>Timing |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- |
| mat4_inverse_trs | 32 | **<span style="color: #cf222e;">4.420</span>** | **<span style="color: #1a7f37;">4.159</span>** | **<span style="color: #ede7f7;">1.063×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +6.3% | 4.628 | 1.11× | yes |
| mat4_inverse_trs | 64 | **<span style="color: #0969da;">6.741</span>** | **<span style="color: #0969da;">6.762</span>** | **<span style="color: #fffefd;">0.997×</span>** | Within&nbsp;5% | — | 4.754 | 0.70× | no |
| mat4_multiply | 32 | **<span style="color: #cf222e;">7.343</span>** | **<span style="color: #1a7f37;">6.818</span>** | **<span style="color: #e9e2f5;">1.077×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +7.7% | 18.633 | 2.73× | yes |
| mat4_multiply | 64 | **<span style="color: #0969da;">12.414</span>** | **<span style="color: #0969da;">12.346</span>** | **<span style="color: #fefdfe;">1.005×</span>** | Within&nbsp;5% | — | 19.719 | 1.60× | no |
| mat4_transpose | 32 | **<span style="color: #cf222e;">4.073</span>** | **<span style="color: #1a7f37;">3.096</span>** | **<span style="color: #a488d8;">1.315×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +31.5% | 4.577 | 1.48× | yes |
| mat4_transpose | 64 | **<span style="color: #0969da;">6.490</span>** | **<span style="color: #0969da;">6.476</span>** | **<span style="color: #fefeff;">1.002×</span>** | Within&nbsp;5% | — | 4.587 | 0.71× | no |
| mat4_vector | 32 | **<span style="color: #cf222e;">2.633</span>** | **<span style="color: #1a7f37;">1.998</span>** | **<span style="color: #a387d8;">1.318×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +31.8% | 3.122 | 1.56× | yes |
| mat4_vector | 64 | **<span style="color: #0969da;">3.621</span>** | **<span style="color: #0969da;">3.609</span>** | **<span style="color: #fefeff;">1.003×</span>** | Within&nbsp;5% | — | 3.201 | 0.89× | no |
| vec3_cross | 32 | **<span style="color: #0969da;">0.540</span>** | **<span style="color: #0969da;">0.526</span>** | **<span style="color: #f8f5fc;">1.026×</span>** | Within&nbsp;5% | — | 1.147 | 2.18× | yes |
| vec3_cross | 64 | **<span style="color: #0969da;">1.321</span>** | **<span style="color: #0969da;">1.320</span>** | **<span style="color: #ffffff;">1.001×</span>** | Within&nbsp;5% | — | 1.487 | 1.13× | no |
| vec4_add | 32 | **<span style="color: #0969da;">0.720</span>** | **<span style="color: #0969da;">0.715</span>** | **<span style="color: #fdfcfe;">1.007×</span>** | Within&nbsp;5% | — | 1.366 | 1.91× | yes |
| vec4_add | 64 | **<span style="color: #0969da;">1.653</span>** | **<span style="color: #0969da;">1.650</span>** | **<span style="color: #ffffff;">1.001×</span>** | Within&nbsp;5% | — | 1.819 | 1.10× | no |
| vec4_dot | 32 | **<span style="color: #0969da;">0.275</span>** | **<span style="color: #0969da;">0.271</span>** | **<span style="color: #fbfafd;">1.014×</span>** | Within&nbsp;5% | — | 0.850 | 3.14× | yes |
| vec4_dot | 64 | **<span style="color: #0969da;">0.764</span>** | **<span style="color: #0969da;">0.763</span>** | **<span style="color: #ffffff;">1.001×</span>** | Within&nbsp;5% | — | 1.171 | 1.54× | no |
| vec4_length | 32 | **<span style="color: #cf222e;">0.394</span>** | **<span style="color: #1a7f37;">0.201</span>** | **<span style="color: #6f42c1;">1.961×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +96.1% | 0.666 | 3.31× | yes |
| vec4_length | 64 | **<span style="color: #0969da;">0.400</span>** | **<span style="color: #0969da;">0.400</span>** | **<span style="color: #ffffff;">1.000×</span>** | Within&nbsp;5% | — | 0.617 | 1.54× | no |
| vec4_normalize | 32 | **<span style="color: #cf222e;">0.623</span>** | **<span style="color: #1a7f37;">0.448</span>** | **<span style="color: #8f6ccf;">1.389×</span>** | <span style="color: #6f42c1;">Direct&nbsp;Neon</span> | +38.9% | 1.538 | 3.43× | yes |
| vec4_normalize | 64 | **<span style="color: #0969da;">0.921</span>** | **<span style="color: #0969da;">0.926</span>** | **<span style="color: #fefdfc;">0.994×</span>** | Within&nbsp;5% | — | 1.676 | 1.81× | no |
| vec4_scale_add | 32 | **<span style="color: #0969da;">0.757</span>** | **<span style="color: #0969da;">0.727</span>** | **<span style="color: #f3f0fa;">1.040×</span>** | Within&nbsp;5% | — | 1.373 | 1.89× | yes |
| vec4_scale_add | 64 | **<span style="color: #0969da;">1.656</span>** | **<span style="color: #0969da;">1.683</span>** | **<span style="color: #fdf9f7;">0.984×</span>** | Within&nbsp;5% | — | 1.819 | 1.08× | yes |
