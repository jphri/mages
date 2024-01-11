# Magos

This game is in a really early stage. Nothing is set in stone for now.
This game will be just some dungeon crawler with some stuff, nothing too fancy, i guess. The objective of this project it was to make a cross-platform project, that runs on PC and Raspberry Pi 4, so the performance target is a Raspberry Pi 4 (this means your PC will likely have enough juice to run it unless you are using a dinosaur pre-2000 PCs, which in case: based).

## Building
There are 4 dependencies in this project:
1. glad (opengl es 3.1)
2. stb_vorbis
3. stb_image
4. SDL2 
The only dependency you need to care is SDL2, the rest is header only or custom generated (glad, in specific).

### Linux
To build on linux, just ```make``` it and it probably is going to compile. Remember to download the SDL2 development package.
```
$ sudo apt install libsdl2-dev # debian
$ sudo pacman -S sdl2 # arch
```

### Windows
On windows, is a bit more complex. It compiles with MinGW-w64.

1. Download the SDL2 pre-compiled binary from its repository 
2. Extract the directory inside the root directory of the source code
3. run this command on powershell (in the project directory):
```
> mingw32-make SDL2_PREFIX=path/of/sdl2/directory PLATFORM=x86_64-w64-mingw32 OUTPUT=game.exe <target> #target = "all", if you just want to build it
```

And it might build ~~probably~~. If it does not, read the Makefile to see what variables you should be set, and i'm not wrong because *it works on my machine™*.

## License

The license is MIT. Just do whatever you want, just don't lie about the source origin and you should be fine, at least respect that, you sick.

