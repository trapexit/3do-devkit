# Original Examples

## Portfolio 1.2

### 3DLib1.2
3D graphics library with nine demos (3d_example1-6, 3d_stereo, perftest_3d, showmodel) including progressive 3D rendering, stereoscopic display, benchmarking, and model viewing.

### 3DOBounce
Cel-animated 3DO logo bouncing around the screen using the Portfolio library. Demonstrates VRAM page management and sprite movement.

### 3DOBounceSnd
Same bouncing 3DO logo with added sound effects. Integrates the audio folio with cel-based animation.

### 3DOOrbit
Rotating 3DO logo using cel animation cycling with control pad input via the Event Broker.

### aaPlayer
Simple cel/anim player for loading and displaying image files and animations with speed adjustment.

### Access Manager Test
Tests access to system services through the Access Manager with control pad polling.

### AnimSample
Basic cel animation using the Portfolio animation system — loading, playing, and controlling an animation sequence.

### AssetManager
Block-file based asset management system with a test program demonstrating resource management.

### audio
Comprehensive audio demos: audiomon (DSP monitor), testaudio (all-in-one test), playmf (MIDI player), playsoundfile/spoolsoundfile (AIFF playback), loopsoundfile, and multiple ta_* template-attach examples (attach, customdelay, envelope, pitchnotes, sweps, timer, tuning, tweakknobs) demonstrating DSP instrument control.

### ColorEcho
Interactive colorful patterns responsive to control pad input, demonstrating color operations and visual feedback.

### CPlusExamples
C++ examples on 3DO: **HelloWorld** (C++ class output), **Strings** (CString class), **Lists** (CList container), **Application** (base class with events), **Shapes** (drawing lines/rects/text), **Sound** (sound file playback class), **Instrument** (AIFF instrument class), **SampleAppl** (combined demo moving a shape, changing color, playing sound).

### CPlusLib
C++ runtime library for 3DO with malloc/free/wrapper functions for CFront-compiled programs.

### eventbroker
Event Broker demos: cpdump (peripheral status), focus (input focus switching), lookie (event reporting), luckie (control pad reading), maus (mouse reading).

### FontLibrary
Font rendering library with an example program demonstrating loading fonts and displaying text.

### JoyJoy
Joystick/joypad interaction demo reading control pad events and displaying them on screen.

### Jumpstart2
High-level API examples: **JSAnimation** (play ANIM files), **JSInteractiveMusic** (interactive music), **JSIntMusicThread** (thread-based music), **JSPlayBgndMusic** (background music), **JSInteractiveSound** (sound effects), **JSMoveCel** (sprite movement with D-pad), **JSShowCel** (simple sprite display), **JSBasicSlideShow** (image slideshow), **JSSlideShowVDL** (slideshow with custom VDLs).

### LREX
Displays LRForm cels — a pre-rendered 3D animation format using the 3DO graphics pipeline.

### MemTest
Tests available DRAM memory on the 3DO, reporting allocation statistics and memory usage.

### Menu
Graphical menu for browsing and launching executable programs via control pad navigation.

### MessageSample
Inter-task message passing with a MessageClient/MessageSample server via kernel message ports.

### PerfTest
Performance benchmark measuring cel rendering speed and graphics throughput.

### PPMto3DO
Converts PPM images to 3DO native formats; includes makecel for creating 3DO cels.

### SlideShow
Image slideshow with VDL support using custom color lookup tables and screen transitions.

### Streaming_1.9a7
DataStreaming framework (v1.9a7) for streaming audio/video from CD. Includes example players (NuPlayer for Cinepak, PlaySA/PlaySAnim, SlideStream, TestDS/TestSA) and tools (Weaver, Chunkify, SFtoStream, SquashSnd, DumpAIFF, DumpStream, SAudioTool, AnimToSanm, CtlMaker).

### Symanim
Plays symmetric animation effects using the 3DO animation playback system.

### TaskDemo
Creating and managing concurrent tasks (threads) with SpawnThread.

### UFO
Interactive UFO-themed demo with cel animation and background music using PlayBgndMusic.

### UserFolioTemplate
Template for creating custom 3DO system folios with test program demonstrating folio structure.

### VersionChecker
Checks and reports versions of the 3DO operating system and system components.

---

## Portfolio 1.3

### Audio Monitor (audiomon)
Real-time DSP resource utilization display showing instrument and sample allocation.

