# `#if 0` Dead Code Cleanup — Design Spec

- **Date:** 2026-07-01
- **Status:** Draft
- **Workstream:** #if 0 dead code removal (lowest-hanging fruit)
- **Follow-up workstreams:** Y2038 audit, #ifdef cleanup

---

## 1. Motivation

The LT OS codebase contains ~41 `#if 0` blocks in first-party source code. These blocks represent commented-out dead code that:
- Degrades readability by hiding active code among dead code
- May contain stale symbol references (function prototypes, static variables) that are now only referenced within the `#if 0` block
- Suggests incomplete cleanup from the pre-open-source era at Roku

Don W., the LT OS architect, values clean, readable code — this cleanup aligns with that philosophy.

## 2. Scope

### In scope
- All `#if 0 ... #endif` blocks in first-party code under:
  - `lt/lt/source/`
  - `lt/lt/include/`
  - `lt/platforms/` (our code, not vendor SDKs)
  - `lt-firmware-example/`
- Dependent dead code: static declarations, helper functions, or variables only referenced from within a targeted `#if 0` block

### Out of scope (for this workstream)
- Third-party libraries (lwIP, mbedtls, speex, opus, etc.)
- Vendor SDK code (STM32 HAL, Espressif SDK, Apache NimBLE)
- `#ifdef` / `#ifndef` / `#else` conditional compilation cleanup (separate workstream)
- Y2038 data rollover audit (separate workstream)

## 3. Investigation Phase

For each `#if 0` block found, the following analysis is performed:

1. **Read the block:** Read the full `#if 0 ... #endif` range and 20 lines of surrounding context.
2. **Read file-level changelog (LOG block):** Many LT files have a textual changelog at the bottom. Check for any mention of the disabled code.
3. **Check git history (secondary):** Most files were imported from a prior VCS, so git blame often points only to the import commit. When multiple git commits exist, check for relevant messages.
4. **Classify the block** into one of:

   | Tier | Category | Criteria |
   |---|---|---|
   | **Easy** | debug-scaffold | Temporary print/log statements, test shims, diagnostic loops |
   | **Easy** | temporary-disable | Has an explicit comment with date/reason for the disable |
   | **Medium** | abandoned-feature | A complete function or feature that was replaced or deprecated |
   | **Medium** | experimental | Partial/experimental code with known issues |
   | **Complex** | unknown | No comments, no git history, purpose unclear |
   | **N/A** | keep | Has a valid reason to stay (e.g., references platform-specific code, or is a valid but currently-disabled optimization) |

5. **Check for dependent dead code:** If the block references static functions, variables, or extern declarations that are only used from within the `#if 0` block, flag those for removal too.

Blocks requiring a git blame check are noted; blocks where both history and changelogs are silent default to **Easy** (obvious debug scaffold) or **Complex** (no context).

## 4. Categorization and Output

The findings are presented to the reviewer as a structured table with columns:

| File | Line | Tier (Easy/Medium/Complex) | Category | Truncated description | Git context | Dependent code affected | Recommendation |
|---|---|---|---|---|---|---|---|

This table serves as the review artifact — each row is approved or rejected.

## 5. Removal Process (3 PRs)

### PR 1 — Easy blocks
- Remove `debug-scaffold` and `temporary-disable` blocks
- ~10-15 blocks expected
- Low-risk; straightforward diff per block
- Build verify affected platform targets
- **No dependent dead code expected**

### PR 2 — Medium blocks
- Remove `abandoned-feature` and `experimental` blocks
- ~15-20 blocks expected
- Each removal reviewed for dependent declarations
- Build verify + unit tests on affected platform targets

### PR 3 — Complex blocks
- Remove or justify each `unknown` and `keep` block
- ~5-10 blocks expected
- Each handled individually with reviewer sign-off
- May require consulting the original author (identifiable from changelogs)

## 6. Verification

- **Build check:** For each PR, build for `platforms/linux` (the fastest build target). If ESP32-specific files are affected, also build for ESP32.
- **No behavioral change:** `#if 0` blocks are already dead — removal should not change runtime behavior. The one risk is removing a static declaration that another (active) `#if 1` block also references; this is caught during the dependent-code check in step 5 of the investigation phase.
- **Review-ability:** Each PR's diff is scoped to its tier. No Easy/Medium/Complex cross-contamination.

## 7. Risks and Mitigations

| Risk | Mitigation |
|---|---|
| A `#if 0` block contains symbols used outside the block | Investigation phase explicitly checks for this; suspended blocks flagged as keep if dependencies exist |
| Build breaks after removal | PR-level build verification catches this before merge |
| Block has no git history and no changelog context | Classified as Complex; reviewed individually |
| A block looks dead but is intentionally preserved for reference | Investigation phase checks for inline documentation; author may be identifiable from changelogs |

## 8. Design Decision Context

The hybrid approach (investigate → batch by complexity → multiple PRs) was chosen over:
- **Single bulk PR:** Too large for careful review of ~41 blocks across many files.
- **Approach C pure:** Full categorization upfront avoids rework — each tier is its own PR, and we never revisit a block.
- **No action at all:** Leaves dead code accumulating, reducing codebase quality over time.

Written after reviewing ~4,797 C/H files, identifying ~41 #if 0 blocks, and confirming no overlap with PR #4 (README spelling fixes).