# C++23, compile-time computation, and IndiTrans performance

Assessment date: 2026-09-26. Repository: current `dev` branch, `d8b7140`.

This is a separate technical assessment of C++ and its toolchain. It does not change the pronunciation exploration, select a linguistic architecture, or upgrade the repository's language standard.

## Conclusion

**Continue using C++ as the implementation language. C++23 provides useful incremental benefits, especially for compile-time utilities, but the largest opportunities are data layout and allocation reduction. Most of those are already possible in C++20.**

For IndiTrans, the strongest direction is:

- Immutable, contiguous script data and lookup structures, prepared before runtime.
- Small value types, explicit buffer ownership, and bounded temporary storage.
- Ordinary functions and selectively templated policies that the compiler can inline.
- Compile-time validation of indices, table dimensions, mappings, and supported combinations.
- C++23 features where they simplify these mechanisms enough to justify the compiler/library requirements.

Changing `-std=c++20` to `-std=c++23` does not automatically improve speed or memory use. A newer optimizer can improve C++20 code too. The standard version, compiler version, standard library, build flags, and data representation are separate variables.

## 1. What the current implementation makes possible

Source evidence comes from [inditrans.cpp](../native/src/inditrans.cpp), [type_defs.h](../native/src/type_defs.h), [utilities.h](../native/src/utilities.h), [trie.h](../native/src/trie.h), and [the data generator](../tool/utils/script_data.dart).

| Current mechanism | Opportunity | C++23 required? |
|---|---|---|
| Compiled script blob parsed into maps/vectors on first use | Emit fixed descriptors and offset tables ahead of time | No |
| Heap nodes and `unordered_map` children in tokenizer tries | Generate flat nodes/edges with compact offsets | No |
| Writer copies views into vectors | Reference immutable indexed arrays directly | No |
| Cached maps keyed by strings | Resolve a name once to a compact internal script identifier | No |
| Three-entry Tamil substitution hash map held by writer | Small constant array or direct switch | No |
| Entire input scanned into a variant vector; capacity reserved by byte count | Better capacity accounting or bounded buffering where semantics permit | No |
| Small token/index representation | Preserve compact values; validate table compatibility before runtime | No |
| Script-specific branches mixed into reader/writer | Named policy functions, with selective compile-time specialization | No |

These are opportunities, not established speedups. A flat trie can reduce allocation and improve locality, but a poor edge layout may increase lookup work. Replacing a table with many specialized instructions can reduce branches while enlarging the Wasm binary and instruction-cache footprint.

The existing unused [char_trie.h](../native/src/char_trie.h) is not a proven static-trie generator. Its packed layout, 16-bit offsets, and token reconstruction need review before reuse. Prefer an intentionally specified representation over assuming that an existing packed struct is portable or correct.

## 2. Metaprogramming already available in C++20

### 2.1 Constant evaluation and static data

