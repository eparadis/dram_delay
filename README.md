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

A second ADC channel is used to vary the starting point. By disconnecting the
write enable line and using a pull up on the `_WE` line of the DRAM, you turn
the delay into a sampler. The two controls then allow you to do interesting
grainular synth style effects.

This design originally used a 64k x 1 bit DRAM, and has been extended to work
with one additional address line to use a 256k x 1 bit DRAM. Converting back
would be trivial if that suits your requirements.

### Problems

There is no attempt to control the jitter of the sigma-delta conversion, so there
is a fair bit of noise. This shows up as hiss on the output, so I am using a
fairly aggressive low-pass filter there.

I have measured the jitter as almost exactly 1uS by looking at the period of the
write enable pulse on a storage oscilloscope. The main loop seems to run between
168khz and 202khz.

I took all the easily available performance optimizations, most of which were
to not use `digitalWrite` and instead use `bitWrite` on the AVR ports directly.
This is quite easy and worth the effort. Sound quality is reasonable although
I haven't done a full test. The input aliasing filter and output reconstruction
filters are simple RC circuits with parts I had on hand that seemed to sound
good.

The output filter is 10k ohms and 1uF. The input filter is 10k ohms and 100nF.

### Further possibilities

If you put a pull up resistor on the DRAM /WE line, you can use a simple momentary
switch to disconnect the Arduino's ability to overwrite. This leaves whatever is
in the DRAM to repeat forever, like a sampler. You could use some diode or discrete
transistor logic to control this externally.

I have no pursued finding a way to vary the sampling speed of the system. This
is an expected feature for a sampler, and may be a nice addition to a delay.
You could either try to syncronize the sytem to a timer interrupt and then vary
the timer parameters, or simply add a variable length busy loop.

### External hardware

If you want to use the delay like a reverb, use opamps to feedback the output
into the input.

As mentioned, some simple logic or push button will allow you to switch between
delay and looper/sampler modes.

You'll probably want to buffer the inputs and outputs of the ciruit to protect
the hardware from static and over-voltage.

