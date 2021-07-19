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
    along with this program.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include <functional>
#include <string>
#include <vector>

#include "niryo_robot_hardware_interface/hardware_interface.hpp"

#include "common/model/motor_type_enum.hpp"

#include "common/util/util_defs.hpp"

namespace niryo_robot_hardware_interface
{
/**
 * @brief HardwareInterface::HardwareInterface
 * @param nh
 */
HardwareInterface::HardwareInterface(ros::NodeHandle &nh) :
    _nh(nh)
{
    init(nh);
}

/**
 * @brief HardwareInterface::~HardwareInterface
 */
HardwareInterface::~HardwareInterface()
{
    if (_publish_software_version_thread.joinable())
    {
        _publish_software_version_thread.join();
    }

    if (_publish_hw_status_thread.joinable())
    {
        _publish_hw_status_thread.join();
    }
}

/**
 * @brief HardwareInterface::init
 * @param nh
 * @return
 */
bool HardwareInterface::init(ros::NodeHandle &nh)
{
    ROS_DEBUG("HardwareInterface::init - Initializing parameters...");
    initParameters(nh);

    ROS_DEBUG("HardwareInterface::init - Init Nodes...");
    initNodes(nh);

    ROS_DEBUG("HardwareInterface::init - Starting services...");
    startServices(nh);

    ROS_DEBUG("HardwareInterface::init - Starting publishers...");
    startPublishers(nh);

    ROS_DEBUG("HardwareInterface::init - Starting subscribers...");
    startSubscribers(nh);

    return true;
}

/**
 * @brief HardwareInterface::initParameters
 * @param nh
 */
void HardwareInterface::initParameters(ros::NodeHandle &nh)
{
    nh.getParam("publish_hw_status_frequency", _publish_hw_status_frequency);
    nh.getParam("publish_software_version_frequency", _publish_software_version_frequency);

    nh.getParam("/niryo_robot/info/image_version", _rpi_image_version);
    nh.getParam("/niryo_robot/info/ros_version", _ros_niryo_robot_version);

    nh.getParam("simulation_mode", _simulation_mode);
    nh.getParam("gazebo", _gazebo);

    nh.getParam("can_enabled", _can_enabled);
    nh.getParam("ttl_enabled", _ttl_enabled);

    _rpi_image_version.erase(_rpi_image_version.find_last_not_of(" \n\r\t") + 1);
    _ros_niryo_robot_version.erase(_ros_niryo_robot_version.find_last_not_of(" \n\r\t") + 1);

    ROS_DEBUG("HardwareInterface::initParameters - publish_hw_status_frequency : %f",
                            _publish_hw_status_frequency);
    ROS_DEBUG("HardwareInterface::initParameters - publish_software_version_frequency : %f",
                            _publish_software_version_frequency);

    ROS_DEBUG("HardwareInterface::initParameters - image_version : %s",
                            _rpi_image_version.c_str());
    ROS_DEBUG("HardwareInterface::initParameters - ros_version : %s",
                            _ros_niryo_robot_version.c_str());

    ROS_DEBUG("HardwareInterface::initParameters - simulation_mode : %s", _simulation_mode ? "true" : "false");
    ROS_DEBUG("HardwareInterface::initParameters - gazebo : %s", _gazebo ? "true" : "false");

    ROS_DEBUG("HardwareInterface::initParameters - can_enabled : %s", _can_enabled ? "true" : "false");
    ROS_DEBUG("HardwareInterface::initParameters - ttl_enabled : %s", _ttl_enabled ? "true" : "false");
}

/**
 * @brief HardwareInterface::initNodes
 * @param nh
 */
void HardwareInterface::initNodes(ros::NodeHandle &nh)
{
    ROS_DEBUG("HardwareInterface::initNodes - Init Nodes");
    if (!_simulation_mode)
    {
        if (_ttl_enabled)
        {
            ROS_DEBUG("HardwareInterface::initNodes - Start Dynamixel Driver Node");
            ros::NodeHandle nh_ttl(nh, "ttl_driver");
            _ttl_driver = std::make_shared<ttl_driver::TtlDriverCore>(nh_ttl);
            ros::Duration(0.25).sleep();
        }
        else
        {
            ROS_WARN("HardwareInterface::initNodes - DXL communication is disabled for debug purposes");
        }

        if (_can_enabled)
        {
            ROS_DEBUG("HardwareInterface::initNodes - Start CAN Driver Node");
            ros::NodeHandle nh_can(nh, "can_driver");
            _can_driver = std::make_shared<can_driver::CanDriverCore>(nh_can);
            ros::Duration(0.25).sleep();
        }
        else
        {
            ROS_DEBUG("HardwareInterface::initNodes - CAN communication is disabled for debug purposes");
        }

        if (_can_enabled && _ttl_enabled)
        {
            ROS_DEBUG("HardwareInterface::initNodes - Start Joints Interface Node");
            ros::NodeHandle nh_joints(nh, "joints_interface");
            _joints_interface = std::make_shared<joints_interface::JointsInterfaceCore>(nh, nh_joints, _ttl_driver, _can_driver);
            ros::Duration(0.25).sleep();

            ROS_DEBUG("HardwareInterface::initNodes - Start End Effector Interface Node");
            ros::NodeHandle nh_tool(nh, "tools_interface");
            _tools_interface = std::make_shared<tools_interface::ToolsInterfaceCore>(nh_tool, _ttl_driver);
            ros::Duration(0.25).sleep();

            ROS_DEBUG("HardwareInterface::initNodes - Start Tools Interface Node");
            ros::NodeHandle nh_conveyor(nh, "conveyor");
            _conveyor_interface = std::make_shared<conveyor_interface::ConveyorInterfaceCore>(nh_conveyor, _can_driver);
            ros::Duration(0.25).sleep();
        }
        else
        {
            ROS_WARN("HardwareInterface::initNodes - CAN and DXL communication is disabled. Interfaces will not start");
        }

        ROS_DEBUG("HardwareInterface::initNodes - Start CPU Interface Node");
        _cpu_interface = std::make_shared<cpu_interface::CpuInterfaceCore>(nh);
        ros::Duration(0.25).sleep();
    }
    else
    {
        ROS_DEBUG("HardwareInterface::initNodes - Start Fake Interface Node");
        _fake_interface = std::make_shared<fake_interface::FakeInterfaceCore>(nh);
    }
}

/**
 * @brief HardwareInterface::startServices
 * @param nh
 */
void HardwareInterface::startServices(ros::NodeHandle& nh)
{
    _motors_report_service = nh.advertiseService("/niryo_robot_hardware_interface/launch_motors_report",
                                                  &HardwareInterface::_callbackLaunchMotorsReport, this);

    _stop_motors_report_service = nh.advertiseService("/niryo_robot_hardware_interface/stop_motors_report",
                                                       &HardwareInterface::_callbackStopMotorsReport, this);

    _reboot_motors_service = nh.advertiseService("/niryo_robot_hardware_interface/reboot_motors",
                                                  &HardwareInterface::_callbackRebootMotors, this);
}

/**
 * @brief HardwareInterface::startPublishers
 */
void HardwareInterface::startPublishers(ros::NodeHandle &nh)
{
    _hardware_status_publisher = nh.advertise<niryo_robot_msgs::HardwareStatus>(
                                            "/niryo_robot_hardware_interface/hardware_status", 10);
    _publish_hw_status_thread = std::thread(&HardwareInterface::_publishHardwareStatus, this);

    _software_version_publisher = nh.advertise<niryo_robot_msgs::SoftwareVersion>(
                                            "/niryo_robot_hardware_interface/software_version", 10);
    _publish_software_version_thread = std::thread(&HardwareInterface::_publishSoftwareVersion, this);
}

/**
 * @brief HardwareInterface::startSubscribers
 * @param nh
 */
void HardwareInterface::startSubscribers(ros::NodeHandle& /*nh*/)
{
    ROS_DEBUG("HardwareInterface::startSubscribers - no subscribers to start");
}

// ********************
//  Callbacks
// ********************

/**
 * @brief HardwareInterface::_callbackStopMotorsReport
 * @param req
 * @param res
 * @return
 */
bool HardwareInterface::_callbackStopMotorsReport(niryo_robot_msgs::Trigger::Request &req,
                                                  niryo_robot_msgs::Trigger::Response &res)
{
    res.status = niryo_robot_msgs::CommandStatus::FAILURE;

    if (!_simulation_mode)
    {
        ROS_WARN("Hardware Interface - Stop Motor Report");

        if(_can_driver)
            _can_driver->activeDebugMode(false);

        if(_ttl_driver)
            _ttl_driver->activeDebugMode(false);

        res.status = niryo_robot_msgs::CommandStatus::SUCCESS;
        res.message = "";
    }
    else
    {
        res.status = niryo_robot_msgs::CommandStatus::SUCCESS;
        res.message = "Simulation mode : fake stop motor report";
    }

    return (niryo_robot_msgs::CommandStatus::SUCCESS == res.status);
}

/**
 * @brief HardwareInterface::_callbackLaunchMotorsReport
 * @param req
 * @param res
 * @return
 */
bool HardwareInterface::_callbackLaunchMotorsReport(niryo_robot_msgs::Trigger::Request &req,
                                                    niryo_robot_msgs::Trigger::Response &res)
{
    res.status = niryo_robot_msgs::CommandStatus::FAILURE;

    if (!_simulation_mode)
    {
        ROS_WARN("Hardware Interface - Start Motors Report");

        int can_status = niryo_robot_msgs::CommandStatus::FAILURE;
        int ttl_status = niryo_robot_msgs::CommandStatus::FAILURE;

        if(_can_driver)
        {
            _can_driver->activeDebugMode(true);
            can_status = _can_driver->launchMotorsReport();
            _can_driver->activeDebugMode(false);
        }

        if(_ttl_driver)
        {
            _ttl_driver->activeDebugMode(true);
            ttl_status = _ttl_driver->launchMotorsReport();
            _ttl_driver->activeDebugMode(false);
        }

        ROS_WARN("Hardware Interface - Motors report ended");

        if ((niryo_robot_msgs::CommandStatus::SUCCESS == can_status) &&
            (niryo_robot_msgs::CommandStatus::SUCCESS == ttl_status))
        {
            res.status = niryo_robot_msgs::CommandStatus::SUCCESS;
            res.message = "Hardware interface seems working properly";
        }
        else
        {
            res.status = niryo_robot_msgs::CommandStatus::FAILURE;
            res.message = "Steppers status: ";
            res.message += (can_status == niryo_robot_msgs::CommandStatus::SUCCESS) ? "Ok" : "Error";
            res.message += ", Dxl status: ";
            res.message += (ttl_status == niryo_robot_msgs::CommandStatus::SUCCESS) ? "Ok" : "Error";
        }
    }
    else
    {
        res.status = niryo_robot_msgs::CommandStatus::SUCCESS;
        res.message = "Simulation mode : fake launch motor report";
    }

    return (niryo_robot_msgs::CommandStatus::SUCCESS == res.status);
}

/**
 * @brief HardwareInterface::_callbackRebootMotors
 * @param req
 * @param res
 * @return
 */
bool HardwareInterface::_callbackRebootMotors(niryo_robot_msgs::Trigger::Request &req,
                                              niryo_robot_msgs::Trigger::Response &res)
{
    res.status = niryo_robot_msgs::CommandStatus::FAILURE;

    if (!_simulation_mode)
    {
        if(_ttl_driver)
            res.status = _ttl_driver->rebootMotors();

        if (niryo_robot_msgs::CommandStatus::SUCCESS == res.status)
        {
            res.message = "Reboot motors done";

            _joints_interface->sendMotorsParams();

            int resp_learning_mode_status = 0;
            std::string resp_learning_mode_message = "";
            _joints_interface->activateLearningMode(false, resp_learning_mode_status, resp_learning_mode_message);
            _joints_interface->activateLearningMode(true, resp_learning_mode_status, resp_learning_mode_message);
        }
        else
        {
            res.message = "Reboot motors Problems";
        }
    }
    else
    {
        res.status = niryo_robot_msgs::CommandStatus::SUCCESS;
        res.message = "Simulation mode : fake reboot motor service";
    }

    return (niryo_robot_msgs::CommandStatus::SUCCESS == res.status);
}

/**
 * @brief HardwareInterface::_publishHardwareStatus
 */
void HardwareInterface::_publishHardwareStatus()
{
    ros::Rate publish_hardware_status_rate = ros::Rate(_publish_hw_status_frequency);

    while (ros::ok())
    {
        ttl_driver::DxlArrayMotorHardwareStatus ttl_motor_state;
        can_driver::StepperArrayMotorHardwareStatus can_motor_state;

        niryo_robot_msgs::BusState ttl_bus_state;
        niryo_robot_msgs::BusState can_bus_state;

        bool need_calibration = false;
        bool calibration_in_progress = false;

        int cpu_temperature = 0;

        niryo_robot_msgs::HardwareStatus msg;
        msg.header.stamp = ros::Time::now();

        if (!_simulation_mode)
        {
            if (_ttl_driver)
            {
                ttl_motor_state = _ttl_driver->getHwStatus();
                ttl_bus_state = _ttl_driver->getBusState();
            }
            if (_can_driver)
            {
                can_motor_state = _can_driver->getHwStatus();
                can_bus_state = _can_driver->getBusState();
            }
            if (_joints_interface)
            {
                _joints_interface->getCalibrationState(need_calibration, calibration_in_progress);
            }

            cpu_temperature = _cpu_interface->getCpuTemperature();
        }
        else
        {
            ttl_motor_state = _fake_interface->getTtlHwStatus();
            can_motor_state = _fake_interface->getCanHwStatus();

            ttl_bus_state = _fake_interface->getTtlBusState();
            can_bus_state = _fake_interface->getCanBusState();

            cpu_temperature = _fake_interface->getCpuTemperature();

            _fake_interface->getCalibrationState(need_calibration, calibration_in_progress);
        }

        msg.rpi_temperature = cpu_temperature;
        msg.hardware_version = 1;

        msg.connection_up = (ttl_bus_state.connection_status && can_bus_state.connection_status);

        std::string error_message = can_bus_state.error;
        if (!ttl_bus_state.error.empty())
        {
            error_message += "\n";
            error_message += ttl_bus_state.error;
        }

        msg.error_message = error_message;

        msg.calibration_needed = need_calibration;
        msg.calibration_in_progress = calibration_in_progress;

        std::vector<int32_t> temperatures;
        std::vector<double> voltages;
        std::vector<int32_t> hw_errors;
        std::vector<std::string> hw_errors_msg;
        std::vector<std::string> motor_types;
        std::vector<std::string> motor_names;

        for (auto const& hw_status : can_motor_state.motors_hw_status)
        {
            temperatures.emplace_back(hw_status.temperature);
            voltages.emplace_back(hw_status.voltage);
            hw_errors.emplace_back(hw_status.error);
            hw_errors_msg.emplace_back("");
            motor_types.emplace_back("Niryo Stepper");
            std::string joint_name = "";
            if (_joints_interface)
            {
                joint_name = _joints_interface->jointIdToJointName(hw_status.motor_identity.motor_id,
                                                                   hw_status.motor_identity.motor_type);
            }
            else if (_fake_interface)
            {
                joint_name = _fake_interface->jointIdToJointName(hw_status.motor_identity.motor_id,
                                                                 hw_status.motor_identity.motor_type);
            }

            joint_name = joint_name == "" ? ("Stepper " + std::to_string(hw_status.motor_identity.motor_id))
                                          : joint_name;

            motor_names.emplace_back(joint_name);
        }

        // for each motor gather info in the dedicated vectors
        for (auto const& hw_status : ttl_motor_state.motors_hw_status)
        {
            temperatures.emplace_back(static_cast<int32_t>(hw_status.temperature));
            voltages.emplace_back(hw_status.voltage);
            hw_errors.emplace_back(static_cast<int32_t>(hw_status.error));
            hw_errors_msg.emplace_back(hw_status.error_msg);

            switch (hw_status.motor_identity.motor_type)
            {
                case static_cast<uint8_t>(common::model::EMotorType::XL320):
                    motor_types.emplace_back("DXL XL-320");
                break;

                case static_cast<uint8_t>(common::model::EMotorType::XL330):
                    motor_types.emplace_back("DXL XL-330");
                break;

                case static_cast<uint8_t>(common::model::EMotorType::XL430):
                    motor_types.emplace_back("DXL XL-430");
                break;

                case static_cast<uint8_t>(common::model::EMotorType::XC430):
                    motor_types.emplace_back("DXL XC-430");
                break;
                default:
                    motor_types.emplace_back("DXL UNKOWN");
                break;
            }

            std::string joint_name = "";
            if (_joints_interface)
            {
                joint_name = _joints_interface->jointIdToJointName(hw_status.motor_identity.motor_id,
                                                                   hw_status.motor_identity.motor_type);
            }
            else if (_fake_interface)
            {
                joint_name = _fake_interface->jointIdToJointName(hw_status.motor_identity.motor_id,
                                                                 hw_status.motor_identity.motor_type);
            }

            joint_name = (joint_name == "") ? "Tool" : joint_name;
            motor_names.emplace_back(joint_name);
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

/**
 * @brief HardwareInterface::_publishSoftwareVersion
 */
void HardwareInterface::_publishSoftwareVersion()
{
    ros::Rate publish_software_version_rate = ros::Rate(_publish_software_version_frequency);
    while (ros::ok())
    {
        can_driver::StepperArrayMotorHardwareStatus stepper_motor_state;
        std::vector<std::string> motor_names;
        std::vector<std::string> firmware_versions;
        std::vector<std::shared_ptr<common::model::JointState> > joints_state;

        if (!_simulation_mode)
        {
            if (_can_driver)
            {
                stepper_motor_state = _can_driver->getHwStatus();
            }
            if (_joints_interface)
            {
                joints_state = _joints_interface->getJointsState();
            }

            for (std::shared_ptr<common::model::JointState> jState : joints_state)
            {
                motor_names.push_back(jState->getName());
            }
        }
        else
        {
            stepper_motor_state = _fake_interface->getCanHwStatus();

            motor_names.push_back("joint_1");
            motor_names.push_back("joint_2");
            motor_names.push_back("joint_3");
            motor_names.push_back("joint_4");
            motor_names.push_back("joint_5");
            motor_names.push_back("joint_6");
        }

        for (auto const& hw_status : stepper_motor_state.motors_hw_status)
        {
            firmware_versions.push_back(hw_status.firmware_version);
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

}  // namespace niryo_robot_hardware_interface
