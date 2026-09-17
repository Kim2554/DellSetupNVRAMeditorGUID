# DellSetupNVRAMeditorGUID
Experimental UEFI tool for viewing and editing Dell Setup NVRAM variables, developed to access hidden Intel Advanced Menu on the Inspiron 15 3511 BIOS v1.48.0



# Dell Setup NVRAM Editor

> ⚠️ **EXPERIMENTAL / TEST VERSION — NOT 100% WORKING**

A UEFI utility for viewing and editing **Setup NVRAM variables** directly from the UEFI environment.

This project was originally developed to help access and experiment with the **Intel Advanced Menu / Intel Advanced BIOS settings** on the **Dell Inspiron 15 3511**, specifically **BIOS version 1.48.0**.

The project is experimental and is **not intended to be considered a finished or fully compatible BIOS modification tool**.

---

## 🎯 Project Purpose

The main purpose of this project is to provide access to hidden or normally inaccessible **Intel Advanced BIOS settings** through their underlying UEFI NVRAM variables.

On the Dell Inspiron 15 3511, many Intel Advanced settings exist in the firmware but are not normally exposed through the standard Dell BIOS interface.

This tool allows these variables to be inspected and, where supported, modified directly.

The primary development and testing target is:

**Dell Inspiron 15 3511 — BIOS 1.48.0**

Compatibility with other BIOS versions and systems is not guaranteed.

---

## ✨ Features

The tool can:

* Read UEFI NVRAM variables
* Enumerate and display Setup variables
* Organize variables by **VarStore**
* Display VarStore names and GUIDs
* Display variable offsets
* Display variable sizes
* Display current variable values
* Edit supported numeric values
* Edit OneOf-style values
* Edit checkbox values
* Edit raw variable data
* Modify individual fields/offsets
* Stage changes before saving
* Write modified variables back to NVRAM
* Read variables back after writing
* Verify whether a write was accepted by firmware
* Access multiple Setup/CPU/platform VarStores
* Provide access to variables used by hidden Intel Advanced settings

The exact settings available depend on the firmware and the VarStores exposed by the system.

---

## 🧩 Intel Advanced Settings

Depending on the firmware, the underlying variables may include settings related to areas such as:

* CPU configuration
* CPU Ratio
* Per-Core Ratio
* Turbo Ratio
* CPU Flex Ratio
* Overclocking Feature
* Overclocking Lock
* CFG Lock
* CPU power limits
* VR settings
* Current limits
* TDC settings
* Load-Line settings
* IMON settings
* cTDP settings
* CPU thermal settings
* Memory-related settings
* Platform configuration
* Chipset configuration
* Other Intel Advanced BIOS options

**Not every setting is available or writable on every system.**

The tool does not automatically make every Intel feature functional. A firmware setting may exist in the BIOS data while still being restricted by hardware, microcode, firmware policy, or other platform mechanisms.

---

## 💾 NVRAM Access

The tool accesses UEFI variables using UEFI Runtime Services.

### Read

`GetVariable()`

Used to retrieve the current contents and attributes of a UEFI variable.

### Write

`SetVariable()`

Used to modify or create a UEFI variable.

UEFI defines `GetVariable()`, `GetNextVariableName()`, `SetVariable()`, and `QueryVariableInfo()` as part of its Variable Services.

The tool therefore operates on the **NVRAM variables themselves**, rather than directly modifying the BIOS firmware image.

---

## ⚠️ Important: This Is Not a BIOS ROM Editor

This tool does **not modify or flash the BIOS firmware image**.

Instead, it reads and writes **UEFI variables stored in the system's NVRAM** using the UEFI Runtime Services `GetVariable()` and `SetVariable()`.

The variables may contain settings used by the system firmware, including Setup/BIOS configuration values.

If a variable has the **Non-Volatile (NV)** attribute, changes written successfully by `SetVariable()` are intended to persist across system resets and power cycles. However, the actual behavior depends on the variable attributes and the platform firmware. :contentReference[oaicite:0]{index=0}

This means that changing a variable can affect the system's firmware configuration even though the BIOS ROM itself is not modified.

---

## ⚠️ Limitations

Directly changing an NVRAM variable is not necessarily equivalent to changing the same setting through the manufacturer's BIOS interface.

The normal BIOS Setup interface may perform additional:

* Validation
* Dependency checking
* Callback processing
* Hardware configuration
* Firmware-specific processing

before committing a setting.

This tool may bypass some of those mechanisms.

Therefore:

> **A variable being editable does not mean that changing it is safe or that the firmware will actually use the new value correctly.**

---

## ⚠️ Compatibility

### Primary Test System

**Dell Inspiron 15 3511**

**BIOS:** 1.48.0

This is the primary platform for which the project was developed and tested.

Compatibility with other systems is not guaranteed.

It may not work correctly with:

* Other Dell models
* Other BIOS versions
* Different motherboard revisions
* Different Intel CPU generations
* Different VarStore layouts
* Protected variables
* Authenticated variables
* Firmware-specific implementations

Even identical variable names or offsets do not guarantee identical behavior between different BIOS versions or platforms.

---

# 🚨 WARNING — USE AT YOUR OWN RISK

Incorrect NVRAM values can cause serious firmware problems.

Possible consequences include:

* BIOS configuration corruption
* BIOS Setup malfunction
* Boot failure
* Boot loops
* Recovery mode
* Hardware configuration problems
* Loss of access to BIOS settings
* Failed BIOS initialization
* Requirement for BIOS recovery
* In severe cases, external firmware recovery or an SPI programmer may be required

**Do not change an unknown variable simply because it is writable.**

A writable variable is not necessarily a safe variable.

---

## 🛡️ Recommended Procedure

Before experimenting:

1. Make a backup of the original BIOS/firmware if possible.
2. Record the original NVRAM value.
3. Record the VarStore, GUID, offset, and size.
4. Change **one variable at a time**.
5. Save the change.
6. Reboot.
7. Verify BIOS functionality.
8. Verify that Windows still boots.
9. Keep a record of every modification.

Avoid changing multiple unknown variables at once.

---

## 🧪 Project Status

**Experimental / Work in Progress**

This software is primarily a research and testing tool.

It is **not 100% complete** and may contain bugs.

Some functions may work on the Dell Inspiron 15 3511 BIOS 1.48.0 but fail on another BIOS version or another system.

Unexpected behavior should be expected during development.

---

## ❗ Important Notice

This project was created specifically as an experimental way to access and modify the underlying NVRAM configuration used by hidden **Intel Advanced BIOS settings** on the Dell Inspiron 15 3511.

It does **not** guarantee that enabling a hidden option will actually unlock the corresponding hardware capability.

For example, exposing an Intel Advanced option does not automatically guarantee that CPU overclocking, higher ratios, additional power limits, voltage control, or other features will work.

Actual behavior depends on the CPU, firmware, microcode, power delivery, thermal limits, and platform restrictions.

---

## Disclaimer

This software is provided **AS IS** for research and experimental purposes.

**Use at your own risk.**

The author is not responsible for:

* Firmware corruption
* Boot failure
* BIOS configuration problems
* Data loss
* Hardware damage
* Loss of BIOS functionality
* Any other damage resulting from the use of this software

  ## Controls

| Key | Action |
|---|---|
| `↑ / ↓` | Navigate |
| `← / →` | Change value |
| `Enter` | Edit / Select |
| `Esc` | Back |
| `F9` | Reload |
| `F10` | Save to NVRAM |
