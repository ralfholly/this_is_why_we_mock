#include <bits/stdc++.h>
using namespace std;

#include "gmock/gmock.h"
using namespace testing;

namespace testing {
namespace this_is_why_we_mock {
class ISensor {
public:
    enum class Status { good, bad };
    virtual ~ISensor() = default;
    virtual int getTemperature(Status* status) = 0;
};

class Sensor : public ISensor {
public:
    int getTemperature(Status* status) override;
};

class MockSensor : public ISensor {
public:
    MOCK_METHOD(int, getTemperature, (Status* status));
};

class IHeater {
public:
    virtual ~IHeater() = default;
    virtual void on() = 0;
    virtual void off() = 0;
};

class Heater : public IHeater {
public:
    void on() override;
    void off() override;
};

class MockHeater : public IHeater {
public:
    MOCK_METHOD(void, on, ());
    MOCK_METHOD(void, off, ());
};

class Controller {
public:
    Controller(ISensor& sensor, IHeater& heater, int target, int hysteresis)
        : m_sensor(sensor)
          , m_heater(heater)
          , m_target(target)
          , m_hysteresis(hysteresis) {
              assert(m_hysteresis >= 0 && m_hysteresis < m_target);
          }

    void run() {
        while (step()) {
            ;
        }
    }

