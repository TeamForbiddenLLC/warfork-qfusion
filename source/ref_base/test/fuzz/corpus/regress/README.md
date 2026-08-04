# Regression inputs

Each file here crashed a BSP loader before the fix named below. They are
replayed by `r_q3bsp_fuzz_test` and `cm_q3bsp_fuzz_test` on every CI run —
through *both* loaders, since it is the same file format and a crasher for one
parser is worth pointing at the other.

They must all either load or be rejected via `Com_Error`. A crash or sanitizer
report is a regression.

| input | was | fixed by |
|---|---|---|
| `patch-flatness-stack-overflow.bsp` | stack overflow: `Patch_FlatnessTest` recursed without bound on degenerate control points | depth cap in `qcommon/patch.c` |
| `stale-loader-statics-uaf.bsp` | use-after-free on the *next* map load: a rejected map longjmps past `Mod_Finish`, leaving `r_q3bsp.c`'s file-scope arrays pointing into a released mempool | `Mod_ResetLoaderState` at loader entry |
| `cm-brushside-bad-planenum.bsp` | SEGV: `CMod_LoadBrushSides_RBSP` indexed `map_planes` with an unchecked `planenum` | range checks in both brushside loaders |
| `ref-even-patch-grid-overread.bsp` | heap overread in `Patch_Evaluate` | reject even-dimensioned Bézier control grids |
| `cm-even-patch-grid-overread.bsp` | heap overread in `Patch_GetFlatness` | same, on the collision side |

The last two share a root cause: Bézier patches are 2n+1 on each axis, and both
patch walkers step by two and read `p + 2*cp[0] + 2`. That lands exactly on the
final control point when both dimensions are odd, and a full row past the end
when either is even — so "odd" is a memory-safety requirement, not a convention.
