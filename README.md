# Slightly Rubbish Drum Sequencer
Crude MIDI drum machine for live jamming, using PortMidi. Build it using ``gcc -o drum drum.c -lportmidi``. This was developed and tested on Xubuntu, so compatibility with other OSes is unknown!

## Why'd you do it?
Simply put, I couldn't find a simple, tight MIDI drum machine program. Everything I tried had *incredibly* wonky MIDI timing, and wasn't suited to live jamming, so I knocked together the basic sequencer in half a day. I planned to find an actual drum machine, but this is way cheaper... cheap as free. Plus, I have the freedom to add whatever I want!

## What's it do?
It plays a looped drum pattern of variable length, with simplified, on-the-fly entry of drums using a MIDI keyboard. Drum sounds start from C3 on the keyboard, and you can change to MIDI programs 1-10 using keys C2-B2. It also sends a clock out signal, so you can sync it up with other bits of gear, with the drum machine being the master. Sync it with a 303 for pure acid goodness!

When first running the program, it gives you lists of MIDI inputs and outputs to choose from, and also lets you choose the pattern length, tempo and number of drum sounds. Then, simply press the appropriate key on your keyboard to toggle a drum on or off! The keyboard input is quantized live. To erase a drum, hold down the key, then release once done.

## How's it done?
It uses the plain C library PortMidi, and parses the MIDI data manually, because there's basically no documentation or examples. A basic event is laid out like so:

``(channel | (type<<4) | (value_1<<8) | (value_2<<16))``

When sending a note on, the event type is 0x9. A note on message consists of 0x9c, where c is the MIDI channel. Contrary to what I knew about the MIDI format, sending a note off is actually the same as sending a note on, but with 0 velocity. Usually the event type would be 0x8, but after looking at the MIDI data being received, I was proven "wrong"! In any case, the macro above is a huge help, and is used extensively in this project.

Figuring out clock sync was harder than expected, because there's no concrete info on how to program it. But after monitoring the input from another MIDI device, I figured out that you have to send MIDI message 0xf8 6 times between each beat. No clue why, but it works (at least, with my TD-3!)