### Audio Test (testaudio)
Comprehensive all-in-one audio folio test validating hardware functionality.

### Play MIDI File (playmf)
Plays standard MIDI files using the Juggler and score-playing routines from music.lib.

### Play Sound File (playsoundfile)
Plays one or more AIFF sound files using SoundFilePlayer routines.

### Spool Sound File (spoolsoundfile)
Threaded AIFF playback managing buffers from disk, demonstrating streaming audio.

### Template Attach (ta_attach)
Creates software-generated samples attached to a sample player instrument for looped playback.

### Template Custom Delay (ta_customdelay)
Real-time echo effects via delay line with knob-tweakable mix controls.

### Template Envelope (ta_envelope)
Creates and modifies two ADSR envelopes attached to sawtooth instruments for dynamic amplitude shaping.

### Template Pitch Notes (ta_pitchnotes)
Plays an AIFF sample at several pitches using MIDI note numbers mapped to frequencies.

### Template Spool (ta_spool)
Uses music.lib sound spooler to fill/refill audio buffers from disk for playing large samples.

### Template Sweeps (ta_sweeps)
Rapid amplitude and frequency modulation of a sawtooth instrument via knob tweaking.

### Template Timer (ta_timer)
Examines and changes the audio clock rate; uses cues to signal tasks at specific times.

### Template Tuning (ta_tuning)
Creates and applies tuning tables to instruments for microtonal/alternate tuning.

### Template Tweak Knobs (ta_tweakknobs)
Explores all knob names on a DSP instrument through permutations for parameter testing.

### Template DC Square (ta_dcsq)
DC-offset square wave synthesis on the audio DSP.

### Template Faders (ta_faders)
Fader/mixer level control on DSP instruments for audio fade in/out.

### Template Long Sample (ta_longsamp)
Playing long audio samples exceeding typical memory constraints.

### Template Make Sample (ta_makesamp)
Programmatically creates synthetic waveform samples and plays them.

### Template Random Knobs (ta_randknobs)
Randomly tweaks all available knobs on an instrument for sound design exploration.

### Patch Demo (patchdemo)
Loads DSP patch files describing instrument attachment configurations for sound design.

### Event Broker - cpdump
Prints control port peripheral status summary for debugging input devices.

### Event Broker - focus
Switches input focus between listeners via the Event Broker.

### Event Broker - lookie
Reports all events sent to a registered listener, demonstrating event reception.

### Event Broker - luckie
Reads events from the first control pad using the Event Utility Library.

### Event Broker - maus
Reads events from the first mouse using the Event Utility Library.

---

## Portfolio 2.5

### Audio
Extensive audio demos organized by category:
- **beep** — Plays a synthetic waveform with audio timer delay
- **capture_audio** — Captures DSP output to a file on the development station
- **minmax_audio** — Samples DSP output for max/min values to check levels
- **playmf** — MIDI file player with Juggler and PIMap instrument mapping
- **playsample** — Interactive AIFF sample player with control pad triggers
- **playsoundfile** — High-level SoundFilePlayer API for AIFF playback
- **simple_envelope** — 3-point envelope to ramp sawtooth amplitude cleanly
- **spoolsoundfile** — Threaded AIFF playback for background streaming
- **ta_attach** — Paired software-generated samples linked for looped playback
- **ta_customdelay** — Delay line with knob-controllable mix and decay
- **ta_envelope** — Two envelopes on sawtooth instruments for dynamic amplitude
- **ta_pitchnotes** — AIFF sample at different pitches via MIDI note mapping
- **ta_spool** — Sound spooler filling buffers and signaling tasks
- **ta_sweeps** — Rapid amplitude/frequency modulation via knob tweaking
- **ta_timer** — Audio clock rate changes and cue-based task signaling
- **ta_tuning** — Creating and applying tuning tables for microtonal scales
- **ta_tweakknobs** — Exploring all DSP knob permutations
- **tsc_soundfx** — ScoreContext as a sound effects manager with dynamic voice allocation
- **Advanced_Sound_Player/tsp_algorithmic** — Interactive AIFF branching with static/conditional paths
- **Advanced_Sound_Player/tsp_rooms** — Adaptive soundtrack that transitions on room changes
- **Advanced_Sound_Player/tsp_spoolsoundfile** — Threaded streaming audio with loop support
- **Advanced_Sound_Player/tsp_switcher** — Interactive soundtrack switching between three sound files
- **Coal_River** — MIDI file playback with special interpreter functions and PIMap mapping
- **DrumBox** — Loads and plays up to eight rhythm sounds in a 16-beat sequencer
- **Juggler/tj_canon** — Simple musical canon using Juggler and score routines
- **Juggler/tj_multi** — Juggler test with non-musical events and software-based timing
- **Juggler/tj_simple** — Basic Juggler test with two sequences and print function
- **PatchDemo** — Sample DSP patch files for instrument attachment experimentation
- **Songs** — MIDI song files and PIMaps (Bobs_Fugue, Coal_River, DrumBox, My_Groove, Zap)

