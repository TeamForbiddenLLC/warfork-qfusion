#!/usr/bin/env bash
# Fuzz and regression harness for the network parsers.
#
# The engine parses attacker-controlled bytes in three places that can be linked
# standalone: msg.c (read primitives), snap_read.c (snapshots from a server or a
# demo file) and net_chan.c (fragment reassembly + zlib, both pre-auth). This
# script builds those harnesses and runs them.
#
# Note the fuzzers only *generate* input - AddressSanitizer is what turns an
# overrun into a failure. Both builds here enable it.

set -euo pipefail

MODE="all"
DURATION=60
JOBS="$(nproc 2>/dev/null || echo 4)"
CLEAN=0
PARALLEL=1
TARGETS=()

ALL_FUZZERS=(msg_fuzz snap_fuzz netchan_fuzz info_fuzz)

usage() {
    cat <<'EOF'
Usage: ./scripts/fuzz-test.sh [mode] [options]

Modes:
    all            Corpus regression test, then fuzz (default)
    corpus         Only the deterministic regression test (gcc + ASan)
    fuzz           Only the libFuzzer targets (clang + ASan)
    repro FILE     Replay one crash artifact against its fuzzer

Options:
    -t, --time N     Seconds to run each fuzzer (default: 60)
    -f, --fuzzer N   Run only this fuzzer; repeatable
                     (msg_fuzz, snap_fuzz, netchan_fuzz, info_fuzz)
    -j, --jobs N     Build parallelism (default: nproc)
    --sequential     Run fuzzers one at a time instead of concurrently
    --clean          Remove the build directories first
    -h, --help       Show this help

Corpora persist in source/build-fuzz/corpus/<target>/ and are reused, so
successive runs get deeper. Crash artifacts are written to
source/build-fuzz/artifacts/ and the script exits non-zero if any appear.

Examples:
    ./scripts/fuzz-test.sh                          # regression + 60s per fuzzer
    ./scripts/fuzz-test.sh corpus                   # quick, for CI or pre-commit
    ./scripts/fuzz-test.sh fuzz -t 600              # 10 minutes per target
    ./scripts/fuzz-test.sh fuzz -f snap_fuzz -t 300
    ./scripts/fuzz-test.sh repro source/build-fuzz/artifacts/crash-abc123
EOF
}

REPRO_FILE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        all|corpus|fuzz) MODE="$1"; shift ;;
        repro)
            MODE="repro"; shift
            [[ $# -gt 0 ]] || { echo "error: repro needs a crash artifact path" >&2; exit 1; }
            REPRO_FILE="$1"; shift ;;
        -t|--time)    DURATION="$2"; shift 2 ;;
        -f|--fuzzer)  TARGETS+=("$2"); shift 2 ;;
        -j|--jobs)    JOBS="$2"; shift 2 ;;
        --sequential) PARALLEL=0; shift ;;
        --clean)      CLEAN=1; shift ;;
        -h|--help)    usage; exit 0 ;;
        *)            echo "error: unknown argument '$1'" >&2; usage; exit 1 ;;
    esac
done

