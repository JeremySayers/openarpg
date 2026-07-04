# OpenARPG

An open source action RPG, built in C with [raylib](https://www.raylib.com), free for anyone to play, modify, and contribute to.

This project just started — there's no gameplay yet, just a window that opens. Follow along or jump in; see [CLAUDE.md](CLAUDE.md) for the current architecture plans and coding conventions.

## Building

Requires [CMake](https://cmake.org) 3.24+. Raylib itself is fetched and built automatically, no separate setup needed.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Run it:
- macOS/Linux: `build/openarpg/openarpg`
- Windows: `build/openarpg/Release/openarpg.exe`

For a debug build, use `-DCMAKE_BUILD_TYPE=Debug`. On macOS, pass `-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"` to produce a universal binary.

For a web build (requires an activated [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)):

```sh
emcmake cmake -B build-web -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --config Release
```

Output lands at `build-web/openarpg/openarpg.html`. Serve it over HTTP rather than opening the file directly (browsers block wasm/file loading over `file://`):

```sh
cd build-web/openarpg && python3 -m http.server
```

## Contributing

There's no formal contributing guide yet — the project is too young for one to mean much. For now: open an issue or PR, and see [CLAUDE.md](CLAUDE.md) for coding conventions and the direction things are headed.

## License

Licensed under the [MIT License](LICENSE). Built with [raylib](https://www.raylib.com).
