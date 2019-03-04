# loopsong
Loop a song

## Features
`loopsong` is a simple program using Allegro 5 ([https://liballeg.org/](https://liballeg.org/))
to perform song looping.
This program should accept any audio formats that Allegro supports for
its audio stream API.

Input files use the following format:
```ini
# comment
song=/path/to/song.ogg
start=start_seconds
end=end_seconds
```
The `song` field corresponds to the file path to the audio file to loop.
The `start` and `end` fields map to the beginning and end loop points
for the audio, specified in seconds.

## Build

This project uses the CMake build system, which one can obtain at
[https://cmake.org/download/](https://cmake.org/download/).

After cloning the repository,
```bash
git clone https://github.com/codylico/loopsong.git
```

the user should make and enter a `build` directory,
```bash
mkdir build
cd build
```

then run CMake from that build directory.
```bash
cmake path/to/source/of/loopsong
```

CMake should generate an IDE project or set of build files.
If using a Makefile-based build configuration, then run make.
```bash
make
```

The `loopsong` program should be built.

## Usage

A typical invocation of `loopsong` looks like the following:

```bash
loopsong [options] (file.ini) (seconds)
```

The `(file.ini)` argument corresponds to the input file described
earlier. The `(seconds)` argument specifies the amount of time
for which to play and loop the song.

Optional arguments are:
+ `-f (seconds)`: The `loopsong` program can fade out the song
  towards the end of playback. Set to zero for no fade out.
+ `-k (seconds)`: Playback can start from the seek position
  given in seconds.

Other arguments configure how `loopsong` adjusts the playback time.
+ `-m`: The default mode, _middle_, cuts the song off exactly after
  the amount of playback time specified. Using `-m` will likely result
  in ends of playback that sound abrupt or cut off unless the user
  employs the fading option (`-f`).
+ `-s`: The _short_ mode aims to end with the actual end of the song,
  preferring to finish early rather than play a partial song.
+ `-l`: The _long_ mode also aims to end with the actual end of the song.
  However, playback time may finish later to accomodate a proper
  conlusion to the song, rather than cut off playback midway through
  the song.

## License

This project uses the Unlicense, which makes the source effectively
public domain. Go to [http://unlicense.org/](http://unlicense.org/)
to learn more about the Unlicense.

Contributions to this project should likewise be provided under a
public domain dedication.
