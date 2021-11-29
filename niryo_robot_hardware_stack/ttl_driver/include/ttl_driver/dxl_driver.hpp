/*
dxl_driver.hpp
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

#ifndef DXL_DRIVER_HPP
#define DXL_DRIVER_HPP

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cassert>

#include "abstract_dxl_driver.hpp"
#include "common/common_defs.hpp"

#include "xc430_reg.hpp"
#include "xl430_reg.hpp"
#include "xl330_reg.hpp"
#include "xl320_reg.hpp"

namespace ttl_driver
{


/**
 * @brief The DxlDriver class
 */
template<typename reg_type>
class DxlDriver : public AbstractDxlDriver
{
    public:
        DxlDriver(std::shared_ptr<dynamixel::PortHandler> portHandler,
                  std::shared_ptr<dynamixel::PacketHandler> packetHandler);

        std::string str() const override;
        std::string interpretErrorState(uint32_t hw_state) const override;

    public:
        int checkModelNumber(uint8_t id) override;
        int readFirmwareVersion(uint8_t id, std::string &version) override;

        int readTemperature(uint8_t id, uint8_t& temperature) override;
        int readVoltage(uint8_t id, double &voltage) override;
        int readHwErrorStatus(uint8_t id, uint8_t& hardware_error_status) override;

        int syncReadFirmwareVersion(const std::vector<uint8_t> &id_list, std::vector<std::string> &firmware_list) override;
        int syncReadTemperature(const std::vector<uint8_t> &id_list, std::vector<uint8_t>& temperature_list) override;
        int syncReadVoltage(const std::vector<uint8_t> &id_list, std::vector<double> &voltage_list) override;
        int syncReadRawVoltage(const std::vector<uint8_t> &id_list, std::vector<double> &voltage_list) override;
        int syncReadHwStatus(const std::vector<uint8_t> &id_list, std::vector<std::pair<double, uint8_t> >& data_list) override;

        int syncReadHwErrorStatus(const std::vector<uint8_t> &id_list, std::vector<uint8_t> &hw_error_list) override;

    protected:
        // AbstractTtlDriver interface
        std::string interpretFirmwareVersion(uint32_t fw_version) const override;

    public:
        // AbstractMotorDriver interface : we cannot define them globally in AbstractMotorDriver
        // as it is needed here for polymorphism (AbstractMotorDriver cannot be a template class and does not
        // have access to reg_type). So it seems like a duplicate of StepperDriver

        int changeId(uint8_t id, uint8_t new_id) override;

        int readMinPosition(uint8_t id, uint32_t &min_pos) override;
        int readMaxPosition(uint8_t id, uint32_t &max_pos) override;

        int writeVelocityProfile(uint8_t id, const std::vector<uint32_t>& data_list) override;

        int writeTorqueEnable(uint8_t id, uint8_t torque_enable) override;

        int writePositionGoal(uint8_t id, uint32_t position) override;
        int writeVelocityGoal(uint8_t id, uint32_t velocity) override;

        int syncWriteTorqueEnable(const std::vector<uint8_t> &id_list, const std::vector<uint8_t> &torque_enable_list) override;
        int syncWritePositionGoal(const std::vector<uint8_t> &id_list, const std::vector<uint32_t> &position_list) override;
        int syncWriteVelocityGoal(const std::vector<uint8_t> &id_list, const std::vector<uint32_t> &velocity_list) override;

        int readVelocityProfile(uint8_t id, std::vector<uint32_t>& data_list) override;

        int readPosition(uint8_t id, uint32_t &present_position) override;
        int readVelocity(uint8_t id, uint32_t &present_velocity) override;

        int syncReadPosition(const std::vector<uint8_t> &id_list, std::vector<uint32_t> &position_list) override;
        int syncReadVelocity(const std::vector<uint8_t> &id_list, std::vector<uint32_t> &velocity_list) override;
        int syncReadJointStatus(const std::vector<uint8_t> &id_list, std::vector<std::array<uint32_t, 2> > &data_array_list) override;

    public:
        // AbstractDxlDriver interface
        int writePID(uint8_t id, const std::vector<uint32_t>& data) override;
        int readPID(uint8_t id, std::vector<uint32_t> &data_list) override;

        int writeControlMode(uint8_t id, uint8_t data) override;
        int readControlMode(uint8_t id, uint8_t& data) override;

        int writeLed(uint8_t id, uint8_t led_value) override;
        int syncWriteLed(const std::vector<uint8_t> &id_list, const std::vector<uint8_t> &led_list) override;

        int writeTorqueGoal(uint8_t id, uint16_t torque) override;
        int syncWriteTorqueGoal(const std::vector<uint8_t> &id_list, const std::vector<uint16_t> &torque_list) override;

        int readLoad(uint8_t id, uint16_t &present_load) override;
        int syncReadLoad(const std::vector<uint8_t> &id_list, std::vector<uint16_t> &load_list) override;

private:
        int writePositionPGain(uint8_t id, uint16_t gain);
        int writePositionIGain(uint8_t id, uint16_t gain);
        int writePositionDGain(uint8_t id, uint16_t gain);
        int writeVelocityPGain(uint8_t id, uint16_t gain);
        int writeVelocityIGain(uint8_t id, uint16_t gain);
        int writeFF1Gain(uint8_t id, uint16_t gain);
        int writeFF2Gain(uint8_t id, uint16_t gain);

