# Nancy SID Player

A terminal-based SID player using sidplayfp library with a text user interface (TUI).

## Features

- **File Browser**: Navigate directories and browse SID files (.sid, .psid, .rsid, .mus, .str, .prg)
- **STIL Integration**: Display STIL (SID Tune Information List) database information
- **Vim-style Navigation**: Use h/j/k/l keys for intuitive directory navigation
- **Playback Controls**: Play, pause, stop, next/previous track
- **Track Information**: Display title, author, copyright, and track count
- **Multi-track Support**: Navigate between subtunes in SID files
- **Real-time Display**: Shows playing status and elapsed time
- **HVSC Compatible**: Designed to work with High Voltage SID Collection

## Dependencies

- libsidplayfp
- ncurses
- libpulse-simple
- cmake (build system)

### Ubuntu/Debian
```bash
sudo apt-get install libsidplayfp-dev libncurses-dev libpulse-dev cmake build-essential
```

### Fedora/RHEL
```bash
sudo dnf install sidplayfp-devel ncurses-devel pulseaudio-libs-devel cmake gcc-c++
```

### Arch Linux
```bash
sudo pacman -S sidplayfp ncurses pulseaudio cmake base-devel
```

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

```bash
./nancyplayer
```

### Controls

- **j/k or ↑/↓**: Navigate up/down in file browser
- **h or BACKSPACE**: Go to parent directory
- **l or ENTER**: Play selected SID file or enter directory
- **SPACE**: Pause/resume playback
- **s**: Stop playback
- **n**: Next track (subtune)
- **p**: Previous track (subtune)
- **q**: Quit

### Interface Layout

```
┌─────────────────────────────────────────────────────────────┐
│ Nancy SID Player                          Press 'q' to quit │
├─────────────────────────────────────────────────────────────┤
│ Player                                                      │
│ File: /path/to/file.sid                                     │
│ Title: Song Title                                           │
│ Author: Composer Name                                       │
│ Copyright: (C) Year                                         │
│ Track: 1/3                      Time: 01:23    [PLAYING]    │
├──────────────────────┬──────────────────────────────────────┤
│ File Browser         │ STIL Information                     │
│ Path: /current/path  │ Title: Amazing Demo Song             │
│ [DIR] subfolder      │ Artist: Famous Composer             │
│ [SID] music.sid      │ Copyright: (C) 1989 Demo Group      │
│ [SID] track.sid      │ Comment: This is an amazing song     │
│                      │ that was composed for a famous demo │
├──────────────────────┴──────────────────────────────────────┤
│ Files: 15 | Path: /HVSC/MUSICIANS/A/Author                  │
├─────────────────────────────────────────────────────────────┤
│ j/k: Up/Down | h: Parent | l/ENTER: Play/Enter | q: Quit   │
└─────────────────────────────────────────────────────────────┘
```

## File Format Support

- **.sid**: Standard SID files
- **.psid**: PlaySID format
- **.rsid**: RealSID format  
- **.mus**: Compute! Sidplayer format
- **.str**: SIDTracker format
- **.prg**: C64 program files

## License

This project is open source. Please check individual component licenses for sidplayfp and other dependencies.
