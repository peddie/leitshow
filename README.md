leitshow
==============

This project is a music-synced lightshow.  It attempts to map an audio
stream onto a series of intensity commands for channels, presumably
then connected to lights, in a way that makes the light/music
combination entertaining.

Hardware
-----------

The hardware matters little, as long as you have a device that can
listen for the code word and then read channel intensities and turn
them into fet duty cycles.

### Example setup (at the Oasis):

Henry set up a bunch of cheap LED rolls from china to be driven by a
dsPIC motor controller board.  We elected to use 3 channels (R, G, B),
so the PIC simply receives the code word followed by 3 8-bit values
that indicate the desired intensity for each channel.  The motor
controller board already has 3 channels for driving brushless motors,
so we just directly translate the intensity commands to duty cycles
for all the bottom FETs.  We connect the LED rolls to a 12V power
supply and run the negative side through these motor control phases,
and we're done.

Get Dependencies
------------

    sudo apt-get install libfftw3-dev libpulse-dev

The makefile wants gcc-4.5; if you don't have it, you can change that
to just `gcc` and remove `-flto` from the definition of `OPTFLAGS` and
`LDOPTFLAGS`.

Build
------------

    make

Run
------------

    ./leitshow

At this point, I sometimes use the `pavucontrol` program to choose
what audio stream to send to the lightshow program.  When I don't
explicitly assign a channel with `pavucontrol`, pulseaudio sometimes
gives the lightshow the microphone audio stream by default.

Customize
------------

The lightshow will attempt to send commands via `/dev/ttyUSB0`.  You
can change this, along with the number of channels and a few other
things, in the `#define`s section at the beginning of `test.c`.

Bugs
----------

PulseAudio seems to like to send the program the microphone stream
rather than the music stream.  Not sure if this can be worked around
or if I need to switch to using JACK.

The lightshow doesn't work that well as a music-synced lightshow; it's
not that enjoyable to watch.  More algorithm development is needed.

Thanks
-----------

This lightshow is a crappy approximation of the one at tEp at MIT.
Almost every aspect of it was inspired by the tEp one, and if I hadn't
worked on a few of the incarnations of the tEp lightshow, this would
have taken a lot longer.  That lightshow was written, at various
times, by Frostbyte, Penguin, sMark, Elmo and Knosepiq.

Henry Hallam, as is his custom, got all the hardware working in a
remarkably short period of time.