        int readPositionPGain(uint8_t id, uint16_t& gain);
        int readPositionIGain(uint8_t id, uint16_t& gain);
        int readPositionDGain(uint8_t id, uint16_t& gain);
        int readVelocityPGain(uint8_t id, uint16_t& gain);
        int readVelocityIGain(uint8_t id, uint16_t& gain);
        int readFF1Gain(uint8_t id, uint16_t& gain);
        int readFF2Gain(uint8_t id, uint16_t& gain);

};

// definition of methods

/**
 * @brief DxlDriver<reg_type>::DxlDriver
 */
template<typename reg_type>
DxlDriver<reg_type>::DxlDriver(std::shared_ptr<dynamixel::PortHandler> portHandler,
                               std::shared_ptr<dynamixel::PacketHandler> packetHandler) :
    AbstractDxlDriver(std::move(portHandler),
                      std::move(packetHandler))
{}

//*****************************
// AbstractMotorDriver interface
//*****************************

/**
 * @brief DxlDriver<reg_type>::str
 * @return
 */
template<typename reg_type>
std::string DxlDriver<reg_type>::str() const
{
    return common::model::HardwareTypeEnum(reg_type::motor_type).toString() + " : " + AbstractDxlDriver::str();
}

/**
 * @brief DxlDriver<reg_type>::interpretErrorState
 * @return
 */
template<typename reg_type>
std::string DxlDriver<reg_type>::interpretErrorState(uint32_t /*hw_state*/) const
{
    return "";
}

/**
 * @brief DxlDriver<reg_type>::interpretFirmwareVersion
 * @param fw_version
 * @return
 */
template<typename reg_type>
std::string DxlDriver<reg_type>::interpretFirmwareVersion(uint32_t fw_version) const
{
    std::string version = std::to_string(static_cast<uint8_t>(fw_version));

    return version;
}

