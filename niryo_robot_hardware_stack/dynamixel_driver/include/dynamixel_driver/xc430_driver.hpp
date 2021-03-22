/*
    xc430_driver.hpp
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
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef XC430_DRIVER_HPP
#define XC430_DRIVER_HPP

#include <boost/shared_ptr.hpp>

#include <vector>
#include <string>
#include <iostream>

#include "dynamixel_driver/xdriver.hpp"
#include "dynamixel_sdk/dynamixel_sdk.h"

#define XC430_PROTOCOL_VERSION 2.0
#define XC430_MODEL_NUMBER 1060

// Table here : https://emanual.robotis.com/docs/en/dxl/x/xc430-w240/
#define XC430_ADDR_MODEL_NUMBER 0
#define XC430_ADDR_FIRMWARE_VERSION 6
#define XC430_ADDR_ID 7
#define XC430_ADDR_BAUDRATE 8
#define XC430_ADDR_RETURN_DELAY_TIME 9
#define XC430_ADDR_DRIVE_MODE 10
#define XC430_ADDR_OPERATING_MODE 11
#define XC430_ADDR_HOMING_OFFSET 20 // EEPROM (not all addresses)
#define XC430_ADDR_TEMPERATURE_LIMIT 31
#define XC430_ADDR_MAX_VOLTAGE_LIMIT 32
#define XC430_ADDR_MIN_VOLTAGE_LIMIT 34
#define XC430_ADDR_MAX_POSITION_LIMIT 48
#define XC430_ADDR_MIN_POSITION_LIMIT 52
#define XC430_ADDR_ALARM_SHUTDOWN 63

#define XC430_ADDR_TORQUE_ENABLE 64
#define XC430_ADDR_LED 65
#define XC430_ADDR_STATUS_RETURN_LEVEL 68
#define XC430_ADDR_HW_ERROR_STATUS 70
#define XC430_ADDR_GOAL_PWM 100
#define XC430_ADDR_GOAL_VELOCITY 104
#define XC430_ADDR_GOAL_POSITION 116 // RAM (not all addresses)
#define XC430_ADDR_MOVING 122
#define XC430_ADDR_PRESENT_PWM 124
#define XC430_ADDR_PRESENT_LOAD 126
#define XC430_ADDR_PRESENT_VELOCITY 128
#define XC430_ADDR_PRESENT_POSITION 132
#define XC430_ADDR_PRESENT_VOLTAGE 144
#define XC430_ADDR_PRESENT_TEMPERATURE 146

namespace DynamixelDriver {
    /**
     * @brief The XC430Driver class
     */
    class XC430Driver : public XDriver
    {
    public:
        XC430Driver(boost::shared_ptr<dynamixel::PortHandler>& portHandler,
                    boost::shared_ptr<dynamixel::PacketHandler>& packetHandler);

        uint32_t rad_pos_to_xc430_pos(double position_rad);
        double xc430_pos_to_rad_pos(int32_t position_dxl);

        //from XDriver interface
        int checkModelNumber(uint8_t id);

        // eeprom write
        int changeId(uint8_t id, uint8_t new_id);
        int changeBaudRate(uint8_t id, uint32_t new_baudrate);
        int setReturnDelayTime(uint8_t id, uint32_t return_delay_time);
        int setLimitTemperature(uint8_t id, uint32_t temperature);
        int setMaxTorque(uint8_t id, uint32_t torque);
        int setReturnLevel(uint8_t id, uint32_t return_level);
        int setAlarmShutdown(uint8_t id, uint32_t alarm_shutdown);

        // eeprom read
        int readReturnDelayTime(uint8_t id, uint32_t *return_delay_time);
        int readLimitTemperature(uint8_t id, uint32_t *limit_temperature);
        int readMaxTorque(uint8_t id, uint32_t *max_torque);
        int readReturnLevel(uint8_t id, uint32_t *return_level);
        int readAlarmShutdown(uint8_t id, uint32_t *alarm_shutdown);

        // ram write
        int setTorqueEnable(uint8_t id, uint32_t torque_enable);
        int setLed(uint8_t id, uint32_t led_value);
        int setGoalPosition(uint8_t id, uint32_t position);
        int setGoalVelocity(uint8_t id, uint32_t velocity);
        int setGoalTorque(uint8_t id, uint32_t torque);

        int syncWriteLed(std::vector<uint8_t> &id_list, std::vector<uint32_t> &led_list);
        int syncWriteTorqueEnable(std::vector<uint8_t> &id_list, std::vector<uint32_t> &torque_enable_list);
        int syncWritePositionGoal(std::vector<uint8_t> &id_list, std::vector<uint32_t> &position_list);
        int syncWriteVelocityGoal(std::vector<uint8_t> &id_list, std::vector<uint32_t> &velocity_list);
        int syncWriteTorqueGoal(std::vector<uint8_t> &id_list, std::vector<uint32_t> &torque_list);

        // ram read
        int readPosition(uint8_t id, uint32_t *present_position);
        int readVelocity(uint8_t id, uint32_t *present_velocity);
        int readLoad(uint8_t id, uint32_t *present_load);
        int readTemperature(uint8_t id, uint32_t *temperature);
        int readVoltage(uint8_t id, uint32_t *voltage);
        int readHardwareStatus(uint8_t id, uint32_t *hardware_status);

        int syncReadPosition(std::vector<uint8_t> &id_list, std::vector<uint32_t> &position_list);
        int syncReadVelocity(std::vector<uint8_t> &id_list, std::vector<uint32_t> &velocity_list);
        int syncReadLoad(std::vector<uint8_t> &id_list, std::vector<uint32_t> &load_list);
        int syncReadTemperature(std::vector<uint8_t> &id_list, std::vector<uint32_t> &temperature_list);
        int syncReadVoltage(std::vector<uint8_t> &id_list, std::vector<uint32_t> &voltage_list);
        int syncReadHwErrorStatus(std::vector<uint8_t> &id_list, std::vector<uint32_t> &hw_error_list);

        // custom write
        int customWrite(uint8_t id, uint32_t value, uint8_t reg_address, uint8_t byte_number);
    };

} //DynamixelDriver

#endif
