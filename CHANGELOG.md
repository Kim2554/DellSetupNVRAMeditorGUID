# Changelog

All notable changes to this project will be documented in this file.

## [0.1.0-alpha] - Initial Experimental Release

### Added

* Initial experimental UEFI NVRAM editor.
* UEFI Setup NVRAM variable browsing.
* VarStore-based variable organization.
* VarStore GUID and offset information.
* Editing of supported NVRAM values.
* Support for numeric, OneOf, and checkbox values.
* Raw variable editing where supported.
* NVRAM write support through UEFI Runtime Services.
* Read-back verification after writing variables.
* Support for multiple Setup-related VarStores.
* Initial support and testing for the Dell Inspiron 15 3511.
* Developed and tested primarily with BIOS version 1.48.0.
* Intended for experimentation with hidden Intel Advanced BIOS settings.

### Known Issues

* This is an experimental test release.
* The software is not guaranteed to work on all Dell systems or BIOS versions.
* Some NVRAM variables may be read-only or protected.
* Some firmware may reject NVRAM write operations.
* Direct NVRAM editing may not trigger all BIOS Setup/HII validation or callbacks.
* Changing incorrect NVRAM values may cause boot or BIOS configuration problems.
* Additional bugs and compatibility issues may exist.

### Compatibility

**Primary test platform:**

* Dell Inspiron 15 3511
* BIOS 1.48.0

Other systems and BIOS versions are currently unverified.

### Safety

Always record the original value before modifying a variable.

Change one variable at a time and make sure a working BIOS recovery method is available before experimenting.
