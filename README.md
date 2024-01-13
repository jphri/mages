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

Also, there are 4 variables on Makefile: CC, SDL2_PREFIX, DEBUG and SANITIZE.
* CC = the compiler.
* SDL2_PREFIX = the directory where the binaries of SDL2 are. 

On linux, they are on /usr so they are already set, on Windows or cross compilation, you have to provide your own.

* DEBUG = debug symbols

Set to yes to enable it. This reduces performance, having DEBUG variable set is for debug symbols, helps us to not ship shit to your computer ~~although this game is already a very shitty one~~.

* SANITIZE = add sanitization options

Set to yes to enable it. This is intended to developers, reduces the performance to the fucking hell but gives us a lot of information in execution.

### Linux
Remember to download the SDL2 package.
```
$ sudo apt install libsdl2-dev # debian
$ sudo pacman -S sdl2 # arch
```

To compile:
```
make
```

### Windows
On windows, is a bit more complex. It compiles with MinGW-w64.

1. Download the SDL2 pre-compiled binary from its repository 
2. Extract the directory inside the root directory of the source code
3. run this command on powershell (in the project directory):
```
> mingw32-make SDL2_PREFIX=path/of/sdl2/x86_64-w64-mingw32 CC=x86_64-w64-mingw32-gcc <target> #target = "all", if you just want to build it
```

## License

The license is MIT. Just do whatever you want, just don't lie about the source origin and you should be fine, at least respect that, you sick.
