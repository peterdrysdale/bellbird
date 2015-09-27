Audio Troubleshooting
=====================

In some Linux systems, Bellbird will fail and crash with error

    audio_open_alsa: failed to set sample rate .....

This error arises since the sound card is unable to accept data
at the sample rate which Bellbird is delivering it.
ALSA contains a resampling feature which overcomes this problem.
Please ensure the dmix ALSA plugin is installed (Most distributions
have this installed with the default ALSA package).
The user should create a file `.asoundrc` in their home directory 
with the following contents.

```
pcm.!default {
	type plug
	slave.pcm "dmixer"
}

pcm.dmixer  {
 	type dmix
 	ipc_key 1024
 	slave {
		pcm "hw:1,0"
		period_time 0
		period_size 1024
		buffer_size 4096
		rate 48000
	}
	bindings {
		0 0
		1 1
	}
}

ctl.dmixer {
	type hw
	card 0
}
```

Note the card number maybe different (0, 1, 2 ..). Select the rate based
on the sound card (typically 48000 or 44100).
