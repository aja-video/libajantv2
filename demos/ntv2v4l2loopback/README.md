# ntv2v4l2loopback Demo

## Purpose

## Background
For many years, AJA provided a V4L2 kernel driver to use with its NTV2 Video I/O boards, but it was
separate from (and not integrated with) the standard NTV2 kernel driver that comes with its Open Source
SDK and its popular “Retail” software installer package ([available from AJA](https://www.aja.com/)).

This demo replaces the old, unmaintained V4L2 driver, and replaces it with a user-space application
that inserts itself into the V4L2 subsystem through its loopback API, which allows the creation of
virtual video devices that V4L2 applications can read from as if they were ordinary video devices.

### Linux
This command-line application captures video and audio from NTV2-compatible AJA devices
and plays it back using V4L2 loopback for Video and ALSA loopback for Audio.

**NOTE:** The V4L2 loopback interface, namely `/dev/v4l2loopback` that’s used to create loopback devices
on-the-fly is not available on kernel version 5.15. To run this demo on the older kernel, uncomment the
line that defines `AJA_BUILD_FOR_LINUX_KERNEL_5_15` in `ntv2loopback.h`.

### Windows
This DLL captures video and audio from NTV2-compatible AJA devices and plays it back using DirectShow.

---
## Setup

### Linux

#### 1. V4L2 loopback module
Clone `https://github.com/umlaeute/v4l2loopback/`, then build and install it for your distro:

```bash
make && sudo make install && sudo depmod -a
```

#### 2. V4L utils

```bash
sudo apt-get install v4l-utils
```

#### 3. ALSA loopback module & utils

```bash
sudo apt-get install libasound2-dev alsa-utils
```

#### 4. PKG Config
Determine if `pkg-config` is installed:

```bash
pkg-config --version
```

If it’s not, then install it:

```bash
sudo apt-get install pkg-config
```

#### 5. AJA NTV2 SDK
Follow the instructions at `https://github.com/aja-video/libajantv2` to build the SDK and demos.


### Windows

#### 1. AJA NTV2 SDK
Follow the instructions at `https://github.com/aja-video/libajantv2` to build the SDK and demos.
`v4l2loopback.dll` is built with the rest of the demo applications.

#### 2. Register DirectShow filter and output pins
Run the following in a `cmd.exe` window that has elevated (Administrator) privileges
(after `cd`ing into the build folder containing `v4l2loopback.dll`):

```bash
regsvr32 path\to\v4l2loopback.dll
```

----
## Run

### Linux

#### 1. Load V4L2 loopback module:

On modern kernels (e.g. 6.8), run these commands:

   ```bash
   sudo modprobe v4l2loopback
   sudo chmod 666 /dev/v4l2loopback
   ```

On the older 5.15 kernel, use this command instead:

   ```bash
   sudo modprobe v4l2loopback devices=2 video_nr=0,1 card_label="","AJA virtual webcam device 1"
   ```

#### 2. Load ALSA loopback module:

   ```bash
   sudo modprobe snd-aloop
   ```

#### 3. Run ntv2v4lloopback ('0' represents the first loopback device):

   ```bash
   sudo bld/libajantv2/demos/ntv2v4l2loopback -c1 -puyvy --vdev /dev/video0 -u "hw:Loopback,0,0" --adev 1
   sudo chmod 666 /dev/video0
   ffplay -f v4l2 -i /dev/video0
   ffplay -f alsa -channels 2 -sample_rate 44100 -i plughw:CARD=Loopback,DEV=1
   ```


### Windows

#### 1. Use VLC

```bash
1. Choose Media | Open Capture Device.
2. Choose AJA Virtual Camera under video device name and audio device name.
3. Click Play.
```

#### 2. Use GraphEdit

```bash
1. Insert following filters:
AJA Virtual Camera under DirectShow filters, Video Renderer under DirectShow filters and Internal Speakers under Audio Renderers.
2 Click Play.
```

#### 3. Use OBS

```bash
1. Insert Video Capture Device and choose AJA Virtual Camera.
2. Make sure DirectSound is enabled on the properties page.
3. Audio and Video will start playing automatically once you save the properties.
```

----
## Troubleshooting

### Linux

* The demo must run as `root` (superuser) to create V4L2 loopback devices on-demand.

* To get a list of open V4L2 loopback devices:

```bash
sudo v4l2-ctl --list-devices
```

* `/dev/video0` is created by default when you load the V4L2 loopback module.

* To delete a V4L2 loopback device:

```bash
sudo ./v4l2loopback-ctl delete N
```

* To get video format details of the loopback device:

```bash
sudo v4l2-ctl -d /dev/videoN --all
```

* To have VLC play video content from a V4L2 loopback device:

```bash
sudo vlc v4l2:///dev/videoN
```

* Be sure the ALSA PKG config file is available:

```bash
find /usr -name alsa.pc
```

* Be sure the ALSA module is installed and available to PKG Config:

```bash
pkg-config --cflags --libs alsa
```

* List all audio devices available for output:

```bash
aplay --list-devices
```

* On occasion, this seems to help if there are issues playing audio with ALSA loopback:

```bash
sudo alsa force-reload
```

### Windows

* To unregister filter and output pins:

Run the following in a `cmd.exe` window that has elevated (Administrator) privileges
(after `cd`ing into the build folder containing `v4l2loopback.dll`):

```bash
regsvr32 /u v4l2loopback.dll
```
