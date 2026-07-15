#pragma once

#include <Wire.h>

#include <array>
#include <optional>

class Imu {
 public:
  // Accel in g, gyro in deg/s.
  struct Sample {
    float ax, ay, az, gx, gy, gz;
  };

 private:
  uint8_t m_mpu_address;

  TwoWire m_wire{0};
  // Raw sample
  std::array<int32_t, 3> m_gyro_bias{};
  Sample m_current_sample{};

  explicit Imu(const uint8_t mpu_address) : m_mpu_address{mpu_address} {}

 public:
  bool updateCurrentSample();
  const Sample& currentSample() const { return m_current_sample; }
  void calibrateGyroBias(int maxSamples = 50);

  friend std::optional<Imu> createImu(int sda, int scl, uint8_t mpu_address);
};
