# ─── NFsim as a static library ─────────────────────────────────────────────────
# Builds vendored NFsim source code as libnfsim.a for embedding inside bngsim.
#
# NFsim source: vendored in src/ (fork of https://github.com/RuleWorld/nfsim @ 9a97437)
# License: MIT (LICENSE.txt, Copyright 2016 Sneddon/Faeder/Emonet)
#
# This CMake file compiles all NFsim source files EXCEPT:
#   - main() in NFsim.cpp (we extract the library entry points only)
#   - NFtest/ directory (test harnesses not needed)
#   - muParser/ directory (replaced by ExprTk via nfsim_funcparser.h)
#   - mttest.cpp (standalone MT test)
#
# The resulting static library provides:
#   - NFinput::initializeFromXML()  → parse BNG XML into System*
#   - System::prepareForSimulation()
#   - System::sim() / System::stepTo()
#   - System::setParameter() / System::updateSystemWithNewParameters()
#   - System::seedRNG() / System::getRNG() (per-instance RNG)
# ────────────────────────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.20)

# NFsim source is vendored right here — no external path needed
set(NFSIM_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ─── Collect all NFsim .cpp and .c source files ───────────────────────────────

set(NFSIM_SOURCES
    # Core simulation engine
    ${NFSIM_SRC}/NFcore/system.cpp
    ${NFSIM_SRC}/NFcore/molecule.cpp
    ${NFSIM_SRC}/NFcore/moleculeType.cpp
    ${NFSIM_SRC}/NFcore/complex.cpp
    ${NFSIM_SRC}/NFcore/complexList.cpp
    ${NFSIM_SRC}/NFcore/reactionClass.cpp
    ${NFSIM_SRC}/NFcore/observable.cpp
    ${NFSIM_SRC}/NFcore/templateMolecule.cpp
    ${NFSIM_SRC}/NFcore/compartment.cpp

    # Molecule lists
    ${NFSIM_SRC}/NFcore/moleculeLists/moleculeList.cpp

    # Reaction selector
    ${NFSIM_SRC}/NFcore/reactionSelector/directSelector.cpp
    ${NFSIM_SRC}/NFcore/reactionSelector/logClassSelector.cpp

    # Reactions subsystem
    ${NFSIM_SRC}/NFreactions/NFreactions.cpp
    ${NFSIM_SRC}/NFreactions/reactions/reaction.cpp
    ${NFSIM_SRC}/NFreactions/reactions/DORreaction.cpp
    ${NFSIM_SRC}/NFreactions/reactantLists/reactantList.cpp
    ${NFSIM_SRC}/NFreactions/reactantLists/reactantTree.cpp
    ${NFSIM_SRC}/NFreactions/mappings/mapping.cpp
    ${NFSIM_SRC}/NFreactions/mappings/mappingSet.cpp
    ${NFSIM_SRC}/NFreactions/mappings/mappingGenerator.cpp
    ${NFSIM_SRC}/NFreactions/transformations/transformation.cpp
    ${NFSIM_SRC}/NFreactions/transformations/transformationSet.cpp
    ${NFSIM_SRC}/NFreactions/transformations/moleculeCreator.cpp
    ${NFSIM_SRC}/NFreactions/transformations/speciesCreator.cpp

    # Functions (ExprTk-based — muParser replaced by nfsim_funcparser.h)
    ${NFSIM_SRC}/NFfunction/function.cpp
    ${NFSIM_SRC}/NFfunction/funcParser.cpp
    ${NFSIM_SRC}/NFfunction/compositeFunction.cpp
    ${NFSIM_SRC}/NFfunction/localFunction.cpp

    # Input: XML parser, command line, RNF runner
    ${NFSIM_SRC}/NFinput/NFinput.cpp
    ${NFSIM_SRC}/NFinput/commandLineParser.cpp
    ${NFSIM_SRC}/NFinput/parseFuncXML.cpp
    ${NFSIM_SRC}/NFinput/parseSymRxns.cpp
    ${NFSIM_SRC}/NFinput/rnfRunner.cpp
    ${NFSIM_SRC}/NFinput/walk.cpp

    # TinyXML (XML parser, bundled with NFsim)
    ${NFSIM_SRC}/NFinput/TinyXML/tinystr.cpp
    ${NFSIM_SRC}/NFinput/TinyXML/tinyxml.cpp
    ${NFSIM_SRC}/NFinput/TinyXML/tinyxmlerror.cpp
    ${NFSIM_SRC}/NFinput/TinyXML/tinyxmlparser.cpp

    # Output
    ${NFSIM_SRC}/NFoutput/NFoutput.cpp

    # NFstream (output stream wrapper)
    ${NFSIM_SRC}/NFscheduler/NFstream.cpp
    # Scheduler.cpp excluded — MPI job scheduler, references NFsim.cpp's main() functions

    # Utilities (RNG, string ops, conversions)
    ${NFSIM_SRC}/NFutil/random.cpp
    ${NFSIM_SRC}/NFutil/conversion.cpp
    ${NFSIM_SRC}/NFutil/stringOperations.cpp
    ${NFSIM_SRC}/NFutil/MTrand/mtrand.cpp

    # Nauty (canonical graph labeling for complexes)
    ${NFSIM_SRC}/nauty24/nauty.c
    ${NFSIM_SRC}/nauty24/nautil.c
    ${NFSIM_SRC}/nauty24/nausparse.c
)

# ─── Build the static library ─────────────────────────────────────────────────
add_library(nfsim STATIC ${NFSIM_SOURCES})

# C++17 (ExprTk requires it; also eliminates std::auto_ptr from old muParser)
target_compile_features(nfsim PRIVATE cxx_std_17)
set_target_properties(nfsim PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

# Position-independent code (required when linking into shared library / Python extension)
set_target_properties(nfsim PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ─── Include directories ──────────────────────────────────────────────────────
target_include_directories(nfsim
    PUBLIC
        ${NFSIM_SRC}
        # ExprTk header (used by nfsim_funcparser.h, transitively included via NFfunction.hh)
        ${CMAKE_CURRENT_SOURCE_DIR}/../exprtk
    PRIVATE
        ${NFSIM_SRC}/nauty24
        ${NFSIM_SRC}/NFutil
        ${NFSIM_SRC}/NFutil/MTrand
        ${NFSIM_SRC}/NFscheduler
        ${NFSIM_SRC}/NFreactions/transformations
        ${NFSIM_SRC}/NFreactions/reactions
        ${NFSIM_SRC}/NFreactions/reactantLists
        ${NFSIM_SRC}/NFreactions/mappings
        ${NFSIM_SRC}/NFreactions
        ${NFSIM_SRC}/NFoutput
        ${NFSIM_SRC}/NFinput
        ${NFSIM_SRC}/NFinput/TinyXML
        ${NFSIM_SRC}/NFinput/json
        ${NFSIM_SRC}/NFfunction
        ${NFSIM_SRC}/NFcore
        ${NFSIM_SRC}/NFcore/reactionSelector
        ${NFSIM_SRC}/NFcore/moleculeLists
)

# ─── Suppress warnings from NFsim's legacy C++ code ──────────────────────────
target_compile_options(nfsim PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-deprecated-declarations>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-unused-variable>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-unused-but-set-variable>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-sign-compare>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-reorder>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-parentheses>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-format>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-unused-result>
    # Clang-specific
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-tautological-compare>
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-deprecated-copy>
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-overloaded-virtual>
    # NFsim's legacy observable.cpp exceeds MSVC's default section limit.
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)

# Suppress warnings for C files (nauty)
set_source_files_properties(
    ${NFSIM_SRC}/nauty24/nauty.c
    ${NFSIM_SRC}/nauty24/nautil.c
    ${NFSIM_SRC}/nauty24/nausparse.c
    PROPERTIES COMPILE_FLAGS "-w"
)
