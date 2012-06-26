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

The makefile assumes `gcc` 4.5 or higher so it can do link-time
optimization; if you don't have it, just remove `-flto` from the
definition of `OPTFLAGS` and `LDOPTFLAGS`.

Build
------------

    make

Run
------------

    ./leitshow

At this point, we sometimes use the `pavucontrol` program to choose
what audio stream to send to the lightshow program.  When we don't
explicitly assign a channel with `pavucontrol`, pulseaudio sometimes
gives the lightshow the microphone audio stream by default.

Customize
------------

The lightshow will attempt to send commands via `/dev/ttyUSB0`.  You
can change this, along with the number of channels and a few other
things, in the `#define`s section at the beginning of `test.c`.
Below, we highlight the most useful parameters for tuning.

### Calculation of statistics

Right now we just calculate the power from the complex FFT data.  You
could use the phase data for something, or maybe decorrelate based on
the complex dot product.

 - `BUFSIZE` lets you adjust how big the chunks of PulseAudio data
   are.  The smaller this is, the more often you'll recompute things
   and send commands.  Keep it a power of 2 for maximum FFTW speed.

 - `BUFFER_CYCLE` lets you reuse some of the audio data (a value of 1
   doesn't reuse any; larger reuses that many in a window).  This
   smooths things out and gives a lower minimum frequency at a given
   update rate.

 - `BIN_SHRINKAGE` makes the light show look at only the bottom
   portion of the frequency spectrum.

### Channel gain adaptation (`CHAN_GAIN`)

 - `CHAN_GAIN_FILTER_CUTOFF_HZ` is the cutoff frequency for the
   adaptive loop that adjusts channel gains to keep all channels
   active.

 - `CHAN_GAIN_GOAL_ACTIVITY` is roughly how much (from 0 to 1) power
   we want each channel to have.  The gains will slowly adapt to match
   the actual powers to these values.

You can probably figure out the rest, but they're not as important.

### Differentiation and decorrelation (`DECORR`)

 - `DECORR_BASE_CHANNEL` is the index of the channel from which the
   others will compute difference signals.

 - `DECORR_PERCENT_DERIV` is what fraction (from 0 to 1) of the
   non-base channels will be computed from the direct derivative of
   that frequency bin signal.  The rest will come from differencing
   with the base channel.

### Thresholding (`THRESH`)

Thresholding is like the channel gain adaptation, but it turns the
channels completely off to keep things interesting.

 - `THRESH_FILTER_CUTOFF_HZ` and `THRESH_GOAL_ACTIVITY` are just like
   the corresponding values for `CHAN_GAIN`.  The lightshow will
   slowly adapt the thresholds to keep the channel on for
   `THRESH_GOAL_ACTIVITY[i]` of the time and off for the rest.

### Envelope filtering (`BIN_FILTER`)

 - `BIN_FILTER_CUTOFF_HZ` is the array of cutoff frequencies for
   low-pass filters on each channel.  Don't make these too fast, or
   the light show will get super flickery.

Bugs
----------

Please let me know!

Thanks
-----------

This lightshow is a crappy approximation of the one at tEp at MIT.
Almost every aspect of it was inspired by the tEp one, and if I hadn't
worked on a few of the incarnations of the tEp lightshow, this would
have taken a lot longer.  That lightshow was written, at various
times, by Frostbyte, Penguin, sMark, Elmo and Knosepiq.

Henry Hallam, as is his custom, got all the hardware working in a
remarkably short period of time (and helped me resurrect it over the
phone!)

Dany (Knosepiq) provided endless advice and suggestions for tuning.
