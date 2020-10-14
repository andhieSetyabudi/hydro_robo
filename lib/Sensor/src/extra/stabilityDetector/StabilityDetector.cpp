#include "StabilityDetector.h"

StabilityDetector::StabilityDetector(const float precision)
{
  resetValue();
  this->precision = precision;
}

void StabilityDetector::resetValue()
{
  this->stable = false;
  this->stableCount = 0;
  this->index = 0;
  for (uint8_t i = 0; i < arrayLength(valueBuffer); i++)
    this->valueBuffer[i] = 0.00f;
  this->deviasion = 0;
}

void StabilityDetector::pushToBuffer(const float value)
{
  this->valueBuffer[index] = value;
  if (++index >= arrayLength(valueBuffer)) index = 0;
  this->deviasion = getDeviasion(valueBuffer);
  if (abs(deviasion) <= precision)
    stableCount++;
  else
    stableCount = 0;
  stable = (stableCount >= buffer_size_);
  if (stable) stableCount = buffer_size_;
}
