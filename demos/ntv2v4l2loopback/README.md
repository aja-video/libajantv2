# virtual-webcam

## Purpose

### Linux
This command-line application captures video and audio from NTV2-compatible AJA devices such as Kona5 and plays it back using V4L2 loopback for Video and ALSA loopback for Audio.

### Windows
This command-line application captures video and audio from NTV2-compatible AJA devices such as Kona5 and plays it back using DirectShow.

## Background
AJA already has a V4L2 driver that you can use with NTV2 Video I/O boards. However, it was designed to be a standalone driver since it would not load correctly in conjunction with AJA's main ntv2 kernel driver. In order to circumvent that issue, we've built a user space application that plugs into the V4L2 subsystem using the loopback API. The loopback API allows you to create virtual video devices that any v4l2 application can read as if they were an ordinary video device. This application captures video from an NTV2 Video I/O board and redirects it to the virtual video device. As an added advantage, you can create a worflow that behaves like a virtual webcam. The audio portion is handled similarly with ALSA loopback API.

## Build

Navigate to the virtual-webcam folder in a terminal window.
```bash
cmake -S . -B build
cmake --build build
```

### Linux
The V4L2 loopback interface namely /dev/v4l2loopback that is used to create loopback devices on-the-fly is not available on kernel version 5.15. If you need to run this demo app on that version of the kernel, then you need to uncomment the line that defines AJA_BUILD_FOR_LINUX_KERNEL_5_15 in ntv2loopback.h.

## Setup

### Linux

#### 1. V4L2 loopback module
Clone https://github.com/umlaeute/v4l2loopback/ so you can build and install this driver for your distro.
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
Determine if its installed:
```bash
pkg-config --version
```
Use this to install:
```bash
sudo apt-get install pkg-config
```

#### 5. AJA NTV2 SDK
Follow the instructions at https://github.com/aja-video/libajantv2 to build the SDK.

### Windows

#### 1. Register DirectShow filter and output pins.
```bash
regsvr32 virtual-webcam.dll
```

## Run

### Linux

#### 1. Load V4L2 loopback module:
   ```bash
   sudo modprobe v4l2loopback
   sudo chmod 666 /dev/v4l2loopback
   ```
#### 2. Load ALSA loopback module:
   ```bash
   sudo modprobe snd-aloop
   ```
#### 3. Run virtual webcam ('N' represents the zero-based index of the loopback device):
   ```bash
   sudo ./virtual-webcam -d 1 -h sdi -c 1 -p uyvy -i /dev/videoN -u "hw:Loopback,0,0" -a 1
   sudo chmod 666 /dev/videoN
   ffplay -f v4l2 -i /dev/videoN
   ffplay -f alsa -channels 2 -sample_rate 44100 -i plughw:CARD=Loopback,DEV=1
   ```

##### If you need to run this demo app on kernel 5.15 then you will need to use this command to load the V4L2 loopback module instead:
   ```bash
   sudo modprobe v4l2loopback devices=2 video_nr=0,1 card_label="","AJA virtual webcam device 1"
   ```

### Windows

#### 1. Using VLC
```bash
1. Choose Media | Open Capture Device.
2. Choose AJA Virtual Camera under video device name and audio device name.
3. Click Play.
```

#### 2. Using GraphEdit
```bash
1. Insert following filters:
AJA Virtual Camera under DirectShow filters, Video Renderer under DirectShow filters and Internal Speakers under Audio Renderers.
2 Click Play.
```

#### 3. Using OBS
```bash
1. Insert Video Capture Device and choose AJA Virtual Camera.
2. Make sure DirectSound is enabled on the properties page.
3. Audio and Video will start playing automatically once you save the properties.
```

## Troubleshooting

### Linux

* This application needs to run as superuser to create V4L2 loopback devices on-demand.

* Get a list of open V4L2 loopback devices:
```bash
v4l2-ctl --list-devices
```

* /dev/video0 is created by default when you load the V4L2 loopback module.

* You can delete any V4L2 loopback device:
```bash
sudo ./v4l2loopback-ctl delete N
```

* Get video format details of the loopback device:
```bash
v4l2-ctl -d /dev/videoN --all
```

* VLC to playout the video content from a V4L2 loopback device:
```bash
vlc v4l2:///dev/videoN
```

* Make sure the ALSA PKG config file is available:
```bash
find /usr -name alsa.pc
```

* Make sure the ALSA module is installed and available to PKG Config:
```bash
pkg-config --cflags --libs alsa
```

* Get a list of all the audio devices available for output:
```bash
aplay --list-devices
```

* I've had occational issues playing audio with ALSA loopback. Sometimes this seems to helps:
```bash
sudo alsa force-reload
```

### Windows

* Unregister filter and outputs pins:
```bash
regsvr32 /u virtual-webcam.dll
```
