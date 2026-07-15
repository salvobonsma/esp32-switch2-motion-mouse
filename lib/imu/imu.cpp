#include "imu.h"

#include <Wire.h>

#include <array>

namespace {
constexpr float k_linear_factor{16384.0f};
constexpr float k_angular_factor{131.0f};

void writeRegister(TwoWire& wire, const uint8_t mpu_address, const uint8_t reg,
                   const uint8_t value) {
  wire.beginTransmission(mpu_address);
  wire.write(reg);
  wire.write(value);
  wire.endTransmission(true);
}

bool prepareRawRead(TwoWire& wire, const uint8_t mpu_address) {
  wire.beginTransmission(mpu_address);
  // https://www.i2cdevlib.com/devices/mpu6050#registers
  wire.write(0x3B);
  if (wire.endTransmission(false) != 0) {
    return false;
  }
  if (wire.requestFrom(mpu_address, static_cast<uint8_t>(14),
                       static_cast<uint8_t>(true)) != 14) {
    return false;
  }
  return true;
}

int16_t read16(TwoWire& wire) {
  const uint8_t hi{static_cast<uint8_t>(wire.read())};
  const uint8_t lo{static_cast<uint8_t>(wire.read())};
  return static_cast<int16_t>(hi << 8 | lo);
}
}  // namespace

std::optional<Imu> createImu(const int sda, const int scl,
                             const std::uint8_t mpu_address) {
  Imu imu{mpu_address};

  if (const bool success{imu.m_wire.begin(sda, scl)}; !success) return {};

  // Probe: does the device ACK its address?
  imu.m_wire.beginTransmission(mpu_address);
  if (imu.m_wire.endTransmission(true) != 0) {
    return {};
  }

  // https://www.i2cdevlib.com/devices/mpu6050#registers
  writeRegister(imu.m_wire, imu.m_mpu_address, 0x6B,
                0x00);  // Power management 1
  writeRegister(imu.m_wire, imu.m_mpu_address, 0x1C,
                0x00);  // Accel config (range)
  writeRegister(imu.m_wire, imu.m_mpu_address, 0x1B,
                0x00);  // Gyro config (range)
  writeRegister(imu.m_wire, imu.m_mpu_address, 0x19,
                7);  // Sample rate (1khz / 7)
  writeRegister(imu.m_wire, imu.m_mpu_address, 0x1A,
                0x03);  // Low-pass filter ~44 Hz

  return {imu};
}

bool Imu::updateCurrentSample() {
  if (!prepareRawRead(m_wire, m_mpu_address)) return false;

  // https://www.i2cdevlib.com/devices/mpu6050#registers
  m_current_sample.ax = static_cast<float>(read16(m_wire)) / k_linear_factor;
  m_current_sample.ay = static_cast<float>(read16(m_wire)) / k_linear_factor;
  m_current_sample.az = static_cast<float>(read16(m_wire)) / k_linear_factor;
  read16(m_wire);  // Discard temperature
  m_current_sample.gx =
      static_cast<float>(read16(m_wire) - m_gyro_bias[0]) / k_angular_factor;
  m_current_sample.gy =
      static_cast<float>(read16(m_wire) - m_gyro_bias[1]) / k_angular_factor;
  m_current_sample.gz =
      static_cast<float>(read16(m_wire) - m_gyro_bias[2]) / k_angular_factor;

  return true;
}

void Imu::calibrateGyroBias(const int maxSamples) {
  int taken{0};
  std::array<int32_t, 3> sum{};
  for (int i{0}; i < maxSamples; ++i) {
    if (!prepareRawRead(m_wire, m_mpu_address)) continue;
    for (int j{0}; j < 4; ++j) read16(m_wire);  // Skip accel and temp
    sum[0] += read16(m_wire);
    sum[1] += read16(m_wire);
    sum[2] += read16(m_wire);
    ++taken;
    delay(1);
  }
  if (taken > 0) {
    m_gyro_bias[0] = sum[0] / taken;
    m_gyro_bias[1] = sum[1] / taken;
    m_gyro_bias[2] = sum[2] / taken;
  }
}