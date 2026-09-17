# Safety notes for this build (AllVarStore_Separate / v1)

This variant groups the UI by VarStore instead of by BIOS Form, and is
otherwise a smaller/simpler reimplementation. Compared to other builds in
this project's history, this specific build does **not** include the
following write-safety checks:

- No confirmation screen before F10 writes. Pressing F10 calls
  `SetVariable()` on every dirty store immediately — there is no
  "review changes, press Enter to confirm" step.
- No check that a variable was changed externally (e.g. by the real BIOS
  Setup menu) between when it was loaded and when it is written.
- No read-back verification after `SetVariable()`. Success is judged only
  by the `EFI_STATUS` the write call returns, not by re-reading and
  comparing the variable.
- "Dirty" is set whenever an edit dialog is confirmed, not by comparing
  bytes against the originally-loaded value.

## Practical implications

- **F10 writes to real NVRAM immediately.** There is no undo after that
  point except by knowing the original value and writing it back yourself.
- Double-check the value you're about to save (Left/Right shows it live in
  the editor) before pressing Enter to stage it, and be deliberate about
  when you press F10.
- Keep a record of each VarStore's original bytes/values before changing
  anything, per the general recommended procedure in the main README.
- Change one variable at a time and reboot to verify before changing
  another.

## What we verified before publishing this build

- The per-question `StoreId` / `Offset` / `Size` / `Min` / `Max` values in
  `src/ia_data.c` were diffed against an earlier, more heavily-tested
  build of this project and are byte-identical (only their order within
  the array differs, which does not affect correctness since lookups are
  done by StoreId, not array position).
- `src/main.c` was syntax/warning-checked with `-Wall -Wextra -Wpedantic`
  (gcc, freestanding, as a C-correctness sanity check only — this does not
  replace building with the project's actual `clang`/`lld-link` toolchain
  or testing on real hardware).
- We did not add or modify any write-safety logic in this build. If you
  want the confirm/verify behavior described above, look for a build in
  this repo's history that includes it, or open an issue/PR.

Use at your own risk, same as stated in the main README.
