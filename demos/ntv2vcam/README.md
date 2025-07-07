# ntv2vcam Demo

## Background
In the past, AJA provided a V4L2 kernel driver to use with its NTV2 Video I/O boards, but it was
separate from (and not integrated with) the standard NTV2 kernel driver that comes with its Open Source
SDK and its popular “Retail” software installer package ([available from AJA](https://www.aja.com/)).
The two drivers could not coexist. In addition, the old driver was seldom maintained, and failed to build
with newer Linux kernels and V4L2 versions.

The objectives of this demo are two-fold:
-	On **Linux**, it builds a command-line program that inserts itself into the V4L2 subsystem through
	its loopback API, which allows the creation of virtual video devices that V4L2 applications can read
	from as if they were ordinary video devices. Underneath, the normal, well-tested NTV2 device driver
	operates the AJA device, which benefits from frequent updates and testing on many Linux kernels.

	The demo also captures audio from NTV2 devices, making the audio stream available to applications
	through the ALSA loopback facility.

-	On **Windows**, the same basic code builds a DLL that, when registered, allows **DirectShow**
	applications to stream video and audio from AJA devices through the normal NTV2 device driver.


## Setup

-	**Linux**

	1.	**v4l2loopback module**

		Clone [https://github.com/umlaeute/v4l2loopback/](https://github.com/umlaeute/v4l2loopback/),
		**cd** into it, then build and install it for your distro:

		```bash
		make && sudo make install && sudo depmod -a
		```

	2.	**v4l-utils**

		```bash
		sudo apt-get install v4l-utils
		```

	3.	**ALSA loopback module & utils**

		```bash
		sudo apt-get install libasound2-dev alsa-utils
		```

	4.	**pkg-config**

		Determine if `pkg-config` is installed:

		```bash
		pkg-config --version
		```

		If not, then install it:

		```bash
		sudo apt-get install pkg-config
		```

	5.	**AJA NTV2 SDK**

		Follow the instructions
		at [https://github.com/aja-video/libajantv2](https://github.com/aja-video/libajantv2)
		to build the SDK and demos.

		**ntv2vcam** is built with the rest of the demo applications.

-	**Windows**

	1.	**AJA NTV2 SDK**

		Follow the instructions
		at [https://github.com/aja-video/libajantv2](https://github.com/aja-video/libajantv2)
		to build the SDK and demos.

		**ntv2vcam.dll** is built with the rest of the demo applications.

	2.	**Register DirectShow filter and output pins**

		Run the following in a **cmd.exe** window that has Administrator privileges
		(but first **cd** into the build folder containing **ntv2vcam.dll**):
		```bash
		regsvr32 ntv2vcam.dll
		```


## Run

-	**Linux**

	1.	**Load V4L2 loopback module:**

		Run these commands:

		```bash
		sudo modprobe v4l2loopback
		sudo chmod 666 /dev/v4l2loopback
		```

		**IMPORTANT:** If **/dev/v4l2loopback** fails to be created:
		-	Unload the V4L2 loopback module:

			```bash
			sudo modprobe -r v4l2loopback
			```

		-	Reload the V4L2 loopback module using this command instead:

			```bash
			sudo modprobe v4l2loopback devices=2 video_nr=0,1 card_label="","AJA NTV2 VCAM 1"
			```

		-	Uncomment the line that defines `AJA_MISSING_DEV_V4L2LOOPBACK` in `ntv2loopback.h`.
		-	Rebuild the SDK and demos.

	2.	**Load ALSA loopback module:**

		```bash
		sudo modprobe snd-aloop
		```

	3.	**Run ntv2vcam**

		Note that ‘0’ represents the first loopback device, and ‘bld’ is the build directory:
		```bash
		sudo bld/libajantv2/demos/ntv2vcam/ntv2vcam  --vdev /dev/video0  --adev "hw:Loopback,0,0"
		sudo chmod 666 /dev/video1
		ffplay -f v4l2 -i /dev/video1
		ffplay -f alsa -channels 2 -sample_rate 44100 -i plughw:CARD=Loopback,DEV=1
		```


-	**Windows**

	1.	**VLC**

		1.	Choose “Media | Open Capture Device”.
		2.	Choose “AJA Virtual Camera” under ”Video Device” name and ”Audio Device” name.
		3.	Click “Play”.

	2.	**GraphEdit**

		1.	Be sure to run the 64-bit version of **GraphEdit**.
		2.	Insert the following filters:
			-	“AJA Virtual Camera” under DirectShow filters”,
			-	“Video Renderer” under DirectShow filters”.
			-	“Internal Speakers” under Audio Renderers”.
		3.	Click “Play”.

	3.	**OBS**

		1.	Insert “Video Capture Device” and choose “AJA Virtual Camera”.
		2.	Make sure “DirectSound” is enabled on the Properties page.
		3.	Audio and Video will start playing automatically once you save the properties.


## Troubleshooting

-	**Linux**
	-	The demo must run as **root** (superuser) to create V4L2 loopback devices on-demand.

	-	To get a list of open V4L2 loopback devices:
		```bash
		sudo v4l2-ctl --list-devices
		```

	-	**/dev/video0** is created by default when you load the V4L2 loopback module.

	-	To delete a V4L2 loopback device:
		```bash
		sudo v4l2loopback-ctl delete N
		```

	-	To get video format details of the loopback device:
		```bash
		sudo v4l2-ctl -d /dev/videoN --all
		```

	-	To have VLC play video content from a V4L2 loopback device:
		```bash
		sudo vlc v4l2:///dev/videoN
		```

	-	Be sure the ALSA PKG config file is available:
		```bash
		find /usr -name alsa.pc
		```

	-	Be sure the ALSA module is installed and available to PKG Config:
		```bash
		pkg-config --cflags --libs alsa
		```

	-	List all audio devices available for output:
		```bash
		aplay --list-devices
		```

	-	On occasion, this seems to help if there are issues playing audio with ALSA loopback:
		```bash
		sudo alsa force-reload
		```

-	**Windows**
	-	To unregister filter and output pins, run the following in a **cmd.exe** window that has
		elevated (Administrator) privileges (but first **cd** into the build folder containing
		**ntv2vcam.dll**):

		```bash
		regsvr32 /u ntv2vcam.dll
		```
