#!/usr/bin/env python

import rospy

# Command Status
from niryo_robot_msgs.msg import CommandStatus

# Services
from tools_interface.srv import PingDxlTool
from tools_interface.srv import OpenGripper, CloseGripper
from tools_interface.srv import PullAirVacuumPump, PushAirVacuumPump
from niryo_robot_rpi.srv import SetDigitalIO
from niryo_robot_rpi.srv import SetIOMode


class ToolRosCommandInterface:

    def __init__(self, state_ros_communication_problem):
        self._digital_outputs = rospy.get_param("/niryo_robot_rpi/digital_outputs")

        namespace = rospy.get_param("~namespace_topics")
        self.__service_open_gripper = rospy.ServiceProxy(namespace + 'open_gripper',
                                                         OpenGripper)
        self.__service_close_gripper = rospy.ServiceProxy(namespace + 'close_gripper',
                                                          CloseGripper)

        self.__service_pull_air_vacuum_pump = rospy.ServiceProxy(namespace + 'pull_air_vacuum_pump',
                                                                 PullAirVacuumPump)
        self.__service_push_air_vacuum_pump = rospy.ServiceProxy(namespace + 'push_air_vacuum_pump',
                                                                 PushAirVacuumPump)

        self.__service_ping_dxl_tool = rospy.ServiceProxy(namespace + 'ping_and_set_dxl_tool', PingDxlTool)

        self.__service_activate_digital_output_tool = rospy.ServiceProxy('/niryo_robot_rpi/set_digital_io',
                                                                         SetDigitalIO)
        self.__service_setup_digital_output_tool = rospy.ServiceProxy('/niryo_robot_rpi/set_digital_io_mode',
                                                                      SetIOMode)

        self.__state_ros_communication_problem = state_ros_communication_problem
        rospy.logdebug("Interface between Tools Commander and ROS Control has been started.")

    # Gripper

    def open_gripper(self, gripper_id, open_position, open_speed, open_hold_torque, open_max_torque):
        try:
            resp = self.__service_open_gripper(gripper_id, open_position, open_speed, open_hold_torque, open_max_torque)
            return resp.state
        except rospy.ServiceException:
            rospy.logerr("ROS Tool Interface - Failed to Open Gripper")
            return self.__state_ros_communication_problem

    def close_gripper(self, gripper_id, close_position, close_speed, close_hold_torque, close_max_torque):
        try:
            resp = self.__service_close_gripper(gripper_id, close_position, close_speed, close_hold_torque,
                                                close_max_torque)
            return resp.state
        except rospy.ServiceException:
            rospy.logerr("ROS Tool Interface - Failed to Close Gripper")
            return self.__state_ros_communication_problem

    # Vacuum
    def pull_air_vacuum_pump(
            self, vp_id, vp_pull_air_velocity, vp_pull_air_position,
            vp_pull_air_max_torque, vp_pull_air_hold_torque):
        try:
            resp = self.__service_pull_air_vacuum_pump(
                vp_id, vp_pull_air_velocity, vp_pull_air_position,
                vp_pull_air_max_torque, vp_pull_air_hold_torque)
            return resp.state
        except rospy.ServiceException:
            rospy.logerr("ROS Tool Interface - Failed to Pull Air")
            return self.__state_ros_communication_problem

    def push_air_vacuum_pump(self, vp_id, vp_push_air_velocity, vp_push_air_position, vp_push_air_max_torque):
        try:
            resp = self.__service_push_air_vacuum_pump(
                vp_id, vp_push_air_velocity,
                vp_push_air_position, vp_push_air_max_torque)
            return resp.state
        except rospy.ServiceException:
            rospy.logerr("ROS Tool Interface - Failed to Push Air")
            return self.__state_ros_communication_problem

    # Others
    def ping_dxl_tool(self):
        try:
            resp = self.__service_ping_dxl_tool()
            if (resp is not None):
                return resp.state, resp.id
            else:
                return CommandStatus.TOOL_ROS_INTERFACE_ERROR, "Cannot ping dxl tool"
        except rospy.ServiceException:
            rospy.logerr("ROS Tool Interface - Failed to Ping Dynamixel Tool - "
                         "An error has occurred or another ping was running")
            return CommandStatus.TOOL_ROS_INTERFACE_ERROR, "Cannot ping dxl tool"

    def digital_output_tool_setup(self, gpio_name):
        if rospy.get_param("/niryo_robot/hardware_version") == "ned2":
            return CommandStatus.SUCCESS, "Success"

        try:
            resp = self.__service_setup_digital_output_tool(gpio_name, SetDigitalIO.Request.OUTPUT)  # set output
            return resp.status, resp.message
        except rospy.ServiceException:
            rospy.logerr("ROS Tool Interface - Failed to get digital output setup")
            return CommandStatus.TOOL_ROS_INTERFACE_ERROR, "Digital IO panel service failed"

    def digital_output_tool_activate(self, gpio_name, activate):
        try:
            resp = self.__service_activate_digital_output_tool(gpio_name, bool(activate))
            return resp.status, resp.message
        except rospy.ServiceException:
            rospy.logerr("ROS Tool Interface - Failed to activate digital output")
            return CommandStatus.TOOL_ROS_INTERFACE_ERROR, "Digital IO panel service failed"
