# NFsim — Vendored Fork

This directory contains a vendored and modified copy of NFsim, the network-free
stochastic simulator for rule-based models.

## Provenance

- **Upstream**: https://github.com/RuleWorld/nfsim
- **Forked from**: commit `abb1291` (master, 2026-02-16)
- **License**: MIT (see `LICENSE.txt`)
- **Copyright**: 2016 Michael Sneddon, James Faeder, Thierry Emonet

## Modifications from upstream

All modifications serve the goal of embedding NFsim as a library inside bngsim.

### Step 1.5: muParser → ExprTk replacement
- **Removed**: `src/NFfunction/muParser/` (8 .cpp files, all headers)
- **Added**: `src/NFfunction/nfsim_funcparser.h` — drop-in `mu::Parser` replacement
  backed by ExprTk (header-only, C++17). Preserves the full muParser API surface.
- **Modified**: `src/NFfunction/NFfunction.hh` — includes `nfsim_funcparser.h`
- **Modified**: `src/NFfunction/{funcParser,function,compositeFunction,localFunction}.cpp`
  — removed `using namespace mu`, simplified constant registration

### Step 1.6: Per-System RNG
- **Added**: `src/NFutil/nfsim_rng.h` — per-instance MT19937 RNG class
- **Modified**: `src/NFcore/NFcore.hh` — `NfsimRNG rng_` member on System
- **Modified**: `src/NFcore/system.cpp` — uses `rng_.*` instead of global `RANDOM()`
- **Modified**: `src/NFcore/reactionSelector/{direct,logClass}Selector.cpp` — `sys_->getRNG()`
- **Modified**: `src/NFreactions/reactions/DORreaction.cpp` — `system->getRNG().random()`

### Step 1.7: ExprTk underscore fix
- **Modified**: `src/NFfunction/nfsim_funcparser.h` — `remap_name()` transparently
  maps `_X` → `u_X` for ExprTk compatibility (ExprTk rejects `_`-leading identifiers)

### Removed files
- `src/NFfunction/muParser/` — replaced by ExprTk
- `src/NFtest/` — test harnesses not needed for library use
- `src/NFutil/MTrand/mttest.cpp` — standalone MT test

## Build

This directory is built as `libnfsim.a` by `CMakeLists.txt` in this directory.
It is compiled as part of the bngsim build when `-DBNGSIM_BUILD_NFSIM=ON` is set.
No external `NFSIM_SOURCE_DIR` is needed — the source is self-contained here.
