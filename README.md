# Retro Handheld Save Manager (RHSM)

**Repository:** https://github.com/Jelly-Fish-Dev/RHSM

RHSM is a native [OnionOS](https://github.com/OnionUI/Onion) app for the Miyoo Mini and Mini Plus. It finds your RetroArch save files and save states and groups them by game. Each game is shown with its proper name and box art. From there you can browse, back up, restore, delete, copy between cores, and export your saves.

A desktop companion is planned. It will run in the background on your computer. When you put the SD card in your computer, or when a Mini Plus is on the same WiFi network, it will fetch names and box art and back up your saves to your computer. You will manage everything through a web page in your browser, laid out like the app on the device.

> **Status:** early development, **version 0.0.3**. The device app can scan and list your saves, but can't manage them yet.

### What works today
- Scans `Saves/CurrentProfile/saves/<core>/` and `states/<core>/` for battery saves (`.srm`, `.sav`) and save states (`.state.auto`, `.stateN` and their thumbnails).
- Groups saves by game across RetroArch cores. For example, Snes9x and Supafaust saves for the same ROM show up as one game.
- Works out each game's system by finding its ROM in `Roms/<SYSTEM>/`.
- Stores the index in SQLite at `Saves/RHSM/index.db`.
- Shows a game list on the device, with a detail panel for the selected game and the box art Onion has already downloaded (`Roms/<SYSTEM>/Imgs/<ROM name>.png`).

## Version and roadmap

RHSM stays below 1.0 until every milestone below is done. The version is `0.0.<milestones done>`:

- the last number goes up by one as each milestone is finished
- **1.0.0** means all nine milestones are done

The current version is in the [VERSION](VERSION) file. The app shows it in its footer, and release zips are named after it.

| # | Milestone | Status |
|---|---|---|
| 1 | Toolchain and hello world: Docker cross-compile, app in Onion's Apps list, deploy/log/gdb over SSH | Done (0.0.1) |
| 2 | Scanner and database: saves grouped by game across cores, SQLite index, on-screen list with Onion box art | Done (0.0.2) |
| 3 | System tabs: filter the list by system, L2/R2 to switch tabs | Done (0.0.3) |
| 4 | UI: grid and detail screens with Onion theme fonts, save-state thumbnails | Next |
| 5 | Local metadata: names from Onion's game lists and bundled libretro databases | Planned |
| 6 | Save management: backup, restore, delete, rename, copy to another core | Planned |
| 7 | Scraper on the Mini Plus: names and box art over WiFi | Planned |
| 8 | Desktop companion: background daemon with a web UI like the device app | Planned |
| 9 | Export and sync: zip export, WebDAV upload, release packaging | Planned |

## Getting the code

RHSM shares its drawing, font, button and logging code with Life Counter through the Miyoo-UI library. Clone it next to RHSM:

```sh
git clone https://github.com/Jelly-Fish-Dev/RHSM.git
git clone https://github.com/Jelly-Fish-Dev/Miyoo-UI.git   # must sit at ../Miyoo-UI
cd RHSM
```

To keep Miyoo-UI somewhere else, pass `MUI_DIR=<path>` to make or set it in `local.mk`.

## Components

| Part | Location | Tech | State |
|---|---|---|---|
| Device app | `src/` | C99, SDL 1.2 (Onion's build), SDL_ttf, SDL_image, SQLite, built with the Miyoo Mini cross-compiler | In progress |
| Desktop companion | `companion/` | Python 3.11+, FastAPI, watchdog, Pillow | Planned |
| Shared rules | `spec/` | JSON: which emulator core belongs to which system, how save filenames map to slots, how game names are cleaned up | Planned (lives in `src/scan/cores.c` for now) |

## Features (planned)

- Looks up game names and box art in this order:
  1. Art and names Onion has already downloaded (done for art)
  2. Bundled libretro game databases
  3. ScreenScraper or libretro-thumbnails over WiFi (Mini Plus only)
  4. The desktop companion (for the original Mini, which has no WiFi)
- Timestamped backup and restore, delete, rename, copy to another core, zip export.
- Desktop companion:
  - starts at login and runs in the background
  - notices when the SD card is inserted, or reaches a Mini Plus over FTP
  - keeps a library of save backups on your computer
  - has a web interface at `http://localhost:7657` that you can use with a keyboard, a mouse or a game controller

## Building

### Device app
You need Docker, and nothing else on your computer. The first `make` builds a `rhsm-toolchain` image from [docker/Dockerfile](docker/Dockerfile). It starts from Onion's `aemiii91/miyoomini-toolchain` image and adds the libraries the cross-gdb needs. On Apple Silicon the image runs under emulation, which is slower but works.

```sh
make            # cross-compile and stage dist/App/RHSM/
make debug      # same, as a debug build (-O0 -g3, not stripped)
make zip        # dist/RHSM-<version>.zip, unzip onto the SD card root
make shell      # shell inside the toolchain container
make check      # check the source layers (see below); runs on your computer, no Docker needed
make clean
```

To install, copy `dist/App/RHSM/` to `/mnt/SDCARD/App/` on the SD card, or unzip the release zip onto the root of the card. RHSM then shows up in Onion's **Apps** list.

Controls: **UP/DOWN** select a game, **L1/R1** (or LEFT/RIGHT) page through the list, **L2/R2** switch between system tabs (All, GB, PS, SFC, ...), **MENU** exit.

### Source layout
The device app is split into layers. Each layer may only include headers from its own layer and the layers listed above it, and `make check` enforces this.

| Layer | Folder | What it does |
|---|---|---|
| util | `src/util/` | String and formatting helpers |
| platform | `src/platform/` | SD card paths (including Onion's box art location) |
| model | `src/model/` | The library data types: games, save files, save kinds |
| scan | `src/scan/` | Filename parsing, ROM folder lookup, folder walk, core-to-system table |
| db | `src/db/` | SQLite schema, storing a scan, loading the library |
| core | `src/core/` | The scan → database → library pipeline, and the text listing |
| ui | `src/ui/` | RHSM's theme colours, system names, and the screens |
| app | `src/app/` | The on-device app loop, and `--dump` |
| main | `src/main.c` | Chooses between the app and `--dump` |

Drawing (`mui/gfx.h`), box art loading (`mui/image.h`), button mapping (`mui/input.h`), logging (`mui/log.h`), the base theme and the font all come from Miyoo-UI. Any layer may include it.

### Testing on the device (Mini Plus over WiFi)
All testing happens on the device; there is no desktop build. On the Mini Plus, turn on SSH in **Apps → Tweaks → Network**. The default login is `onion` / `onion`. Run `ssh-copy-id onion@<ip>` once so you aren't asked for the password every time.

Put your device's IP in `local.mk`, which git ignores:

```make
RHSM_HOST ?= 192.168.1.50
RHSM_SSH_PASS ?= onion    # optional: lets make answer the SSH password prompt itself
```

Add `DEBUG=1` to any command below to use the debug build.

```sh
make deploy     # build and copy App/RHSM to the device over SSH
make log        # print the app's log.txt (the previous run is kept as log.prev.txt)
```

You can also run the scanner over SSH without using the screen. It prints every game and save it finds:

```sh
ssh onion@<ip> 'cd /mnt/SDCARD/App/RHSM && ./bin/rhsm --dump'
```

On an original Mini (no WiFi), copy `dist/App/RHSM/` to the SD card yourself and read `App/RHSM/log.txt` from the card afterwards.

### Remote debugging with gdb
```sh
make deploy DEBUG=1
make debug-on       # copy gdbserver to the device and turn on debug mode
# launch RHSM from the Apps menu: it waits for a debugger on port 2345
make gdb DEBUG=1    # cross-gdb in the container, connected to the device
make debug-off      # back to normal launching
```

`make gdb` loads [scripts/rhsm.gdb](scripts/rhsm.gdb), which prints the save library once it has loaded and then lets the app run. It uses the unstripped binary in `build/debug/`, so you get full symbols even though the copy on the device is stripped.

### Desktop companion (planned)
```sh
cd companion
uv sync
uv run rhsmd run        # start the daemon + web UI
uv run rhsmd install    # register to start at login
uv run pytest
```

## AI disclosure

This project was developed with the help of AI. Claude (Anthropic) was used for research, testing, debugging and setting up the build environments. The maintainer wrote the code and used AI to clean up and test it. This software was tested on real hardware (a Miyoo Mini Plus running OnionOS).

## Third-party components

- [SQLite](https://www.sqlite.org/) 3.46.1 amalgamation in `third_party/sqlite/` (public domain)
- [Exo 2](https://github.com/googlefonts/Exo-2.0) font, bold instance, shipped by Miyoo-UI in `res/fonts/` (SIL Open Font License 1.1, see `OFL.txt` there)
- Built with OnionOS's [Miyoo Mini toolchain](https://hub.docker.com/r/aemiii91/miyoomini-toolchain) and its SDL 1.2 libraries

## License

TBD