/**
 * @brief DxlDriver<reg_type>::changeId
 * @param id
 * @param new_id
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::changeId(uint8_t id, uint8_t new_id)
{
    return write<typename reg_type::TYPE_ID>(reg_type::ADDR_ID, id, new_id);
}

/**
 * @brief DxlDriver<reg_type>::checkModelNumber
 * @param id
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::checkModelNumber(uint8_t id)
{
    uint16_t model_number = 0;
    int ping_result = getModelNumber(id, model_number);

    if (ping_result == COMM_SUCCESS)
    {
        if (model_number && model_number != reg_type::MODEL_NUMBER)
        {
            return PING_WRONG_MODEL_NUMBER;
        }
    }

    return ping_result;
}

/**
 * @brief DxlDriver<reg_type>::readFirmwareVersion
 * @param id
 * @param version
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readFirmwareVersion(uint8_t id, std::string &version)
{
    int res = COMM_RX_FAIL;
    uint8_t data{};
    res = read<typename reg_type::TYPE_FIRMWARE_VERSION>(reg_type::ADDR_FIRMWARE_VERSION, id, data);
    version = interpretFirmwareVersion(data);
    return res;
}

/**
 * @brief DxlDriver<reg_type>::readMinPosition
 * @param id
 * @param pos
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readMinPosition(uint8_t id, uint32_t &pos)
{
    return read<typename reg_type::TYPE_MIN_POSITION_LIMIT>(reg_type::ADDR_MIN_POSITION_LIMIT, id, pos);
}

/**
 * @brief DxlDriver<reg_type>::readMaxPosition
 * @param id
 * @param pos
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readMaxPosition(uint8_t id, uint32_t &pos)
{
    return read<typename reg_type::TYPE_MAX_POSITION_LIMIT>(reg_type::ADDR_MAX_POSITION_LIMIT, id, pos);
}

// ram write

/**
 * @brief DxlDriver<reg_type>::writeVelocityProfile
 * Write velocity profile for dxl
 * @param id
 * @param data_list [ velocity, acceleration]
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writeVelocityProfile(uint8_t id, const std::vector<uint32_t>& data_list)
{
    // in mode control Position Control Mode, velocity profile in datasheet is used to write velocity (except xl320)
    int res = 0;

    int tries = 10;
    while (tries > 0)
    {
        tries--;
        res = write<typename reg_type::TYPE_PROFILE>(reg_type::ADDR_PROFILE_VELOCITY, id, data_list.at(0));
        if (COMM_SUCCESS == res)
            break;
    }
    tries = 10;
    while (tries > 0)
    {
        tries--;
        res = write<typename reg_type::TYPE_PROFILE>(reg_type::ADDR_PROFILE_ACCELERATION, id, data_list.at(1));
        if (COMM_SUCCESS == res)
            break;
    }

    if (COMM_SUCCESS != res)
        return res;

    return res;
}

/**
 * @brief DxlDriver<reg_type>::writeTorqueEnable
 * @param id
 * @param torque_enable
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writeTorqueEnable(uint8_t id, uint8_t torque_enable)
{
    return write<typename reg_type::TYPE_TORQUE_ENABLE>(reg_type::ADDR_TORQUE_ENABLE, id, torque_enable);
}

/**
 * @brief DxlDriver<reg_type>::writePositionGoal
 * @param id
 * @param position
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writePositionGoal(uint8_t id, uint32_t position)
{
    return write<typename reg_type::TYPE_GOAL_POSITION>(reg_type::ADDR_GOAL_POSITION, id, position);
}

/**
 * @brief DxlDriver<reg_type>::writeVelocityGoal
 * @param id
 * @param velocity
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writeVelocityGoal(uint8_t id, uint32_t velocity)
{
    return write<typename reg_type::TYPE_GOAL_VELOCITY>(reg_type::ADDR_GOAL_VELOCITY, id, velocity);
}

/**
 * @brief DxlDriver<reg_type>::syncWriteTorqueEnable
 * @param id_list
 * @param torque_enable_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncWriteTorqueEnable(const std::vector<uint8_t> &id_list, const std::vector<uint8_t> &torque_enable_list)
{
    return syncWrite<typename reg_type::TYPE_TORQUE_ENABLE>(reg_type::ADDR_TORQUE_ENABLE, id_list, torque_enable_list);
}

/**
 * @brief DxlDriver<reg_type>::syncWritePositionGoal
 * @param id_list
 * @param position_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncWritePositionGoal(const std::vector<uint8_t> &id_list, const std::vector<uint32_t> &position_list)
{
    return syncWrite<typename reg_type::TYPE_GOAL_POSITION>(reg_type::ADDR_GOAL_POSITION, id_list, position_list);
}

/**
 * @brief DxlDriver<reg_type>::syncWriteVelocityGoal
 * @param id_list
 * @param velocity_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncWriteVelocityGoal(const std::vector<uint8_t> &id_list, const std::vector<uint32_t> &velocity_list)
{
    return syncWrite<typename reg_type::TYPE_GOAL_VELOCITY>(reg_type::ADDR_GOAL_VELOCITY, id_list, velocity_list);
}

// ram read
template<typename reg_type>
int DxlDriver<reg_type>::readVelocityProfile(uint8_t id, std::vector<uint32_t>& data_list)
{
    data_list.clear();
    typename reg_type::TYPE_PROFILE velocity_profile{};
    typename reg_type::TYPE_PROFILE acceleration_profile{};

    int res = read<typename reg_type::TYPE_PROFILE>(reg_type::ADDR_PROFILE_VELOCITY, id, velocity_profile);
    if (COMM_SUCCESS == res)
      res = read<typename reg_type::TYPE_PROFILE>(reg_type::ADDR_PROFILE_ACCELERATION, id, acceleration_profile);

    data_list.emplace_back(velocity_profile);
    data_list.emplace_back(acceleration_profile);

    return res;
}


/**
 * @brief DxlDriver<reg_type>::readPosition
 * @param id
 * @param present_position
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readPosition(uint8_t id, uint32_t& present_position)
{
    return read<typename reg_type::TYPE_PRESENT_POSITION>(reg_type::ADDR_PRESENT_POSITION, id, present_position);
}

/**
 * @brief DxlDriver<reg_type>::readTemperature
 * @param id
 * @param temperature
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readTemperature(uint8_t id, uint8_t& temperature)
{
    return read<typename reg_type::TYPE_PRESENT_TEMPERATURE>(reg_type::ADDR_PRESENT_TEMPERATURE, id, temperature);
}

/**
 * @brief DxlDriver<reg_type>::readVoltage
 * @param id
 * @param voltage
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readVoltage(uint8_t id, double& voltage)
{
  typename reg_type::TYPE_PRESENT_VOLTAGE voltage_mV{};
  int res = read<typename reg_type::TYPE_PRESENT_VOLTAGE>(reg_type::ADDR_PRESENT_VOLTAGE, id, voltage_mV);
  voltage = static_cast<double>(voltage_mV) / reg_type::VOLTAGE_CONVERSION;
  return res;
}

/**
 * @brief DxlDriver<reg_type>::readHwErrorStatus
 * @param id
 * @param hardware_status
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readHwErrorStatus(uint8_t id, uint8_t& hardware_status)
{
    return read<typename reg_type::TYPE_HW_ERROR_STATUS>(reg_type::ADDR_HW_ERROR_STATUS, id, hardware_status);
}

/**
 * @brief DxlDriver<reg_type>::syncReadPosition
 * @param id_list
 * @param position_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncReadPosition(const std::vector<uint8_t> &id_list, std::vector<uint32_t> &position_list)
{
  return syncRead<typename reg_type::TYPE_PRESENT_POSITION>(reg_type::ADDR_PRESENT_POSITION, id_list, position_list);
}

/**
 * @brief DxlDriver<reg_type>::writePID
 * @param id
 * @param data
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writePID(uint8_t id, const std::vector<uint32_t> &data)
{
    int tries = 10;
    int res;

    // only rewrite params which is not success
    while (tries > 0)
    {
        tries--;
        res = writePositionPGain(id, data.at(0));
        if (res == COMM_SUCCESS)
            break;
    }
    if (res != COMM_SUCCESS)
        return res;

    tries = 10;
    while (tries > 0)
    {
        tries--;
        res = writePositionIGain(id, data.at(1));
        if (res == COMM_SUCCESS)
            break;
    }
    if (res != COMM_SUCCESS)
        return res;

    tries = 10;
    while (tries > 0)
    {
        tries--;
        res = writePositionDGain(id, data.at(2));
        if (res == COMM_SUCCESS)
            break;
    }
    if (res != COMM_SUCCESS)
        return res;

    tries = 10;
    while (tries > 0)
    {
        tries--;
        res = writeVelocityPGain(id, data.at(3));
        if (res == COMM_SUCCESS)
            break;
    }
    if (res != COMM_SUCCESS)
        return res;

    tries = 10;
    while (tries > 0)
    {
        tries--;
        res = writeVelocityIGain(id, data.at(4));
        if (res == COMM_SUCCESS)
            break;
    }
    if (res != COMM_SUCCESS)
        return res;

    tries = 10;    
    while (tries > 0)
    {
        tries--;
        res = writeFF1Gain(id, data.at(5));
        if (res == COMM_SUCCESS)
            break;
    }
    if (res != COMM_SUCCESS)
        return res;

    tries = 10;
    while (tries > 0)
    {
        tries--;
        res = writeFF2Gain(id, data.at(6));
        if (res == COMM_SUCCESS)
            break;
    }

    return res;
}

/**
 * @brief DxlDriver<reg_type>::syncReadFirmwareVersion
 * @param id_list
 * @param firmware_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncReadFirmwareVersion(const std::vector<uint8_t> &id_list, std::vector<std::string> &firmware_list)
{
    int res = COMM_RX_FAIL;
    std::vector<uint8_t> data_list;
    res = syncRead<typename reg_type::TYPE_FIRMWARE_VERSION>(reg_type::ADDR_FIRMWARE_VERSION, id_list, data_list);
    for(auto const& data : data_list)
      firmware_list.emplace_back(interpretFirmwareVersion(data));
    return res;
}

/**
 * @brief DxlDriver<reg_type>::syncReadTemperature
 * @param id_list
 * @param temperature_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncReadTemperature(const std::vector<uint8_t> &id_list, std::vector<uint8_t>& temperature_list)
{
    return syncRead<typename reg_type::TYPE_PRESENT_TEMPERATURE>(reg_type::ADDR_PRESENT_TEMPERATURE, id_list, temperature_list);
}

/**
 * @brief DxlDriver<reg_type>::syncReadVoltage
 * @param id_list
 * @param voltage_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncReadVoltage(const std::vector<uint8_t> &id_list, std::vector<double> &voltage_list)
{
  voltage_list.clear();
  std::vector<typename reg_type::TYPE_PRESENT_VOLTAGE> v_read;
  int res = syncRead<typename reg_type::TYPE_PRESENT_VOLTAGE>(reg_type::ADDR_PRESENT_VOLTAGE, id_list, v_read);
  for(auto const& v : v_read)
      voltage_list.emplace_back(static_cast<double>(v) / reg_type::VOLTAGE_CONVERSION);
  return res;
}

/**
 * @brief DxlDriver<reg_type>::syncReadRawVoltage
 * @param id_list
 * @param voltage_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncReadRawVoltage(const std::vector<uint8_t> &id_list, std::vector<double> &voltage_list)
{
  voltage_list.clear();
  std::vector<typename reg_type::TYPE_PRESENT_VOLTAGE> v_read;
  int res = syncRead<typename reg_type::TYPE_PRESENT_VOLTAGE>(reg_type::ADDR_PRESENT_VOLTAGE, id_list, v_read);
  for(auto const& v : v_read)
      voltage_list.emplace_back(static_cast<double>(v));
  return res;
}

/**
 * @brief DxlDriver<reg_type>::syncReadHwStatus
 * @param id_list
 * @param data_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncReadHwStatus(const std::vector<uint8_t> &id_list,
                                          std::vector<std::pair<double, uint8_t> >& data_list)
{
    data_list.clear();

    std::vector<std::array<uint8_t, 3> > raw_data;
    int res = syncReadConsecutiveBytes<uint8_t, 3>(reg_type::ADDR_PRESENT_VOLTAGE, id_list, raw_data);

    for (auto const& data : raw_data)
    {
        // Voltage is first reg, uint16
        uint16_t v = ((uint16_t)data.at(1) << 8) | data.at(0);
        double voltage = static_cast<double>(v);

        // Temperature is second reg, uint8
        uint8_t temperature = data.at(2);

        data_list.emplace_back(std::make_pair(voltage, temperature));
    }

    return res;
}

/**
 * @brief DxlDriver<reg_type>::syncReadHwErrorStatus
 * @param id_list
 * @param hw_error_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncReadHwErrorStatus(const std::vector<uint8_t> &id_list, std::vector<uint8_t> &hw_error_list)
{
    return syncRead<typename reg_type::TYPE_HW_ERROR_STATUS>(reg_type::ADDR_HW_ERROR_STATUS, id_list, hw_error_list);
}

//*****************************
// AbstractDxlDriver interface
//*****************************


/**
 * @brief DxlDriver<reg_type>::writeLed
 * @param id
 * @param led_value
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writeLed(uint8_t id, uint8_t led_value)
{
    return write<typename reg_type::TYPE_LED>(reg_type::ADDR_LED, id, led_value);
}

/**
 * @brief DxlDriver<reg_type>::syncWriteLed
 * @param id_list
 * @param led_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncWriteLed(const std::vector<uint8_t> &id_list, const std::vector<uint8_t> &led_list)
{
    return syncWrite<typename reg_type::TYPE_LED>(reg_type::ADDR_LED, id_list, led_list);
}

/**
 * @brief DxlDriver<reg_type>::writeTorqueGoal
 * @param id
 * @param torque
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writeTorqueGoal(uint8_t id, uint16_t torque)
{
    return write<typename reg_type::TYPE_GOAL_TORQUE>(reg_type::ADDR_GOAL_TORQUE, id, torque);
}

/**
 * @brief DxlDriver<reg_type>::syncWriteTorqueGoal
 * @param id_list
 * @param torque_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncWriteTorqueGoal(const std::vector<uint8_t> &id_list, const std::vector<uint16_t> &torque_list)
{
    return syncWrite<typename reg_type::TYPE_GOAL_TORQUE>(reg_type::ADDR_GOAL_TORQUE, id_list, torque_list);
}

// read

/**
 * @brief DxlDriver<reg_type>::readPID
 * @param id
 * @param data_list
 * @return
 *  TODO(cc) bulk read all in one shot
 */
