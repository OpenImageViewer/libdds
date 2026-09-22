# libdds

A high-performance, standalone CPU-only port of [DirectXTex 2.1.1](https://github.com/microsoft/DirectXTex),
designed to preserve as much of the original upstream code as possible. The commit
history clearly separates **unmodified extraction → correctness fixes → optimizations**.

It retains the DirectX namespace and CPU image-processing APIs without WIC, COM,
WRL or Direct3D runtime dependencies.

## History

| Layer | Contents |
| --- | --- |
| DirectXTex 2.1.1 extraction | 22 unmodified upstream CPU files and license, followed by bundled dependencies, with only line endings normalized. |
| Build and correctness | Portable build, alignment/rounding fixes, overflow checks, DDS validation, tests and CI. |
| Optimizations | Caller-owned output, borrowed DDS views, prepared conversion, SIMD packing, writer dispatch, BC4/BC5 palette caching and unity grouping. |

Upstream files are imported unmodified before portability changes, correctness
fixes and optimizations. Upstream updates follow the same order.

## Performance against DirectXTex 2.1.1

**AMD Ryzen 9 9950X** · Windows x64 · Clang-CL 22.1.8 Release.
Each image includes its **complete mip chain**.
Inputs are in memory and use the same repeated random-colour tile in both builds.

The optimized build uses borrowed DDS input and writes decoded/converted pixels
into freshly allocated caller buffers, without pooling. All times are **ms per
image, lower is better**. Both speedups use **DirectXTex 2.1.1, single-thread**
within each table as their baseline and are calculated from unrounded measurements.

<img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt=""> 2–<3×&nbsp;&nbsp; <img src="docs/benchmarks/gain-strong.svg" width="4" height="14" alt=""> 3–<5×&nbsp;&nbsp; <img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt=""> <strong>≥5×</strong>. Green marks stable gains. Uncertain results remain plain with a note.

### Warm-cache input

One input per worker is reused after warmup. Large inputs may exceed CPU caches.

<table>
<thead>
<tr><th rowspan="2" align="left">Base size</th><th rowspan="2" align="left">Format</th><th rowspan="2">DirectXTex 2.1.1<br><sub>1 worker</sub></th><th colspan="2">Optimized<br><sub>1 worker</sub></th><th colspan="2">Optimized parallel<sup><a href="#benchmark-note-1">1</a></sup><br><sub>8 workers</sub></th></tr>
<tr><th>Time</th><th>Speedup</th><th>Time</th><th>Speedup</th></tr>
</thead>
<tbody>
<tr><th rowspan="6" align="left" valign="top">64&nbsp;×&nbsp;64</th><td>BGRA8<sup><a href="#benchmark-note-2">2</a></sup></td><td align="right">0.000982&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">0.000266&nbsp;ms</td><td align="right">3.70×<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">0.000051&nbsp;ms</td><td align="right">19.18×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BGRX8</td><td align="right">0.0107&nbsp;ms</td><td align="right">0.0058&nbsp;ms</td><td align="right">1.83×</td><td align="right">0.0014&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>7.76×</strong></td></tr>
<tr><td>BC1</td><td align="right">0.0321&nbsp;ms</td><td align="right">0.0201&nbsp;ms</td><td align="right">1.60×</td><td align="right">0.0033&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">9.68×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC3</td><td align="right">0.0344&nbsp;ms</td><td align="right">0.0218&nbsp;ms</td><td align="right">1.57×</td><td align="right">0.0040&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">8.63×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC4</td><td align="right">0.0187&nbsp;ms</td><td align="right">0.0058&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-strong.svg" width="4" height="14" alt="">&nbsp;3.25×</td><td align="right">0.0015&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">12.74×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC5</td><td align="right">0.0237&nbsp;ms</td><td align="right">0.0062&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-strong.svg" width="4" height="14" alt="">&nbsp;3.81×</td><td align="right">0.0015&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">16.20×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><th rowspan="6" align="left" valign="top">1024&nbsp;×&nbsp;1024</th><td>BGRA8</td><td align="right">1.90&nbsp;ms</td><td align="right">0.19&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>10.08×</strong></td><td align="right">0.11&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">17.51×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BGRX8</td><td align="right">4.91&nbsp;ms</td><td align="right">2.07&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.37×</td><td align="right">0.56&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">8.81×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC1</td><td align="right">5.96&nbsp;ms</td><td align="right">2.34&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.55×</td><td align="right">0.54&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">11.07×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC3</td><td align="right">6.95&nbsp;ms</td><td align="right">2.69&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.58×</td><td align="right">0.56&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">12.41×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC4</td><td align="right">6.23&nbsp;ms</td><td align="right">2.23&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.79×</td><td align="right">0.52&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">12.01×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC5</td><td align="right">7.78&nbsp;ms</td><td align="right">2.31&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-strong.svg" width="4" height="14" alt="">&nbsp;3.37×</td><td align="right">0.48&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">16.22×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><th rowspan="6" align="left" valign="top">2048&nbsp;×&nbsp;2048</th><td>BGRA8</td><td align="right">8.95&nbsp;ms</td><td align="right">0.79&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">11.32×<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">0.78&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>11.54×</strong></td></tr>
<tr><td>BGRX8</td><td align="right">20.94&nbsp;ms</td><td align="right">9.29&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.25×</td><td align="right">2.33&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">9.00×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC1</td><td align="right">22.54&nbsp;ms</td><td align="right">9.32&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.42×</td><td align="right">2.07&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">10.91×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC3</td><td align="right">26.29&nbsp;ms</td><td align="right">11.24&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">2.34×<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">2.39&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">11.00×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC4</td><td align="right">24.81&nbsp;ms</td><td align="right">10.07&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.46×</td><td align="right">2.05&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">12.10×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC5</td><td align="right">30.62&nbsp;ms</td><td align="right">10.38&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.95×</td><td align="right">2.20&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">13.89×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><th rowspan="6" align="left" valign="top">8192&nbsp;×&nbsp;8192</th><td>BGRA8</td><td align="right">145.48&nbsp;ms</td><td align="right">18.55&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>7.84×</strong></td><td align="right">13.15&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>11.06×</strong></td></tr>
<tr><td>BGRX8</td><td align="right">353.73&nbsp;ms</td><td align="right">147.27&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.40×</td><td align="right">31.98&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>11.06×</strong></td></tr>
<tr><td>BC1</td><td align="right">336.99&nbsp;ms</td><td align="right">148.61&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.27×</td><td align="right">35.82&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>9.41×</strong></td></tr>
<tr><td>BC3</td><td align="right">387.57&nbsp;ms</td><td align="right">174.53&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.22×</td><td align="right">42.04&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">9.22×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC4</td><td align="right">391.26&nbsp;ms</td><td align="right">170.10&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.30×</td><td align="right">38.49&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>10.17×</strong></td></tr>
<tr><td>BC5</td><td align="right">489.11&nbsp;ms</td><td align="right">185.61&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.64×</td><td align="right">45.92&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">10.65×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
</tbody>
</table>

### Cold/streaming input<sup><a href="#benchmark-note-4">4</a></sup>

<table>
<thead>
<tr><th rowspan="2" align="left">Base size</th><th rowspan="2" align="left">Format</th><th rowspan="2">DirectXTex 2.1.1<br><sub>1 worker</sub></th><th colspan="2">Optimized<br><sub>1 worker</sub></th><th colspan="2">Optimized parallel<sup><a href="#benchmark-note-1">1</a></sup><br><sub>8 workers</sub></th></tr>
<tr><th>Time</th><th>Speedup</th><th>Time</th><th>Speedup</th></tr>
</thead>
<tbody>
<tr><th rowspan="6" align="left" valign="top">64&nbsp;×&nbsp;64</th><td>BGRA8</td><td align="right">0.0020&nbsp;ms</td><td align="right">0.0012&nbsp;ms</td><td align="right">1.72×</td><td align="right">0.000453&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-strong.svg" width="4" height="14" alt="">&nbsp;4.42×</td></tr>
<tr><td>BGRX8</td><td align="right">0.0130&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">0.0063&nbsp;ms</td><td align="right">2.06×<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">0.0031&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">4.15×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC1</td><td align="right">0.0336&nbsp;ms</td><td align="right">0.0214&nbsp;ms</td><td align="right">1.57×</td><td align="right">0.0039&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">8.57×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC3</td><td align="right">0.0357&nbsp;ms</td><td align="right">0.0222&nbsp;ms</td><td align="right">1.61×</td><td align="right">0.0037&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">9.72×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC4</td><td align="right">0.0207&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">0.0060&nbsp;ms</td><td align="right">3.46×<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">0.0011&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">19.55×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC5</td><td align="right">0.0249&nbsp;ms</td><td align="right">0.0063&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-strong.svg" width="4" height="14" alt="">&nbsp;3.93×</td><td align="right">0.0011&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">22.80×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><th rowspan="6" align="left" valign="top">1024&nbsp;×&nbsp;1024</th><td>BGRA8</td><td align="right">2.05&nbsp;ms</td><td align="right">0.30&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>6.81×</strong></td><td align="right">0.18&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>11.33×</strong></td></tr>
<tr><td>BGRX8</td><td align="right">5.09&nbsp;ms</td><td align="right">2.45&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.08×</td><td align="right">0.50&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>10.18×</strong></td></tr>
<tr><td>BC1</td><td align="right">6.04&nbsp;ms</td><td align="right">2.32&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.61×</td><td align="right">0.47&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>12.88×</strong></td></tr>
<tr><td>BC3</td><td align="right">7.53&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">2.78&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">2.71×<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">0.63&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">11.99×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC4</td><td align="right">7.12&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">2.29&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">3.10×<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">0.54&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">13.30×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC5</td><td align="right">8.06&nbsp;ms</td><td align="right">2.29&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-strong.svg" width="4" height="14" alt="">&nbsp;3.52×</td><td align="right">0.43&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>18.54×</strong></td></tr>
<tr><th rowspan="6" align="left" valign="top">2048&nbsp;×&nbsp;2048</th><td>BGRA8</td><td align="right">9.22&nbsp;ms</td><td align="right">1.15&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>7.99×</strong></td><td align="right">0.78&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>11.90×</strong></td></tr>
<tr><td>BGRX8</td><td align="right">21.65&nbsp;ms</td><td align="right">9.48&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.28×</td><td align="right">1.98&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">10.93×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC1</td><td align="right">23.01&nbsp;ms</td><td align="right">9.53&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.41×</td><td align="right">1.93&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">11.90×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC3</td><td align="right">26.08&nbsp;ms</td><td align="right">11.30&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">2.31×<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">2.50&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">10.42×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC4</td><td align="right">25.78&nbsp;ms</td><td align="right">10.33&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.50×</td><td align="right">2.02&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">12.78×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC5</td><td align="right">31.80&nbsp;ms</td><td align="right">10.79&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.95×</td><td align="right">2.36&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">13.50×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><th rowspan="6" align="left" valign="top">8192&nbsp;×&nbsp;8192</th><td>BGRA8</td><td align="right">156.49&nbsp;ms</td><td align="right">18.18&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>8.61×</strong></td><td align="right">12.98&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>12.05×</strong></td></tr>
<tr><td>BGRX8</td><td align="right">357.41&nbsp;ms</td><td align="right">149.59&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.39×</td><td align="right">38.34&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>9.32×</strong></td></tr>
<tr><td>BC1</td><td align="right">339.02&nbsp;ms</td><td align="right">155.33&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">2.18×<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">41.44&nbsp;ms<sup><a href="#benchmark-note-3">3</a></sup></td><td align="right">8.18×<sup><a href="#benchmark-note-3">3</a></sup></td></tr>
<tr><td>BC3</td><td align="right">387.93&nbsp;ms</td><td align="right">179.34&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.16×</td><td align="right">41.31&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>9.39×</strong></td></tr>
<tr><td>BC4</td><td align="right">411.62&nbsp;ms</td><td align="right">180.84&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.28×</td><td align="right">42.65&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>9.65×</strong></td></tr>
<tr><td>BC5</td><td align="right">504.46&nbsp;ms</td><td align="right">187.46&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-good.svg" width="4" height="14" alt="">&nbsp;2.69×</td><td align="right">44.97&nbsp;ms</td><td align="right"><img src="docs/benchmarks/gain-major.svg" width="4" height="14" alt="">&nbsp;<strong>11.22×</strong></td></tr>
</tbody>
</table>

For native BGRA8, the optimized path borrows input pixels. Every mode copies
all output pixels to a consumer buffer. Output hashes match across both implementations
and all workers. Results include correctness fixes and API/allocation changes,
and depend on the input pattern and machine.

<a id="benchmark-note-1"></a><sup>1</sup> Batch time divided by all images completed across eight workers on eight
performance cores. Each worker processes whole images serially. This measures
throughput. The speedup includes optimization and concurrency against the
DirectXTex 2.1.1 single-thread baseline.

<a id="benchmark-note-2"></a><sup>2</sup> This uncompressed mip chain is only 21.3 KiB and is repeatedly processed after
warmup, favoring CPU caches. The tiny parallel value represents combined
throughput, not individual-image latency.

<a id="benchmark-note-3"></a><sup>3</sup> Timing variation exceeds 5%, or the speedup uses such a measurement.
Treat the result as indicative.

<a id="benchmark-note-4"></a><sup>4</sup> Each worker cycles through independent input buffers totaling at least
128 MiB and two images, exceeding the shared CPU cache. Inputs stay in RAM.
Code is warmed up and disk I/O is excluded. Hardware prefetching and cache
hits remain possible.

## Build

Requires CMake 3.20 and C++17. Dependencies are bundled.

```sh
cmake -S . -B build/libdds -DCMAKE_BUILD_TYPE=Release
cmake --build build/libdds --config Release
ctest --test-dir build/libdds -C Release --output-on-failure
```

For embedding, use `add_subdirectory` and link `libdds::libdds`. Include
`DirectXTex.h`. `LIBDDS_BUILD_TESTS` defaults on for standalone builds and off
when embedded. Do not link libdds and full DirectXTex into the same executable.

## Further reading

### Where libdds gains performance

The performance work targets two costs around DirectXTex's CPU algorithms: moving
pixels between allocations and repeating decisions inside pixel loops. The changes
remove that work while reusing existing decoding and conversion arithmetic. The
[performance section above](#performance-against-directxtex-211) contains the measured gains.
This section explains how the commits produce them.

#### Avoid moving the same pixels twice

An application may already hold both the DDS bytes and its final output buffer.
Loading through an owning image and decoding into another temporary image adds
storage work before those pixels reach their destination.

The *validated DDS views* commit removes the input copy for eligible layouts.
Its [view loader](DirectXTex/DirectXTexDDS.cpp#L2027) validates the DDS and creates
surface descriptors pointing into the caller's bytes. Native pixels can be consumed
there. Compressed pixels still need decoding, but their encoded data need not be
copied first. Layouts requiring repair or stronger alignment retain the owning
fallback, and borrowed input must stay alive and unchanged.

The *caller-owned decompression* and *caller-owned conversion* commits remove the
other intermediate: they write directly into the application's destination.
This avoids allocating and initializing a whole-image result only to copy it again.
Conversion can still allocate temporary scanlines. These storage savings require callers
to adopt the new APIs. Existing allocating overloads remain available.

#### Resolve setup once, then process pixels

After removing avoidable storage work, repeated format handling becomes another
source of cost. The *prepared scanline conversion* commit resolves format metadata
and transfer flags once per image in CPU conversion and BC encoding/decoding.
[Scanline processing](DirectXTex/DirectXTexConvert.cpp#L3154) reuses that state and
skips conversion arithmetic when numeric and channel semantics already match and
no transfer-function change remains.

The *decompression writer selection* commit similarly chooses the RGBA8 or generic
output path before the block loop. The *batched RGBA8 packing* commit gives
conversion and decompression a [shared writer](DirectXTex/DirectXTexConvert.cpp#L1655)
that packs four pixels at a time using SSE2 or NEON. It preserves the existing
rounding order and stores the remaining pixels individually. The *unity
grouping* commit makes these shared stores available for compiler inlining by
building compression and conversion together.

#### Reuse calculations within each block

BC4 and BC5 have another repeated calculation: pixels select from eight
interpolated values per channel. The *interpolation palette caching* commit
[computes those values once per block](DirectXTex/BC4BC5.cpp#L398), then indexes them
for each pixel. It retains the original interpolation formulas and signed-endpoint
rules. The earlier *unaligned BC4/BC5 payload safety* fix supplies safe local word
loads independently of this optimization.

These changes affect different workloads differently. Native images benefit mainly
from avoided allocation and copying. Compressed images also benefit from reduced
setup, packing and interpolation work, but still pay for decoding and writing every
output pixel. Their combined speedups cannot be attributed to any one commit.

#### Verify that work disappeared and pixels stayed correct

The development profiles record the
disappearance of per-block format-table searches. Later
borrowed-view profiles show the
owning input allocation/copy path disappearing for the measured array workload.
These observations support the implementation's intended savings. The end-to-end
benchmarks measure the combined effect.

The [tests](Tests/Tests.cpp) compare borrowed and owning paths, check padded outputs
and guards, and exercise rounding, transfer functions and partial blocks. Exhaustive
BC4/BC5 endpoint/index cases feed upstream comparisons. The commit notes report
SIMD/scalar CPU and upstream-parity validation.

### License

[MIT license](LICENSE). Bundled dependency licenses remain in their directories.
