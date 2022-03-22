Audio Digital Delay Effect with Arduino and 64k x 1 bit DRAM

This uses the comparator on the Arduino as a sigma-delta ADC, which is then
written to the DRAM. The DRAM is used like a circular buffer, where the
current address is read and output to a reconstruction filter. Then the same
address is overwritten with the latest from the sigma-delta ADC. Finally,
the address is incremented. This means the value we first read out will have
sat there since we last passed that address - and thus we have a delay.

I use a non-blocking ADC conversion to read a potentiometer to control how much
of the DRAM is used. This varies the length of the delay, although the steps
available get fairly coarse due to the inverse relationship between length and
time.

This design could use a 256k x 1 bit DRAM for four times the delay length with
little additional complication.

### Problems

There is no attempt to control the jitter of the sigma-delta conversion, so there
is a fair bit of noise. This shows up as hiss on the output, so I am using a
fairly aggressive low-pass filter there.

Although I did take steps to optimize the performance of the main loop, it only
runs at about 139 khz. This is okay, but don't expect hifi sound. It does
reproduce drum samples and voice recognizably, but expect either a lot of added
hiss or very attenuated high frequencies, depending on how far you go with the
output LPF.

### Further possibilities

If you put a pull up resistor on the DRAM /WE line, you can use a simple momentary
switch to disconnect the Arduino's ability to overwrite. This leaves whatever is
in the DRAM to repeat forever, like a sampler.

Another way to control the buffer is to have two potentiometers: one for the 
start location and one for the length of the buffer. With the above Write disable,
you can sample some audio, then play different parts of the buffer as a loop and
at different lengths. You can do this on a Korg Volca Sample and it's fun stuff.

One thing to figure out is how to actually slow down the system in a reasonable,
fine-grained way so that you can alter the pitch of a "captured" loop. Adding
some type of variable but fast busy loop seems worth a shot.