template<typename reg_type>
int DxlDriver<reg_type>::readPID(uint8_t id, std::vector<uint32_t>& data_list)
{
    int res = 0;
    data_list.clear();

    uint16_t pos_p_gain{0};
    if (COMM_SUCCESS != readPositionPGain(id, pos_p_gain))
        res++;

    data_list.emplace_back(pos_p_gain);

    uint16_t pos_i_gain{0};
    if (COMM_SUCCESS != readPositionIGain(id, pos_i_gain))
        res++;

    data_list.emplace_back(pos_i_gain);

    uint16_t pos_d_gain{0};
    if (COMM_SUCCESS != readPositionDGain(id, pos_d_gain))
        res++;

    data_list.emplace_back(pos_d_gain);

    uint16_t vel_p_gain{0};
    if (COMM_SUCCESS != readVelocityPGain(id, vel_p_gain))
        res++;

    data_list.emplace_back(vel_p_gain);

    uint16_t vel_i_gain{0};
    if (COMM_SUCCESS != readVelocityIGain(id, vel_i_gain))
        res++;

    data_list.emplace_back(vel_i_gain);

    uint16_t ff1_gain{0};
    if (COMM_SUCCESS != readFF1Gain(id, ff1_gain))
        res++;

    data_list.emplace_back(ff1_gain);

    uint16_t ff2_gain{0};
    if (COMM_SUCCESS != readFF2Gain(id, ff2_gain))
        res++;

    data_list.emplace_back(ff2_gain);

    if(res > 0)
    {
        std::cout << "Failures during read PID gains: " << res << std::endl;
        return COMM_TX_FAIL;
    }

    return COMM_SUCCESS;
}

