#ifndef ADAPTIVE_AGC_H
#define ADAPTIVE_AGC_H
#include "Arduino.h"
#include <trend_detector.h>

/**
 * Adaptive Automatic Gain Control (AGC) class
 * 
 * Implements adaptive signal processing with automatic range detection:
 * - Fast tracking for rapid signal changes (flicker, beats, etc.)
 * - Slow tracking for baseline adaptation (ambient changes)
 * - Automatic min/max range learning via conditional peak/valley tracking
 * - Dynamic range expansion to map learned signal range to output range
 * 
 * The system continuously adapts to signal characteristics without calibration.
 */
class AdaptiveAGC
{
public:
  /**
   * Constructor
   * 
   * @param fast_window Number of samples for fast response tracking
   * @param slow_window Number of samples for slow baseline tracking
   * @param trend_window Number of samples for rate-of-change smoothing
   * @param trend_sense Sensitivity multiplier for trend detection
   * @param settled_window Threshold for settled state detection
   * @param initial_value Initial value to prime the system with
   * @param range_expansion Multiplier for scaling learned range to output
   * @param pwm_max Maximum PWM output value (e.g., 4095 for 12-bit)
   * @param base Minimum PWM base value to add to output
   */
  AdaptiveAGC(long fast_window, long slow_window, long trend_window, 
              float trend_sense, float settled_window, float initial_value,
              float range_expansion, float pwm_max, float base);
  
  /**
   * Process one input sample through the AGC
   * 
   * @param input Raw input value (e.g., light level, audio amplitude)
   * @return PWM output value, clamped and scaled appropriately
   */
  float process(float input);
  
  /**
   * Get the currently learned dynamic range (max - min)
   * @return The difference between learned high and low signal levels
   */
  float getRange();
  
  /**
   * Get the learned baseline (minimum) value
   * @return The learned low signal level
   */
  float getBase();
  
  /**
   * Get the current scaled value before PWM conversion
   * @return The amplified value (useful for debugging/display)
   */
  float getScaledValue();
  
  /**
   * Check if the signal is currently settled (not changing rapidly)
   * @return true if trend is within settled window
   */
  bool isSettled();
  
  /**
   * Get the fast mean value (tracks rapid changes)
   * @return Fast-tracked signal average
   */
  float getFastMean();
  
  /**
   * Get the slow mean value (tracks baseline)
   * @return Slow-tracked signal average
   */
  float getSlowMean();

private:
  TrendDetector *_main;           // Tracks actual signal
  TrendDetector *_high;           // Learns peak levels
  TrendDetector *_low;            // Learns valley levels
  float _range_expansion;         // Scaling factor for output
  float _pwm_max;                 // Maximum PWM value
  float _base;                    // Minimum PWM base level
  float _last_scaled_value;       // Last computed scaled value
  float _last_range;              // Last computed range
  float _last_base;               // Last computed base
};

AdaptiveAGC::AdaptiveAGC(long fast_window, long slow_window, long trend_window,
                         float trend_sense, float settled_window, float initial_value,
                         float range_expansion, float pwm_max, float base)
{
  _main = new TrendDetector(fast_window, slow_window, trend_window, trend_sense, settled_window, initial_value);
  _high = new TrendDetector(fast_window, slow_window, trend_window, trend_sense, settled_window, initial_value);
  _low = new TrendDetector(fast_window, slow_window, trend_window, trend_sense, settled_window, initial_value);
  
  _range_expansion = range_expansion;
  _pwm_max = pwm_max;
  _base = base;
  _last_scaled_value = 0;
  _last_range = 0;
  _last_base = initial_value;
}

float AdaptiveAGC::process(float input)
{
  // Update main tracker
  _main->sample(input);
  float fast_mean = _main->fast_mean();
  
  // Conditionally update high/low trackers
  // This makes them converge to signal extremes
  if(input > fast_mean) {
    _high->sample(input);
  } else {
    _low->sample(input);
  }
  
  // Calculate learned range and baseline
  _last_range = _high->slow_mean() - _low->slow_mean();
  _last_base = _low->slow_mean();
  
  // Calculate signal position above baseline
  float val = _main->fast_mean() - _last_base;
  
  // Scale to output range
  _last_scaled_value = val * _range_expansion;
  
  // Convert to integer PWM value with clamping
  int pwm_val = (int)_last_scaled_value;
  if(pwm_val < 0)
    pwm_val = 0;
  else if(pwm_val > _pwm_max - _base)
    pwm_val = _pwm_max - _base;
  
  return _base + pwm_val;
}

float AdaptiveAGC::getRange()
{
  return _last_range;
}

float AdaptiveAGC::getBase()
{
  return _last_base;
}

float AdaptiveAGC::getScaledValue()
{
  return _last_scaled_value;
}

bool AdaptiveAGC::isSettled()
{
  return _main->settled();
}

float AdaptiveAGC::getFastMean()
{
  return _main->fast_mean();
}

float AdaptiveAGC::getSlowMean()
{
  return _main->slow_mean();
}

#endif
