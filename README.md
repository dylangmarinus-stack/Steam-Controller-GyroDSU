# SteamControllerGyroDSU

DSU (Cemuhook protocol) server for motion data for the **Steam Controller 2** on SteamOS/Linux.

Inspired by [SteamDeckGyroDSU](https://github.com/kmicki/SteamDeckGyroDSU) by kmicki.

## Features
- Supports up to **4 Steam Controllers simultaneously** (slots 0-3)
- Works via **Bluetooth**, **USB-C**, and **Proteus Puck dongle**
- Works along side **SteamDeckGyroDSU**
- Automatic gyro bias calibration
- Runs as a background service — starts automatically with your Steam Deck
- Compatible with **Eden**, **Ryujinx**, **Cemu**, and any other Cemuhook-compatible emulator

## Install

Open this page in the browser on your **Steam Deck** in Desktop Mode.

Download [SteamControllerGyroDSU](https://github.com/dylangmarinus-stack/Steam-Controller-GyroDSU/releases/latest/download/SteamControllerGyroDSU.desktop), save it to your Desktop and double-click it.

## Emulator Setup

Point your emulator at:
- **IP:** `127.0.0.1`
- **Port:** `26761`

Slot 0 = first controller, Slot 1 = second, etc.

Puck controller channels are detected independently, even though they share the
receiver's serial number. Empty channels do not occupy emulator slots.

## Troubleshooting

With the controller paired and awake, inspect detection and service logs:

```sh
"$HOME/SteamControllerGyroDSU/SteamControllerGyroDSU" --probe
journalctl --user -u SteamControllerGyroDSU.service -b -n 40 --no-pager
```

The probe lists HID interfaces, not paired controllers. A puck can expose several
controller channels plus a receiver control interface. A running service with
`0.0 samples/s` is not receiving usable motion samples. The original v1.0.0
release can repeatedly open an empty puck channel and skip the active one;
it needs a build containing the HID-path detection fix.

## Works alongside SteamDeckGyroDSU

Run [SteamDeckGyroDSU](https://github.com/kmicki/SteamDeckGyroDSU) on port `26760` to use the Steam Deck's built-in gyro at the same time as your Steam Controllers.

Add both servers in your emulator:
- `127.0.0.1:26760` → Steam Deck gyro
- `127.0.0.1:26761` → Steam Controllers

## Uninstall

Double-click **Uninstall SteamControllerGyroDSU** on your Desktop.

## Notes
- Works via **Bluetooth**, **USB-C**, and **Proteus Puck dongle**
- Gyro auto-calibration activates when the controller is held still for ~2 seconds
- Tested on SteamOS with Eden, Ryujinx, Sudachi, Suyu

## License
MIT License

Copyright (c) 2026 Daqueyo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
