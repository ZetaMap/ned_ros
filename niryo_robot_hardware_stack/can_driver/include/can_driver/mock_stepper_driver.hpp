/*
    can_driver/mock_stepper_driver.hpp
    Copyright (C) 2020 Niryo
    All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http:// www.gnu.org/licenses/>.
*/

#ifndef CAN_MOCK_STEPPER_DRIVER_H
#define CAN_MOCK_STEPPER_DRIVER_H

#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <thread>

#include "can_driver/abstract_stepper_driver.hpp"
#include "common/model/stepper_calibration_status_enum.hpp"
#include "common/model/abstract_single_motor_cmd.hpp"
#include "common/model/conveyor_state.hpp"
#include "can_driver/fake_can_data.hpp"

namespace can_driver
{

class MockStepperDriver : public AbstractStepperDriver
{
public:
    MockStepperDriver(FakeCanData data);
    virtual ~MockStepperDriver() override;

    virtual std::string str() const override;

    // AbstractCanDriver interface
public:
    virtual bool canReadData() const override;

    virtual int ping(uint8_t id) override;
    virtual int scan(const std::set<uint8_t> &motors_to_find, std::vector<uint8_t> &id_list) override;
    virtual uint8_t sendUpdateConveyorId(uint8_t old_id, uint8_t new_id) override;
    virtual uint8_t sendTorqueOnCommand(uint8_t id, int torque_on) override;
    virtual uint8_t sendRelativeMoveCommand(uint8_t id, int steps, int delay) override;
    virtual uint8_t sendPositionCommand(uint8_t id, int cmd) override;
    virtual uint8_t sendPositionOffsetCommand(uint8_t id, int cmd, int absolute_steps_at_offset_position) override;
    virtual uint8_t sendCalibrationCommand(uint8_t id, int offset, int delay, int direction, int timeout) override;
    virtual uint8_t sendSynchronizePositionCommand(uint8_t id, bool begin_traj) override;
    virtual uint8_t sendMicroStepsCommand(uint8_t id, int micro_steps) override;
    virtual uint8_t sendMaxEffortCommand(uint8_t id, int effort) override;
    virtual uint8_t sendConveyorOnCommand(uint8_t id, bool conveyor_on, uint8_t conveyor_speed, uint8_t direction) override;
    virtual uint8_t readData(uint8_t &id, int &control_byte, std::array<uint8_t, MAX_MESSAGE_LENGTH> &rxBuf, std::string &error_message) override;

    virtual int32_t interpretePositionStatus(const std::array<uint8_t, MAX_MESSAGE_LENGTH> &data) override;
    virtual uint32_t interpreteTemperatureStatus(const std::array<uint8_t, MAX_MESSAGE_LENGTH> &data) override;
    virtual std::string interpreteFirmwareVersion(const std::array<uint8_t, MAX_MESSAGE_LENGTH> &data) override;
    virtual std::tuple<common::model::EStepperCalibrationStatus, int32_t> interpreteCalibrationData(const std::array<uint8_t, MAX_MESSAGE_LENGTH> &data) override;
    virtual std::tuple<bool, uint8_t, uint16_t> interpreteConveyorData(const std::array<uint8_t, MAX_MESSAGE_LENGTH> &data) override;

private:

    std::map<uint8_t, FakeCanData::FakeStepperRegister> _map_fake_registers;
    uint8_t _fake_conveyor_id{6};
    std::vector<uint8_t> _id_list;
    
    // using for fake event can
    static constexpr uint8_t MAX_IDX = 2; // index for joints in _id_list
    static constexpr uint8_t MAX_ID_JOINT = 3;
    uint8_t _current_id;
    uint8_t _next_index = 0;
    uint8_t _next_control_byte = CAN_DATA_POSITION;
    std::map<uint8_t, std::tuple<common::model::EStepperCalibrationStatus, int32_t>> _calibration_status;
    // fake time for calibration
    int _fake_time = 0;

private:
    void initializeFakeData(FakeCanData data);


};  // class MockStepperDriver

/**
 * @brief MockStepperDriver::canReadData
 */
inline
bool MockStepperDriver::canReadData() const
{
    return true;
}

}  // namespace can_driver

#endif  // CAN_MOCK_STEPPER_DRIVER_HPP
