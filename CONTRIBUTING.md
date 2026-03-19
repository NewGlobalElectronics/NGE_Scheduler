# Contributing to NGE_Scheduler

## Adding a new port

1. Follow the procedure in [`docs/porting_guide.md`](docs/porting_guide.md).
2. Create `ports/<vendor>_<device>/bsp/bsp.h`, `bsp/bsp.c`, and `main.c`.
3. Add a row to the **Supported platforms** table in `README.md`.
4. Add an entry to `CHANGELOG.md` under `[Unreleased]`.
5. Verify the three demo tasks produce observable output on your target.

## Modifying `core/`

- Every change must preserve MISRA-C:2012 Required rule compliance.
- Any new Advisory deviation must be added to `docs/misra_deviations.md`
  with a full justification and compensating measure.
- Run a static analysis pass (e.g., PC-lint Plus, Polyspace, or cppcheck
  `--addon=misra`) before submitting.
- Do not add MCU-specific headers to `core/`.

## Code style

- C99, no compiler extensions.
- Fixed-width types (`u08`, `u16`, `u32`) everywhere; no plain `int` for
  data variables.
- All unsigned integer literals use the `U` suffix; `u32` literals use
  the `UL` suffix only when the value exceeds `UINT16_MAX`.
- Opening brace on the same line as the control statement for `if`/`for`/
  `while`; functions have the brace on a new line.
- Column limit: 80 characters.
