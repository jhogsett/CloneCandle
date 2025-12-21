Expansion Ideas

1. Detect that all three channels have been in the settled state for a long time, for example one minute, and then turn off the PWM channels

2. Come up with other, more interesting and sophisticated lighting patterns than simply mapping the scaled main average to light level

3. Support a rotary encoder and LCD display (via a backpack I2C interface) for tools and settings

4. Settings: timing for the main trend detect object, timing for the range trend detect objects, brightness level (when scaling the 0.0 to 1.0 value up to potentially 0-4095; currently set to scale up to 256 for low brightness), idle timeout period (none, 1 minute, 5 minutes), which light patterns to use (see #2 above)

5. Tools: use as a simple lamp with the brightness level set to and amazing rangs of 0 through 12287 (using one, two and three lamps), strobe effect, other lighting effects not reliant on the light sensors, the ability to record and playback sequences of lighting patterns computed based on the light sensors

Crazy Ideas

1. Make the lamps and sensors interactive together and take advantage of the feedback (other than the default way they do now which is the lamps causing the range to tamp down, which isn't attractive, so normally I avoid feedback from the lamps to the sensors)