### EventBroker
- **Control_Port_Peripherals** — Moves a cel with joystick, leaves image on trigger, supports mouse input
- **EventBroker_Tests** — cpdump, focus, lookie, luckie, maus programs for Event Broker testing
- **Joystick** — Joystick programming with data filtering, calibration, and cel movement
- **LightGun** — Lightgun hit detection with coordinate display in Debugger Terminal

### FileSystem
- **allocate** — File system space pre-allocation and storage management
- **ls** — Directory listing utility (Unix ls-style)
- **type** — File content display utility (Unix cat-style)
- **walker** — Recursive file system directory traversal and enumeration

### Fonts
- **Font_Library_Example** — Loading and rendering text with the Font Library
- **FontViewer** — Viewing and inspecting font files with character display
- **Kanji_FontViewer** — Specialized viewer for Kanji (Japanese) font files

### Graphics
- **3DO_Bounce** — Interactive bouncing 3DO logo with physics and sound
- **3DO_Orbit** — Rotating 3DO logo using frame-cycled cel animation
- **aaplayer** — Simple cel/anim player with speed control
- **AnimSample** — C++ example demonstrating basic cel animation loading and playback
- **ColorEcho** — Colorful interactive patterns with color manipulation
- **lrex** — LRForm cel display — pre-rendered 3D animation format
- **Performance_Test** — Cel rendering speed benchmark and frame rate measurement
- **Slideshow_24bit** — 24-bit color slideshow using double-height frame buffer technique
- **Slideshow** — Image slideshow with VDL support (basic and animation VDL flavors)
- **symanim** — Symmetric animation effects with mirror/kaleidoscope visuals

### Jumpstart
- **Jumpstart2/Jumpstart_Animation** — Plays ANIM files with high-level API
- **Jumpstart2/Jumpstart_Interactive_Music** — Interactive background music responding to events
- **Jumpstart2/JSIntMusicThread** — Thread-based music playback architecture
- **Jumpstart2/JSPlayBgndMusic** — Simple background music loading and looping
- **Jumpstart2/Jumpstart_Interactive_Sound** — Event-driven sound effect triggers
- **Jumpstart2/Jumpstart_Move_Cel** — Sprite movement with D-pad input
- **Jumpstart2/Jumpstart_Show_Cel** — Simplest sprite display example
- **Jumpstart2/JSBasicSlideShow** — Sequential image display
- **Jumpstart2/JSSlideShowVDL** — Slideshow with custom VDL enhanced color
- **UFO** — UFO-themed game demo with cel animation and background music

### Kernel
- **allocmem** — Memory allocation with AllocMem()
- **memdebug** — Memory debugging library for tracking leaks and usage
- **msgpassing** — Inter-task message passing with ports and async messaging
- **signals** — Kernel signal system for task synchronization
- **timerread** — Reading the system timer for elapsed time measurement
- **timersleep** — Putting tasks to sleep via kernel timer for scheduling

### Miscellaneous
- **Compression** — Using the compression folio for data compression/decompression
- **Menu** — Graphical program launcher menu with text rendering and navigation

### NVRAM
- **Access** — Low-level NVRAM read/write access for saved data
- **File_Management** — File-based NVRAM management (create, read, write, delete)
- **Storage_Tuner** — NVRAM storage performance tuning and analysis utility

---

## Toolkit 1.3.1

### FontLib v1.0a4
Font rendering library with example program for loading fonts and displaying text.

### Image24
- **ppmto3do** — PPM-to-3DO converter with new -z switch for 24-bit images and custom CLUTs
- **Slideshow 24bit** — 24-bit slideshow using double-height frame buffers with auto-advance and interpolation

### NVRAM1.0d2
Early NVRAM library for reading/writing non-volatile RAM with font and list utilities.

