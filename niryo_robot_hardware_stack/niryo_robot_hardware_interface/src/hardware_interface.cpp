/*
    hardware_interface.cpp
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

#include <functional>

#include "niryo_robot_hardware_interface/hardware_interface.hpp"

#include "model/dxl_motor_type_enum.hpp"
#include "model/stepper_motor_type_enum.hpp"

namespace NiryoRobotHardwareInterface
{
    HardwareInterface::HardwareInterface(ros::NodeHandle &nh) : _nh(nh)
    {
        initParams();
        initNodes();
        initPublishers();
    }

    bool HardwareInterface::_callbackStopMotorsReport(niryo_robot_msgs::Trigger::Request &req, niryo_robot_msgs::Trigger::Response &res)
    {
        if (!_simulation_mode)
        {
            ROS_WARN("Hardware Interface - Stop Motor Report");
            _stepper_driver->activeDebugMode(false);
            _dynamixel_driver->activeDebugMode(false);
            res.status = niryo_robot_msgs::CommandStatus::SUCCESS;
            res.message = "";
            return true;
        }
        return true;
    }

    bool HardwareInterface::_callbackLaunchMotorsReport(niryo_robot_msgs::Trigger::Request &req, niryo_robot_msgs::Trigger::Response &res)
    {
        if (!_simulation_mode)
        {
            ROS_WARN("Hardware Interface - Start Motors Report");
            _stepper_driver->activeDebugMode(true);
            _dynamixel_driver->activeDebugMode(true);
            int stepper_status = _stepper_driver->launchMotorsReport();
            int dxl_status = _dynamixel_driver->launchMotorsReport();
            _stepper_driver->activeDebugMode(false);
            _dynamixel_driver->activeDebugMode(false);
            ROS_WARN("Hardware Interface - Motors report ended");
            if ((stepper_status == niryo_robot_msgs::CommandStatus::SUCCESS) and (dxl_status == niryo_robot_msgs::CommandStatus::SUCCESS))
            {
                res.status = niryo_robot_msgs::CommandStatus::SUCCESS;
                res.message = "Hardware interface seems working properly";
            }
            else
            {
                res.status = niryo_robot_msgs::CommandStatus::FAILURE;
                res.message = "Steppers status: ";
                res.message += (stepper_status == niryo_robot_msgs::CommandStatus::SUCCESS) ? "Ok" : "Error";
                res.message += ", Dxl status: ";
                res.message += (dxl_status == niryo_robot_msgs::CommandStatus::SUCCESS) ? "Ok" : "Error";
            }
        }
        return true;
    }

    bool HardwareInterface::_callbackRebootMotors(niryo_robot_msgs::Trigger::Request &req, niryo_robot_msgs::Trigger::Response &res)
    {
        if (!_simulation_mode)
        {
            res.status = _dynamixel_driver->rebootMotors();
            if (res.status != COMM_SUCCESS)
            {
                res.message = "Reboot motors Problems";
                return true;
            }
            res.message = "Reboot motors done";

            _joints_interface->sendMotorsParams();
            int resp_learning_mode_status;
            std::string resp_learning_mode_message = "";
            _joints_interface->activateLearningMode(false, resp_learning_mode_status, resp_learning_mode_message);
            _joints_interface->activateLearningMode(true, resp_learning_mode_status, resp_learning_mode_message);
        }
        return true;
    }

    void HardwareInterface::initNodes()
    {
        ROS_DEBUG("HardwareInterface::initNodes - Init Nodes");
        if (!_simulation_mode)
        {

            if (!_dxl_enabled)
            {
                ROS_WARN("HardwareInterface::initNodes - DXL communication is disabled for debug purposes");
            }
            else
            {
                ROS_DEBUG("HardwareInterface::initNodes - Start Dynamixel Driver Node");
                _dynamixel_driver.reset(new DynamixelDriver::DynamixelDriverCore());
                ros::Duration(0.25).sleep();
            }

            if (!_can_enabled)
            {
                ROS_DEBUG("HardwareInterface::initNodes - CAN communication is disabled for debug purposes");
            }
            else
            {
                ROS_DEBUG("HardwareInterface::initNodes - Start Stepper Driver Node");
                _stepper_driver.reset(new StepperDriver::StepperDriverCore());
                ros::Duration(0.25).sleep();
            }

            if (_can_enabled && _dxl_enabled)
            {
                ROS_DEBUG("HardwareInterface::initNodes - Start Joints Interface Node");
                _joints_interface.reset(new JointsInterface::JointsInterfaceCore(_dynamixel_driver, _stepper_driver));
                ros::Duration(0.25).sleep();

                ROS_DEBUG("HardwareInterface::initNodes - Start End Effector Interface Node");
                _tools_interface.reset(new ToolsInterface::ToolsInterfaceCore(_dynamixel_driver));
                ros::Duration(0.25).sleep();

                ROS_DEBUG("HardwareInterface::initNodes - Start Tools Interface Node");
                _conveyor_interface.reset(new ConveyorInterface::ConveyorInterfaceCore(_stepper_driver));
                ros::Duration(0.25).sleep();
            }
            else
            {
                ROS_WARN("HardwareInterface::initNodes - CAN or DXL communication is disabled. Interfaces will not start");
            }

            ROS_DEBUG("HardwareInterface::initNodes - Start CPU Interface Node");
            _cpu_interface.reset(new CpuInterface::CpuInterfaceCore());
            ros::Duration(0.25).sleep();
        }
        else
        {
            ROS_DEBUG("HardwareInterface::initNodes - Start Fake Interface Node");
            _fake_interface.reset(new FakeInterface::FakeInterfaceCore());
        }
    }

    void HardwareInterface::initPublishers()
    {
        ROS_DEBUG("Hardware Interface - Init Publisher");
        _hardware_status_publisher = _nh.advertise<niryo_robot_msgs::HardwareStatus>("niryo_robot_hardware_interface/hardware_status", 10);
        _publish_hardware_status_thread.reset(new std::thread(&HardwareInterface::_publishHardwareStatus, this));

        _software_version_publisher = _nh.advertise<niryo_robot_msgs::SoftwareVersion>("niryo_robot_hardware_interface/software_version", 10);
        _publish_software_version_thread.reset(new std::thread(&HardwareInterface::_publishSoftwareVersion, this));

        _motors_report_service = _nh.advertiseService("/niryo_robot_hardware_interface/launch_motors_report", &HardwareInterface::_callbackLaunchMotorsReport, this);
        _stop_motors_report_service = _nh.advertiseService("/niryo_robot_hardware_interface/stop_motors_report", &HardwareInterface::_callbackStopMotorsReport, this);
        _reboot_motors_service = _nh.advertiseService("/niryo_robot_hardware_interface/reboot_motors", &HardwareInterface::_callbackRebootMotors, this);
    }

    void HardwareInterface::initParams()
    {

        ROS_DEBUG("Hardware Interface - Init Params");
        ros::param::get("~publish_hw_status_frequency", _publish_hw_status_frequency);
        ros::param::get("~publish_software_version_frequency", _publish_software_version_frequency);

        ros::param::get("/niryo_robot/info/image_version", _rpi_image_version);
        ros::param::get("/niryo_robot/info/ros_version", _ros_niryo_robot_version);

        ros::param::get("~simulation_mode", _simulation_mode);
        ros::param::get("~gazebo", _gazebo);

        ros::param::get("~can_enabled", _can_enabled);
        ros::param::get("~dxl_enabled", _dxl_enabled);
        
        _rpi_image_version.erase(_rpi_image_version.find_last_not_of(" \n\r\t") + 1);
        _ros_niryo_robot_version.erase(_ros_niryo_robot_version.find_last_not_of(" \n\r\t") + 1);

        ROS_DEBUG("Hardware Interface - publish_hw_status_frequency : %f", _publish_hw_status_frequency);
        ROS_DEBUG("Hardware Interface - publish_software_version_frequency : %f", _publish_software_version_frequency);

        ROS_DEBUG("Hardware Interface - can_enabled : %s", _can_enabled ? "true" : "false");
        ROS_DEBUG("Hardware Interface - dxl_enabled : %s", _dxl_enabled ? "true" : "false");
    }

    void HardwareInterface::_publishHardwareStatus()
    {
        ros::Rate publish_hardware_status_rate = ros::Rate(_publish_hw_status_frequency);
        while (ros::ok())
        {
            std::vector<int32_t> temperatures;
            std::vector<double> voltages;
            std::vector<int32_t> hw_errors;
            std::vector<std::string> hw_errors_msg;

            dynamixel_driver::DxlArrayMotorHardwareStatus dxl_motor_state;
            stepper_driver::StepperArrayMotorHardwareStatus stepper_motor_state;

            niryo_robot_msgs::BusState dxl_bus_state;
            niryo_robot_msgs::BusState can_bus_state;

            std::vector<common::model::JointState> joints_state;

            int cpu_temperature;
            std::string error_message;

            bool need_calibration = false;
            bool calibration_in_progress = false;

            std::vector<std::string> motor_names;
            std::vector<std::string> motor_types;

            niryo_robot_msgs::HardwareStatus msg;
            msg.header.stamp = ros::Time::now();

            if (!_simulation_mode)
            {
                if (_dxl_enabled)
                {
                    dxl_motor_state = _dynamixel_driver->getHwStatus();
                    dxl_bus_state = _dynamixel_driver->getDxlBusState();
                }
                if (_can_enabled)
                {
                    stepper_motor_state = _stepper_driver->getHwStatus();
                    can_bus_state = _stepper_driver->getCanBusState();
                }
                if (_can_enabled && _dxl_enabled)
                {
                    joints_state = _joints_interface->getJointsState();
                    _joints_interface->getCalibrationState(need_calibration, calibration_in_progress);
                }

                cpu_temperature = _cpu_interface->getCpuTemperature();
            }
            else
            {
                dxl_motor_state = _fake_interface->getDxlHwStatus();
                stepper_motor_state = _fake_interface->getStepperHwStatus();

                dxl_bus_state = _fake_interface->getDxlBusState();
                can_bus_state = _fake_interface->getCanBusState();

                cpu_temperature = _fake_interface->getCpuTemperature();

                _fake_interface->getCalibrationState(need_calibration, calibration_in_progress);
            }

            msg.rpi_temperature = cpu_temperature;
            msg.hardware_version = 1;
            
            msg.connection_up = (dxl_bus_state.connection_status && can_bus_state.connection_status);

            error_message = "";
            error_message += can_bus_state.error;
            if (dxl_bus_state.error != "")
            {
                error_message += "\n";
            }
            error_message += dxl_bus_state.error;
            msg.error_message = error_message;

            msg.calibration_needed = need_calibration;
            msg.calibration_in_progress = calibration_in_progress;

            motor_names.clear();
            motor_types.clear();

            for (int i = 0; i < stepper_motor_state.motors_hw_status.size(); i++)
            {
                temperatures.push_back(stepper_motor_state.motors_hw_status.at(i).temperature);
                voltages.push_back(stepper_motor_state.motors_hw_status.at(i).voltage);
                hw_errors.push_back(stepper_motor_state.motors_hw_status.at(i).error);
                hw_errors_msg.push_back("");
                motor_types.push_back("Niryo Stepper");
                std::string joint_name = "";
                if (!_simulation_mode)
                    joint_name = _joints_interface->jointIdToJointName(stepper_motor_state.motors_hw_status.at(i).motor_identity.motor_id,
                                                                       static_cast<uint8_t>(common::model::EStepperMotorType::MOTOR_TYPE_STEPPER));
                else
                    joint_name = _fake_interface->jointIdToJointName(stepper_motor_state.motors_hw_status.at(i).motor_identity.motor_id,
                                                                     static_cast<uint8_t>(common::model::EStepperMotorType::MOTOR_TYPE_STEPPER));

                joint_name = joint_name == "" ? ("Stepper " + std::to_string(dxl_motor_state.motors_hw_status.at(i).motor_identity.motor_id)) : joint_name;
                motor_names.push_back(joint_name);
            }

            // for each motor gather info in the dedicated vectors
            for (int i = 0; i < dxl_motor_state.motors_hw_status.size(); i++)
            {
                temperatures.push_back(dxl_motor_state.motors_hw_status.at(i).temperature);
                voltages.push_back(dxl_motor_state.motors_hw_status.at(i).voltage);
                hw_errors.push_back(dxl_motor_state.motors_hw_status.at(i).error);
                hw_errors_msg.push_back(dxl_motor_state.motors_hw_status.at(i).error_msg);

                switch(dxl_motor_state.motors_hw_status.at(i).motor_identity.motor_type)
                {
                    case static_cast<uint8_t>(common::model::EDxlMotorType::MOTOR_TYPE_XL320):
                        motor_types.push_back("DXL XL-320");
                    break;
                        
                    case static_cast<uint8_t>(common::model::EDxlMotorType::MOTOR_TYPE_XL330):
                        motor_types.push_back("DXL XL-330");
                    break;
                    
                    case static_cast<uint8_t>(common::model::EDxlMotorType::MOTOR_TYPE_XL430):
                        motor_types.push_back("DXL XL-430");
                    break;
                    
                    case static_cast<uint8_t>(common::model::EDxlMotorType::MOTOR_TYPE_XC430):
                        motor_types.push_back("DXL XC-430");
                    break;
                    default:
                        motor_types.push_back("DXL UNKOWN");
                    break;
                }
                
                std::string joint_name = "";
                if (!_simulation_mode)
                    joint_name = _joints_interface->jointIdToJointName(dxl_motor_state.motors_hw_status.at(i).motor_identity.motor_id, 
                                                                       dxl_motor_state.motors_hw_status.at(i).motor_identity.motor_type);
                else
                    joint_name = _fake_interface->jointIdToJointName(dxl_motor_state.motors_hw_status.at(i).motor_identity.motor_id, 
                                                                     dxl_motor_state.motors_hw_status.at(i).motor_identity.motor_type);

                joint_name = (joint_name == "") ? "Tool" : joint_name;
                motor_names.push_back(joint_name);
            }

            msg.motor_names = motor_names;
            msg.motor_types = motor_types;

            msg.temperatures = temperatures;
            msg.voltages = voltages;
            msg.hardware_errors = hw_errors;
            msg.hardware_errors_message = hw_errors_msg;

            _hardware_status_publisher.publish(msg);

            publish_hardware_status_rate.sleep();
        }
    }

    void HardwareInterface::_publishSoftwareVersion()
    {
        ros::Rate publish_software_version_rate = ros::Rate(_publish_software_version_frequency);
        while (ros::ok())
        {

            stepper_driver::StepperArrayMotorHardwareStatus stepper_motor_state;
            std::vector<std::string> motor_names;
            std::vector<std::string> firmware_versions;
            std::vector<common::model::JointState> joints_state;

            if (!_simulation_mode)
            {
                if (_can_enabled)
                {
                    stepper_motor_state = _stepper_driver->getHwStatus();
                }
                if (_can_enabled && _dxl_enabled)
                {
                    joints_state = _joints_interface->getJointsState();
                }

                for (int i = 0; i < joints_state.size(); i++)
                {
                    motor_names.push_back(joints_state.at(i).getName());
                }
            }
            else
            {
                stepper_motor_state = _fake_interface->getStepperHwStatus();

                motor_names.push_back("joint_1");
                motor_names.push_back("joint_2");
                motor_names.push_back("joint_3");
                motor_names.push_back("joint_4");
                motor_names.push_back("joint_5");
                motor_names.push_back("joint_6");
            }

            for (int i = 0; i < stepper_motor_state.motors_hw_status.size(); i++)
            {
                firmware_versions.push_back(stepper_motor_state.motors_hw_status.at(i).firmware_version);
            }

            niryo_robot_msgs::SoftwareVersion msg;
            msg.motor_names = motor_names;
            msg.stepper_firmware_versions = firmware_versions;
            msg.rpi_image_version = _rpi_image_version;
            msg.ros_niryo_robot_version = _ros_niryo_robot_version;

            _software_version_publisher.publish(msg);
            publish_software_version_rate.sleep();
        }
    }

} // namespace NiryoRobotHardwareInterface
