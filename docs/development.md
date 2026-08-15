# PikaTalk development workflow

Target environment: openSUSE Tumbleweed + KDE Plasma 6.

PikaTalk is a Qt 6 / Kirigami desktop application. This document is the local build, run, and debug cycle.

## 1. Install development dependencies

```bash
sudo zypper install \
  cmake \
  ninja \
  gcc-c++ \
  kf6-extra-cmake-modules \
  kf6-kirigami-devel \
  kf6-ki18n-devel \
  kf6-kcoreaddons-devel \
  kf6-kiconthemes-devel \
  kf6-qqc2-desktop-style \
  kf6-qqc2-desktop-style-devel \
  qt6-base-devel \
  qt6-declarative-devel \
  qt6-quickcontrols2-devel
```

`qt6-sql-devel` and the SQLite driver are pulled in by `qt6-base-devel` on Tumbleweed.

## 2. Configure the build

From the repository root:

```bash
cmake -B build -G Ninja --install-prefix "$HOME/.local"
```

This creates an out-of-source Debug build in `build/`. A local-only git branch may cause CMake/ECM to print `fatal: no upstream configured for branch ...`; that message is harmless and does not fail the configure step.

## 3. Build

```bash
cmake --build build
```

The executable is `build/bin/pikatalk`.

Automated checks (`applicationpaths_test`, `database_test`, `appcontroller_test`, and `appstreamtest`):

```bash
ctest --test-dir build --output-on-failure
```

## 4. Launch

From the repository root:

```bash
./build/bin/pikatalk
```

Optional local install, which places the binary in `~/.local/bin` and the desktop file in `~/.local/share/applications`:

```bash
cmake --install build
pikatalk
```

Installing the desktop file is what lets Plasma/xdg-desktop-portal recognize app ID `org.radilabs.pikatalk`. Running from `build/bin/pikatalk` without installing still starts the application.

## 5. Make a visible UI change

Edit a string in `src/Main.qml`, for example a sidebar label.

QML is compiled into the binary, so a rebuild is required.

## 6. Rebuild and relaunch

```bash
cmake --build build
./build/bin/pikatalk
```

There is no live QML reload; rebuild and relaunch after QML edits.

## 7. Logs and debugging

Startup diagnostics and Qt/QML messages are written to stderr, including:

* data / config / cache directories
* SQLite database path
* directory or database errors
* QML load failures

Example:

```text
PikaTalk data directory: /home/naorw/.local/share/Radilabs/PikaTalk
PikaTalk sqlite database: /home/naorw/.local/share/Radilabs/PikaTalk/pikatalk.sqlite
```

Capture them with:

```bash
./build/bin/pikatalk 2> pikatalk.log
```

A broken QML identifier produces a line such as:

```text
qrc:/qt/qml/org/radilabs/pikatalk/Main.qml:LINE: Cannot assign to non-existent property "..."
Failed to load the PikaTalk QML interface
```

Plasma also records the same messages in the user journal:

```bash
journalctl --user -n 50 | grep pikatalk
```

Useful environment variables:

* `QT_LOGGING_RULES="*.debug=true"` for more Qt logging
* `QML_IMPORT_TRACE=1` if a QML module fails to load

Local data locations are documented in `docs/local-storage.md`.