`constexpr` functions can compute tables and validate relationships at compile time when used in a constant-expression context. They can also run at runtime. `consteval` requires an immediate invocation to be evaluated at compile time. `constinit` enforces static initialization but does not make an object immutable. None of these keywords independently guarantees small code or fast runtime execution. [Immediate functions proposal](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1073r3.html), [Clang language support](https://clang.llvm.org/cxx_status.html).

For IndiTrans, useful compile-time work includes:

- Checking that every script's category arrays obey the shared inventory sizes.
- Validating token indices, expansion references, edge offsets, and terminal values.
- Computing small consonant-class masks and capability tables.
- Sorting/deduplicating lookup keys while distinguishing intentional aliases from conflicting mappings.
- Checking that generated edges are ordered and all indices fit their chosen integer widths.
- Selecting code paths for a small number of known behavior profiles.

Compile-time validation catches structural mistakes. It cannot establish that a linguistic mapping is correct or that all possible input strings are handled correctly.

### 2.2 Templates, concepts, and `if constexpr`

Templates and `if constexpr` can remove disabled functionality from a specialization. Concepts can express policy requirements clearly, such as supported input/output types or required operations. Concepts improve interface checking; they do not themselves accelerate execution.

A useful scale is a handful of behavior families: direct table conversion, conversion with bounded contextual handling, or an optional analysis path. Avoid generating a separate complete loop for every source script × target script × option combination. Keep script data runtime-selectable where sharing it is cheap.

Class-valued non-type template parameters can encode small configuration objects in C++20. They should describe behavior, not turn an entire script database into template arguments. Large type-level datasets produce verbose errors, long builds, and potentially many redundant instantiations.

### 2.3 Compile-time allocation has an important limit

C++20 permits transient allocation during constant evaluation for supported operations: allocated memory must be released within that evaluation. This enables temporary work with constexpr-capable containers, but does not generally let a heap-backed vector escape evaluation as persistent static storage. C++23 does not turn the current map/heap-trie graph into a permanent constexpr object. [P0784R7](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0784r7.html).

The durable result should normally be fixed arrays, integral offsets, and descriptors. A builder may calculate counts first and materialize fixed storage second. Alternatively, the existing external generator can emit the final arrays directly. `consteval` is not a mechanism for reading arbitrary files during C++ compilation.

## 3. What C++23 adds

### 3.1 Features directly useful for compile-time work

**`if consteval`.** A function can select a path for constant evaluation and a different path for runtime execution. This is useful for a simple compile-time validator and an optimized runtime implementation sharing one interface. It is different from `if constexpr`: the latter chooses code during template instantiation based on a constant condition. Neither selects a runtime input's language or script at compile time. [P1938R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1938r3.html).

**More flexible `constexpr` functions.** C++23 relaxes restrictions on their definitions, making reusable generic utilities easier to express. A particular invocation must still satisfy constant-expression rules to run at compile time. This reduces friction; it does not make arbitrary library calls constexpr. [P2448R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2448r2.html).

**Static constexpr locals inside constexpr functions.** Small lookup tables can live next to the function that owns them while remaining available in constant evaluation. C++20 can obtain similar results with namespace/class-scope tables, so this is primarily a locality/readability benefit. [P2647R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2647r1.html).

Illustrative C++23 fragment, not a repository change or benchmark:

```cpp
#include <array>
#include <cstddef>
#include <cstdint>

constexpr std::uint8_t nasal_for_stop_series(std::size_t series) {
    static constexpr std::array<std::uint8_t, 5> nasals{4, 9, 14, 19, 24};
    return nasals.at(series);
}

static_assert(nasal_for_stop_series(2) == 14);
```

This demonstrates scoped constant data, not an anuswara algorithm. A real caller must determine whether a stop-series lookup applies and handle invalid input according to its contract. Moving `nasals` to namespace scope makes the basic design available in C++20. No runtime-performance advantage is implied by the C++23 spelling.

### 3.2 Other C++23 features and their practical value

| Feature | Possible benefit here | Qualification |
|---|---|---|
| More constexpr library support, including `bitset` and `unique_ptr` | Easier validators and temporary builders | Does not make standard maps constexpr or allow persistent constexpr heap graphs |
| `std::to_underlying` | Clear enum-to-index conversion | Equivalent casts are already inexpensive |
| `std::byteswap` | Portable byte-order conversion for data assets | Does not solve alignment, lifetime, or bounds validation |
| `std::expected` | Explicit parse/load errors without exceptions on ordinary error paths | Improves API clarity; checked access must respect exception-disabled builds |
| `std::flat_map` / `flat_set` | Contiguous associative storage for some dynamic tables | Default storage still allocates; fixed generated arrays can be simpler for immutable data |
| `std::mdspan` | Non-owning view of multidimensional tables or optional numeric kernels | No allocation or automatic vectorization benefit; a simple span may suffice |

These library additions and their implementation status are tracked separately from language features. [libc++ C++23 status](https://libcxx.llvm.org/Status/Cxx23.html).

**`std::string::resize_and_overwrite`** deserves a separate caveat. It permits writing directly into string storage, avoiding unnecessary initialization/copying in suitable construction patterns. The current C ABI returns a buffer with explicit ownership; `std::string` cannot simply relinquish its allocation. Replacing the builder may introduce a final copy for that ABI. It is more immediately relevant to a C++ string-returning path or a revised caller-buffer interface than a drop-in optimization. [P1072R10](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1072r10.html).

**Explicit object parameters and static call operators** can simplify certain policy/helper patterns. Ordinary free functions or existing template techniques already offer static dispatch. Adopt the new syntax when it improves clarity, not because it promises faster generated code. **`[[assume]]` and `std::unreachable`** provide optimization information but are inappropriate for unchecked Unicode, loaded assets, or uncertain token indices: violating their assumptions can cause undefined behavior. [Clang feature status](https://clang.llvm.org/cxx_status.html), [GCC language status](https://gcc.gnu.org/projects/cxx-status.html).

### 3.3 What C++23 does not provide

- Standard static reflection or general automatic enum/member introspection.
- A standard SIMD facility that makes this scalar transliterator automatically vectorized.
- General constexpr `std::map`/`std::unordered_map` construction for the existing data structures.
- Automatic conversion of heap graphs into compact static arrays.
- Automatic removal of allocations, dynamic dispatch, or boundary copies.

Do not attribute later-standard facilities or compiler extensions to C++23. Modules and coroutine-based generators are also weak motivations here: this core has one main translation unit, and a generator can introduce state/lifetime/allocation costs without solving the input reader's buffering contract. Feature availability is documented in the [compiler](https://clang.llvm.org/cxx_status.html) and [library](https://libcxx.llvm.org/Status/Cxx23.html) support matrices.

## 4. Where computation should happen

| Stage | Appropriate work | Main cost |
|---|---|---|
| External data generation | Parse JSON, normalize keys, build/minimize large tries, emit portable arrays | Generator tooling and reproducibility |
| C++ constant evaluation | Validate generated structure; compute small masks/tables; select fixed profiles | Compiler time/memory and evaluation limits |
| Runtime initialization | Validate optional resource headers and bind immutable views | Cold-start time; avoid reconstructing large pointer graphs |
| Per conversion | Decode input, apply selected bounded policies, write output | Latency, allocations, cache behavior |

**Use both generated data and C++ metaprogramming.** The existing Dart authoring pipeline is not an obstacle to a highly optimized C++ runtime. It can emit layout-efficient data; C++ can validate it with `static_assert` and consume it without dynamic initialization.

Putting large JSON parsing or trie minimization into template instantiation would transfer work to every consumer's compiler. A build-time generator can run once when data changes, while distributed sources contain reproducible final tables. Small constexpr builders are still attractive when their entire logic remains easy to inspect.

For optional separately loaded resources, compile-time generation applies when producing the pack, not when the end user's library loads it. Runtime validation remains necessary. Built-in and loaded resources may share an offset-based view without sharing their ownership mechanism.

## 5. Memory layout and runtime costs matter more than syntax

### 5.1 Compact immutable tables

A plausible generated representation consists of a UTF-8 byte blob, arrays of offsets/lengths, per-script/category descriptors, and flat lookup nodes/edges. It can remove script parsing and much lazy map construction.

Offsets can be smaller than native pointers and avoid pointer relocation for each entry. For illustration, a common 64-bit `string_view` occupies 16 bytes, while two 32-bit fields occupy 8 bytes. That is an ABI-dependent comparison, not a measured saving for this repository. On Wasm32, pointer/length pairs are already commonly 8 bytes. Count metadata and padding before claiming gains.

Use the narrowest widths whose bounds are validated. A 16-bit offset is only appropriate if the indexed region fits; slicing data into blocks adds lookup complexity. Avoid packed, potentially unaligned native structs as a file format. Explicit integer layouts or aligned arrays are easier to share across native and Wasm.

Immutable tables also remove the need to synchronize mutations of their contents. They do not automatically make per-request state, initialization, or resource replacement thread-safe.

### 5.2 Allocation discipline

The current `reserve(input.size())` uses input bytes as a count of variant slots. Its reserved storage is roughly `input_bytes × sizeof(TokenOrString)`, excluding allocator overhead. An Indic UTF-8 byte is not a token; alias expansion also means code-point count is not a universal exact capacity bound. A replacement requires real counts or a carefully bounded growth/buffering strategy.

Prefer views into immutable data, explicit output-capacity growth, and reusable per-instance/request scratch space. `std::span` and `string_view` avoid ownership but do not extend lifetimes. Thread-local scratch can multiply memory across threads, while a global scratch buffer prevents independent concurrent use. A bounded arena may simplify temporary allocation, but its size and overflow behavior need an explicit contract.

Polymorphic allocators (`std::pmr`, available before C++23) can consolidate dynamic allocation where it remains necessary. They do not eliminate the node/hash overhead of the current trie. A flat representation may be more valuable than changing the allocator under an unchanged graph.

The present output builder uses `realloc`, while return/release paths use incompatible deletion mechanisms. Resolve that ownership contract before using its allocation behavior as a trusted baseline. C++23 smart-pointer features do not correct a mismatched deleter automatically.

### 5.3 Separation without expensive abstraction

Custom deterministic handling can live in named, directly called helpers or small policy types. A loop specialization can receive a policy by value/reference and inline it; state can remain in a compact request object. Extraction need not create an additional full-input pass, a callback allocation, or a virtual invocation per token.

Selectively specializing a loop can remove repeated capability checks. Conversely, specializing every script pair can expand executable code substantially. Compare:

1. One shared loop with direct helpers and compact capability flags.
2. A few loop variants chosen once per call.
3. Function-pointer dispatch at word/span granularity when code sharing matters more.

Existing functions already have inlining opportunities; adding templates alone may change nothing. Prefer readable procedural algorithms and compact tables over recursive template machinery when both can produce the same layout and assembly.

## 6. Compatibility with the actual deployment targets

### Repository findings

- [Native CMake](../native/src/CMakeLists.txt) requests C++20. It sets `CXX_STANDARD_REQUIRED`, whereas the documented CMake initialization variable is `CMAKE_CXX_STANDARD_REQUIRED` and the target property is `CXX_STANDARD_REQUIRED`; the existing plain variable does not enforce the target requirement.
- [Makefile](../Makefile) and most [shell](../tool/build_wasm.sh)/[PowerShell](../tool/build_wasm.ps1) Wasm recipes explicitly select C++20. The shell standalone release recipe omits a `-std=` setting.
- Native CMake advertises a 3.10 minimum. CMake added `CXX_STANDARD 23` support in 3.20, so a standard upgrade also needs an honest build-tool support policy. [CMake 3.20 release notes](https://cmake.org/cmake/help/latest/release/3.20.html), [CXX_STANDARD documentation](https://cmake.org/cmake/help/v4.0/prop_tgt/CXX_STANDARD.html).
- Apple podspecs declare iOS 9.0/macOS 10.11 targets but do not explicitly select a C++ language standard. Those declarations do not prove that the current package/toolchain works on those OS versions.
- Android uses the selected Android NDK configuration; CI does not establish a complete compiler-and-library floor for every distributed platform.
- Local executables report Clang 22.1.8 and GCC 16.2.1. That says nothing about consumers' NDK, Xcode, Emscripten, or MSVC libraries.

### Compiler, library, and runtime are separate checks

| Target | What needs verification before requiring C++23 |
|---|---|
| Linux/native | Compiler language features and the selected libstdc++/libc++; Clang can use either |
| Windows | Actual MSVC or MinGW toolchain, language mode, library availability, and packaging |
| Android | NDK Clang plus bundled libc++, build flags, and supported ABI/API configuration |
| Apple | Apple Clang language support, SDK headers, libc++ availability, deployment-target constraints |
| Browser/Node Wasm | Pinned Emscripten compiler/library combination, feature availability, emitted Wasm requirements |

Official matrices: [Clang](https://clang.llvm.org/cxx_status.html), [libstdc++](https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html), [libc++](https://libcxx.llvm.org/Status/Cxx23.html), [MSVC](https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance?view=msvc-170), [Apple](https://developer.apple.com/xcode/cpp/), [Android NDK](https://developer.android.com/ndk/guides/cpp-support).

Language-only improvements can have fewer runtime-deployment implications than library APIs requiring new runtime symbols. Check features individually using standard feature-test macros and small build probes; successful parsing of `-std=c++23` is insufficient. A few well-contained compatibility helpers can make sense, but a broad private replacement standard library would defeat simplicity.

Browsers do not interpret C++23. They execute the resulting Wasm; browser requirements come from emitted Wasm features and host APIs. C++23 does not inherently require SIMD, threads, or a new browser runtime. Emscripten's optimizer and linking configuration remain independent choices. [Emscripten optimization guide](https://emscripten.org/docs/optimizing/Optimizing-Code.html).

The C ABI can remain stable when internal C++ changes. Keep STL objects and template layouts inside the library, with explicit lengths and ownership at native/JS/Dart boundaries. This limits consumer coupling but does not remove the requirement to build the implementation with a supported compiler.

## 7. How to distinguish real improvements

There are three comparisons, each answering a different question:

| Comparison | What it measures |
|---|---|
| Same code/compiler/flags, C++20 versus C++23 | Effect of language mode and any mode-dependent library behavior |
| Same C++20 code, old versus new compiler | Optimizer/toolchain improvement |
| Same compiler, current structures versus static tables/policies | Value of the actual representation/implementation change |

Measure native and Wasm separately, with identical output checks. Record cold initialization, first script-pair use, warmed throughput, short-call latency, allocations, resident/peak memory, raw/compressed artifact size, compile time, and compiler peak memory. Include ordinary conversions, conversions requiring custom handling, mixed/preserved spans, and long inputs.

Do not use the Makefile's current `-O0 -g` native test/CLI builds to conclude that native or C++20 is slow. Compare optimized builds. `-O2`/`-O3`, `-Os`/`-Oz`, LTO, and profile-guided optimization trade speed against size differently; none is a C++23 feature. Emscripten has both LLVM and Wasm optimization stages, so measure the final linked artifact. [Emscripten optimization guide](https://emscripten.org/docs/optimizing/Optimizing-Code.html).

Static precomputation can reduce heap allocation while increasing read-only data or download size. A compile-time-generated structure still occupies runtime memory if lookup needs it. A fully specialized function may erase a data table but replace it with more instructions. Track total footprint instead of counting only heap bytes.

## 8. Adoption judgment

**C++23 is a reasonable target if the supported toolchains can be made consistent, but it is not a prerequisite for the desired architecture.**

The most compelling reasons to adopt it here are cleaner constant-evaluation utilities, more convenient scoped tables, and selected standard-library facilities that replace bespoke code. The strongest performance argument remains generating compact immutable data and keeping the runtime path small.

Three viable policies remain:

- **C++20 baseline:** implement the important data/layout optimizations now and retain broad compatibility.
- **C++23 baseline:** useful when a deliberate compiler/SDK support floor is acceptable; choose a small documented feature subset.
- **C++20 runtime with newer build-time tools:** generate optimized portable tables using a newer toolchain while consumers compile the runtime with C++20. The existing Dart generator already supports this separation in principle.

My preference is to design around flat immutable data, selective static dispatch, and compile-time validation first. Adopt C++23 when its concrete simplifications outweigh the platform support cost. Keep metaprogramming focused on producing and checking useful artifacts; avoid a generic framework whose compiler complexity exceeds the runtime work it removes.

## Validation and limits

This assessment is based on current source/build inspection and the linked standards proposals and official implementation documentation. No compiler flags, APIs, generated artifacts, or pronunciation documents were changed. The C++ fragment is explanatory and was not compiled. No performance measurements or cross-platform C++23 builds were run, so compatibility and speedups remain to be demonstrated.

Local document links and whitespace were checked. The repository documentation audit was used as an index; the release validator was also run. Neither validates the proposed C++23 techniques.
