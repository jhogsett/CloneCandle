#ifndef TREND_DETECTOR_H
#define TREND_DETECTOR_H
#include "Arduino.h"
#include <windowed_mean.h>

class TrendDetector
{
public:
  TrendDetector(long fast_window, long slow_window, long trend_window, float trend_sense, float settled_window, float primed_value);
  float sample(float sample);
  float fast_mean(void);
  float slow_mean(void);
  float mean(void);
  bool settled(void);

private:
  WindowedMean *_fast_mean;
  WindowedMean *_slow_mean;
  WindowedMean *_trend_mean;
  float _trend_sense;
  float _settled_window;
};

TrendDetector::TrendDetector(long fast_window, long slow_window, long trend_window, float trend_sense, float settled_window, float primed_value){
  _fast_mean = new WindowedMean(fast_window, primed_value);
  _slow_mean = new WindowedMean(slow_window, primed_value);
  _trend_mean = new WindowedMean(trend_window, 0.0);
  _trend_sense = trend_sense;
  _settled_window = settled_window;
}

float TrendDetector::sample(float sample){
  float trend;

  _fast_mean->sample(sample);
  _slow_mean->sample(_fast_mean->mean());

  if(_slow_mean->mean() > _fast_mean->mean())
    trend = _trend_sense * (_fast_mean->mean() - _slow_mean->mean());
  else if(_slow_mean->mean() < _fast_mean->mean())
    trend = _trend_sense * (_fast_mean->mean() - _slow_mean->mean());

  _trend_mean->sample(trend);
  return _trend_mean->mean();
}

float TrendDetector::fast_mean(void){
  return _fast_mean->mean();
}

float TrendDetector::slow_mean(void){
  return _slow_mean->mean();
}

float TrendDetector::mean(void){
  return _trend_mean->mean();
}

bool TrendDetector::settled(void){
  return _trend_mean->mean() >= -_settled_window && _trend_mean->mean() <= _settled_window;
}
#endif
