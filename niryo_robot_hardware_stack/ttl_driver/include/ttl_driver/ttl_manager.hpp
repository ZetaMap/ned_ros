/*
ttl_manager.hpp
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

#ifndef TTL_DRIVER_HPP
#define TTL_DRIVER_HPP

#include <memory>
#include <ros/ros.h>
#include <string>
#include <thread>
#include <queue>
#include <functional>
#include <algorithm>
#include <set>

#include "dynamixel_sdk/dynamixel_sdk.h"

#include "ttl_driver/MotorCommand.h"
#include "niryo_robot_msgs/MotorHeader.h"
#include "niryo_robot_msgs/SetInt.h"
#include "niryo_robot_msgs/CommandStatus.h"

#include "common/model/i_bus_manager.hpp"

// drivers
#include "ttl_driver/abstract_motor_driver.hpp"
#include "common/model/dxl_motor_state.hpp"
#include "common/model/synchronize_motor_cmd.hpp"
#include "common/model/single_motor_cmd.hpp"
#include "common/model/stepper_calibration_status_enum.hpp"

namespace ttl_driver
{

/**
 * Parameters for DXL
*/
constexpr float TTL_BUS_PROTOCOL_VERSION = 2.0;
constexpr int TTL_FAIL_OPEN_PORT         = -4500;

constexpr int TTL_FAIL_PORT_SET_BAUDRATE = -4501;
constexpr int TTL_FAIL_SETUP_GPIO        = -4502;

constexpr int TTL_SCAN_OK                = 0;
constexpr int TTL_SCAN_MISSING_MOTOR     = -50;
constexpr int TTL_SCAN_UNALLOWED_MOTOR   = -51;
constexpr int TTL_WRONG_TYPE             = -52;

/**
 * Parameters for Stepper
*/

/**
 * @brief The TtlManager class manages the different motor drivers connected on the TTL bus
 * it is used by ttl_interface_core to send or receive data to the ttl bus
 * it also manages the lifecycle of all motors driver (do we need to add also the end effector driver in it ?)
 *
 */
class TtlManager : public common::model::IBusManager
{
    public:
        enum class EType
        {
            TOOL,
            CONVOYER,
            JOINT,
            END_EFFECTOR
        };

    public:
        TtlManager(ros::NodeHandle& nh);
        virtual ~TtlManager() override;

        bool init(ros::NodeHandle& nh) override;

        // commands
        void addHardwareComponent(common::model::EHardwareType type,
                                  uint8_t id, EType type_used);
        int changeId(common::model::EHardwareType motor_type, uint8_t old_id, uint8_t new_id);

        int writeSynchronizeCommand(const std::shared_ptr<common::model::AbstractTtlSynchronizeMotorCmd >& cmd);
        int writeSingleCommand(const std::shared_ptr<common::model::AbstractTtlSingleMotorCmd >& cmd);

        void executeJointTrajectoryCmd(std::vector<std::pair<uint8_t, uint32_t> > cmd_vec);

        int rebootMotors();
        int rebootMotor(uint8_t motor_id);

        int setLeds(int led);

        int sendCustomCommand(common::model::EHardwareType motor_type, uint8_t id, int reg_address, int value, int byte_number);
        int readCustomCommand(common::model::EHardwareType motor_type, uint8_t id, int32_t reg_address, int &value, int byte_number);

        bool readPositionStatus();
        bool readEndEffectorStatus();
        bool readHwStatus();

        int getAllIdsOnBus(std::vector<uint8_t> &id_list);

        //calibration
        void startCalibration();
        void resetCalibration();
        bool isCalibrationInProgress() const;
        int32_t getCalibrationResult(uint8_t id) const;
        common::model::EStepperCalibrationStatus getCalibrationStatus() const;
        void updateCurrentCalibrationStatus();
        // getters
        uint32_t getPosition(common::model::JointState& motor_state);
        int getLedState() const;

        std::vector<std::shared_ptr<common::model::JointState> > getMotorsStates() const;
        template<class T>
        T getHardwareState(uint8_t motor_id) const;

        std::vector<uint8_t> getRemovedMotorList() const;

        // IBusManager Interface
        void removeMotor(uint8_t id) override;
        bool isConnectionOk() const override;
        bool hasEndEffector() const;

        int scanAndCheck() override;
        bool ping(uint8_t id) override;

