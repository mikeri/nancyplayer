# Nancy SID Player

A terminal-based SID player using libsidplayfp with ReSID-fp emulation and a modern text user interface (TUI).

Everything (including most of this README) is vibe coded with Claude, so don't trust anything, because AI is evil and bad.

## Features

- **File Browser**: Navigate directories and browse SID files (.sid, .psid, .rsid, .mus, .str, .prg)
- **STIL Integration**: Display STIL (SID Tune Information List) database information with multi-line comment support
- **Search Functionality**: Fast search through HVSC using Songlengths.md5 indexing (trigger with `/`)
- **Vim-style Navigation**: Use j/k for up/down, h/l for parent/enter, Shift+J/K for next/prev track
- **Playback Controls**: Play, pause, stop with real-time status
- **Track Information**: Display title, author, copyright, track count, and playback time
- **Multi-track Support**: Navigate between subtunes in SID files
- **Terminal Resize Support**: Automatically adapts to window size changes
- **HVSC Required**: Requires proper High Voltage SID Collection setup
- **Configurable Themes**: 256-color support with multiple built-in themes (default, dark, light, synthwave, retro, bumblebee)
- **XDG Compliance**: Configuration follows XDG Base Directory specification

## Dependencies

- **libsidplayfp**: SID emulation library
- **resid-builder**: ReSID-fp emulation engine (part of libsidplayfp)
- **ncurses**: Terminal user interface
- **libpulse-simple**: Audio output
- **cmake**: Build system

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

## Setup

Nancy SID Player requires the High Voltage SID Collection (HVSC) to function properly.

1. **Download HVSC**: Get the latest HVSC from [https://www.hvsc.c64.org/](https://www.hvsc.c64.org/)
2. **Extract to ~/Music/C64Music** (or your preferred location)
3. **Configure path**: Edit `~/.config/nancyplayer/config` if using a different location:
   ```
   theme=default
   hvsc_root=/path/to/your/C64Music
   ```

## Usage

```bash
./nancyplayer
```

On first run, Nancy SID Player will:
- Create configuration directory at `~/.config/nancyplayer/`
- Generate default config file and theme files
- Validate HVSC directory and show helpful error if not found

### Controls

#### File Browser
- **j/k**: Navigate up/down in file browser
- **h**: Go to parent directory  
- **l/ENTER**: Play selected SID file or enter directory
- **/**: Search mode (type to search, ESC to exit)

#### Playback
- **SPACE**: Pause/resume playback
- **s**: Stop playback
- **J** (Shift+j): Next track (subtune)
- **K** (Shift+k): Previous track (subtune)

#### General
- **q**: Quit


## Configuration

Nancy SID Player uses XDG Base Directory specification for configuration:

- **Config directory**: `~/.config/nancyplayer/` (or `$XDG_CONFIG_HOME/nancyplayer/`)
- **Main config**: `~/.config/nancyplayer/config`
- **Themes**: `~/.config/nancyplayer/themes/`

### Available Themes
- **default**: Clean white-on-black theme
- **dark**: Dark theme with subdued colors
- **light**: Light theme with dark text on light backgrounds  
- **synthwave**: Retro cyberpunk colors (magenta/cyan)
- **retro**: Classic green terminal theme
- **bumblebee**: Warm yellow/orange theme (based on cmus)

Change theme by editing the config file:
```
theme=bumblebee
hvsc_root=/home/user/Music/C64Music
```

## File Format Support

- **.sid**: Standard SID files
- **.psid**: PlaySID format
- **.rsid**: RealSID format  
- **.mus**: Compute! Sidplayer format
- **.str**: SIDTracker format
- **.prg**: C64 program files

## Technical Details

- **Emulation**: Uses libsidplayfp with ReSID-fp for accurate SID chip emulation
- **Audio**: PulseAudio output with configurable sample rate and buffer size
- **Memory Management**: Proper cleanup prevents segfaults during file switching
- **Threading**: Background audio playback with main thread UI
- **Performance**: Efficient search indexing and optimized scrolling behavior

## License

This project is open source. Please check individual component licenses for sidplayfp and other dependencies.
