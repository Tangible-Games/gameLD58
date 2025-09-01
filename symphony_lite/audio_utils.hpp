#pragma once

namespace Symphony {
namespace Audio {
const float kPcm8bitMin = 0.0f;
const float kPcm8bitMax = 255.0f;
const float kPcm8bitMidPoint = 128.0f;

const float kPcm16bitMin = -32768.0f;
const float kPcm16bitMax = 32767.0f;
const float kPcm16bitMidPoint = 0.0f;

template <typename SampleType>
float ConvertPcmToFloat(SampleType sample);

template <>
float ConvertPcmToFloat<uint8_t>(uint8_t sample) {
  return (((float)sample) - kPcm8bitMidPoint) / (kPcm8bitMax - kPcm8bitMin);
}

template <>
float ConvertPcmToFloat<int16_t>(int16_t sample) {
  return (((float)sample) - kPcm16bitMidPoint) / (kPcm16bitMax - kPcm16bitMin);
}

template <typename SampleType>
SampleType ConvertFloatToPcm(float f);

template <>
uint8_t ConvertFloatToPcm(float f) {
  return f * (kPcm8bitMax - kPcm8bitMin) + kPcm8bitMidPoint;
}

template <>
int16_t ConvertFloatToPcm(float f) {
  return f * (kPcm16bitMax - kPcm16bitMin) + kPcm16bitMidPoint;
}
}  // namespace Audio
}  // namespace Symphony