        size_t getNbMotors() const override;
        void getBusState(bool& connection_state, std::vector<uint8_t>& motor_id, std::string& debug_msg) const override;
        std::string getErrorMessage() const override;
    private:

        int setupCommunication();

        void checkRemovedMotors();

    private:
        std::shared_ptr<dynamixel::PortHandler> _portHandler;
        std::shared_ptr<dynamixel::PacketHandler> _packetHandler;

        std::string _device_name;
        int _uart_baudrate{1000000};

        std::vector<uint8_t> _all_motor_connected; // with all dxl motors connected (including the tool)
        std::vector<uint8_t> _removed_motor_id_list;

        // state of a component for a given id
        std::map<uint8_t, std::shared_ptr<common::model::AbstractHardwareState> > _state_map;
        // map of associated ids for a given hardware type
        std::map<common::model::EHardwareType, std::vector<uint8_t> > _ids_map;
        // map of drivers for a given hardware type (xl, stepper, end effector)
        std::map<common::model::EHardwareType, std::shared_ptr<AbstractTtlDriver> > _driver_map;

        double _calibration_timeout{30.0};
        common::model::EStepperCalibrationStatus _calibration_status{common::model::EStepperCalibrationStatus::CALIBRATION_UNINITIALIZED};

        // for hardware control
        bool _is_connection_ok{false};
        std::string _debug_error_message;

        int _hw_fail_counter_read{0};
        
        int _led_state{-1};

        std::string _led_motor_type_cfg;

        static constexpr int MAX_HW_FAILURE = 500;
        static constexpr int CALIBRATION_IDLE = 0;
        static constexpr int CALIBRATION_IN_PROGRESS = 1;
        static constexpr int CALIBRATION_SUCCESS = 2;
        static constexpr int CALIBRATION_ERROR = 3;
        std::map<int, common::model::EStepperCalibrationStatus> _map_calibration_status {
                                        {CALIBRATION_SUCCESS, common::model::EStepperCalibrationStatus::CALIBRATION_OK},
                                        {CALIBRATION_IN_PROGRESS, common::model::EStepperCalibrationStatus::CALIBRATION_IN_PROGRESS},
                                        {CALIBRATION_ERROR, common::model::EStepperCalibrationStatus::CALIBRATION_FAIL},
                                        {CALIBRATION_IDLE, common::model::EStepperCalibrationStatus::CALIBRATION_UNINITIALIZED}};

        bool _use_simu_gripper = true;
};

// inline getters

/**
 * @brief TtlManager::getMotorState
 * @param motor_id
 * @return
 */
template<class T>
T TtlManager::getHardwareState(uint8_t motor_id) const
{
    if (!_state_map.count(motor_id) && _state_map.at(motor_id))
        throw std::out_of_range("TtlManager::getMotorsState: Unknown motor id");

    return *(dynamic_cast<T*>(_state_map.at(motor_id).get()));
}

inline
bool TtlManager::isConnectionOk() const
{
    return _is_connection_ok;
}

/**
 * @brief TtlManager::getNbMotors
 * @return
 */
inline
size_t TtlManager::getNbMotors() const
{
    return _state_map.size();
}

/**
 * @brief TtlManager::getRemovedMotorList
 * @return
 */
inline
std::vector<uint8_t> TtlManager::getRemovedMotorList() const
{
    return _removed_motor_id_list;
}

/**
 * @brief TtlManager::getErrorMessage
 * @return
 */
inline
std::string TtlManager::getErrorMessage() const
{
    return _debug_error_message;
}

/**
 * @brief TtlManager::getLedState
 * @return
 * TODO(CC) to be removed from TtlManager : not the purpose of the manager to register states
 */
inline
int TtlManager::getLedState() const
{
    return _led_state;
}

/**
 * @brief TtlManager::getBusState
 * @param connection_state
 * @param motor_id
 * @param debug_msg
 */
inline
void TtlManager::getBusState(bool &connection_state, std::vector<uint8_t> &motor_id,
                             std::string &debug_msg) const
{
    debug_msg = _debug_error_message;
    motor_id = _all_motor_connected;
    connection_state = isConnectionOk();
}

/**
 * @brief TtlManager::hasEndEffector
 * @return
 */
inline
bool TtlManager::hasEndEffector() const
{
    return _driver_map.count(common::model::EHardwareType::END_EFFECTOR);
}

} // ttl_driver

#endif // TTLDRIVER_HPP
