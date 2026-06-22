# ESP32-S3 Project Instructions

## Project Overview

This repository is an ESP-IDF application for the DNESP32S3 development board.

- Project name: `my_esp32s3_app`
- Target chip: ESP32-S3
- ESP-IDF version: v5.3.2
- SPI Flash size: 16 MB
- USB interface: USB Serial/JTAG
- Expected serial port: `/dev/ttyACM0`
- Project path: `/home/blackboard/work/my_esp32s3_app`
- ESP-IDF installation: `/home/blackboard/esp/esp-idf`

Keep application code in this repository. Do not modify the ESP-IDF installation to implement application features.

## Environment Detection

The project is stored inside WSL2 Ubuntu.

Before running commands, detect whether the current shell is Windows PowerShell or WSL Linux.

If already inside WSL, run commands directly:

```bash
cd /home/blackboard/work/my_esp32s3_app
source /home/blackboard/esp/esp-idf/export.sh
```

If running from Windows PowerShell, execute Linux commands through WSL:

```powershell
wsl -d Ubuntu -- bash -lc "cd /home/blackboard/work/my_esp32s3_app && source /home/blackboard/esp/esp-idf/export.sh && <command>"
```

Do not invoke `wsl` from inside an existing WSL shell.

## Initial Inspection

Before editing:

1. Read this `AGENTS.md`.
2. Inspect the top-level `CMakeLists.txt`.
3. Inspect `main/CMakeLists.txt` and source files under `main/`.
4. Run `git status`.
5. Preserve all existing uncommitted user changes.
6. Briefly explain the intended implementation before making substantial changes.

Do not perform unrelated refactoring.

## Development Rules

- Use ESP-IDF APIs and existing project patterns.
- Keep changes focused on the requested feature.
- Prefer clear, small functions with descriptive names.
- Check return values from ESP-IDF APIs where failures are possible.
- Use `ESP_LOGI`, `ESP_LOGW`, and `ESP_LOGE` for runtime messages.
- Avoid unnecessary dynamic allocation.
- Do not block ESP-IDF system tasks indefinitely.
- Use FreeRTOS delays such as `vTaskDelay(pdMS_TO_TICKS(...))`, not busy-wait loops.
- Do not hard-code GPIO assignments without documenting them.
- Keep hardware-specific definitions in clearly named constants or configuration headers.
- Add short comments only where behavior is not self-explanatory.

## Protected Environment

Do not modify, move, or delete:

```text
/home/blackboard/esp/esp-idf
/home/blackboard/.espressif
```

Do not reinstall ESP-IDF, Python environments, toolchains, WSL, VS Code Server, or USB drivers unless explicitly requested.

Do not delete or regenerate `sdkconfig` unless required by the task and the reason is explained first.

Maintain these project settings:

```text
Target: ESP32-S3
Flash size: 16 MB
Flash mode: DIO
Flash frequency: 80 MHz
```

## Build Commands

Load ESP-IDF before using `idf.py`:

```bash
source /home/blackboard/esp/esp-idf/export.sh
```

Normal build:

```bash
idf.py build
```

Reconfigure CMake only when necessary:

```bash
idf.py reconfigure
```

Clean generated object files:

```bash
idf.py clean
```

Use a full clean only when ordinary rebuilding cannot resolve a configuration or dependency problem:

```bash
idf.py fullclean
idf.py build
```

Do not use `fullclean` routinely.

## Build Verification

After changing source code or build configuration:

1. Run `idf.py build`.
2. Treat compiler warnings introduced by the change as problems to fix.
3. Report whether the build succeeded.
4. Report the firmware size when relevant.
5. Do not claim completion if the project does not build.

A successful build is required unless the user explicitly requests analysis-only work.

## Flashing and Monitoring

Do not access hardware unless the user explicitly requests flashing or runtime verification.

Before flashing, verify the serial port:

```bash
ls -l /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

Expected port:

```text
/dev/ttyACM0
```

Build and flash:

```bash
idf.py -p /dev/ttyACM0 flash
```

Flash and monitor:

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

Exit the serial monitor with:

```text
Ctrl+]
```

If `/dev/ttyACM0` does not exist, do not guess another port. Ask the user to reconnect the USB device through `usbipd-win` and verify the actual device path.

Do not run `erase-flash` unless explicitly requested and the destructive effect has been stated.

## USB Passthrough

`usbipd-win` is a Windows-side tool. Its commands must run in Windows PowerShell, not inside WSL.

The board is normally identified as:

```text
VID:PID 303a:1001
USB JTAG/serial debug unit
```

The BUSID can change after unplugging or restarting Windows.

Typical Windows commands are:

```powershell
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

Do not bind unrelated keyboard, mouse, fingerprint, camera, or Bluetooth devices.

## Git Rules

Before editing:

```bash
git status
```

After editing:

```bash
git diff
git status
```

Do not overwrite, discard, reset, or revert user changes.

Do not run destructive Git commands such as:

```text
git reset --hard
git clean -fd
git checkout -- <file>
```

unless the user explicitly requests the operation and understands its effect.

Do not create commits, branches, tags, or push changes unless explicitly requested.

When suggesting a commit, use a concise message such as:

```text
feat: add LED blink task
fix: correct GPIO initialization
docs: update build instructions
```

## Generated Files

Do not edit generated files under:

```text
build/
```

The project `.gitignore` should normally exclude:

```gitignore
build/
sdkconfig.old
```

Do not add build artifacts, binaries, object files, temporary logs, or editor caches to Git.

## Configuration Changes

Use:

```bash
idf.py menuconfig
```

for ESP-IDF configuration changes.

When changing configuration:

1. Explain the setting being changed.
2. Preserve ESP32-S3 as the target.
3. Preserve 16 MB Flash size.
4. Save the resulting configuration.
5. Rebuild with `idf.py build`.
6. Report relevant changes made to `sdkconfig`.

## Runtime Verification

When runtime verification is requested, inspect logs for:

```text
ESP-IDF v5.3.2
SPI Flash Size : 16MB
Project name: my_esp32s3_app
```

Distinguish normal boot messages from errors.

`app_main()` returning is not automatically a crash. Determine whether the application is intended to finish or keep running.

For continuously running behavior, use FreeRTOS tasks or an intentional loop containing an appropriate delay.

## Completion Report

At the end of each task, report:

- Files changed
- Behavior implemented
- Build command executed
- Build result
- Hardware verification performed, if any
- Remaining limitations or risks
- Suggested Git commit message

Do not claim that flashing or hardware behavior was verified unless it was actually performed and confirmed from device output.


## External Board Documentation

The original DNESP32S3 vendor documentation is stored outside this repository.

Windows path:

`E:\Files\2.项目\12.【正点原子】DNESP32S3开发板`

WSL path:

`/mnt/e/Files/2.项目/12.【正点原子】DNESP32S3开发板`

Rules:

- Treat this directory as read-only.
- Do not move, rename, delete, or modify vendor files.
- Read only the documents relevant to the current task.
- Consult the hardware manual, schematic, and pinout before assigning GPIOs.
- Do not copy large videos, archives, installers, or SDK packages into this repository.