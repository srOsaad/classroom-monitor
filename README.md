# Classroom System

Two Qt Quick apps for a classroom LAN:

- **Teacher** (`admin_app`): monitors and controls student machines (live screen
  grid, remote control, two-way audio, messaging/signals, clipboard, run command).
  Runs a WebSocket server on TCP 9000 and broadcasts discovery on UDP 45454.
- **Student** (`client_app`): runs hidden, streams its screen, obeys the teacher.
- `shared`: protocol, discovery beacon, and the neumorphic UI set.

## Qt requirements

Developed with **Qt 6.11.2**, Desktop **MinGW 64-bit** kit (MinGW 13.1.0). Any
Qt 6.5+ should work; qmake is the build system.

In the **Qt Maintenance Tool** (or Online Installer), under **Qt 6.11.2** select:

- **MinGW 13.1.0 64-bit** (the prebuilt Qt libraries for MinGW) - the kit.
- **Additional Libraries > Qt Multimedia** (not ticked by default).
- **Additional Libraries > Qt WebSockets** (not ticked by default).

Qt Quick, Quick Controls 2, Quick Effects, and Qt Network ship with the base Qt
install, so they have no separate checkbox.

Under **Developer and Designer Tools** select:

- **MinGW 13.1.0 64-bit** (the compiler toolchain, separate from the Qt libs).
- **Qt Creator** (recommended; qmake comes with Qt). CMake/Ninja are not needed.

Each app's `.pro` pulls in:

```
QT += quick quickcontrols2 quickeffects network websockets multimedia
CONFIG += c++17            # console is added for Teacher only
```

Controls use the Basic style (`import QtQuick.Controls.Basic`), so no style
packages or env vars are needed. `quickeffects` is required for the `MultiEffect`
shadows (the build fails without it). Student omits `console` so it runs with no
window on Windows.

## Build

```
qmake admin_app/admin_app.pro  && make
qmake client_app/client_app.pro && make
```

Or open either `.pro` in Qt Creator.

## Run

Start **Teacher** first. It shows an 8-character pairing code (kept across
restarts; **Regenerate** mints a new one). Start **Student** on each machine and
type the code, or pass it as an argument: `Student ABCD2345`.

Once paired, Student hides (frameless, click-through, off the taskbar) and
auto-starts on login via `HKCU\...\CurrentVersion\Run`. Use `uninstall.bat` to
remove that entry and its settings; `deploy.bat <Qt-bin>` builds self-contained
bundles under `dist\`.

## Ports

- TCP 9000: WebSocket control and data (teacher server).
- UDP 45454: discovery beacon broadcast by the teacher.

## Security

Plain `ws://` on a trusted LAN only: the pairing code keeps students out of the
wrong classroom, not an attacker off the wire. Deploy Student only on machines
you administer, with monitoring disclosed. For untrusted networks, switch to
`wss://` and per-client credentials.
