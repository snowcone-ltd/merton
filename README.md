`merton` is a work-in-progress multi-system emulator frontend build with [`libmatoya`](https://github.com/chrisd1100/libmatoya). It uses a its own [simple interface](https://github.com/snowcone-ltd/merton/blob/main/src/core.h) for interacting with emulator cores in addition to being compatible with the `libretro` API.

<img src='https://user-images.githubusercontent.com/328897/214193293-cc25bdc6-640b-43af-a1ba-ee4deb693be4.png' width=400></img>

### Design Goals

- Small binary footprint
- Simple user interface
- Cross platform support
- Limited, straightforward feature set
- Works well "out of the box"
- Intuitive ROM and game loading
- Uncomplicated code base
- Curated list of systems and cores w/ seamless core loading

If you're interested in being part of the journey, join us on [Discord](https://discord.gg/VJYMmZM6z8).

### Systems

- Atari 2600
- Nintendo Entertainment System (Famicom)
- Master System / SG-1000 / Game Gear
- TurboGrafx-16 (PC Engine) / CD-ROM / Super CD-ROM / Arcade Card
- Genesis (Mega Drive)
- Gameboy
- TODO: Neo Geo
- Super Nintendo Entertainment System (Super Famicom)
- TODO: 3DO
- TODO: Sega Saturn
- PlayStation
- Nintendo 64
- TODO: WonderSwan
- Game Boy Advance

### Building

`merton` is still a work in progress so we are not publishing formal releases yet.

In order to build `merton`, you need to clone and build [`libmatoya`](https://github.com/chrisd1100/libmatoya) side-by-side to this repo (the makefile will look for the `libmatoya` static library during linking).

Other than `libmatoya`, `merton` has no dependencies outside of what's shipped with the basic developer toolchains. Run `make` or `nmake` (on Windows) to output the binary.