### PALBounceSound 1.0b1
PAL-compatible bouncing 3DO logo with sound, handling PAL-sized higher resolution screens.

### PALSlideshow 1.0b1
PAL-compatible slideshow with custom VDL support at PAL resolution.

### PALsymanim 1.0b1
PAL-compatible symmetric animation player at PAL screen resolution and timing.

### Streaming_1.9a8
DataStreaming framework (v1.9a8) with PAL-compatible PALTestDS. Includes players (NuPlayer, PlaySA, PlaySAnim, SlideStream, TestDS, TestSA) and tools (Weaver, Chunkify, SFtoStream, SquashSnd, DumpAIFF, DumpStream, SAudioTool, AnimToSanm, CtlMaker).

---

## Toolkit 1.3

### MemoryLeaks
Demonstrates memory leak detection using a customized Lib3DO.lib with memory tracking extensions.

---

## Toolkit 1.4

### 3DLib
3D graphics library with nine demos: 3d_example1-6, 3d_stereo, perftest_3d, showmodel. Updated for Toolkit 1.4.

### 3DOBounce / 3DOBounceSnd / 3DOOrbit
Bouncing 3DO logo (with and without sound), and rotating 3DO logo animation.

### aaPlayer / AnimSample / ColorEcho / LREX / Symanim
Cel/anim player, basic cel animation, interactive color patterns, LRForm display, and symmetric animation effects.

### Access Manager Test / AssetManager
System service access testing and block-file based asset management.

### CPlusExamples
C++ examples: HelloWorld, Strings, Lists, Application, Shapes, Sound, Instrument, SampleAppl.

### FontLibExample / FontView
Font rendering demo and font file inspector.

### JoyJoy / MemTest / Menu / MessageSample
Joystick interaction, memory testing, graphical menu launcher, inter-task message passing.

### Jumpstart2
JSAnimation, JSInteractiveMusic, JSIntMusicThread, JSPlayBgndMusic, JSInteractiveSound, JSMoveCel, JSShowCel, JSBasicSlideShow, JSSlideShowVDL.

### Image24
ppmto3do converter and 24-bit slideshow with double-height frame buffers.

### Kanji TextBox
KFontViewer and KTextBox for Kanji (Japanese) font viewing and text box rendering.

### PAL Variants
PALBounceSnd, PALSlideshow, PALsymanim — PAL-compatible versions of bounce/slideshow/animation.

### PerfTest / SlideShow
Performance benchmark and image slideshow with VDL support.

### Storage Manager
Mounting, unmounting, and managing storage volumes on the 3DO.

### Streaming
DataStreaming 2.0 with ShuttlePlayer (fast-forward/fast-reverse with Cinepak) and examples (NuPlayer, PlaySA, PlaySAnim, SlideStream, TestDS, TestSA, PALTestDS).

### TaskDemo / UserFolioTemplate / VersionChecker
Concurrent task creation, custom folio template, and system version checking.

### UFO
Interactive UFO-themed demo with cel animation, sprites, and background music.

---

## Toolkit 1.5

### 3DLib
3D graphics library with nine demos (3d_example1-6, 3d_stereo, perftest_3d, showmodel). Updated for Toolkit 1.5.

### ChunkMatcher
C++ stream chunk analysis tool for identifying and extracting chunks (Anim, Cinepak, SAudio, SAnim, SCel, Join) from 3DO stream files.

### DumpMF
Parses and dumps MIDI file (.mf) contents to text. Includes ParseMF and MFToText for debugging MIDI data.

### EZSqueeze
Compressed movie playback framework with EZQPlayer (sample player with multiple test movies), EZQSubscriber, and MovieToEZQ conversion tool.

### Lib3DO.lib
Updated core utility library for Toolkit 1.5 providing cel manipulation, animation loading, display, I/O, text, and timer utilities.

### LoopMIDIFiles
Plays MIDI files in a continuous loop with control pad integration for start/stop.

### Storage Manager
Demonstrates mounting, unmounting, and managing storage volumes on the 3DO.

### Streaming
DataStreaming 2.1 framework with advanced subscribers (ControlSubscriber, CPakSubscriber, JoinSubscriber, ProtoSubscriber, SAnimSubscriber, SAudioSubscriber) and examples (NuPlayer, PlaySA, PlaySAnim, SlideStream, TestSA, PALTestDS, ShuttlePlayer, ProtoPlayer).
