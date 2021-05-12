/*
    JointHardwareInterface.hpp
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

#ifndef JOINT_HARDWARE_INTERFACE_HPP
#define JOINT_HARDWARE_INTERFACE_HPP

//std
#include <memory>
#include <algorithm>

//ros
#include <ros/ros.h>

//niryo
#include <hardware_interface/joint_state_interface.h>
#include <hardware_interface/joint_command_interface.h>
#include <hardware_interface/robot_hw.h>

#include "joints_interface/calibration_manager.hpp"
#include "stepper_driver/stepper_driver_core.hpp"
#include "dynamixel_driver/dxl_driver_core.hpp"

#include "model/joint_state.hpp"

namespace JointsInterface {

    class JointHardwareInterface : public hardware_interface::RobotHW
    {

    public:
        JointHardwareInterface(
            std::shared_ptr<DynamixelDriver::DxlDriverCore> dynamixel,
            std::shared_ptr<StepperDriver::StepperDriverCore> stepper);

        void sendInitMotorsParams();
        int calibrateJoints(int mode, std::string &result_message);
        void deactivateLearningMode();
        void setNeedCalibration();
        void activateLearningMode();
        void synchronizeMotors(bool synchronize);

        void setCommandToCurrentPosition();

        bool needCalibration() const;
        bool isCalibrationInProgress() const;

        std::string jointIdToJointName(uint8_t id) const;
        const std::vector<std::shared_ptr<common::model::JointState> >& getJointsState() const;

        // RobotHW interface
    public:
        virtual void read(const ros::Time &/*time*/, const ros::Duration &/*period*/) override;
        virtual void write(const ros::Time &/*time*/, const ros::Duration &/*period*/) override;

    private:
        void initJoints();

        void initPublisherSubscribers();
        void initServices();
        bool setMotorPID(const std::shared_ptr<common::model::DxlMotorState> &dxlState);

    private:
        ros::NodeHandle _nh;

        hardware_interface::JointStateInterface _joint_state_interface;
        hardware_interface::PositionJointInterface _joint_position_interface;

        std::shared_ptr<DynamixelDriver::DxlDriverCore> _dynamixelCore;
        std::shared_ptr<StepperDriver::StepperDriverCore> _stepperCore;
        std::unique_ptr<CalibrationManager> _calibration_manager;

        std::map<uint8_t, std::string> _map_stepper_name;
        std::map<uint8_t, std::string> _map_dxl_name;

        std::vector<std::shared_ptr<common::model::JointState> > _joint_list;

        double _gear_ratio_1, _gear_ratio_2, _gear_ratio_3;
        double _direction_1, _direction_2, _direction_3;

        bool _learning_mode;
    };

    /**
     * @brief JointHardwareInterface::getJointsState
     * @return
     */
    inline
    bool JointHardwareInterface::isCalibrationInProgress() const
    {
        return _calibration_manager->CalibrationInprogress();
    }

    /**
     * @brief JointHardwareInterface::getJointsState
     * @return
     */
    inline
    const std::vector<std::shared_ptr<common::model::JointState> >&
    JointHardwareInterface::getJointsState() const
    {
        return _joint_list;
    }
} // JointsInterface

#endif