if [[ ${#TARGETS[@]} -eq 0 ]]; then
    TARGETS=("${ALL_FUZZERS[@]}")
fi

for t in "${TARGETS[@]}"; do
    found=0
    for known in "${ALL_FUZZERS[@]}"; do
        [[ "$t" == "$known" ]] && found=1
    done
    if [[ "$found" == "0" ]]; then
        echo "error: unknown fuzzer '$t' (have: ${ALL_FUZZERS[*]})" >&2
        exit 1
    fi
done

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

ASAN_DIR="$ROOT/source/build-asan"
FUZZ_DIR="$ROOT/source/build-fuzz"
CORPUS_ROOT="$FUZZ_DIR/corpus"
ARTIFACT_DIR="$FUZZ_DIR/artifacts"

if [[ "$CLEAN" == "1" ]]; then
    echo "==> Cleaning build directories"
    rm -rf "$ASAN_DIR" "$FUZZ_DIR"
fi

if [[ ! -f third-party/angelscript/sdk/angelscript/include/angelscript.h ]]; then
    echo "==> Initialising git submodules"
    git submodule update --init --recursive
fi

# ---------------------------------------------------------------------------
# Deterministic regression corpus
#
# Replays one crafted packet per defect found in the network audit. Fast, no
# clang needed, and it is what catches a reintroduced bug in CI - the fuzzers
# rediscover such a bug only by chance.
# ---------------------------------------------------------------------------
# Third-party subprojects are chatty on both stdout and stderr, so run quietly
# and dump the log only when something actually fails.
run_quiet() {
    local log="$1"; shift
    if ! "$@" > "$log" 2>&1; then
        echo "error: command failed: $*" >&2
        echo "--- last 40 lines of $log ---" >&2
        tail -40 "$log" >&2
        exit 1
    fi
}

run_corpus() {
    mkdir -p "$ASAN_DIR"
    echo "==> Configuring regression build ($ASAN_DIR)"
    # 'env' rather than a bare assignment prefix: prefixing a *function* call
    # leaks the variable into the rest of the script, which would then hand the
    # gcc setting to the clang fuzz configure in 'all' mode
    run_quiet "$ASAN_DIR/configure.log" \
        env CC="${CC:-gcc-12}" CXX="${CXX:-g++-12}" cmake \
        -S "$ROOT/source" -B "$ASAN_DIR" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DBUILD_UNIT_TEST=ON \
        -DUSE_ASAN=ON

    echo "==> Building msg_corpus_test"
    run_quiet "$ASAN_DIR/build.log" cmake --build "$ASAN_DIR" --target msg_corpus_test -j"$JOBS"

    local bin
    bin="$(find "$ASAN_DIR" -name 'msg_corpus_test*' -type f -perm -u+x | head -1)"
    if [[ -z "$bin" ]]; then
        echo "error: msg_corpus_test binary not found after build" >&2
        exit 1
    fi

    echo "==> Running regression corpus"
    "$bin"
}

# ---------------------------------------------------------------------------
# libFuzzer targets
# ---------------------------------------------------------------------------
build_fuzzers() {
    if ! command -v clang >/dev/null; then
        echo "error: clang not found in PATH (libFuzzer is clang-only)" >&2
        exit 1
    fi

    mkdir -p "$FUZZ_DIR"
    echo "==> Configuring fuzz build ($FUZZ_DIR)"
    run_quiet "$FUZZ_DIR/configure.log" \
        env CC=clang CXX=clang++ cmake \
        -S "$ROOT/source" -B "$FUZZ_DIR" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DBUILD_FUZZERS=ON

    echo "==> Building fuzzers: ${TARGETS[*]}"
    for t in "${TARGETS[@]}"; do
        run_quiet "$FUZZ_DIR/build-$t.log" cmake --build "$FUZZ_DIR" --target "$t" -j"$JOBS"
    done
}

fuzzer_bin() {
    find "$FUZZ_DIR" -name "$1*" -type f -perm -u+x | head -1
}

# libFuzzer shells out to llvm-symbolizer to name newly covered functions, which
# stalls the run outright when the symbolizer is missing or slow. Coverage is
# unaffected, so turn the printing off.
FUZZ_COMMON_ARGS=(-print_funcs=0 -print_final_stats=1)

run_one_fuzzer() {
    local target="$1"
    local bin corpus log
    bin="$(fuzzer_bin "$target")"
    corpus="$CORPUS_ROOT/$target"
    log="$FUZZ_DIR/$target.log"

    mkdir -p "$corpus"

    "$bin" "$corpus" \
        -max_total_time="$DURATION" \
        -artifact_prefix="$ARTIFACT_DIR/" \
        "${FUZZ_COMMON_ARGS[@]}" > "$log" 2>&1
}

report_one() {
    local target="$1"
    local log="$FUZZ_DIR/$target.log"
    local runs

    runs="$(grep -oE 'Done [0-9]+ runs' "$log" 2>/dev/null | grep -oE '[0-9]+' || true)"

    if grep -q "ERROR: AddressSanitizer\|ERROR: libFuzzer\|SEGV\|deadly signal" "$log" 2>/dev/null; then
        printf '  %-14s CRASH\n' "$target"
        echo "    ---"
        grep -E "ERROR|SUMMARY" "$log" | head -3 | sed 's/^/    /'
        echo "    full log: $log"
        return 1
    fi

    printf '  %-14s ok (%s runs)\n' "$target" "${runs:-0}"
    return 0
}

run_fuzzers() {
    build_fuzzers
    mkdir -p "$ARTIFACT_DIR"

    echo "==> Fuzzing ${DURATION}s per target (corpus: $CORPUS_ROOT)"

    if [[ "$PARALLEL" == "1" ]]; then
        local pids=()
        for t in "${TARGETS[@]}"; do
            run_one_fuzzer "$t" &
            pids+=($!)
        done
        # a crashing fuzzer exits non-zero; collect them all rather than
        # aborting the batch on the first one
        for pid in "${pids[@]}"; do
            wait "$pid" || true
        done
    else
        for t in "${TARGETS[@]}"; do
            run_one_fuzzer "$t" || true
        done
    fi

    echo "==> Results"
    local failed=0
    for t in "${TARGETS[@]}"; do
        report_one "$t" || failed=1
    done

    if [[ "$failed" == "1" ]]; then
        echo
        echo "Crash artifacts in $ARTIFACT_DIR"
        echo "Replay one with: ./scripts/fuzz-test.sh repro <artifact>"
        return 1
    fi
    return 0
}

# ---------------------------------------------------------------------------
# Replay a single artifact
# ---------------------------------------------------------------------------
run_repro() {
    local artifact="$1"

    if [[ ! -f "$artifact" ]]; then
        echo "error: no such file: $artifact" >&2
        exit 1
    fi

    build_fuzzers

    # artifacts are named crash-<sha>/leak-<sha>/timeout-<sha> with no hint of
    # which target produced them, so try each built fuzzer in turn
    local hit=0
    for t in "${TARGETS[@]}"; do
        local bin
        bin="$(fuzzer_bin "$t")"
        echo "==> $t"
        if "$bin" "$artifact" -print_funcs=0 2>&1 | tee "$FUZZ_DIR/repro-$t.log" | grep -qE "ERROR|SUMMARY"; then
            echo "    reproduced under $t"
            hit=1
        fi
    done

    if [[ "$hit" == "0" ]]; then
        echo "==> Artifact did not reproduce (already fixed, or it belongs to a target not built)"
        return 0
    fi
    return 1
}

case "$MODE" in
    corpus) run_corpus ;;
    fuzz)   run_fuzzers ;;
    repro)  run_repro "$REPRO_FILE" ;;
    all)    run_corpus; echo; run_fuzzers ;;
esac
