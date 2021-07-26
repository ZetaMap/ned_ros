#!/usr/bin/env python

# Lib
import time
import threading
import rosgraph_msgs.msg
import rospy
import actionlib

# Command Status
from niryo_robot_msgs.msg import CommandStatus, SoftwareVersion

# Messages
from actionlib_msgs.msg import GoalStatus

from geometry_msgs.msg import Pose, Point, Quaternion

from std_msgs.msg import Bool
from std_msgs.msg import Int32
from std_msgs.msg import String

from sensor_msgs.msg import CameraInfo
from sensor_msgs.msg import CompressedImage
from sensor_msgs.msg import JointState

from conveyor_interface.msg import ConveyorFeedbackArray
from niryo_robot_msgs.msg import HardwareStatus
from niryo_robot_msgs.msg import RobotState
from niryo_robot_msgs.msg import RPY
from niryo_robot_rpi.msg import DigitalIOState
from niryo_robot_tools_commander.msg import ToolCommand

# Services
from conveyor_interface.srv import ControlConveyor, SetConveyor, SetConveyorRequest
from niryo_robot_arm_commander.srv import GetFK, GetIK
from niryo_robot_arm_commander.srv import JogShift, JogShiftRequest
from niryo_robot_msgs.srv import GetNameDescriptionList, SetBool, SetInt, Trigger
from niryo_robot_tools_commander.srv import SetTCP, SetTCPRequest
from niryo_robot_vision.srv import SetImageParameter
from niryo_robot_rpi.srv import GetDigitalIO, SetDigitalIO
from niryo_robot_vision.srv import DebugMarkers, DebugMarkersRequest, DebugColorDetection, DebugColorDetectionRequest
from niryo_robot_programs_manager.srv import SetProgramAutorun, SetProgramAutorunRequest, GetProgramAutorunInfos, GetProgramList, ManageProgram, ManageProgramRequest, GetProgram, GetProgramRequest, ExecuteProgram, ExecuteProgramRequest
from std_srvs.srv import Trigger as StdTrigger

# Actions
from niryo_robot_arm_commander.msg import RobotMoveAction, RobotMoveGoal
from niryo_robot_arm_commander.msg import ArmMoveCommand
from niryo_robot_tools_commander.msg import ToolActionGoal, ToolResult, ToolAction

# Enums
from niryo_robot_python_ros_wrapper.ros_wrapper_enums import *


class NiryoRosWrapperException(Exception):
    pass


