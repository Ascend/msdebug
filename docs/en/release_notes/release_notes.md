# MindStudio Debugger Release Notes

## Version Mapping

### Product Versions

| Product Name| Version  | Version Type|
|------|-------|------|
| msDebug | 26.0.0 | Internal test version|
| msDebug | 8.3.0 | Official version|

### Related Product Versions

| msDebug | makeself | libedit | ncurses
|----------|-----------------|----------|-------|
| 26.0.0 | release-2.5.0 or later| openEuler-24.03-LTS-SP1-release | 6.6
| 8.3.0 | release-2.5.0 or later| openEuler-24.03-LTS-SP1-release | openEuler-24.03-LTS-SP1-release

## Version Compatibility

None

## Feature Updates

### 26.0.0

#### 1. New Features

Function:

1. Supports board debugging without setting the kernel object path.
2. Supports skipping the number of elements during memory reading.
3. Supports debugging in the shared_memory operator scenario.
4. Supports coredump debugging and board debugging for operators compiled using ASC.
5. Supports printing of structure variables transferred from the host to the kernel.
6. Added the function of backtracking the call stack in non-inline compilation mode to the coredump parsing function.

Build and release:

1. Changed the minimum permission required by the root user on folders during installation to `700`.
2. Resolved the issue that the UT compilation fails on GCC 7 or 12.
3. Standardized the installation package names.
4. Resolved the issue that the incorrect path of the libtinfo dynamic library is found during compilation.
5. Added the deliverable `libform.so.5` to resolve the dependency of some environments on this file.
6. Supports Unix Makefiles build.

Documentation:

1. Comprehensively optimized and reconstructed the documents to improve usability.

#### 2. Deleted Features

None

#### 3. Bug Fixes

1. Fixed the issue that the tool hangs when the `finish` command is executed in some scenarios.
2. Fixed the issue that the `ascend info cores` command output contains multiple `*` addresses in the mix operator scenario.
3. Fixed the issue that garbled characters are displayed when the `var` command is used to print `uint8_t *` variables such as `gm`.
4. Optimized the error messages of some functions.

### 8.3.0

#### 1. New Features

This issue is the first official release. The functions are as follows:

1. Board debugging: supports breakpoint display, variable printing, register printing, memory printing, code line-level single-step debugging, core information display and switching, and call stack display.
2. Core dump file parsing: supports call stack display, register display, and variable display.

#### 2. Deleted Features

None

#### 3. Bug Fixes

None
