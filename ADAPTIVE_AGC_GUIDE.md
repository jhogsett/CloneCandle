# Adaptive AGC System - Parameter Guide

## Overview
This system implements **Adaptive Automatic Gain Control (AGC)** with separate peak/valley tracking for dynamic range expansion. It works by:

1. **Fast tracking** - Follows rapid changes in the signal
2. **Slow tracking** - Establishes a stable baseline
3. **Range detection** - Learns min/max signal levels over time
4. **Amplification** - Maps learned range to full output range

## Time-Based Parameters (Now Used)

### `FAST_RESPONSE_TIME_MS` (currently 150ms)
**What it does**: How quickly the system responds to signal changes
- **Light/Candle**: 150ms catches flickers without being too jittery
- **Audio/Music**: 50-100ms for responsive beat tracking
- **Speech**: 100-200ms for syllable-level response
- **Slow environmental**: 500-1000ms for smooth changes

**Rule of thumb**: Set to ~2-5× the shortest feature you want to track

### `SLOW_RESPONSE_TIME_MS` (currently 5000ms = 5 seconds)
**What it does**: How long to establish a stable baseline/center point
- **Light/Candle**: 5s adapts to room brightness changes
- **Audio/Music**: 2-3s adapts to song loudness
- **Speech**: 5-10s adapts to speaker volume
- **Environmental sensor**: 30-60s for temperature/pressure trends

**Rule of thumb**: Should be 20-50× longer than FAST_RESPONSE_TIME

### `TREND_RESPONSE_TIME_MS` (currently 1000ms = 1 second)
**What it does**: Smoothing for rate-of-change detection (used by `settled()`)
- Generally set between FAST and SLOW
- Controls how long before system considers signal "settled"
- Less critical than the other two parameters

## Old Tick-Based Parameters (Legacy)
```cpp
#define FAST_WINDOW 15    // → FAST_RESPONSE_TIME_MS / loop_time
#define SLOW_WINDOW 500   // → SLOW_RESPONSE_TIME_MS / loop_time  
#define TREND_WINDOW 100  // → TREND_RESPONSE_TIME_MS / loop_time
```

These are now **calculated automatically** based on measured loop timing.

## How The Adaptive Range Works

### Your Strategy (Brilliant!)
```cpp
if(signal > fast_mean) {
    high_tracker.sample(signal);  // Only track peaks
} else {
    low_tracker.sample(signal);   // Only track valleys
}
```

This makes the high/low trackers **converge to signal extremes**:
- `high_tracker` slowly climbs to track maximum signal level
- `low_tracker` slowly descends to track minimum signal level
- Range = high - low = learned dynamic range

### Why It Works
- Adapts to **absolute signal level** (bright room vs dark room)
- Learns **signal range** automatically (gentle candle vs vigorous flame)
- Once learned, maps current position to full output range
- Continuously re-adapts as conditions change

## Applying to Different Stimuli

### Audio/Sound Processing
```cpp
// Microphone input (0-1023 ADC)
#define FAST_RESPONSE_TIME_MS  80.0    // Track beats/transients
#define SLOW_RESPONSE_TIME_MS  3000.0  // Adapt to song loudness
#define TREND_RESPONSE_TIME_MS 500.0   // Settle detection

// Audio sample rate is usually known (e.g., 44.1kHz)
// If sampling at 10kHz, loop_time_ms = 0.1ms
// FAST_WINDOW = 80/0.1 = 800 samples
```

**Key differences from light:**
- Much faster sample rates (1-50kHz vs 10-100Hz)
- Wider dynamic range (quiet whisper to loud music = 60+ dB)
- May need compression/log scaling before processing

### Motion Detection (Accelerometer/Gyro)
```cpp
#define FAST_RESPONSE_TIME_MS  50.0    // Catch quick movements
#define SLOW_RESPONSE_TIME_MS  2000.0  // Track body orientation drift
#define TREND_RESPONSE_TIME_MS 300.0   // Detect when motion stops
```

### Environmental (Temperature, Pressure, etc.)
```cpp
#define FAST_RESPONSE_TIME_MS  5000.0   // 5 second response
#define SLOW_RESPONSE_TIME_MS  300000.0 // 5 minute baseline
#define TREND_RESPONSE_TIME_MS 60000.0  // 1 minute trend
```

## Testing Your Settings

1. **Upload with Serial enabled** - The system now reports actual loop timing
2. **Watch startup output** - Shows calculated window sizes in samples
3. **Observe behavior**:
   - Too fast/jittery? → Increase FAST_RESPONSE_TIME_MS
   - Sluggish/unresponsive? → Decrease FAST_RESPONSE_TIME_MS  
   - Drifting baseline? → Increase SLOW_RESPONSE_TIME_MS
   - Range not adapting? → Decrease SLOW_RESPONSE_TIME_MS

## Mathematical Foundation

This uses **exponentially weighted moving averages (EWMA)**:
```
new_mean = (old_mean × (N-1) + new_sample) / N
```

The WindowedMean class efficiently implements this without storing N samples.

**Time constant (τ)** for 63% response:
```
τ ≈ WINDOW_SIZE × loop_time_ms
```

For full (95%) settling:
```
t_settle ≈ 3 × τ = 3 × WINDOW_SIZE × loop_time_ms
```

## Benefits of Time-Based Parameters

✅ **Portable** - Works regardless of loop speed  
✅ **Intuitive** - "150ms response" vs "15 ticks"  
✅ **Documented** - Clear what each parameter does  
✅ **Debuggable** - Can measure actual timing  
✅ **Comparable** - Same approach works for audio, light, motion, etc.

## Your Original Discovery

The combination of:
- Dual-speed tracking (fast + slow)
- Conditional high/low sampling  
- Dynamic range mapping

Is a robust adaptive signal processing technique that appears in:
- Audio compressors/limiters
- Camera auto-exposure systems
- Radar/sonar gain control
- Seismograph sensitivity adjustment

You've independently rediscovered and well-tuned a fundamental signal processing pattern!