class NiryoRosWrapper:

    LOGS_LEVELS = {
        rospy.INFO: 'INFO',
        rospy.WARN: 'WARNING',
        rospy.ERROR: 'ERROR',
        rospy.FATAL: 'FATAL',
        rospy.DEBUG: 'DEBUG'
    }

    def __init__(self):
        # - Getting ROS parameters
        self.__service_timeout = rospy.get_param("/niryo_robot/python_ros_wrapper/service_timeout")
        self.__action_connection_timeout = rospy.get_param("/niryo_robot/python_ros_wrapper/action_connection_timeout")
        self.__action_execute_timeout = rospy.get_param("/niryo_robot/python_ros_wrapper/action_execute_timeout")
        self.__action_preempt_timeout = rospy.get_param("/niryo_robot/python_ros_wrapper/action_preempt_timeout")
        self.__tool_command_list = rospy.get_param("/niryo_robot_tools/command_list")
        self.__simulation_mode = rospy.get_param("/niryo_robot/simulation_mode")
        self.__hardware_version = rospy.get_param("/niryo_robot/hardware_version")
        self.__can_enabled = rospy.get_param("/niryo_robot_hardware_interface/can_enabled")
        self.__dxl_enabled = rospy.get_param("/niryo_robot_hardware_interface/dxl_enabled")

        # - Publishers
        # Highlight publisher (to highlight blocks in Blockly interface)
        self.__highlight_block_publisher = rospy.Publisher('/niryo_robot_blockly/highlight_block', String,
                                                           queue_size=10)

        # Break point publisher (for break point blocks in Blockly interface)
        self.__break_point_publisher = rospy.Publisher('/niryo_robot_blockly/break_point', Int32, queue_size=10)

        # -- Subscribers
        # - Pose
        self.__joints = None
        rospy.Subscriber('/joint_states', JointState,
                         self.__callback_sub_joint_states)

        self.__pose = None
        rospy.Subscriber('/niryo_robot/robot_state', RobotState,
                         self.__callback_sub_robot_state)

        # - Hardware
        self.__learning_mode_on = None
        rospy.Subscriber('/niryo_robot/learning_mode/state', Bool,
                         self.__callback_sub_learning_mode)

        self.__hw_status = None
        rospy.Subscriber('/niryo_robot_hardware_interface/hardware_status', HardwareStatus,
                         self.__callback_sub_hardware_status)

        self.__digital_io_state = None
        rospy.Subscriber('/niryo_robot_rpi/digital_io_state', DigitalIOState,
                         self.__callback_sub_digital_io_state)

        self.__current_tool_id = None
        rospy.Subscriber('/niryo_robot_tools_commander/current_id', Int32,
                         self.__callback_sub_current_tool_id)

        self.__max_velocity_scaling_factor = None
        rospy.Subscriber('/niryo_robot/max_velocity_scaling_factor', Int32,
                         self.__callback_sub_max_velocity_scaling_factor)

        # - Software

        self.__software_version = None
        rospy.Subscriber('/niryo_robot_hardware_interface/software_version', SoftwareVersion,
                         self.__callback_software_version)

        # - Vision
        self.__compressed_image_message = None
        rospy.Subscriber('/niryo_robot_vision/compressed_video_stream', CompressedImage,
                         self.__callback_sub_stream_video, queue_size=1)

        self.__camera_intrinsics_message = None
        rospy.Subscriber('/niryo_robot_vision/camera_intrinsics', CameraInfo,
                         self.__callback_camera_intrinsics, queue_size=1)

        # - Conveyor

        self.__conveyors_feedback = None
        rospy.Subscriber('/niryo_robot/conveyor/feedback', ConveyorFeedbackArray,
                         self.__callback_sub_conveyors_feedback)

        # - Logs

        self.__logs = []
        rospy.Subscriber(
            '/rosout_agg', rosgraph_msgs.msg.Log, self.__callback_rosout_agg
        )

        # - Blockly

        self.__highlighted_block = None
        rospy.Subscriber(
            '/niryo_robot_blockly/highlight_block', String, self.__callback_highlight_block
        )

        self.__save_current_position_event = threading.Event()
        rospy.Subscriber('/niryo_robot/blockly/save_current_point', Int32, self.__callback_save_current_position)

        # - Action server
        # Robot action
        self.__robot_action_server_name = '/niryo_robot_arm_commander/robot_action'
        self.__robot_action_server_client = actionlib.SimpleActionClient(self.__robot_action_server_name,
                                                                         RobotMoveAction)

        # Tool action
        self.__tool_action_server_name = '/niryo_robot_tools_commander/action_server'
        self.__tool_action_server_client = actionlib.SimpleActionClient(self.__tool_action_server_name,
                                                                        ToolAction)

    def __del__(self):
        del self

    @classmethod
    def wait_for_nodes_initialization(cls, simulation_mode=False):
        params_checked = [
            '/niryo_robot_poses_handlers/initialized',
            '/niryo_robot_arm_commander/initialized',
        ]
        while not all([rospy.has_param(param) for param in params_checked]):
            rospy.sleep(0.1)
        if simulation_mode:
            rospy.sleep(1)  # Waiting to be sure Gazebo is open

    # -- Publishers
    # Blockly
    def highlight_block(self, block_id):
        msg = String()
        msg.data = block_id
        self.__highlight_block_publisher.publish(msg)

    def break_point(self):
        import sys

        msg = Int32()
        msg.data = 1
        self.__break_point_publisher.publish(msg)

        # Close program
        sys.exit()

    # -- Subscribers callbacks

    def __callback_sub_joint_states(self, joint_states):
        self.__joints = list(joint_states.position[:6])

    def __callback_sub_robot_state(self, pose):
        self.__pose = pose

    def __callback_sub_learning_mode(self, learning_mode):
        self.__learning_mode_on = learning_mode.data

    def __callback_sub_current_tool_id(self, msg):
        self.__current_tool_id = msg.data

    def __callback_sub_max_velocity_scaling_factor(self, msg):
        self.__max_velocity_scaling_factor = msg.data

    def __callback_software_version(self, msg):
        self.__software_version = msg

    def __callback_sub_hardware_status(self, hw_status):
        self.__hw_status = hw_status

    def __callback_sub_digital_io_state(self, digital_io_state):
        self.__digital_io_state = digital_io_state

    def __callback_sub_stream_video(self, compressed_image_message):
        self.__compressed_image_message = compressed_image_message

    def __callback_camera_intrinsics(self, camera_info_message):
        self.__camera_intrinsics_message = camera_info_message

    def __callback_sub_conveyors_feedback(self, conveyors_feedback):
        self.__conveyors_feedback = conveyors_feedback

    def __callback_rosout_agg(self, log):
        formatted_log = '[{}] [{}.{}]: {} - {}'.format(
            NiryoRosWrapper.LOGS_LEVELS[log.level],
            log.header.stamp.secs,
            log.header.stamp.nsecs,
            log.name,
            log.msg,
        )
        self.__logs.append(formatted_log)

    def __callback_highlight_block(self, block):
        self.__highlighted_block = block.data

    def __callback_save_current_position(self, _res):
        self.__save_current_position_event.set()

    # -- Service & Action executors
    def __call_service(self, service_name, service_msg_type, *args):
        """
        Wait for the service called service_name
        Then call the service with args

        :param service_name:
        :param service_msg_type:
        :param args: Tuple of arguments
        :raises NiryoRosWrapperException: Timeout during waiting of services
        :return: Response
        """
        # Connect to service
        try:
            rospy.wait_for_service(service_name, self.__service_timeout)
        except rospy.ROSException as e:
            raise NiryoRosWrapperException(e)

        # Call service
        try:
            service = rospy.ServiceProxy(service_name, service_msg_type)
            response = service(*args)
            return response
        except rospy.ServiceException as e:
            raise NiryoRosWrapperException(e)

    def __execute_robot_move_action(self, goal):
        # Connect to server
        if not self.__robot_action_server_client.wait_for_server(rospy.Duration(self.__action_connection_timeout)):
            rospy.logwarn("ROS Wrapper - Failed to connect to Robot action server")

            raise NiryoRosWrapperException('Action Server is not up : {}'.format(self.__robot_action_server_name))
        # Send goal and check response
        goal_state, response = self.__send_goal_and_wait_for_completed(goal)

        if response.status == CommandStatus.GOAL_STILL_ACTIVE:
            rospy.loginfo("ROS Wrapper - Command still active: try to stop it")
            self.__robot_action_server_client.cancel_goal()
            self.__robot_action_server_client.stop_tracking_goal()
            rospy.sleep(0.2)
            rospy.loginfo("ROS Wrapper - Trying to resend command ...")
            goal_state, response = self.__send_goal_and_wait_for_completed(goal)

        if goal_state != GoalStatus.SUCCEEDED:
            self.__robot_action_server_client.stop_tracking_goal()

        if goal_state == GoalStatus.REJECTED:
            raise NiryoRosWrapperException('Goal has been rejected : {}'.format(response.message))
        elif goal_state == GoalStatus.ABORTED:
            raise NiryoRosWrapperException('Goal has been aborted : {}'.format(response.message))
        elif goal_state != GoalStatus.SUCCEEDED:
            raise NiryoRosWrapperException('Error when processing goal : {}'.format(response.message))

        return response.status, response.message

    def __send_goal_and_wait_for_completed(self, goal):
        self.__robot_action_server_client.send_goal(goal)
        if not self.__robot_action_server_client.wait_for_result(timeout=rospy.Duration(self.__action_execute_timeout)):
            self.__robot_action_server_client.cancel_goal()
            self.__robot_action_server_client.stop_tracking_goal()
            raise NiryoRosWrapperException('Action Server timeout : {}'.format(self.__robot_action_server_name))

        goal_state = self.__robot_action_server_client.get_state()
        response = self.__robot_action_server_client.get_result()

        return goal_state, response

    # test pour separer tool package de arm package
    def __execute_tool_action(self, goal):
        # Connect to server
        if not self.__tool_action_server_client.wait_for_server(rospy.Duration(self.__action_connection_timeout)):
            rospy.logwarn("ROS Wrapper - Failed to connect to Tool action server")

            raise NiryoRosWrapperException('Action Server is not up : {}'.format(self.__tool_action_server_name))
        # Send goal and check response
        goal_state, response = self.__send_tool_goal_and_wait_for_completed(goal)

        if response.status == CommandStatus.GOAL_STILL_ACTIVE:
            rospy.loginfo("ROS Wrapper - Command still active: try to stop it")
            self.__tool_action_server_client.cancel_goal()
            self.__tool_action_server_client.stop_tracking_goal()
            rospy.sleep(0.2)
            rospy.loginfo("ROS Wrapper - Trying to resend command ...")
            goal_state, response = self.__send_tool_goal_and_wait_for_completed(goal)

        if goal_state != GoalStatus.SUCCEEDED:
            self.__tool_action_server_client.stop_tracking_goal()

        if goal_state == GoalStatus.REJECTED:
            raise NiryoRosWrapperException('Goal has been rejected : {}'.format(response.message))
        elif goal_state == GoalStatus.ABORTED:
            raise NiryoRosWrapperException('Goal has been aborted : {}'.format(response.message))
        elif goal_state != GoalStatus.SUCCEEDED:
            raise NiryoRosWrapperException('Error when processing goal : {}'.format(response.message))

        return response.status, response.message

    # test send goal to tool action server
    def __send_tool_goal_and_wait_for_completed(self, goal):
        self.__tool_action_server_client.send_goal(goal)
        if not self.__tool_action_server_client.wait_for_result(timeout=rospy.Duration(self.__action_execute_timeout)):
            self.__tool_action_server_client.cancel_goal()
            self.__tool_action_server_client.stop_tracking_goal()
            raise NiryoRosWrapperException('Action Server timeout : {}'.format(self.__robot_action_server_name))

        goal_state = self.__tool_action_server_client.get_state()
        response = self.__tool_action_server_client.get_result()

        return goal_state, response

    # --- Functions interface
    def __classic_return_w_check(self, result):
        self.__check_result_status(result)
        return result.status, result.message

    @staticmethod
    def __check_result_status(result):
        if result.status < 0:
            raise NiryoRosWrapperException("Error Code : {}\nMessage : {}".format(result.status, result.message))

    @staticmethod
    def return_success(message=""):
        return CommandStatus.SUCCESS, message

    # -- Main Purpose
    def request_new_calibration(self):
        """
        Call service to set the request calibration value. If failed, raise NiryoRosWrapperException.

        :return: status, message
        :rtype: (int, str)
        """
        try:
            return self.__call_service('/niryo_robot/joints_interface/request_new_calibration', Trigger)
        except rospy.ROSException as e:
            raise NiryoRosWrapperException(str(e))

    def calibrate_auto(self):
        """
        Call service to calibrate motors then wait for its end. If failed, raise NiryoRosWrapperException

        :return: status, message
        :rtype: (int, str)
        """
        return self.__calibrate(calib_type_int=1)

    def calibrate_manual(self):
        """
        Call service to calibrate motors then wait for its end. If failed, raise NiryoRosWrapperException

        :return: status, message
        :rtype: (int, str)
        """
        return self.__calibrate(calib_type_int=2)

    def __calibrate(self, calib_type_int):
        """
        Call service to calibrate motors then wait for its end. If failed, raise NiryoRosWrapperException

        :param calib_type_int: 1 for auto-calibration & 2 for manual calibration
        :return: status, message
        :rtype: (int, str)
        """
        hw_status = rospy.wait_for_message('/niryo_robot_hardware_interface/hardware_status',
                                           HardwareStatus, timeout=5)
        if not hw_status.calibration_needed:
            return self.return_success("Calibration not needed")

        result = self.__call_service('/niryo_robot/joints_interface/calibrate_motors',
                                     SetInt, calib_type_int)
        self.__check_result_status(result)
        # Wait until calibration start
        rospy.sleep(0.2)
        calibration_finished = False
        while not calibration_finished:
            try:
                hw_status = rospy.wait_for_message('/niryo_robot_hardware_interface/hardware_status',
                                                   HardwareStatus, timeout=5)
                if not (hw_status.calibration_needed or hw_status.calibration_in_progress):
                    calibration_finished = True
                else:
                    rospy.sleep(0.1)
            except rospy.ROSException as e:
                raise NiryoRosWrapperException(str(e))
        # Little delay to be sure calibration is over
        rospy.sleep(0.5)
        return result.status, result.message

    def get_learning_mode(self):
        """
        Use /niryo_robot/learning_mode/state topic subscriber to get learning mode status

        :return: ``True`` if activate else ``False``
        :rtype: bool
        """
        timeout = rospy.get_time() + 2.0
        while self.__learning_mode_on is None:
            rospy.sleep(0.05)
            if rospy.get_time() > timeout:
                raise NiryoRosWrapperException('Timeout: could not get learning mode '
                                               '(/niryo_robot/learning_mode/state topic)')
        return self.__learning_mode_on

    def set_learning_mode(self, set_bool):
        """
        Call service to set_learning_mode according to set_bool. If failed, raise NiryoRosWrapperException

        :param set_bool: ``True`` to activate, ``False`` to deactivate
        :type set_bool: bool
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot/learning_mode/activate',
                                     SetBool, set_bool)
        rospy.sleep(0.1)
        return self.__classic_return_w_check(result)

    def get_max_velocity_scaling_factor(self):
        """
        Get the max velocity scaling factor
        :return: max velocity scaling factor
        :rtype: float
        """
        return self.__max_velocity_scaling_factor

    def set_arm_max_velocity(self, percentage):
        """
        Set relative max velocity (in %)

        :param percentage: Percentage of max velocity
        :type percentage: int
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_arm_commander/set_max_velocity_scaling_factor',
                                     SetInt, percentage)
        return self.__classic_return_w_check(result)

    # - Useful functions
    @staticmethod
    def wait(time_sleep):
        rospy.sleep(time_sleep)

    # -- Move

    # - Pose

    def get_joints(self):
        """
        Use /joint_states topic to get joints status

        :return: list of joints value
        :rtype: list[float]
        """
        timeout = rospy.get_time() + 2.0
        while self.__joints is None:
            rospy.sleep(0.05)
            if rospy.get_time() > timeout:
                raise NiryoRosWrapperException('Timeout: could not get joints (/joint_states topic)')
        return self.__joints

    def get_pose(self):
        """
        Use /niryo_robot/robot_state topic to get pose status

        :return: RobotState object (position.x/y/z && rpy.roll/pitch/yaw && orientation.x/y/z/w)
        :rtype: RobotState
        """
        timeout = rospy.get_time() + 2.0
        while self.__pose is None:
            rospy.sleep(0.05)
            if rospy.get_time() > timeout:
                raise NiryoRosWrapperException('Timeout: could not get pose (/niryo_robot/robot_state topic)')
        return self.__pose

    def get_pose_as_list(self):
        """
        Use /niryo_robot/robot_state topic to get pose status

        :return: list corresponding to [x, y, z, roll, pitch, yaw]
        :rtype: list[float]
        """
        p = self.get_pose()
        return [p.position.x, p.position.y, p.position.z, p.rpy.roll, p.rpy.pitch, p.rpy.yaw]

    def move_joints(self, j1, j2, j3, j4, j5, j6):
        """
        Execute Move joints action

        :param j1:
        :type j1: float
        :param j2:
        :type j2: float
        :param j3:
        :type j3: float
        :param j4:
        :type j4: float
        :param j5:
        :type j5: float
        :param j6:
        :type j6: float
        :return: status, message
        :rtype: (int, str)
        """
        goal = RobotMoveGoal()
        goal.cmd.cmd_type = ArmMoveCommand.JOINTS
        goal.cmd.joints = [j1, j2, j3, j4, j5, j6]
        return self.__execute_robot_move_action(goal)

    def move_to_sleep_pose(self):
        """
        Move to Sleep pose which allows the user to activate the learning mode without the risk
        of the robot hitting something because of gravity

        :return: status, message
        :rtype: (int, str)
        """
        return self.move_joints(0.0, 0.50, -1.25, 0.0, 0.0, 0.0)

    def move_pose(self, x, y, z, roll, pitch, yaw):
        """
        Move robot end effector pose to a (x, y, z, roll, pitch, yaw) pose.

        :param x:
        :type x: float
        :param y:
        :type y: float
        :param z:
        :type z: float
        :param roll:
        :type roll: float
        :param pitch:
        :type pitch: float
        :param yaw:
        :type yaw: float
        :return: status, message
        :rtype: (int, str)
        """

        return self.__move_pose_with_cmd(ArmMoveCommand.POSE, x, y, z, roll, pitch, yaw)

    def move_pose_saved(self, pose_name):
        """
        Move robot end effector pose to a pose saved

        :param pose_name:
        :type pose_name: str
        :return: status, message
        :rtype: (int, str)
        """
        x, y, z, roll, pitch, yaw = self.get_pose_saved(pose_name)
        return self.__move_pose_with_cmd(ArmMoveCommand.POSE, x, y, z, roll, pitch, yaw)

    def __move_pose_with_cmd(self, cmd_type, *pose):
        """
        Execute Move pose action

        :param cmd_type: Command Type
        :type cmd_type: ArmMoveCommand -> POSE, LINEAR_POSE
        :param pose: tuple corresponding to x, y, z, roll, pitch, yaw
        :return: status, message
        :rtype: (int, str)
        """
        x, y, z, roll, pitch, yaw = pose
        goal = RobotMoveGoal()
        goal.cmd.cmd_type = cmd_type
        goal.cmd.position.x = x
        goal.cmd.position.y = y
        goal.cmd.position.z = z
        goal.cmd.rpy.roll = roll
        goal.cmd.rpy.pitch = pitch
        goal.cmd.rpy.yaw = yaw
        return self.__execute_robot_move_action(goal)

    def shift_pose(self, axis, value):
        """
        Execute Shift pose action

        :param axis: Value of RobotAxis enum corresponding to where the shift happens
        :type axis: ShiftPose
        :param value: shift value
        :type value: float
        :return: status, message
        :rtype: (int, str)
        """
        goal = RobotMoveGoal()
        goal.cmd.cmd_type = ArmMoveCommand.SHIFT_POSE
        goal.cmd.shift.axis_number = axis
        goal.cmd.shift.value = value
        return self.__execute_robot_move_action(goal)

    def shift_linear_pose(self, axis, value):
        """
        Execute Shift pose action with a linear trajectory

        :param axis: Value of RobotAxis enum corresponding to where the shift happens
        :type axis: ShiftPose
        :param value: shift value
        :type value: float
        :return: status, message
        :rtype: (int, str)
        """
        goal = RobotMoveGoal()
        goal.cmd.cmd_type = ArmMoveCommand.SHIFT_LINEAR_POSE
        goal.cmd.shift.axis_number = axis
        goal.cmd.shift.value = value
        return self.__execute_robot_move_action(goal)

    def move_linear_pose(self, x, y, z, roll, pitch, yaw):
        """
        Move robot end effector pose to a (x, y, z, roll, pitch, yaw) pose, with a linear trajectory

        :param x:
        :type x: float
        :param y:
        :type y: float
        :param z:
        :type z: float
        :param roll:
        :type roll: float
        :param pitch:
        :type pitch: float
        :param yaw:
        :type yaw: float
        :return: status, message
        :rtype: (int, str)
        """
        return self.__move_pose_with_cmd(ArmMoveCommand.LINEAR_POSE, x, y, z, roll, pitch, yaw)

    def stop_move(self):
        """
        Stop the robot movement

        :return: list of joints value
        :rtype: list[float]
        """
        result = self.__call_service('/niryo_robot_commander/stop_command', Trigger)
        return self.__classic_return_w_check(result)

    def set_jog_use_state(self, state):
        """
        Turn jog controller On or Off

        :param state: ``True`` to turn on, else ``False``
        :type state: bool
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot/jog_interface/enable', SetBool, state)
        return self.__classic_return_w_check(result)

    def jog_joints_shift(self, shift_values):
        """
        Make a Jog on joints position

        :param shift_values: list corresponding to the shift to be applied to each joint
        :type shift_values: list[float]
        :return: status, message
        :rtype: (int, str)
        """
        return self.__jog_shift(JogShiftRequest.JOINTS_SHIFT, shift_values)

    def jog_pose_shift(self, shift_values):
        """
        Make a Jog on end-effector position

        :param shift_values: list corresponding to the shift to be applied to each joint
        :type shift_values: list[float]
        :return: status, message
        :rtype: (int, str)
        """
        return self.__jog_shift(JogShiftRequest.POSE_SHIFT, shift_values)

    def __jog_shift(self, cmd, shift_values):
        result = self.__call_service('/niryo_robot/jog_interface/jog_shift_commander', JogShift, cmd, shift_values)

        return self.__classic_return_w_check(result)

    def forward_kinematics(self, j1, j2, j3, j4, j5, j6):
        """
        Compute forward kinematics

        :param j1:
        :type j1: float
        :param j2:
        :type j2: float
        :param j3:
        :type j3: float
        :param j4:
        :type j4: float
        :param j5:
        :type j5: float
        :param j6:
        :type j6: float
        :return: list corresponding to [x, y, z, roll, pitch, yaw]
        :rtype: list[float]
        """
        joints = [j1, j2, j3, j4, j5, j6]
        result = self.__call_service('/niryo_robot/kinematics/forward', GetFK, joints)
        return self.robot_state_msg_to_list(result.pose)

    def inverse_kinematics(self, x, y, z, roll, pitch, yaw):
        """
        Compute inverse kinematics

        :param x:
        :type x: float
        :param y:
        :type y: float
        :param z:
        :type z: float
        :param roll:
        :type roll: float
        :param pitch:
        :type pitch: float
        :param yaw:
        :type yaw: float
        :return: list of joints value
        :rtype: list[float]
        """
        state = RobotState()
        state.position = Point(x, y, z)
        state.rpy = RPY(roll, pitch, yaw)
        result = self.__call_service('/niryo_robot/kinematics/inverse', GetIK, state)
        if not result.success:
            raise NiryoRosWrapperException("Failed to perform invert kinematic")

        return result.joints

    # - Saved Pose

    def get_pose_saved(self, pose_name):
        """
        Get saved pose from robot intern storage
        Will raise error if position does not exist

        :param pose_name: Pose Name
        :type pose_name: str
        :return: x, y, z, roll, pitch, yaw
        :rtype: tuple[float]
        """
        from niryo_robot_poses_handlers.srv import GetPose

        result = self.__call_service('/niryo_robot_poses_handlers/get_pose', GetPose, pose_name)
        self.__check_result_status(result)
        pose = result.pose
        x, y, z = pose.position.x, pose.position.y, pose.position.z
        roll, pitch, yaw = pose.rpy.roll, pose.rpy.pitch, pose.rpy.yaw
        return x, y, z, roll, pitch, yaw

    def save_pose(self, name, x, y, z, roll, pitch, yaw):
        """
        Save pose in robot's memory

        :param name:
        :type name: str
        :param x:
        :type x: float
        :param y:
        :type y: float
        :param z:
        :type z: float
        :param roll:
        :type roll: float
        :param pitch:
        :type pitch: float
        :param yaw:
        :type yaw: float
        :return: status, message
        :rtype: (int, str)
        """
        from niryo_robot_poses_handlers.srv import ManagePose, ManagePoseRequest

        req = ManagePoseRequest()
        req.cmd = ManagePoseRequest.SAVE
        req.pose.name = name
        req.pose.position = Point(x, y, z)
        req.pose.rpy = RPY(roll, pitch, yaw)

        result = self.__call_service('/niryo_robot_poses_handlers/manage_pose',
                                     ManagePose, req)
        return self.__classic_return_w_check(result)

    def delete_pose(self, name):
        """
        Send delete command to the pose manager service

        :param name:
        :type name: str
        :return: status, message
        :rtype: (int, str)
        """
        from niryo_robot_poses_handlers.srv import ManagePose, ManagePoseRequest

        req = ManagePoseRequest()
        req.cmd = ManagePoseRequest.DELETE
        req.pose.name = name
        result = self.__call_service('/niryo_robot_poses_handlers/manage_pose',
                                     ManagePose, req)
        return self.__classic_return_w_check(result)

    def get_saved_pose_list(self, with_desc=False):
        """
        Ask the pose manager service which positions are available

        :param with_desc: If it returns the poses descriptions
        :type with_desc: bool
        :return: list of positions name
        :rtype: list[str]
        """
        result = self.__call_service('/niryo_robot_poses_handlers/get_pose_list',
                                     GetNameDescriptionList)
        if with_desc:
            return result.name_list, result.description_list

        return result.name_list

    # - Pick/Place

    def pick_from_pose(self, x, y, z, roll, pitch, yaw):
        """
        Execute a picking from a position. If an error happens during the movement, error will be raised.
        A picking is described as :
        - going over the object
        - going down until height = z
        - grasping with tool
        - going back over the object

        :param x:
        :type x: float
        :param y:
        :type y: float
        :param z:
        :type z: float
        :param roll:
        :type roll: float
        :param pitch:
        :type pitch: float
        :param yaw:
        :type yaw: float
        :return: status, message
        :rtype: (int, str)
       """
        self.release_with_tool()

        self.move_pose(x, y, z + 0.05, roll, pitch, yaw)
        self.move_pose(x, y, z, roll, pitch, yaw)

        self.grasp_with_tool()

        return self.move_pose(x, y, z + 0.05, roll, pitch, yaw)

    def place_from_pose(self, x, y, z, roll, pitch, yaw):
        """
        Execute a placing from a position. If an error happens during the movement, error will be raised.
        A placing is described as :
        - going over the place
        - going down until height = z
        - releasing the object with tool
        - going back over the place

        :param x:
        :type x: float
        :param y:
        :type y: float
        :param z:
        :type z: float
        :param roll:
        :type roll: float
        :param pitch:
        :type pitch: float
        :param yaw:
        :type yaw: float
        :return: status, message
        :rtype: (int, str)
        """
        self.move_pose(x, y, z + 0.05, roll, pitch, yaw)
        self.move_pose(x, y, z, roll, pitch, yaw)

        self.release_with_tool()

        return self.move_pose(x, y, z + 0.05, roll, pitch, yaw)

    def pick_and_place(self, pick_pose, place_pose, dist_smoothing=0.0):
        """
        Execute a pick and place. If an error happens during the movement, error will be raised.
        -> Args param is for development purposes

        :param pick_pose:
        :type pick_pose: list[float]
        :param place_pose:
        :type place_pose: list[float]
        :param dist_smoothing: Distance from waypoints before smoothing trajectory
        :type dist_smoothing: float
        :return: status, message
        :rtype: (int, str)
        """
        self.release_with_tool()

        pick_pose_high = self.copy_position_with_offsets(pick_pose, z_offset=0.05)
        place_pose_high = self.copy_position_with_offsets(place_pose, z_offset=0.05)

        self.execute_trajectory_from_poses([pick_pose_high, pick_pose], dist_smoothing)
        self.grasp_with_tool()

        self.execute_trajectory_from_poses([pick_pose_high, place_pose_high, place_pose], dist_smoothing)

        self.release_with_tool()

        return self.move_pose(*place_pose_high)

    # - Trajectories

    def get_trajectory_saved(self, trajectory_name):
        """
        Get saved trajectory from robot intern storage
        Will raise error if position does not exist

        :param trajectory_name:
        :type trajectory_name: str
        :raises NiryoRosWrapperException: If trajectory file doesn't exist
        :return: list of [x, y, z, qx, qy, qz, qw]
        :rtype: list[list[float]]
        """
        from niryo_robot_poses_handlers.srv import GetTrajectory

        result = self.__call_service('/niryo_robot_poses_handlers/get_trajectory',
                                     GetTrajectory, trajectory_name)
        self.__check_result_status(result)
        list_poses = result.list_poses
        list_poses_raw = []
        for p in list_poses:
            pose_raw = [p.position.x, p.position.y, p.position.z,
                        p.orientation.x, p.orientation.y, p.orientation.z, p.orientation.w]
            list_poses_raw.append(pose_raw)
        return list_poses_raw

    def execute_trajectory_saved(self, trajectory_name):
        """
        Execute trajectory saved in Robot internal storage

        :param trajectory_name:
        :type trajectory_name: str
        :return: status, message
        :rtype: (int, str)
        """
        from niryo_robot_poses_handlers.srv import GetTrajectory

        result = self.__call_service('/niryo_robot_poses_handlers/get_trajectory',
                                     GetTrajectory, trajectory_name)
        list_poses = result.list_poses
        return self.__execute_trajectory_from_formatted_poses(list_poses)

    def execute_trajectory_from_poses(self, list_poses_raw, dist_smoothing=0.0):
        """
        Execute trajectory from a list of pose

        :param list_poses_raw: list of [x, y, z, qx, qy, qz, qw] or list of [x, y, z, roll, pitch, yaw]
        :type list_poses_raw: list[list[float]]
        :param dist_smoothing: Distance from waypoints before smoothing trajectory
        :type dist_smoothing: float
        :return: status, message
        :rtype: (int, str)
        """
        from niryo_robot_poses_handlers.transform_functions import quaternion_from_euler

        if len(list_poses_raw) < 2:
            return "Give me at least 2 points"
        list_poses = []
        for pose in list_poses_raw:
            point = Point(*pose[:3])
            angle = pose[3:]
            if len(angle) == 3:
                quaternion = quaternion_from_euler(*angle)
            else:
                quaternion = angle
            orientation = Quaternion(*quaternion)
            list_poses.append(Pose(point, orientation))
        return self.__execute_trajectory_from_formatted_poses(list_poses, dist_smoothing)

    def execute_trajectory_from_poses_and_joints(self, list_pose_joints, list_type=None, dist_smoothing=0.0):
        """
        Execute trajectory from list of poses and joints

        :param list_pose_joints: List of [x,y,z,qx,qy,qz,qw] or list of [x,y,z,roll,pitch,yaw] or a list of [j1,j2,j3,j4,j5,j6]
        :type list_pose_joints: list[list[float]]
        :param list_type: List of string 'pose' or 'joint', or ['pose'] (if poses only) or ['joint'] (if joints only).
                    If None, it is assumed there are only poses in the list.
        :type list_type: list[string]
        :param dist_smoothing: Distance from waypoints before smoothing trajectory
        :type dist_smoothing: float
        :return: status, message
        :rtype: (int, str)
        """
        if list_type is None:
            list_type = ['pose']
        list_pose_waypoints = []

        if len(list_type) == 1:  # only one type of object
            if list_type[0] == "pose":  # every elem in list is a pose
                list_pose_waypoints = list_pose_joints
            elif list_type[0] == "joint":  # every elem in list is a joint
                list_pose_waypoints = [self.forward_kinematics(*joint) for joint in list_pose_joints]

            else:
                raise NiryoRosWrapperException(
                    'Execute trajectory from poses and joints - Wrong list_type argument : got '
                    + list_type[0] + ", expected 'pose' or 'joint'")

        elif len(list_type) == len(list_pose_joints):
            # convert every joints to poses
            for target, type_ in zip(list_pose_joints, list_type):
                if type_ == 'joint':
                    pose_from_joint = self.forward_kinematics(*target)
                    list_pose_waypoints.append(pose_from_joint)
                elif type_ == 'pose':
                    list_pose_waypoints.append(target)
                else:
                    raise NiryoRosWrapperException(
                        'Execute trajectory from poses and joints - Wrong list_type argument at index ' + str(i) +
                        ' got ' + type_ + ", expected 'pose' or 'joint'")

        else:
            raise NiryoRosWrapperException(
                'Execute trajectory from poses and joints - List of waypoints (size ' + str(len(list_pose_joints)) +
                ') and list of type (size ' + str(len(list_type)) + ') must be the same size.')

        return self.execute_trajectory_from_poses(list_pose_waypoints, dist_smoothing)

    def save_trajectory(self, trajectory_name, list_poses_raw):
        """
        Save trajectory object and send it to the trajectory manager service

        :param trajectory_name: name which will have the trajectory
        :type trajectory_name: str
        :param list_poses_raw: list of [x, y, z, qx, qy, qz, qw] or list of [x, y, z, roll, pitch, yaw]
        :type list_poses_raw: list[list[float]]
        :return: status, message
        :rtype: (int, str)
        """
        from niryo_robot_poses_handlers.srv import ManageTrajectory, ManageTrajectoryRequest
        from niryo_robot_poses_handlers.transform_functions import quaternion_from_euler

        req = ManageTrajectoryRequest()
        req.cmd = ManageTrajectoryRequest.SAVE
        req.name = trajectory_name
        list_poses = []
        for pose in list_poses_raw:
            angle = pose[3:]
            if len(angle) == 3:
                quaternion = quaternion_from_euler(*angle)
            else:
                quaternion = angle
            orientation = Quaternion(*quaternion)
            list_poses.append(Pose(Point(*pose[:3]), orientation))
        req.poses = list_poses

        result = self.__call_service('/niryo_robot_poses_handlers/manage_trajectory',
                                     ManageTrajectory, req)
        return self.__classic_return_w_check(result)

    def draw_spiral(self, radius, angle_step, total_steps):
        """
        Call robot action service to draw a spiral trajectory

        :param radius: maximum distance between the spiral and the starting point
        :param angle_step: rotation between each waypoint creation
        :param total_steps: number of waypoints from the beginning to the end of the spiral
        :return: status, message
        :rtype: (int, str)
        """
        goal = RobotMoveGoal()
        goal.cmd.cmd_type = ArmMoveCommand.DRAW_SPIRAL
        goal.cmd.args = [str(radius), str(angle_step), str(total_steps)]
        return self.__execute_robot_move_action(goal)

    def __execute_trajectory_from_formatted_poses(self, list_poses, dist_smoothing=0.0):

        goal = RobotMoveGoal()
        goal.cmd.cmd_type = ArmMoveCommand.EXECUTE_TRAJ
        goal.cmd.list_poses = list_poses
        goal.cmd.dist_smoothing = dist_smoothing
        return self.__execute_robot_move_action(goal)

    def delete_trajectory(self, trajectory_name):
        """
        Send delete command to the trajectory manager service

        :param trajectory_name: name
        :type trajectory_name: str
        :return: status, message
        :rtype: (int, str)
        """
        from niryo_robot_poses_handlers.srv import ManageTrajectory, ManageTrajectoryRequest
        req = ManageTrajectoryRequest()
        req.cmd = ManageTrajectoryRequest.DELETE
        req.name = trajectory_name
        result = self.__call_service('/niryo_robot_poses_handlers/manage_trajectory',
                                     ManageTrajectory, req)
        return self.__classic_return_w_check(result)

    def get_saved_trajectory_list(self):
        """
        Ask the pose trajectory service which trajectories are available

        :return: list of trajectory name
        :rtype: list[str]
        """
        result = self.__call_service('/niryo_robot_poses_handlers/get_trajectory_list',
                                     GetNameDescriptionList)
        return result.name_list

    # - Useful Pose functions

    @staticmethod
    def copy_position_with_offsets(copied_pose, x_offset=0.0, y_offset=0.0, z_offset=0.0):
        """
        Copy a position and add offset to some coordinates
        """
        new_pose = [v for v in copied_pose]  # Copying all values
        new_pose[0] += x_offset  # adjust x
        new_pose[1] += y_offset  # adjust y
        new_pose[2] += z_offset  # adjust z
        return new_pose

    @staticmethod
    def list_to_robot_state_msg(x, y, z, roll, pitch, yaw):
        """
        Translate (x, y, z, roll, pitch, yaw) to a RobotState Object
        """
        r = RobotState()
        r.position.x = x
        r.position.y = y
        r.position.z = z
        r.rpy.roll = roll
        r.rpy.pitch = pitch
        r.rpy.yaw = yaw
        return r

    @staticmethod
    def robot_state_msg_to_list(robot_state):
        """
        Translate a RobotState Object to (x, y, z, roll, pitch, yaw)
        """
        return (robot_state.position.x, robot_state.position.y, robot_state.position.z,
                robot_state.rpy.roll, robot_state.rpy.pitch, robot_state.rpy.yaw)

    # -- Tools

    def get_current_tool_id(self):
        """
        Use /niryo_robot_hardware/tools/current_id  topic to get current tool id

        :return: Tool Id
        :rtype: ToolID
        """
        timeout = rospy.get_time() + 2.0
        while self.__current_tool_id is None:
            rospy.sleep(0.05)
            if rospy.get_time() > timeout:
                raise NiryoRosWrapperException(
                    'Timeout: could not get current tool id (/niryo_robot_tools_commander/current_id topic)')
        return self.__current_tool_id

    def update_tool(self):
        """
        Call service niryo_robot_tools_commander/update_tool to update tool

        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_tools_commander/update_tool', Trigger)
        return self.__classic_return_w_check(result)

    def grasp_with_tool(self, pin_id=-1):
        """
        Grasp with the tool linked to tool_id.
        This action correspond to
        - Close gripper for Grippers
        - Pull Air for Vacuum pump
        - Activate for Electromagnet

        :param pin_id: [Only required for electromagnet] Pin ID of the electromagnet
        :type pin_id: PinID
        :return: status, message
        :rtype: (int, str)
        """
        tool_id = self.get_current_tool_id()

        if tool_id in (ToolID.GRIPPER_1, ToolID.GRIPPER_2, ToolID.GRIPPER_3, ToolID.GRIPPER_4):
            return self.close_gripper()
        elif tool_id == ToolID.VACUUM_PUMP_1:
            return self.pull_air_vacuum_pump()
        elif tool_id == ToolID.ELECTROMAGNET_1:
            return self.activate_electromagnet(pin_id)

    def release_with_tool(self, pin_id=-1):
        """
        Release with the tool associated to tool_id.
        This action correspond to
        - Open gripper for Grippers
        - Push Air for Vacuum pump
        - Deactivate for Electromagnet

        :param pin_id: [Only required for electromagnet] Pin ID of the electromagnet
        :type pin_id: PinID
        :return: status, message
        :rtype: (int, str)
        """
        tool_id = self.get_current_tool_id()

        if tool_id in (ToolID.GRIPPER_1, ToolID.GRIPPER_2, ToolID.GRIPPER_3, ToolID.GRIPPER_4):
            return self.open_gripper()
        elif tool_id == ToolID.VACUUM_PUMP_1:
            return self.push_air_vacuum_pump()
        elif tool_id == ToolID.ELECTROMAGNET_1:
            return self.deactivate_electromagnet(pin_id)

    # - Gripper
    def open_gripper(self, speed=500):
        """
        Open gripper with a speed 'speed'

        :param speed: Default -> 500
        :type speed: int
        :return: status, message
        :rtype: (int, str)
        """
        return self.__deal_with_gripper(speed, ToolCommand.OPEN_GRIPPER)

    def close_gripper(self, speed=500):
        """
        Close gripper with a speed 'speed'

        :param speed: Default -> 500
        :type speed: int
        :return: status, message
        :rtype: (int, str)
        """
        return self.__deal_with_gripper(speed, ToolCommand.CLOSE_GRIPPER)

    def __deal_with_gripper(self, speed, command_int):
        goal = ToolActionGoal()
        goal.goal.cmd.tool_id = self.get_current_tool_id()
        goal.goal.cmd.cmd_type = command_int
        if command_int == ToolCommand.OPEN_GRIPPER:
            goal.goal.cmd.gripper_open_speed = speed
        else:
            goal.goal.cmd.gripper_close_speed = speed
        return self.__execute_tool_action(goal.goal)

    # - Vacuum
    def pull_air_vacuum_pump(self):
        """
        Pull air

        :return: status, message
        :rtype: (int, str)
        """
        return self.__deal_with_vacuum_pump(ToolCommand.PULL_AIR_VACUUM_PUMP)

    def push_air_vacuum_pump(self):
        """
        Pull air

        :return: status, message
        :rtype: (int, str)
        """
        return self.__deal_with_vacuum_pump(ToolCommand.PUSH_AIR_VACUUM_PUMP)

    def __deal_with_vacuum_pump(self, command_int):
        goal = ToolActionGoal()
        goal.goal.cmd.tool_id = ToolID.VACUUM_PUMP_1
        goal.goal.cmd.cmd_type = command_int

        return self.__execute_tool_action(goal.goal)

    # - Electromagnet
    def setup_electromagnet(self, pin_id):
        """
        Setup electromagnet on pin

        :param pin_id: Pin ID
        :type pin_id:  PinID
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_tools_commander/equip_electromagnet',
                                     SetInt, ToolID.ELECTROMAGNET_1)

        if result.status != CommandStatus.SUCCESS:
            return result.status, result.message

        return self.__deal_with_electromagnet(pin_id, ToolCommand.SETUP_DIGITAL_IO)

    def activate_electromagnet(self, pin_id):
        """
        Activate electromagnet associated to electromagnet_id on pin_id

        :param pin_id: Pin ID
        :type pin_id:  PinID
        :return: status, message
        :rtype: (int, str)
        """
        return self.__deal_with_electromagnet(pin_id, ToolCommand.ACTIVATE_DIGITAL_IO)

    def deactivate_electromagnet(self, pin_id):
        """
        Deactivate electromagnet associated to electromagnet_id on pin_id

        :param pin_id: Pin ID
        :type pin_id:  PinID
        :return: status, message
        :rtype: (int, str)
        """
        return self.__deal_with_electromagnet(pin_id, ToolCommand.DEACTIVATE_DIGITAL_IO)

    def __deal_with_electromagnet(self, pin, command_int):
        goal = ToolActionGoal()
        goal.goal.cmd.tool_id = ToolID.ELECTROMAGNET_1
        goal.goal.cmd.cmd_type = command_int
        goal.goal.cmd.gpio = pin
        return self.__execute_tool_action(goal.goal)

    # - TCP
    def enable_tcp(self, enable=True):
        """
        Enables or disables the TCP function (Tool Center Point).
        If activation is requested, the last recorded TCP value will be applied.
        The default value depends on the gripper equipped.
        If deactivation is requested, the TCP will be coincident with the tool_link.

        :param enable: True to enable, False otherwise.
        :type enable: Bool
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_tools_commander/enable_tcp',
                                     SetBool, enable)
        return self.__classic_return_w_check(result)

    def set_tcp(self, x, y, z, roll, pitch, yaw):
        """
        Activates the TCP function (Tool Center Point)
        and defines the transformation between the tool_link frame and the TCP frame.

        :param x:
        :type x: float
        :param y:
        :type y: float
        :param z:
        :type z: float
        :param roll:
        :type roll: float
        :param pitch:
        :type pitch: float
        :param yaw:
        :type yaw: float
        :return: status, message
        :rtype: (int, str)
        """
        req = SetTCPRequest()
        req.position = Point(x, y, z)
        req.rpy = RPY(roll, pitch, yaw)

        result = self.__call_service('/niryo_robot_tools_commander/set_tcp',
                                     SetTCP, req)
        return self.__classic_return_w_check(result)

    def reset_tcp(self):
        """
        Reset the TCP (Tool Center Point) transformation.
        The PCO will be reset according to the tool equipped.

        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_tools_commander/reset_tcp',
                                     Trigger)
        return self.__classic_return_w_check(result)

    def tool_reboot(self):
        """
        Reboot the motor of the tool equipped. Useful when an Overload error occurs. (cf HardwareStatus)

        :return: success, message
        :rtype: (bool, str)
        """
        result = self.__call_service('/niryo_robot/tools/reboot',
                                     StdTrigger)

        return result.success, result.message

    # - Hardware

    def get_simulation_mode(self):
        """
        The simulation mode
        """
        return self.__simulation_mode

    def set_pin_mode(self, pin_id, pin_mode):
        """
        Set pin number pin_id to mode pin_mode

        :param pin_id:
        :type pin_id: PinID
        :param pin_mode:
        :type pin_mode: PinMode
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_rpi/set_digital_io_mode',
                                     SetDigitalIO, pin_id, pin_mode)
        return self.__classic_return_w_check(result)

    def digital_write(self, pin_id, digital_state):
        """
        Set pin_id state to pin_state

        :param pin_id:
        :type pin_id: PinID
        :param digital_state:
        :type digital_state: PinState
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_rpi/set_digital_io_state',
                                     SetDigitalIO, pin_id, digital_state)
        return self.__classic_return_w_check(result)

    def digital_read(self, pin_id):
        """
        Read pin number pin_id and return its state

        :param pin_id:
        :type pin_id: PinID
        :return: state
        :rtype: PinState
        """
        result = self.__call_service('/niryo_robot_rpi/get_digital_io',
                                     GetDigitalIO, pin_id)
        self.__check_result_status(result)

        if result.state == 0:
            return PinState.LOW
        else:
            return PinState.HIGH

    def get_hardware_version(self):
        """
        Get the robot hardware version
        """
        return self.__hardware_version

    def get_can_enabled(self):
        """
        Get can_enabled
        """
        return self.__can_enabled

    def get_dxl_enabled(self):
        """
        Get dxl_enabled
        """
        return self.__dxl_enabled

    def get_hardware_status(self):
        """
        Get hardware status : Temperature, Hardware version, motors names & types ...

        :return: Infos contains in a HardwareStatus object (see niryo_robot_msgs)
        :rtype: HardwareStatus
        """
        timeout = rospy.get_time() + 2.0
        while self.__hw_status is None:
            rospy.sleep(0.05)
            if rospy.get_time() > timeout:
                raise NiryoRosWrapperException(
                    'Timeout: could not get hardware status (/niryo_robot_hardware_interface/hardware_status topic)')
        return self.__hw_status

    def get_digital_io_state(self):
        """
        Get Digital IO state : Names, modes, states

        :return: Infos contains in a DigitalIOState object (see niryo_robot_msgs)
        :rtype: DigitalIOState
        """
        timeout = rospy.get_time() + 2.0
        while self.__digital_io_state is None:
            rospy.sleep(0.05)
            if rospy.get_time() > timeout:
                raise NiryoRosWrapperException(
                    'Timeout: could not get digital io state (/niryo_robot_rpi/digital_io_state topic)')
        return self.__digital_io_state

    def get_axis_limits(self):
        """
        Return the joints and positions min and max values

        :return: An object containing all the values
        :rtype: dict
        """
        path_pattern = '/niryo_robot/robot_command_validation/{}/{}/{}'
        axis_limits = {
            'joint_limits': {
                'joint_1': None,
                'joint_2': None,
                'joint_3': None,
                'joint_4': None,
                'joint_5': None,
                'joint_6': None,
            },
            'position_limits': {
                'x': None,
                'y': None,
                'z': None,
            },
            'rpy_limits': {
                'roll': None,
                'pitch': None,
                'yaw': None,
            }
        }

        for axis_group in axis_limits:
            for axis in axis_limits[axis_group]:
                try:
                    limits = {
                        'min': rospy.get_param(path_pattern.format(axis_group, axis, 'min')),
                        'max': rospy.get_param(path_pattern.format(axis_group, axis, 'max')),
                    }
                except KeyError as e:
                    return False, str(e)

                axis_limits[axis_group][axis] = limits
        return True, axis_limits

    def reboot_motors(self):
        """
        Reboot the robots motors

        :raises NiryoRosWrapperException:
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_hardware_interface/reboot_motors', Trigger)
        return self.__classic_return_w_check(result)

    def debug_motors(self):
        """
        Debug the motors by going to each stop

        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_commander/motor_debug_start', SetInt, 0)
        return self.__classic_return_w_check(result)


    # - Button

    def __change_button_mode(self, mode):
        result = self.__call_service('/niryo_robot/rpi/change_button_mode', SetInt, mode)
        return self.__classic_return_w_check(result)

    def set_button_do_nothing(self):
        """
        Disable the button
        :return: status, message
        :rtype: (int, str)
        """
        return self.__change_button_mode(0)

    def set_button_trigger_sequence_autorun(self):
        """
        Set the button in trigger sequence autorun mode
        :return: status, message
        :rtype: (int, str)
        """
        return self.__change_button_mode(1)

    def set_button_blockly_save_point(self):
        """
        Set the button in blockly save point mode
        :return: status, message
        :rtype: (int, str)
        """
        return self.__change_button_mode(2)


    # - Software

    def get_software_version(self):
        """
        Get the robot software version

        :return: rpi_image_version, ros_niryo_robot_version, motor_names, stepper_firmware_versions
        :rtype: (str, str, list[str], list[str])
        """
        return self.__software_version

    def set_robot_name(self, name):
        """
        Set the robot name

        :param name: the new name of the robot
        :type name: str
        :return: status, message
        :rtype: int, str
        """
        req = SetString()
        req.data = name
        result = self.__call_service('/niryo_robot/wifi/set_robot_name', SetString, req)
        return self.__classic_return_w_check(result)

    def __call_shutdown_rpi(self, value):
        result = self.__call_service('/niryo_robot_rpi/shutdown_rpi', SetInt, value)
        return self.__classic_return_w_check(result)

    def shutdown_rpi(self):
        """
        Shutdown the rpi
        :return: status, message
        :rtype: (int, str)
        """
        return self.__call_shutdown_rpi(1)

    def reboot_rpi(self):
        """
        Shutdown the rpi
        :return: status, message
        :rtype: (int, str)
        """
        return self.__call_shutdown_rpi(2)


    # - Conveyor

    def set_conveyor(self):
        """
        Scan for conveyor on can bus. If conveyor detected, return the conveyor ID

        :raises NiryoRosWrapperException:
        :return: ID
        :rtype: ConveyorID
        """
        req = SetConveyorRequest()
        req.cmd = SetConveyorRequest.ADD
        result = self.__call_service('/niryo_robot/conveyor/ping_and_set_conveyor',
                                     SetConveyor, req)

        # If no new conveyor is detected, it should not crash
        if result.status in [CommandStatus.NO_CONVEYOR_LEFT, CommandStatus.NO_CONVEYOR_FOUND]:
            rospy.logwarn_throttle(1, 'ROS Wrapper - No new conveyor found')
        else:
            self.__check_result_status(result)
        return result.id

    def unset_conveyor(self, conveyor_id):
        """
        Remove specific conveyor.

        :param conveyor_id: Basically, ConveyorID.ONE or ConveyorID.TWO
        :type conveyor_id: ConveyorID
        :raises NiryoRosWrapperException:
        :return: status, message
        :rtype: (int, str)
        """
        req = SetConveyorRequest()
        req.cmd = SetConveyorRequest.REMOVE
        req.id = conveyor_id
        result = self.__call_service('/niryo_robot/conveyor/ping_and_set_conveyor',
                                     SetConveyor, req)
        return self.__classic_return_w_check(result)

    def control_conveyor(self, conveyor_id, bool_control_on, speed, direction):
        """
        Control conveyor associated to conveyor_id.
        Then stops it if bool_control_on is False, else refreshes it speed and direction

        :param conveyor_id: ConveyorID.ID_1 or ConveyorID.ID_2
        :type conveyor_id: ConveyorID
        :param bool_control_on: True for activate, False for deactivate
        :type bool_control_on: bool
        :param speed: target speed
        :type speed: int
        :param direction: Target direction
        :type direction: ConveyorDirection
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot/conveyor/control_conveyor',
                                     ControlConveyor, conveyor_id, bool_control_on, speed, direction)
        return self.__classic_return_w_check(result)

    def get_conveyors_feedback(self):
        """
        Give conveyors feedback

        :return: List[ID, connection_state, running, speed, direction]
        :rtype: List(int, bool, bool, int, int)
        """
        timeout = rospy.get_time() + 2.0
        while self.__conveyors_feedback is None:
            rospy.sleep(0.05)
            if rospy.get_time() > timeout:
                raise NiryoRosWrapperException(
                    'Timeout: could not get conveyor 1 feedback (/niryo_robot/conveyor/feedback topic)')
        fb = self.__conveyors_feedback
        return fb.conveyors

    # - Vision

    def get_compressed_image(self, with_seq=False):
        """
        Get last stream image in a compressed format

        :return: string containing a JPEG compressed image
        :rtype: str
        """
        timeout = rospy.get_time() + 2.0
        while self.__compressed_image_message is None:
            rospy.sleep(0.05)
            if rospy.get_time() > timeout:
                raise NiryoRosWrapperException(
                    'Timeout: could not video stream message (/niryo_robot_vision/compressed_video_stream topic)')

        if with_seq:
            return self.__compressed_image_message.data, self.__compressed_image_message.header.seq

        return self.__compressed_image_message.data

    def set_brightness(self, brightness_factor):
        """
        Modify image brightness

        :param brightness_factor: How much to adjust the brightness. 0.5 will
            give a darkened image, 1 will give the original image while
            2 will enhance the brightness by a factor of 2.
        :type brightness_factor: float
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_vision/set_brightness', SetImageParameter, brightness_factor)
        return self.__classic_return_w_check(result)

    def set_contrast(self, contrast_factor):
        """
        Modify image contrast

        :param contrast_factor: While a factor of 1 gives original image.
            Making the factor towards 0 makes the image greyer, while factor>1 increases the contrast of the image.
        :type contrast_factor: float
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_vision/set_contrast', SetImageParameter, contrast_factor)
        return self.__classic_return_w_check(result)

    def set_saturation(self, saturation_factor):
        """
        Modify image saturation

        :param saturation_factor: How much to adjust the saturation. 0 will
            give a black and white image, 1 will give the original image while
            2 will enhance the saturation by a factor of 2.
        :type saturation_factor: float
        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_vision/set_saturation', SetImageParameter, saturation_factor)
        return self.__classic_return_w_check(result)

    @staticmethod
    def get_image_parameters():
        """
        Get last stream image parameters: Brightness factor, Contrast factor, Saturation factor.

        Brightness factor: How much to adjust the brightness. 0.5 will give a darkened image,
        1 will give the original image while 2 will enhance the brightness by a factor of 2.

        Contrast factor: While a factor of 1 gives original image.
        Making the factor towards 0 makes the image greyer, while factor>1 increases the contrast of the image.

        Saturation factor: 0 will give a black and white image, 1 will give the original image while
        2 will enhance the saturation by a factor of 2.

        :return:  Brightness factor, Contrast factor, Saturation factor
        :rtype: float, float, float
        """
        from niryo_robot_vision.msg import ImageParameters

        img_param_msg = rospy.wait_for_message('/niryo_robot_vision/video_stream_parameters', ImageParameters,
                                               timeout=5)
        return img_param_msg.brightness_factor, img_param_msg.contrast_factor, img_param_msg.saturation_factor

    def get_target_pose_from_rel(self, workspace_name, height_offset, x_rel, y_rel, yaw_rel):
        """
        Given a pose (x_rel, y_rel, yaw_rel) relative to a workspace, this function
        returns the robot pose in which the current tool will be able to pick an object at this pose.
        The height_offset argument (in m) defines how high the tool will hover over the workspace. If height_offset = 0,
        the tool will nearly touch the workspace.

        :param workspace_name: name of the workspace
        :type workspace_name: str
        :param height_offset: offset between the workspace and the target height
        :type height_offset: float
        :param x_rel:
        :type x_rel: float
        :param y_rel:
        :type y_rel: float
        :param yaw_rel:
        :type yaw_rel: float
        :return: target_pose
        :rtype: RobotState
        """
        from niryo_robot_poses_handlers.srv import GetTargetPose, GetTargetPoseRequest

        result = self.__call_service('/niryo_robot_poses_handlers/get_target_pose', GetTargetPose,
                                     workspace_name, height_offset, x_rel, y_rel, yaw_rel)
        self.__check_result_status(result)
        return result.target_pose

    def get_target_pose_from_cam(self, workspace_name, height_offset, shape, color):
        """
        First detects the specified object using the camera and then returns the robot pose in which the object can
        be picked with the current tool

        :param workspace_name: name of the workspace
        :type workspace_name: str
        :param height_offset: offset between the workspace and the target height
        :type height_offset: float
        :param shape: shape of the target
        :type shape: ObjectShape
        :param color: color of the target
        :type color: ObjectColor
        :return: object_found, object_pose, object_shape, object_color
        :rtype: (bool, RobotState, str, str)
        """
        object_found, rel_pose, obj_shape, obj_color = self.detect_object(workspace_name, shape, color)
        if not object_found:
            return False, None, "", ""
        obj_pose = self.get_target_pose_from_rel(workspace_name, height_offset, rel_pose.x, rel_pose.y, rel_pose.yaw)
        return True, obj_pose, obj_shape, obj_color

    def vision_pick_w_obs_joints(self, workspace_name, height_offset, shape, color, observation_joints):
        """
        Move Joints to observation_joints, then execute a vision pick
        """
        self.move_joints(*observation_joints)
        return self.vision_pick(workspace_name, height_offset, shape, color)

    def vision_pick_w_obs_pose(self, workspace_name, height_offset, shape, color, observation_pose_list):
        """
        Move Pose to observation_pose, then execute a vision pick
        """
        self.move_pose(*observation_pose_list)
        return self.vision_pick(workspace_name, height_offset, shape, color)

    def vision_pick(self, workspace_name, height_offset, shape, color):
        """
        Picks the specified object from the workspace. This function has multiple phases:
        1. detect object using the camera
        2. prepare the current tool for picking
        3. approach the object
        4. move down to the correct picking pose
        5. actuate the current tool
        6. lift the object

        :param workspace_name: name of the workspace
        :type workspace_name: str
        :param height_offset: offset between the workspace and the target height
        :type height_offset: float
        :param shape: shape of the target
        :type shape: ObjectShape
        :param color: color of the target
        :type color: ObjectColor
        :return: object_found, object_shape, object_color
        :rtype: (bool, ObjectShape, ObjectColor)
        """
        object_found, rel_pose, obj_shape, obj_color = self.detect_object(workspace_name, shape, color)
        if not object_found:
            return False, "", ""

        pick_pose = self.get_target_pose_from_rel(
            workspace_name, height_offset, rel_pose.x, rel_pose.y, rel_pose.yaw)
        approach_pose = self.get_target_pose_from_rel(
            workspace_name, height_offset + 0.05, rel_pose.x, rel_pose.y, rel_pose.yaw)

        self.release_with_tool()

        self.move_pose(*self.robot_state_msg_to_list(approach_pose))
        self.move_pose(*self.robot_state_msg_to_list(pick_pose))

        self.grasp_with_tool()

        self.move_pose(*self.robot_state_msg_to_list(approach_pose))
        return True, obj_shape, obj_color

    def move_to_object(self, workspace, height_offset, shape, color):
        """
        Same as `get_target_pose_from_cam` but directly moves to this position

        :param workspace: name of the workspace
        :type workspace: str
        :param height_offset: offset between the workspace and the target height
        :type height_offset: float
        :param shape: shape of the target
        :type shape: ObjectShape
        :param color: color of the target
        :type color: ObjectColor
        :return: object_found, object_shape, object_color
        :rtype: (bool, ObjectShape, ObjectColor)
        """
        obj_found, obj_pose, obj_shape, obj_color = self.get_target_pose_from_cam(
            workspace, height_offset, shape, color)
        if not obj_found:
            return False, "", ""
        self.move_pose(*self.robot_state_msg_to_list(obj_pose))
        return True, obj_shape, obj_color

    def detect_object(self, workspace_name, shape, color):
        """

        :param workspace_name: name of the workspace
        :type workspace_name: str
        :param shape: shape of the target
        :type shape: ObjectShape
        :param color: color of the target
        :type color: ObjectColor
        :return: object_found, object_pose, object_shape, object_color
        :rtype: (bool, RobotState, str, str)
        """
        from niryo_robot_vision.srv import ObjDetection

        ratio = self.get_workspace_ratio(workspace_name)
        response = self.__call_service("/niryo_robot_vision/obj_detection_rel", ObjDetection,
                                       shape, color, ratio, False)
        if response.status == CommandStatus.SUCCESS:
            return True, response.obj_pose, response.obj_type, response.obj_color
        elif response.status == CommandStatus.MARKERS_NOT_FOUND:
            rospy.logwarn_throttle(1, 'ROS Wrapper - Markers Not Found')
        elif response.status == CommandStatus.VIDEO_STREAM_NOT_RUNNING:
            rospy.logwarn_throttle(1, 'Video Stream not running')
        return False, None, "", ""

    def get_camera_intrinsics(self):
        """
        Get calibration object: camera intrinsics, distortions coefficients

        :return: raw camera intrinsics, distortions coefficients
        :rtype: (list, list)
        """
        timeout = rospy.get_time() + 2.0
        while self.__camera_intrinsics_message is None:
            rospy.sleep(0.05)
            if rospy.get_time() > timeout:
                raise NiryoRosWrapperException(
                    'Timeout: could not video stream message (/niryo_robot_vision/camera_intrinsics topic)')
        return self.__camera_intrinsics_message.K, self.__camera_intrinsics_message.D

    def get_debug_markers(self):
        req = DebugMarkersRequest()
        result = self.__call_service('/niryo_robot_vision/debug_markers', DebugMarkers, req)
        return result

    def get_debug_colors(self, color):
        req = DebugColorDetectionRequest()
        req.color = color
        result = self.__call_service('/niryo_robot_vision/debug_colors', DebugColorDetection, req)
        return result

    # - Workspace
    def save_workspace_from_poses(self, name, list_poses_raw):
        """
        Save workspace by giving the poses of the robot to point its 4 corners
        with the calibration Tip. Corners should be in the good order

        :param name: workspace name, max 30 char.
        :type name: str
        :param list_poses_raw: list of 4 corners pose
        :type list_poses_raw: list[list]
        :return: status, message
        :rtype: (int, str)
        """
        from niryo_robot_poses_handlers.srv import ManageWorkspace, ManageWorkspaceRequest

        list_poses = [self.list_to_robot_state_msg(*pose) for pose in list_poses_raw]
        req = ManageWorkspaceRequest()
        req.cmd = ManageWorkspaceRequest.SAVE
        if len(name)>30:
            rospy.logwarn('ROS Wrapper - Workspace name is too long, using : %s instead', name[:30])
        req.workspace.name = name[:30]
        req.workspace.poses = list_poses
        result = self.__call_service('/niryo_robot_poses_handlers/manage_workspace',
                                     ManageWorkspace, req)
        return self.__classic_return_w_check(result)

    def save_workspace_from_points(self, name, list_points_raw):
        """
        Save workspace by giving the poses of its 4 corners in the good order

        :param name: workspace name, max 30 char.
        :type name: str
        :param list_points_raw: list of 4 corners [x, y, z]
        :type list_points_raw: list[list[float]]
        :return: status, message
        :rtype: (int, str)
        """
        from niryo_robot_poses_handlers.srv import ManageWorkspace, ManageWorkspaceRequest

        list_points = [Point(*point) for point in list_points_raw]
        req = ManageWorkspaceRequest()
        req.cmd = ManageWorkspaceRequest.SAVE_WITH_POINTS
        if len(name)>30:
            rospy.logwarn('ROS Wrapper - Workspace name is too long, using : %s instead', name[:30])
        req.workspace.name = name[:30]
        req.workspace.points = list_points
        result = self.__call_service('/niryo_robot_poses_handlers/manage_workspace',
                                     ManageWorkspace, req)
        return self.__classic_return_w_check(result)

    def delete_workspace(self, name):
        """
        Call workspace manager to delete a certain workspace

        :param name: workspace name
        :type name: str
        :return: status, message
        :rtype: (int, str)
        """
        from niryo_robot_poses_handlers.srv import ManageWorkspace, ManageWorkspaceRequest

        req = ManageWorkspaceRequest()
        req.cmd = ManageWorkspaceRequest.DELETE
        req.workspace.name = name
        result = self.__call_service('/niryo_robot_poses_handlers/manage_workspace',
                                     ManageWorkspace, req)
        return self.__classic_return_w_check(result)

    def get_workspace_poses(self, name):
        """
        Get the 4 workspace poses of the workspace called 'name'

        :param name: workspace name
        :type name: str
        :return: List of the 4 workspace poses
        :rtype: list[list]
        """
        from niryo_robot_poses_handlers.srv import GetWorkspaceRobotPoses

        result = self.__call_service('/niryo_robot_poses_handlers/get_workspace_poses', GetWorkspaceRobotPoses, name)
        self.__check_result_status(result)

        poses = result.poses
        list_p_raw = []
        for p in poses:
            pose = [p.position.x, p.position.y, p.position.z, p.rpy.roll, p.rpy.pitch, p.rpy.yaw]
            list_p_raw.append(pose)
        return list_p_raw

    def get_workspace_ratio(self, name):
        """
        Give the length over width ratio of a certain workspace

        :param name: workspace name
        :type name: str
        :return: ratio
        :rtype: float
        """
        from niryo_robot_poses_handlers.srv import GetWorkspaceRatio

        result = self.__call_service('/niryo_robot_poses_handlers/get_workspace_ratio',
                                     GetWorkspaceRatio, name)
        self.__check_result_status(result)
        return result.ratio

    def get_workspace_list(self, with_desc=False):
        """
        Ask the workspace manager service names of the available workspace

        :return: list of workspaces name
        :rtype: list[str]
        """
        result = self.__call_service('/niryo_robot_poses_handlers/get_workspace_list',
                                     GetNameDescriptionList)
        if with_desc:
            return result.name_list, result.description_list
        return result.name_list

    #- Logs

    def get_logs(self):
        """
        Returns a generator iterating over all the logs published

        :return: the last logs
        :rtype: generator[str]
        """
        while len(self.__logs) > 0:
            yield self.__logs.pop(0)

    def purge_logs(self):
        """
        Purge the ros logs and discard the following
        Restart the robot to have logs again

        :return: status, message
        :rtype: (int, str)
        """
        # The request data is ignored by the service
        result = self.__call_service('/niryo_robot_rpi/purge_ros_logs', SetInt, 0)
        return self.__classic_return_w_check(result)

    def purge_logs_on_startup(self, value):
        """
        Purge the ros logs at rpi startup

        :param value: If the rpi have to purge the logs at startup
        :type value: bool
        :return: status, message
        :rtype: (int, str)
        """
        value = 1 if value is True else 0
        result = self.__call_service('/niryo_robot_rpi/set_purge_ros_log_on_startup', SetInt, value)
        return self.__classic_return_w_check(result)

    # - Blockly

    def get_highlighted_block(self):
        """
        Returns the blockly highlighted block

        :return: the highlighted block id
        :rtype: str
        """
        return self.__highlighted_block

    def get_save_point_event(self):
        """
        Returns an event which is set when a pose must be saved

        :return: the event
        :rtype: Event
        """
        return self.__save_current_position_event

    # - Programs

    def get_programs_list(self):
        """
        Get all the programs stored in the robot

        :return: names, descriptions
        :rtype: list[str], list[str]
        """
        result = self.__call_service('/niryo_robot_programs_manager/get_program_list', GetProgramList)
        return result.programs_names, result.programs_description

    def add_program(self, name, language, description, code):
        """
        Create a program

        :param name: the program's name
        :type name: str
        :param language: the program's language
        :type language: ProgramLanguage
        :param description: the program's description
        :type description: str
        :param code: the program's code
        :type code: str

        :return: status, message
        :rtype: (int, str)
        """
        req = ManageProgramRequest()
        req.cmd = 1
        req.name = name
        req.language.used = language
        req.description = description
        req.code = code
        result = self.__call_service('/niryo_robot_programs_manager/manage_program', ManageProgram, req)
        return self.__classic_return_w_check(result)

    def get_program(self, name, language):
        """
        Get a program's code

        :param name: the program's name
        :type name: str
        :param language: the program's language
        :type language: ProgramLanguage
        :return: the program's code
        :rtype: str
        """
        req = GetProgramRequest()
        req.name = name
        req.language.used = language
        result = self.__call_service('/niryo_robot_programs_manager/get_program', GetProgram, req)
        self.__check_result_status(result)
        return result.code

    def set_autorun(self, name, language, mode):
        """
        Set a program as the autorun

        :param name: the name of the program
        :type name: str
        :param language: the language of the program
        :type language: ProgramLanguage
        :param mode: the mode of the autorun
        :type mode: AutorunMode
        """
        req = SetProgramAutorunRequest()
        req.name = name
        req.language.used = language
        req.mode = mode
        result = self.__call_service('/niryo_robot_programs_manager/set_program_autorun', SetProgramAutorun, req)
        return self.__classic_return_w_check(result)

    def start_program(self, name, language):
        """
        Start a program

        :param name: The program's name
        :type name: str
        :param language: the program's language
        :type language: ProgramLanguage
        :return: status, message
        :rtype: (int, str)
        """
        req = ExecuteProgramRequest()
        req.name = name
        req.language.used = language
        result = self.__call_service('/niryo_robot_programs_manager/execute_program', ExecuteProgram, req)
        return self.__classic_return_w_check(result)

    def stop_program(self):
        """
        Stop the currently running program

        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_programs_manager/stop_program', Trigger)
        return self.__classic_return_w_check(result)

    def delete_program(self, name, language):
        """
        Delete a program

        :param name: the program's name
        :type name: str
        :param language: the program's language
        :type language: ProgramLanguage

        :return: status, message
        :rtype: (int, str)
        """
        req = ManageProgramRequest()
        req.cmd = -1
        req.name = name
        req.language.used = language

        result = self.__call_service('/niryo_robot_programs_manager/manage_program', ManageProgram, req)

        return self.__classic_return_w_check(result)

    # - Autorun

    def start_autorun(self):
        """
        Start the program set as autorun

        :return: status, message
        :rtype: (int, str)
        """
        result = self.__call_service('/niryo_robot_programs_manager/execute_program_autorun', Trigger)
        return self.__classic_return_w_check(result)

    def get_autorun(self):
        """
        Get the autorun infos

        :return: language, name, mode
        :rtype: (int, str, int)
        """
        result = self.__call_service('/niryo_robot_programs_manager/get_program_autorun_infos', GetProgramAutorunInfos)
        self.__check_result_status(result)
        return result.language.used, result.name, result.mode