/**
 * @brief DxlDriver<reg_type>::writeControlMode
 * @param id
 * @param control_mode
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writeControlMode(uint8_t id, uint8_t control_mode)
{
    return write<typename reg_type::TYPE_OPERATING_MODE>(reg_type::ADDR_OPERATING_MODE, id, control_mode);
}

/**
 * @brief DxlDriver<reg_type>::readControlMode
 * @param id
 * @param data
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readControlMode(uint8_t id, uint8_t &data)
{
    int res = 0;
    typename reg_type::TYPE_OPERATING_MODE raw{};
    res = read<typename reg_type::TYPE_OPERATING_MODE>(reg_type::ADDR_OPERATING_MODE, id, raw);
    data = static_cast<uint8_t>(raw);
    return res;
}
//other

/**
 * @brief DxlDriver<reg_type>::readLoad
 * @param id
 * @param present_load
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readLoad(uint8_t id, uint16_t& present_load)
{
    return read<typename reg_type::TYPE_PRESENT_LOAD>(reg_type::ADDR_PRESENT_LOAD, id, present_load);
}

/**
 * @brief DxlDriver<reg_type>::syncReadLoad
 * @param id_list
 * @param load_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncReadLoad(const std::vector<uint8_t> &id_list, std::vector<uint16_t> &load_list)
{
    return syncRead<typename reg_type::TYPE_PRESENT_LOAD>(reg_type::ADDR_PRESENT_LOAD, id_list, load_list);
}

/**
 * @brief DxlDriver<reg_type>::readVelocity
 * @param id
 * @param present_velocity
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readVelocity(uint8_t id, uint32_t& present_velocity)
{
    return read<typename reg_type::TYPE_PRESENT_VELOCITY>(reg_type::ADDR_PRESENT_VELOCITY, id, present_velocity);
}

/**
 * @brief DxlDriver<reg_type>::syncReadVelocity
 * @param id_list
 * @param velocity_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncReadVelocity(const std::vector<uint8_t> &id_list, std::vector<uint32_t> &velocity_list)
{
  return syncRead<typename reg_type::TYPE_PRESENT_VELOCITY>(reg_type::ADDR_PRESENT_VELOCITY, id_list, velocity_list);
}

/**
 * @brief DxlDriver::syncReadJointStatus
 * @param id_list
 * @param data_array_list
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::syncReadJointStatus(const std::vector<uint8_t> &id_list, std::vector<std::array<uint32_t, 2> > &data_array_list)
{  
    int res = COMM_TX_FAIL;

    if(id_list.empty())
        return res;

    data_array_list.clear();

    // read torque enable on first id
    typename reg_type::TYPE_TORQUE_ENABLE torque;
    read<typename reg_type::TYPE_TORQUE_ENABLE>(reg_type::ADDR_TORQUE_ENABLE, id_list.at(0), torque);

    //if torque on, read position and velocity
    if (torque)
    {
        res = syncReadConsecutiveBytes<uint32_t, 2>(reg_type::ADDR_PRESENT_VELOCITY, id_list, data_array_list);
    }
    else  //else read position only
    {
        std::vector<uint32_t> position_list;
        res = syncReadPosition(id_list, position_list);
        for (auto p : position_list)
          data_array_list.emplace_back(std::array<uint32_t, 2>{0, p});
    }

    return res;
}

// private
// read
/**
 * @brief DxlDriver<reg_type>::readPositionPGain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readPositionPGain(uint8_t id, uint16_t& gain)
{
    return read<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_POSITION_P_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::readPositionIGain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readPositionIGain(uint8_t id, uint16_t& gain)
{
    return read<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_POSITION_I_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::readPositionDGain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readPositionDGain(uint8_t id, uint16_t& gain)
{
    return read<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_POSITION_D_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::readVelocityPGain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readVelocityPGain(uint8_t id, uint16_t& gain)
{
    return read<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_VELOCITY_P_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::readVelocityIGain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readVelocityIGain(uint8_t id, uint16_t& gain)
{
    return read<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_VELOCITY_I_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::readFF1Gain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readFF1Gain(uint8_t id, uint16_t& gain)
{
    return read<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_FF1_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::readFF2Gain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::readFF2Gain(uint8_t id, uint16_t& gain)
{
    return read<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_FF2_GAIN, id, gain);
}

// write
/**
 * @brief DxlDriver<reg_type>::writePositionPGain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writePositionPGain(uint8_t id, uint16_t gain)
{
    return write<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_POSITION_P_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::writePositionIGain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writePositionIGain(uint8_t id, uint16_t gain)
{
    return write<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_POSITION_I_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::writePositionDGain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writePositionDGain(uint8_t id, uint16_t gain)
{
    return write<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_POSITION_D_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::writeVelocityPGain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writeVelocityPGain(uint8_t id, uint16_t gain)
{
    return write<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_VELOCITY_P_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::writeVelocityIGain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writeVelocityIGain(uint8_t id, uint16_t gain)
{
    return write<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_VELOCITY_I_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::writeFF1Gain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writeFF1Gain(uint8_t id, uint16_t gain)
{
    return write<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_FF1_GAIN, id, gain);
}

/**
 * @brief DxlDriver<reg_type>::writeFF2Gain
 * @param id
 * @param gain
 * @return
 */