    bool step() {
        int temperature = m_sensor.getTemperature(&m_currentStatus);
        if (m_currentStatus == ISensor::Status::good) {
            if (temperature >= m_target) {
                m_heater.off();
            } else if (temperature < m_target - m_hysteresis) {
                m_heater.on();
            } else {
                // leave heater as is.
            }
        } else {
            if (m_previousStatus == ISensor::Status::bad) {
                // Second time in a row bad sensor data.
                // Give up.
                m_heater.off();
                return false;
            }
        }
        m_previousStatus = m_currentStatus;
        return true;
    }

private:
    ISensor& m_sensor;
    IHeater& m_heater;
    int m_target;
    int m_hysteresis;
    Sensor::Status m_currentStatus{ Sensor::Status::good };
    Sensor::Status m_previousStatus { Sensor::Status::good };
};

TEST(this_is_why_we_mock, mock_temperature_experiments) {
    MockSensor sensor;
    Sensor::Status status;
    EXPECT_CALL(sensor, getTemperature).WillOnce(DoAll(SetArgPointee<0>(ISensor::Status::bad), Return(200)));
    EXPECT_EQ(200, sensor.getTemperature(&status));
    EXPECT_EQ(Sensor::Status::bad, status);
    EXPECT_CALL(sensor, getTemperature).WillOnce(DoAll(SetArgPointee<0>(ISensor::Status::good), Return(100)));
    EXPECT_EQ(100, sensor.getTemperature(&status));
    EXPECT_EQ(Sensor::Status::good, status);
}

TEST(this_is_why_we_mock, happy_path) {
    MockSensor sensor;
    MockHeater heater;
    constexpr int targetTemp = 23;
    constexpr int hysteresis = 2;
    Controller controller(sensor, heater, targetTemp, hysteresis);

    // Temperature below target temperature, heater shall be turned on.
    EXPECT_CALL(sensor, getTemperature).WillOnce(Return(0));
    EXPECT_CALL(heater, on);
    EXPECT_TRUE(controller.step());

    // Temperature equals target temperature, heater shall be turned off.
    EXPECT_CALL(sensor, getTemperature).WillOnce(Return(targetTemp));
    EXPECT_CALL(heater, off);
    EXPECT_TRUE(controller.step());

    // Temperature below target temperature but within hysteresis.
    // Heater state shall not change.
    EXPECT_CALL(sensor, getTemperature).WillOnce(Return(targetTemp - hysteresis / 2));
    EXPECT_CALL(heater, off).Times(0);
    EXPECT_CALL(heater, on).Times(0);
    EXPECT_TRUE(controller.step());

    // Temperature below target temperature but within hysteresis.
    // Temperature below target temperature/hysteresis, heater shall be turned on.
    EXPECT_CALL(sensor, getTemperature).WillOnce(Return(targetTemp - hysteresis));
    EXPECT_CALL(heater, off).Times(0);
    EXPECT_CALL(heater, on).Times(0);
    EXPECT_TRUE(controller.step());

    // Temperature below target temperature, outside hysteresis, heater shall be turned on.
    EXPECT_CALL(sensor, getTemperature).WillOnce(Return(targetTemp - hysteresis - 1));
    EXPECT_CALL(heater, on);
    EXPECT_TRUE(controller.step());

    // Temperature below target temperature, whithin hysteresis.
    // Heater state shall not change.
    EXPECT_CALL(sensor, getTemperature).WillOnce(Return(targetTemp - hysteresis + 1));
    EXPECT_CALL(heater, on).Times(0);
    EXPECT_CALL(heater, off).Times(0);
    EXPECT_TRUE(controller.step());

    // Temperature above target temperature, heater shall be turned off.
    EXPECT_CALL(sensor, getTemperature).WillOnce(Return(targetTemp + 1));
    EXPECT_CALL(heater, off);
    EXPECT_TRUE(controller.step());
}

TEST(this_is_why_we_mock, single_sensor_failure_recovery) {
    MockSensor sensor;
    MockHeater heater;
    constexpr int targetTemp = 23;
    constexpr int hysteresis = 2;
    Controller controller(sensor, heater, targetTemp, hysteresis);

    // Temperature below target temperature, sensor status is bad, no heater change.
    EXPECT_CALL(sensor, getTemperature).WillOnce(DoAll(SetArgPointee<0>(ISensor::Status::bad), Return(0)));
    EXPECT_CALL(heater, on).Times(0);
    EXPECT_CALL(heater, off).Times(0);
    EXPECT_TRUE(controller.step());

    // Temperature below target temperature, sensor status is ok, heater shall be turned on.
    EXPECT_CALL(sensor, getTemperature).WillOnce(DoAll(SetArgPointee<0>(ISensor::Status::good), Return(0)));
    EXPECT_CALL(heater, on);
    EXPECT_TRUE(controller.step());

    // Temperature below target temperature, sensor status is bad, no heater change.
    EXPECT_CALL(sensor, getTemperature).WillOnce(DoAll(SetArgPointee<0>(ISensor::Status::bad), Return(0)));
    EXPECT_CALL(heater, on).Times(0);
    EXPECT_CALL(heater, off).Times(0);
    EXPECT_TRUE(controller.step());
}

TEST(this_is_why_we_mock, double_sensor_failure_give_up) {
    MockSensor sensor;
    MockHeater heater;
    constexpr int targetTemp = 23;
    constexpr int hysteresis = 2;
    Controller controller(sensor, heater, targetTemp, hysteresis);

    // Temperature below target temperature, sensor status is bad, no heater change.
    EXPECT_CALL(sensor, getTemperature).WillOnce(DoAll(SetArgPointee<0>(ISensor::Status::bad), Return(0)));
    EXPECT_CALL(heater, on).Times(0);
    EXPECT_CALL(heater, off).Times(0);
    EXPECT_TRUE(controller.step());

    // Temperature below target temperature, sensor status is bad, no heater change.
    EXPECT_CALL(sensor, getTemperature).WillOnce(DoAll(SetArgPointee<0>(ISensor::Status::bad), Return(0)));
    EXPECT_CALL(heater, on).Times(0);
    // Heater explicitly switched of for safety reasons.
    EXPECT_CALL(heater, off);
    // Controller terminates.
    EXPECT_FALSE(controller.step());
}

TEST(this_is_why_we_mock, test_run_method) {
    MockSensor sensor;
    MockHeater heater;
    constexpr int targetTemp = 23;
    constexpr int hysteresis = 2;
    Controller controller(sensor, heater, targetTemp, hysteresis);

    InSequence seq;

    // Temperature below target temperature, heater shall be turned on.
    EXPECT_CALL(sensor, getTemperature).WillOnce(Return(0));
    EXPECT_CALL(heater, on).Times(1);

    // Temperature above target temperature, heater shall be turned off.
    EXPECT_CALL(sensor, getTemperature).WillOnce(Return(targetTemp + 1));
    EXPECT_CALL(heater, off).Times(1);

    // Temperature below target temperature, sensor status is bad, no heater change.
    EXPECT_CALL(sensor, getTemperature).WillOnce(DoAll(SetArgPointee<0>(ISensor::Status::bad), Return(0)));
    EXPECT_CALL(heater, on).Times(0);
    EXPECT_CALL(heater, off).Times(0);

    // Temperature below target temperature, sensor status is bad, no heater change.
    EXPECT_CALL(sensor, getTemperature).WillOnce(DoAll(SetArgPointee<0>(ISensor::Status::bad), Return(0)));
    EXPECT_CALL(heater, on).Times(0);
    // Heater explicitly switched off for safety reasons.
    EXPECT_CALL(heater, off).Times(1);

    // Controller terminates, no endless loop.
    controller.run();
}

} // namespace this_is_why_we_mock
} // namespace testing
