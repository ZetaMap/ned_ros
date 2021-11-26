#!/usr/bin/env python

# Libs
import os
import rospy
from datetime import date, datetime
from StringIO import StringIO
from distutils.dir_util import mkpath

from niryo_robot_reports.CloudAPI import CloudAPI
from niryo_robot_reports.ReportHandler import ReportHandler

# msg
from std_msgs.msg import Empty
from niryo_robot_status.msg import RobotStatus
from niryo_robot_msgs.msg import CommandStatus

# srv

from niryo_robot_database.srv import GetSettings


class ReportsNode:
    def __init__(self):
        rospy.logdebug("Reports Node - Entering in Init")

        self.__get_setting = rospy.ServiceProxy('/niryo_robot_database/settings/get', GetSettings)

        cloud_domain = rospy.get_param('~cloud_domain')
        get_serial_number_response = self.__get_setting('serial_number')
        get_api_key_response = self.__get_setting('api_key')

        self.__able_to_send = get_serial_number_response.status == get_api_key_response.status == CommandStatus.SUCCESS
        if not self.__able_to_send:
            rospy.logwarn('Reports Node - Unable to retrieve the serial number')

        self.__cloud_api = CloudAPI(
            cloud_domain,
            get_serial_number_response.value,
            get_api_key_response.value
        )

        get_report_path_response = self.__get_setting('reports_path')
        if get_report_path_response.status != CommandStatus.SUCCESS:
            rospy.logerr('Unable to retrieve the reports directory path from the database')
        self.__reports_path = os.path.expanduser(get_report_path_response.value)
        if not os.path.isdir(self.__reports_path):
            mkpath(self.__reports_path)

        self.__curent_date = str(date.today())
        report_path = '{}/{}.json'.format(self.__reports_path, self.__curent_date)
        self.__report_handler = ReportHandler(report_path)

        rospy.Subscriber('~new_day', Empty, self.__new_day_callback)
        rospy.Subscriber('/niryo_robot_status/robot_status', RobotStatus, self.__robot_status_callback)

        # Set a bool to mentioned this node is initialized
        rospy.set_param('~initialized', True)

        rospy.logdebug("Reports Node - Node Started")

    # - callbacks

    def __new_day_callback(self, _req):
        current_day = str(date.today())
        if current_day == self.__curent_date:
            return
        report = self.__report_handler.content
        self.__cloud_api.daily_reports.send({
            date: self.__curent_date,
            report: report
        })
        self.__curent_date = current_day
        self.__report_handler.set_path(self.__curent_date)

    def __robot_status_callback(self, req):
        if req.logs_status_str.lower() not in ['error', 'critical']:
            return
        log_io = StringIO(req.logs_message)
        level, from_node, msg, from_file, function, line = map(lambda x: x[x.index(':')+2:], log_io.readlines())
        formatted_log = '{} - {}: {} in {}.{}:{}'.format(level, from_node, msg, from_file, function, line)
        self.__report_handler.add_log(formatted_log, 'ROS', str(datetime.now()))


if __name__ == "__main__":
    rospy.init_node('niryo_robot_reports', anonymous=False, log_level=rospy.INFO)

    try:
        node = ReportsNode()
        rospy.spin()
    except rospy.ROSInterruptException:
        pass