template<typename reg_type>
int DxlDriver<reg_type>::writeFF2Gain(uint8_t id, uint16_t gain)
{
    return write<typename reg_type::TYPE_PID_GAIN>(reg_type::ADDR_FF2_GAIN, id, gain);
}

/*
 *  -----------------   specializations   --------------------
 */

// XL320

template<>
inline int DxlDriver<XL320Reg>::readMinPosition(uint8_t /*id*/, uint32_t &pos)
{
    pos = 0;
    std::cout << "min position hardcoded for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

template<>
inline int DxlDriver<XL320Reg>::readMaxPosition(uint8_t /*id*/, uint32_t &pos)
{
    pos = 1023;
    std::cout << "max position hardcoded for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

template<>
inline std::string DxlDriver<XL320Reg>::interpretErrorState(uint32_t hw_state) const
{
    std::string hardware_message;

    if (hw_state & 1<<0)    // 0b00000001
    {
        hardware_message += "Overload";
    }
    if (hw_state & 1<<1)    // 0b00000010
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "OverHeating";
    }
    if (hw_state & 1<<2)    // 0b00000100
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Input voltage out of range";
    }
    if (hw_state & 1<<7)    // 0b10000000 => added by us : disconnected error
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Disconnection";
    }

    return hardware_message;
}

template<>
inline int DxlDriver<XL320Reg>::readVelocityProfile(uint8_t /*id*/, std::vector<uint32_t>& /*data_list*/)
{
    std::cout << "readVelocityProfile not available for XL320" << std::endl;
    return COMM_SUCCESS;
}

template<>
inline int DxlDriver<XL320Reg>::writeVelocityProfile(uint8_t /*id*/, const std::vector<uint32_t>& /*data_list*/)
{
    std::cout << "writeVelocityProfile not available for XL320" << std::endl;
    return COMM_SUCCESS;
}


template<>
inline int DxlDriver<XL320Reg>::readPosition(uint8_t id, uint32_t& present_position)
{
    typename XL320Reg::TYPE_PRESENT_POSITION raw_data{};
    int res = read<typename XL320Reg::TYPE_PRESENT_POSITION>(XL320Reg::ADDR_PRESENT_POSITION, id, raw_data);
    present_position = raw_data;
    return res;
}

template<>
inline int DxlDriver<XL320Reg>::syncReadPosition(const std::vector<uint8_t> &id_list, std::vector<uint32_t> &position_list)
{
    position_list.clear();
    std::vector<typename XL320Reg::TYPE_PRESENT_POSITION> raw_data_list{};

    int res = syncRead<typename XL320Reg::TYPE_PRESENT_POSITION>(XL320Reg::ADDR_PRESENT_POSITION, id_list, raw_data_list);
    for(auto p : raw_data_list)
        position_list.emplace_back(p);

    return res;
}

template<>
inline int DxlDriver<XL320Reg>::readVelocity(uint8_t id, uint32_t& present_velocity)
{
    typename XL320Reg::TYPE_PRESENT_VELOCITY raw_data{};
    int res = read<typename XL320Reg::TYPE_PRESENT_VELOCITY>(XL320Reg::ADDR_PRESENT_VELOCITY, id, raw_data);
    present_velocity = raw_data;

    return res;
}

template<>
inline int DxlDriver<XL320Reg>::syncReadVelocity(const std::vector<uint8_t> &id_list, std::vector<uint32_t> &velocity_list)
{
    std::vector<typename XL320Reg::TYPE_PRESENT_VELOCITY> raw_data_list;
    int res = syncRead<typename XL320Reg::TYPE_PRESENT_VELOCITY>(XL320Reg::ADDR_PRESENT_VELOCITY, id_list, raw_data_list);
    for(auto v : raw_data_list)
        velocity_list.emplace_back(v);
    return res;
}

/**
 * @brief DxlDriver::syncReadJointStatus
 * @param id_list
 * @param data_array_list
 * @return
 */
