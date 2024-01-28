1ReadMe.rtf/txt
December 13, 1994
Examples:Audio

This is the ReadMe file for the Audio Examples folder.  It briefly describes each 
program maintained in the Audio folder.  For more information on a specific 
program, see the Examples documentation, or see the header for the source code 
in question.

About the AIFF Samples folder

Many of the examples in the Audio folder require sound samples from the “AIFF 
Samples” folder in order to run.  This folder was most recently distributed on the 
Toolkit 1.5 CD.  To use these samples, it is necessary to copy the AIFF Samples 
folder from your Toolkit CD to your working /remote folder.  

First, drag the “AIFF Samples” folder to the “remote” folder on your hard disk.  
Be patient, as the AIFF Samples folder is very large.  Then, rename the copied 
“AIFF Samples” folder to  “aiff” .  To inform the 3DO Debugger about the 
location of the AIFF samples, type the following line in at the 3DO-> prompt:

alias samples /remote/aiff

You can now move the audio example executables and support files into the 
/remote folder, and run them.  Each of the example programs will attempt to 
load the AIFF samples it needs via the alias “$samples”.

To automate this process, insert that line into the AppStartup file, which resides 
in your remote folder.

About the Audio.make makefile

To compile all of the audio examples, you can use the Audio.make file.  Copy the 
Examples:Audio folder to your hard drive.  Then, start MPW and use the Set 
Directory... command to set the current directory to the new Examples:Audio 
folder.  Finally, use the Build... command with Audio as the makefile name.  The 
makefile will play a tune when the make is completed.

Talking to us

We’re always interested in improving our examples.  If you have specific 
comments or questions about these example programs, a good way to contact us 
is via 3DO’s InfoServer BBS.

Examples:Audio

beep

Play synthetic waveform for 2 seconds. This demonstrates loading, 
connecting and playing instruments. It also demonstrates use of the audio 
timer for time delays.

capture_audio [<num frames> [<dest file>]]

This program captures the output from the DSP into a file on the 
development station's filesystem. It can be used to check the sound output 
of your program at important points.

minmax_audio

This program samples the output of the DSP and returns the maximum 
and minimum output values of the DSP. You can use minmax_audio to 
check that your program outputs reasonable, non-clipping levels of audio.

playmf <MIDI file> <PIMap file> [<num repeats>]

This sample application loads a standard MIDI format file, loads 
instruments and AIFF samples described in a PIMap file, and plays the 
MIDI file the specified number of times. It demonstrates usage of the 
Juggler and the score playing routines.

playsample [<sample file> [<rate>]]

This program shows how to load an AIFF sample file and play it using the 
control pad. Use the A button to start the sample, the B button to release 
the sample, and the C button to stop the sample. The X button quits the 
program.

playsoundfile <sample file>...

Calls SoundFilePlayer routines to play one or more AIFF files.

simple_envelope

Simple demonstration of an envelope used to ramp amplitude of a 
sawtooth waveform. A 3-point envelope is used to provide a ramp up to a 
specified amplitude at the start of the sound and a ramp down when the 
sound is to be stopped. This is one technique to avoid audio pops at the 
start and end of sounds.

spoolsoundfile <sound file> [<num repeats>]

Plays an AIFF sound file using a thread to manage playback.

ta_attach

Creates a pair of software-generated samples and attaches them to a 
sample player instrument. The attachments are then linked to one another 
using LinkAttachment() such that they play in a loop.

ta_customdelay [<sample file> [<delay ticks>]]

This program demonstrates how to create and use a delay line to get real-
time echo effects in your program.  It loads the specified AIFF file and 
plays it into a delay line. By tweaking the knobs on the output mixer, you 
can control the mix of delay sound versus original sound, and the speed at 
which the echo will die down.

ta_envelope [<test code>]

Demonstrates creating, attaching, and modifying two envelopes which are 
attached to sawtooth instruments.

ta_pitchnotes [<sample file> [<duration>]]

This program loads and plays the AIFF sample file at several different 
pitches. It does this by selecting a MIDI note number, which the audio 
folio maps to a frequency.

ta_spool

Uses the music.lib sound spooler to fill buffers, parse information in the 
buffers, signal our task when a buffer has been exhausted, and refill the 
buffers.  Use this sample code as a basis for developing your own routines 
for playing large sampled files, or handling other kinds of buffered data.

