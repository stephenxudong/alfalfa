# Alfalfa [![Build Status](https://travis-ci.org/excamera/alfalfa.svg?branch=master)](https://travis-ci.org/excamera/alfalfa)

Alfalfa is a [VP8](https://en.wikipedia.org/wiki/VP8) encoder and
decoder, implemented in explicit state-passing style and developed by
the Systems and Networking Research group at Stanford University. It
is the basis for the
[ExCamera](https://www.usenix.org/conference/nsdi17/technical-sessions/presentation/fouladi)
and [Salsify](https://snr.stanford.edu/salsify) systems.

## License

Almost all the source files are licensed under the [BSD 2-clause
license](https://opensource.org/licenses/bsd-license.php). Alfalfa
links against [x264](https://www.videolan.org/developers/x264.html) to
compute the SSIM (quality) of frames. Because this library is
distributed under the GNU GPL 2+, so is Alfalfa's
[ssim.cc](https://github.com/excamera/alfalfa/blob/master/src/util/ssim.cc)
file, and the overall Alfalfa package.

## Build directions

To build the source, you'll need the following packages:

* `g++` >= 5.0
* `yasm`
* `libxinerama-dev`
* `libxcursor-dev`
* `libglu1-mesa-dev`
* `libboost-all-dev`
* `libx264-dev`
* `libxrandr-dev`
* `libxi-dev`
* `libglew-dev`
* `libglfw3-dev`

### install dependencies
```sh
$ sudo apt install yasm libxinerama-dev libxcursor-dev libglu1-mesa-dev libboost-all-dev libx264-dev libxrandr-dev libxi-dev libglew-dev libglfw3-dev
```

### install logging component
```sh
$ git clone https://github.com/gabime/spdlog.git
$ cd spdlog && mkdir build && cd build
$ cmake .. && make -j && sudo make install
```

### The rest should be straightforward:

```
$ ./autogen.sh
$ ./configure
$ make -j$(nproc)
$ sudo make install
```


## Salsify

Source code for Salsify sender and reciever programs can be found at [`src/salsify`](https://github.com/excamera/alfalfa/tree/master/src/salsify).

First, check the pixal format your camera supports:

```sh
$ sudo apt install v4l-utils
$ v4l2-ctl --device /dev/video0 --list-formats
```

Then, run the receiver program:

```
salsify-receiver [PORT] 1280 720
```

Then, run the sender program:

```
salsify-tcp-sender --device [CAMERA, usually /dev/video0] --pixfmt YUYV [HOST] [PORT] 1337
```

The default pixel format is YUV420. Most webcams support raw YUV420, however the frame rate might be low.