template<>
inline int DxlDriver<XL320Reg>::syncReadJointStatus(const std::vector<uint8_t> &id_list, std::vector<std::array<uint32_t, 2> > &data_array_list)
{
    data_array_list.clear();

    std::vector<std::array<uint16_t, 2> > raw_data;
    int res = syncReadConsecutiveBytes<typename XL320Reg::TYPE_PRESENT_POSITION, 2>(XL320Reg::ADDR_PRESENT_POSITION, id_list, raw_data);
    // invert data
    for (auto const& a : raw_data)
        data_array_list.emplace_back(std::array<uint32_t, 2>{a.at(1), a.at(0)});
    return res;
}

/**
 * @brief DxlDriver<reg_type>::syncWritePositionGoal
 * @param id_list
 * @param position_list
 * @return
 */
template<>
inline int DxlDriver<XL320Reg>::syncWritePositionGoal(const std::vector<uint8_t> &id_list, const std::vector<uint32_t> &position_list)
{
    std::vector<typename XL320Reg::TYPE_GOAL_POSITION> casted_list;
    for (auto const& p : position_list)
    {
        casted_list.emplace_back(static_cast<typename XL320Reg::TYPE_GOAL_POSITION>(p));
    }
    return syncWrite<typename XL320Reg::TYPE_GOAL_POSITION>(XL320Reg::ADDR_GOAL_POSITION, id_list, casted_list);
}

/**
 * @brief DxlDriver<reg_type>::syncWriteVelocityGoal
 * @param id_list
 * @param velocity_list
 * @return
 */
template<>
inline int DxlDriver<XL320Reg>::syncWriteVelocityGoal(const std::vector<uint8_t> &id_list, const std::vector<uint32_t> &velocity_list)
{
    std::vector<typename XL320Reg::TYPE_GOAL_VELOCITY> casted_list;
    for (auto const& v : velocity_list)
    {
        casted_list.emplace_back(static_cast<typename XL320Reg::TYPE_GOAL_VELOCITY>(v));
    }
    return syncWrite<typename XL320Reg::TYPE_GOAL_VELOCITY>(XL320Reg::ADDR_GOAL_VELOCITY, id_list, casted_list);
}

template<>
inline int DxlDriver<XL320Reg>::writeControlMode(uint8_t /*id*/, uint8_t /*data*/)
{
    std::cout << "writeControlMode not available for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

template<>
inline int DxlDriver<XL320Reg>::readControlMode(uint8_t /*id*/, uint8_t& /*data*/)
{
    std::cout << "readControlMode not available for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

// write PID
template<>
inline int DxlDriver<XL320Reg>::writePositionPGain(uint8_t id, uint16_t gain)
{
    return write<typename XL320Reg::TYPE_PID_GAIN>(XL320Reg::ADDR_POSITION_P_GAIN, id, static_cast<typename XL320Reg::TYPE_PID_GAIN>(gain));
}

template<>
inline int DxlDriver<XL320Reg>::writePositionIGain(uint8_t id, uint16_t gain)
{
    return write<typename XL320Reg::TYPE_PID_GAIN>(XL320Reg::ADDR_POSITION_I_GAIN, id, static_cast<typename XL320Reg::TYPE_PID_GAIN>(gain));
}

template<>
inline int DxlDriver<XL320Reg>::writePositionDGain(uint8_t id, uint16_t gain)
{
    return write<typename XL320Reg::TYPE_PID_GAIN>(XL320Reg::ADDR_POSITION_D_GAIN, id, static_cast<typename XL320Reg::TYPE_PID_GAIN>(gain));
}


template<>
inline int DxlDriver<XL320Reg>::writeVelocityPGain(uint8_t /*id*/, uint16_t /*gain*/)
{
    std::cout << "writeVelocityPGain not available for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

template<>
inline int DxlDriver<XL320Reg>::writeVelocityIGain(uint8_t /*id*/, uint16_t /*gain*/)
{
    std::cout << "writeVelocityIGain not available for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

template<>
inline int DxlDriver<XL320Reg>::writeFF1Gain(uint8_t /*id*/, uint16_t /*gain*/)
{
    std::cout << "writeff1Gain not available for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

template<>
inline int DxlDriver<XL320Reg>::writeFF2Gain(uint8_t /*id*/, uint16_t /*gain*/)
{
    std::cout << "writeff2Gain not available for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

// read PID
template<>
inline int DxlDriver<XL320Reg>::readPositionPGain(uint8_t id, uint16_t& gain)
{
    uint8_t raw_data{};
    int res = read<typename XL320Reg::TYPE_PID_GAIN>(XL320Reg::ADDR_POSITION_P_GAIN, id, raw_data);
    gain = raw_data;

    return res;
}

template<>
inline int DxlDriver<XL320Reg>::readPositionIGain(uint8_t id, uint16_t& gain)
{
    uint8_t raw_data{};
    int res = read<typename XL320Reg::TYPE_PID_GAIN>(XL320Reg::ADDR_POSITION_I_GAIN, id, raw_data);
    gain = raw_data;

    return res;
}

template<>
inline int DxlDriver<XL320Reg>::readPositionDGain(uint8_t id, uint16_t& gain)
{
    uint8_t raw_data{};
    int res = read<typename XL320Reg::TYPE_PID_GAIN>(XL320Reg::ADDR_POSITION_D_GAIN, id, raw_data);
    gain = raw_data;

    return res;
}

template<>
inline int DxlDriver<XL320Reg>::readVelocityPGain(uint8_t /*id*/, uint16_t& /*gain*/)
{
    std::cout << "readVelocityPGain not available for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

template<>
inline int DxlDriver<XL320Reg>::readVelocityIGain(uint8_t /*id*/, uint16_t& /*gain*/)
{
    std::cout << "readVelocityIGain not available for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

template<>
inline int DxlDriver<XL320Reg>::readFF1Gain(uint8_t /*id*/, uint16_t& /*gain*/)
{
    std::cout << "readFF1Gain not available for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

template<>
inline int DxlDriver<XL320Reg>::readFF2Gain(uint8_t /*id*/, uint16_t& /*gain*/)
{
    std::cout << "readFF2Gain not available for motor XL320" << std::endl;
    return COMM_SUCCESS;
}

// XL430

template<>
inline std::string DxlDriver<XL430Reg>::interpretErrorState(uint32_t hw_state) const
{
    std::string hardware_message;

    if (hw_state & 1<<0)    // 0b00000001
    {
        hardware_message += "Input Voltage";
    }
    if (hw_state & 1<<2)    // 0b00000100
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "OverHeating";
    }
    if (hw_state & 1<<3)    // 0b00001000
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Motor Encoder";
    }
    if (hw_state & 1<<4)    // 0b00010000
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Electrical Shock";
    }
    if (hw_state & 1<<5)    // 0b00100000
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Overload";
    }
    if (hw_state & 1<<7)    // 0b10000000 => added by us : disconnected error
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Disconnection";
    }
    if (!hardware_message.empty())
        hardware_message += " Error";

    return hardware_message;
}

template<>
inline int DxlDriver<XL430Reg>::writeTorqueGoal(uint8_t /*id*/, uint16_t /*torque*/)
{
    std::cout << "writeTorqueGoal not available for motor XL430" << std::endl;
    return COMM_TX_ERROR;
}

template<>
inline int DxlDriver<XL430Reg>::syncWriteTorqueGoal(const std::vector<uint8_t> &/*id_list*/, const std::vector<uint16_t> &/*torque_list*/)
{
    std::cout << "syncWriteTorqueGoal not available for motor XL430" << std::endl;
    return COMM_TX_ERROR;
}

// XC430

template<>
inline std::string DxlDriver<XC430Reg>::interpretErrorState(uint32_t hw_state) const
{
    std::string hardware_message;

    if (hw_state & 1<<0)    // 0b00000001
    {
        hardware_message += "Input Voltage";
    }
    if (hw_state & 1<<2)    // 0b00000100
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "OverHeating";
    }
    if (hw_state & 1<<3)    // 0b00001000
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Motor Encoder";
    }
    if (hw_state & 1<<4)    // 0b00010000
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Electrical Shock";
    }
    if (hw_state & 1<<5)    // 0b00100000
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Overload";
    }
    if (hw_state & 1<<7)    // 0b10000000 => added by us : disconnected error
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Disconnection";
    }
    if (!hardware_message.empty())
        hardware_message += " Error";

    return hardware_message;
}

