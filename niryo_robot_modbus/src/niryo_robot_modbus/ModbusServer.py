#!/usr/bin/env python

import rospy
import socket
import logging
from threading import Thread

from pymodbus.server.sync import ModbusTcpServer
from pymodbus.device import ModbusDeviceIdentification
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext

from niryo_robot_python_ros_wrapper.ros_wrapper import NiryoRosWrapper

from .WrapperDataBlock import WrapperDataBlock
from .CoilDataBlock import CoilDataBlock
from .DiscreteInputDataBlock import DiscreteInputDataBlock
from .HoldingRegisterDataBlock import HoldingRegisterDataBlock
from .InputRegisterDataBlock import InputRegisterDataBlock


class ModbusServer:

    def __init__(self, address, port):
        self.__ros_wrapper = NiryoRosWrapper()
        self.__ros_wrapper.wait_for_nodes_initialization()

        logger = logging.getLogger("pymodbus.server")
        logger.addHandler(logging.StreamHandler())

        # zero_mode is to start the registers at address 0 instead of 1
        self.store = ModbusSlaveContext(di=DiscreteInputDataBlock(self.__ros_wrapper),
                                        co=CoilDataBlock(self.__ros_wrapper),
                                        hr=HoldingRegisterDataBlock(self.__ros_wrapper),
                                        ir=InputRegisterDataBlock(self.__ros_wrapper),
                                        zero_mode=True)

        self.context = ModbusServerContext(slaves=self.store, single=True)

        self.identity = ModbusDeviceIdentification()
        self.identity.VendorName = 'pymodbus'
        self.identity.VendorUrl = 'http://github.com/bashwork/pymodbus/'
        self.identity.ProductName = 'pymodbus Server'
        self.identity.ModelName = 'pymodbus Server'
        self.identity.MajorMinorRevision = '1.0'

        try:
            self.server = ModbusTcpServer(context=self.context,
                                          framer=None,
                                          identity=self.identity,
                                          address=(address, port))

        except socket.error as err:
            rospy.logerr("ModbusServer.init : TCP server unable to start : %s", err)
            self.server = None

    def start(self):
        t = Thread(target=self.__start_server)
        t.start()

    def __start_server(self):
        if self.server is not None:
            self.server.serve_forever()

    def stop(self):
        rospy.loginfo("Modbus - Stopping ROS subscribers")

        if self.server is not None:
            rospy.loginfo("Modbus - Closing Server")
            self.server.server_close()
            self.server.shutdown()