ta_sweeps

This program quickly modulates the amplitude and frequency of a 
sawtooth instrument via tweaking the control knobs repeatedly.

ta_timer

This program shows how to examine and change the rate of the audio 
clock.  It demonstrates use of cues to signal your task at a specific time. It 
also demonstrates how the audio folio deals with bad audio rate values.

ta_tuning

This program demonstrates how to create a tuning table, how to create a 
tuning, and how to apply a tuning to an instrument.

ta_tweakknobs <dsp instrument>

This program finds the names of all knobs on an instrument and tweaks 
them to NUMSTEPS * (number of knobs) possible permutations. This 
program demonstrates how to find and tweak knobs on an instrument, 
and is useful for testing instrument templates.

tsc_soundfx

Demonstrate use of ScoreContext as a simple sound effects manager. This 
gives you dynamic voice allocation, and a simple MIDI-like interface.

Examples:Audio:Advanced_Sound_Player

tsp_algorithmic

Given a long sound file with several markers defined therein, this 
program demonstrates interactive AIFF sound branching.  The technique 
used to implement this sequence involves the use of static branches, where 
one segment always leads into another, (e.g. after playing the 1st ending 
always goes back to the loop segment), and decision functions where a 
conditional branching is required (e.g. the end of the loop segment either 
goes to the 1st or 2nd ending).

tsp_rooms

Creates a thread to playback a sound track based on a global room 
variable gRoom.  The soundtrack thread, Soundtrack, uses the advanced 
sound player to play a unique sound file for each room. When the main 
task changes rooms, the soundtrack thread adapts the soundtrack to the 
change in room at a musically convenient location.

tsp_spoolsoundfile <sound file> [<num repeats>]

Plays an AIFF sound file using a thread to manage playback.

tsp_switcher

Loops one of three sound files off of disc. The user can select a different 
sound to loop by pressing the A, B, or C buttons on the control pad. The 
last button pressed corresponds to the sound being played.

Examples:Audio:Coal_River

CoalRiver

This program plays a MIDI file using Juggler and Score toolbox
from the music.lib and audio folio, and demonstrates special MIDI
interpreter functions.

The CoalRiver.pimap file refers to some AIFF samples which were 
included in the "AIFF Samples" folder.  It is necessary to copy the "AIFF 
Samples" folder into the remote folder to use them in this program.

Examples:Audio:DrumBox

drumbox <PIMap>

This program loads and plays up to eight rhythm sounds in a repeating 
measure of sixteen beats. Use the D-pad and the A button to place or 
remove a sound from a specific point in the rhythm.

<PIMap> is the path and filename of a PIMap file to use. A standard 
"drumbox.pimap" is provided, so try using drumbox.pimap as the 
argument to drumbox.

Examples:Audio:Juggler

tj_canon <PIMap>

This program shows how to create and play a simple canon using the 
juggler and score-playing routines.

tj_multi

This program tests the juggler using non-musical events and software-
based "timing."  It constructs two synthetic sequences and creates a 
collection based on these two sequences. It then "plays" the collection via a 
simple print function, processed at the time of each event in each 
sequence.

tj_simple

This program tests the juggler using non-musical events and software-
based "timing."  It constructs two synthetic sequences and "plays" the 
sequences via a simple print function, processed at the time of each event 
in each sequence.

Examples:Audio:PatchDemo

This directory contains sample text files which can be provided as input to 
the patchdemo command.  The patchdemo command allows you to 
experiment with DSP instruments by creating patch files which describe 
how to attach them to one another.  The input file format to patchdemo 
are documented in the 3DO Tools for Sound Design manual.

Beginning with the Portfolio 2.5 release, the patchdemo command can be 
accessed via the following command in the 3DO Debugger:

$c/patchdemo <patch file>

To use these patches, it is necessary to copy the "AIFF Samples" folder into 
the remote folder.

Examples:Audio:Songs:

This directory contains five sets of .PIMap and .MF (MIDI format) files 
which can be provided as input to the playmf command.  To hear these 
songs, use the following command in the 3DO Debugger:

$c/playmf <MIDI file> <PIMap file> [<num repeats>]

To hear these songs, it is necessary to copy the "AIFF Samples" folder into 
the remote folder.