template<>
inline int DxlDriver<XC430Reg>::writeTorqueGoal(uint8_t /*id*/, uint16_t /*torque*/)
{
    std::cout << "writeTorqueGoal not available for motor XC430" << std::endl;
    return COMM_TX_ERROR;
}

template<>
inline int DxlDriver<XC430Reg>::syncWriteTorqueGoal(const std::vector<uint8_t> &/*id_list*/, const std::vector<uint16_t> &/*torque_list*/)
{
    std::cout << "syncWriteTorqueGoal not available for motor XC430" << std::endl;
    return COMM_TX_ERROR;
}

// XL330

template<>
inline std::string DxlDriver<XL330Reg>::interpretErrorState(uint32_t hw_state) const
{
    std::string hardware_message;

    if (hw_state & 1<<0)    // 0b00000001
    {
        hardware_message += "Input Voltage";
    }
    if (hw_state & 1<<2)    // 0b00000100
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "OverHeating";
    }
    if (hw_state & 1<<3)    // 0b00001000
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Motor Encoder";
    }
    if (hw_state & 1<<4)    // 0b00010000
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Electrical Shock";
    }
    if (hw_state & 1<<5)    // 0b00100000
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Overload";
    }
    if (hw_state & 1<<7)    // 0b10000000 => added by us : disconnected error
    {
        if (!hardware_message.empty())
            hardware_message += ", ";
        hardware_message += "Disconnection";
    }
    if (!hardware_message.empty())
        hardware_message += " Error";

    return hardware_message;
}

// works with current instead of load

template<>
inline int DxlDriver<XL330Reg>::writeTorqueGoal(uint8_t id, uint16_t torque)
{
    return write<typename XL330Reg::TYPE_GOAL_CURRENT>(XL330Reg::ADDR_GOAL_CURRENT, id, torque);
}

template<>
inline int DxlDriver<XL330Reg>::syncWriteTorqueGoal(const std::vector<uint8_t> &id_list, const std::vector<uint16_t>& torque_list)
{
    return syncWrite<typename XL330Reg::TYPE_GOAL_CURRENT>(XL330Reg::ADDR_GOAL_CURRENT, id_list, torque_list);
}

template<>
inline int DxlDriver<XL330Reg>::readLoad(uint8_t id, uint16_t& present_load)
{
    return read<typename XL330Reg::TYPE_PRESENT_CURRENT>(XL330Reg::ADDR_PRESENT_CURRENT, id, present_load);
}

template<>
inline int DxlDriver<XL330Reg>::syncReadLoad(const std::vector<uint8_t> &id_list, std::vector<uint16_t> &load_list)
{
    return syncRead<typename XL330Reg::TYPE_PRESENT_CURRENT>(XL330Reg::ADDR_PRESENT_CURRENT, id_list, load_list);
}

} // ttl_driver

#endif // DxlDriver
