import os
from hashlib import sha256
from uuid import getnode as get_mac_address


class SerialNumber:
    def __init__(self, serial_number_file_path):
        self.__file_path = serial_number_file_path

    def __file_exists(self):
        return os.path.exists(self.__file_path)

    # - Callable functions
    def read_serial(self):
        if not self.__file_exists():
            self.__create_serial()
        try:
            with open(self.__file_path, 'r') as f:
                return f.read().rstrip()
        except IOError:
            return None

    # -- Private

    @staticmethod
    def __get_serial():
        # https://www.raspberrypi-spy.co.uk/2012/09/getting-your-raspberry-pi-serial-number-using-python/
        # Extract serial from cpuinfo file
        cpuserial = "0000000000000000"
        try:
            f = open('/proc/cpuinfo', 'r')
            for line in f:
                if line[0:6] == 'Serial':
                    cpuserial = line[10:26]
            f.close()
        except IOError:
            cpuserial = "ERROR000000000"

        return cpuserial

    def __generate_raw_serial(self):
        return str(get_mac_address()) + self.__get_serial()

    @staticmethod
    def __hash_serial(raw_serial):
        return sha256(str.encode(raw_serial)).hexdigest()[:16].upper()

    def __write_serial(self, serial):
        try:
            with open(self.__file_path, 'w') as f:
                f.write(serial)
                return True
        except IOError:
            return False

    def __create_serial(self):
        raw_serial = self.__generate_raw_serial()
        hash_serial = self.__hash_serial(raw_serial)
        self.__write_serial(hash_serial)
