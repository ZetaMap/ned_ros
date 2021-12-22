import rospy

# - Messages
from niryo_robot_led_ring.msg import LedRingAnimation, LedRingStatus
from std_msgs.msg import ColorRGBA

# - Services
from niryo_robot_led_ring.srv import LedUser, LedUserRequest, SetLedColor, SetLedColorRequest

# Command Status
from niryo_robot_msgs.msg import CommandStatus


class LedRingRosWrapperException(Exception):
    pass


def check_ned2_version(func):
    '''Decorator that check the robot version'''

    def wrap(*args, **kwargs):
        robot_instance = args[0]
        if robot_instance.hardware_version != 'ned2':
            raise LedRingRosWrapperException(
                "Error Code : {}\nMessage : Wrong robot hardware version, feature only available on Ned2".format(
                    CommandStatus.BAD_HARDWARE_VERSION))

        return func(*args, **kwargs)

    return wrap


class LedRingRosWrapper(object):
    def __init__(self, hardware_version='ned2', service_timeout=1):
        self.__service_timeout = service_timeout
        self.__hardware_version = hardware_version

    @property
    def hardware_version(self):
        return self.__hardware_version

    # - Led Ring
    @check_ned2_version
    def set_led_color(self, led_id, color):
        """
        Lights up an LED in one colour. RGB colour between 0 and 255.

        Example: ::

            from std_msgs.msg import ColorRGBA

            led_ring.set_led_color(5, [15, 50, 255])
            led_ring.set_led_color(5, ColorRGBA(r=15, g=50, b=255))

        :param led_id: Id of the led: between 0 and 29
        :type led_id: int
        :param color: Led color in a list of size 3[R, G, B] or in an ColorRGBA object. RGB channels from 0 to 255.
        :type color: list[float]  or ColorRGBA
        :return: status, message
        :rtype: (int, str)
        """
        color_rgba = color if isinstance(color, ColorRGBA) else ColorRGBA(*(color[:3] + [0]))
        led_request = SetLedColorRequest(led_id=led_id, color=color_rgba)
        result = self.__call_service('/niryo_robot_led_ring/set_led_color', SetLedColor, led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def solid(self, color, wait=False):
        """
        Set the whole Led Ring to a fixed color.

        Example: ::

            from std_msgs.msg import ColorRGBA

            led_ring.solid([15, 50, 255])
            led_ring.solid(ColorRGBA(r=15, g=50, b=255), True)

        :param color: Led color in a list of size 3[R, G, B] or in an ColorRGBA object. RGB channels from 0 to 255.
        :type color: list[float]  or ColorRGBA
        :param wait: The service wait for the animation to finish or not to answer.
                For this method, the action is quickly done, so waiting doesn't take a lot of time.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        color_rgba = [color if isinstance(color, ColorRGBA) else ColorRGBA(*(color[:3] + [0]))]
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.SOLID),
                                          colors=color_rgba, wait_end=wait)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def turn_off(self, wait=False):
        """
        Turn off all Leds

        Example: ::

            led_ring.turn_off()

        :param wait: The service wait for the animation to finish or not to answer.
                For this method, the action is quickly done, so waiting doesn't take a lot of time.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.NONE), wait_end=wait)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def flashing(self, color, period=0, iterations=0, wait=False):
        """
        Flashes a color according to a frequency. The frequency is equal to 1 / period.

        Examples: ::

            from std_msgs.msg import ColorRGBA

            led_ring.flashing([15, 50, 255])
            led_ring.flashing([15, 50, 255], 1, 100, True)
            led_ring.flashing([15, 50, 255], iterations=20, wait=True)

            frequency = 20  # Hz
            total_duration = 10 # seconds
            led_ring.flashing(ColorRGBA(r=15, g=50, b=255), 1./frequency, total_duration * frequency , True)

        :param color: Led color in a list of size 3[R, G, B] or in an ColorRGBA object. RGB channels from 0 to 255.
        :type color: list[float]  or ColorRGBA
        :param period: Execution time for a pattern in seconds. If 0, the default time will be used.
        :type period: float
        :param iterations: Number of consecutive flashes. If 0, the Led Ring flashes endlessly.
        :type iterations: int
        :param wait: The service wait for the animation to finish all iterations or not to answer. If iterations
                is 0, the service answers immediately.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        color_rgba = [color if isinstance(color, ColorRGBA) else ColorRGBA(*(color[:3] + [0]))]
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.FLASHING), wait_end=wait,
                                          colors=color_rgba, period=period, iterations=iterations)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def alternate(self, color_list, period=0, iterations=0, wait=False):
        """
        Several colors are alternated one after the other.

        Examples: ::

            from std_msgs.msg import ColorRGBA

            color_list = [
                ColorRGBA(r=15, g=50, b=255),
                [255, 0, 0],
                [0, 255, 0],
            ]

            led_ring.alternate(color_list)
            led_ring.alternate(color_list, 1, 100, True)
            led_ring.alternate(color_list, iterations=20, wait=True)

        :param color_list: Led color list of lists of size 3[R, G, B] or ColorRGBA objects. RGB channels from 0 to 255.
        :type color_list: list[list[float] or ColorRGBA]
        :param period: Execution time for a pattern in seconds. If 0, the default time will be used.
        :type period: float
        :param iterations: Number of consecutive alternations. If 0, the Led Ring alternates endlessly.
        :type iterations: int
        :param wait: The service wait for the animation to finish all iterations or not to answer. If iterations
                is 0, the service answers immediately.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        color_rgba_list = [c if isinstance(c, ColorRGBA) else ColorRGBA(*(c[:3] + [0])) for c in color_list]
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.ALTERNATE), wait_end=wait,
                                          colors=color_rgba_list, period=period,
                                          iterations=iterations)
        user_led_request.animation_mode.animation = LedRingAnimation.ALTERNATE
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def chase(self, color, period=0, iterations=0, wait=False):
        """
        Movie theater light style chaser animation.

        Examples: ::

            from std_msgs.msg import ColorRGBA

            led_ring.chase(ColorRGBA(r=15, g=50, b=255))
            led_ring.chase([15, 50, 255], 1, 100, True)
            led_ring.chase(ColorRGBA(r=15, g=50, b=255), iterations=20, wait=True)

        :param color: Led color in a list of size 3[R, G, B] or in an ColorRGBA object. RGB channels from 0 to 255.
        :type color: list or ColorRGBA
        :param period: Execution time for a pattern in seconds. If 0, the default time will be used.
        :type period: float
        :param iterations: Number of consecutive chase. If 0, the animation continues endlessly.
            One chase just lights one Led every 3 Leds.
        :type iterations: int
        :param wait: The service wait for the animation to finish all iterations or not to answer. If iterations
                is 0, the service answers immediately.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        color_rgba = [color if isinstance(color, ColorRGBA) else ColorRGBA(*(color[:3] + [0]))]
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.CHASE), wait_end=wait,
                                          colors=color_rgba, period=period, iterations=iterations)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def wipe(self, color, period=0, wait=False):
        """
        Wipe a color across the Led Ring, light a Led at a time.

        Examples: ::

            from std_msgs.msg import ColorRGBA

            led_ring.wipe(ColorRGBA(r=15, g=50, b=255))
            led_ring.wipe([15, 50, 255], 1, True)
            led_ring.wipe(ColorRGBA(r=15, g=50, b=255), wait=True)

        :param color: Led color in a list of size 3[R, G, B] or in an ColorRGBA object. RGB channels from 0 to 255.
        :type color: list[float]  or ColorRGBA
        :param period: Execution time for a pattern in seconds. If 0, the default time will be used.
        :type period: float
        :param wait: The service wait for the animation to finish or not to answer.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        color_rgba = [color if isinstance(color, ColorRGBA) else ColorRGBA(*(color[:3] + [0]))]
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.COLOR_WIPE), wait_end=wait,
                                          colors=color_rgba, period=period)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def rainbow(self, period=0, iterations=0, wait=False):
        """
        Draw rainbow that fades across all Leds at once.

        Examples: ::

            led_ring.rainbow()
            led_ring.rainbow(5, 2, True)
            led_ring.rainbow(wait=True)

        :param period: Execution time for a pattern in seconds. If 0, the default time will be used.
        :type period: float
        :param iterations: Number of consecutive rainbows. If 0, the animation continues endlessly.
        :type iterations: int
        :param wait: The service wait for the animation to finish or not to answer. If iterations
                is 0, the service answers immediately.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.RAINBOW), wait_end=wait,
                                          period=period, iterations=iterations)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def rainbow_cycle(self, period=0, iterations=0, wait=False):
        """
        Draw rainbow that uniformly distributes itself across all Leds.

        Examples: ::

            led_ring.rainbow_cycle()
            led_ring.rainbow_cycle(5, 2, True)
            led_ring.rainbow_cycle(wait=True)

        :param period: Execution time for a pattern in seconds. If 0, the default time will be used.
        :type period: float
        :param iterations: Number of consecutive rainbow cycles. If 0, the animation continues endlessly.
        :type iterations: int
        :param wait: The service wait for the animation to finish or not to answer. If iterations
                is 0, the service answers immediately.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.RAINBOW_CYLE), wait_end=wait,
                                          period=period, iterations=iterations)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def rainbow_chase(self, period=0, iterations=0, wait=False):
        """
        Rainbow chase animation, like the led_ring_chase method.

        Examples: ::

            led_ring.rainbow_chase()
            led_ring.rainbow_chase(5, 2, True)
            led_ring.rainbow_chase(wait=True)

        :param period: Execution time for a pattern in seconds. If 0, the default time will be used.
        :type period: float
        :param iterations: Number of consecutive rainbow cycles. If 0, the animation continues endlessly.
        :type iterations: int
        :param wait: The service wait for the animation to finish or not to answer. If iterations
                is 0, the service answers immediately.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.RAINBOW_CHASE),
                                          wait_end=wait,
                                          period=period, iterations=iterations)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def go_up(self, color, period=0, iterations=0, wait=False):
        """
        Leds turn on like a loading circle, and are then all turned off at once.

        Examples: ::

            from std_msgs.msg import ColorRGBA

            led_ring.go_up(ColorRGBA(r=15, g=50, b=255))
            led_ring.go_up([15, 50, 255], 1, 100, True)
            led_ring.go_up(ColorRGBA(r=15, g=50, b=255), iterations=20, wait=True)


        :param color: Led color in a list of size 3[R, G, B] or in an ColorRGBA object. RGB channels from 0 to 255.
        :type color: list[float] or ColorRGBA
        :param period: Execution time for a pattern in seconds. If 0, the default time will be used.
        :type period: float
        :param iterations: Number of consecutive turns around the Led Ring. If 0, the animation
            continues endlessly.
        :type iterations: int
        :param wait: The service wait for the animation to finish or not to answer. If iterations
                is 0, the service answers immediately.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        color_rgba = [color if isinstance(color, ColorRGBA) else ColorRGBA(*(color[:3] + [0]))]
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.GO_UP), wait_end=wait,
                                          colors=color_rgba, period=period, iterations=iterations)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def go_up_down(self, color, period=0, iterations=0, wait=False):
        """
        Leds turn on like a loading circle, and are turned off the same way.

        Examples: ::

            from std_msgs.msg import ColorRGBA

            led_ring.go_up_down(ColorRGBA(r=15, g=50, b=255))
            led_ring.go_up_down([15, 50, 255], 1, 100, True)
            led_ring.go_up_down(ColorRGBA(r=15, g=50, b=255), iterations=20, wait=True)


        :param color: Led color in a list of size 3[R, G, B] or in an ColorRGBA object. RGB channels from 0 to 255.
        :type color: list[float] or ColorRGBA
        :param period: Execution time for a pattern in seconds. If 0, the default time will be used.
        :type period: float
        :param iterations: Number of consecutive turns around the Led Ring. If 0, the animation
            continues endlessly.
        :type iterations: int
        :param wait: The service wait for the animation to finish or not to answer. If iterations
                is 0, the service answers immediately.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        color_rgba = [color if isinstance(color, ColorRGBA) else ColorRGBA(*(color[:3] + [0]))]
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.GO_UP_AND_DOWN),
                                          wait_end=wait, colors=color_rgba, period=period,
                                          iterations=iterations)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def breath(self, color, period=0, iterations=0, wait=False):
        """
        Variation of the light intensity of the LED ring, similar to human breathing.

        Examples: ::

            from std_msgs.msg import ColorRGBA

            led_ring.breath(ColorRGBA(r=15, g=50, b=255))
            led_ring.breath([15, 50, 255], 1, 100, True)
            led_ring.breath(ColorRGBA(r=15, g=50, b=255), iterations=20, wait=True)

        :param color: Led color in a list of size 3[R, G, B] or in an ColorRGBA object. RGB channels from 0 to 255.
        :type color: list[float] or ColorRGBA
        :param period: Execution time for a pattern in seconds. If 0, the default time will be used.
        :type period: float
        :param iterations: Number of consecutive turns around the Led Ring. If 0, the animation
            continues endlessly.
        :type iterations: int
        :param wait: The service wait for the animation to finish or not to answer. If iterations
                is 0, the service answers immediately.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        color_rgba = [color if isinstance(color, ColorRGBA) else ColorRGBA(*(color[:3] + [0]))]
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.BREATH), wait_end=wait,
                                          colors=color_rgba, period=period, iterations=iterations)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def snake(self, color, period=0, iterations=0, wait=False):
        """
        A small coloured snake (certainly a python :D ) runs around the LED ring.

        Examples: ::

            from std_msgs.msg import ColorRGBA

            led_ring.snake(ColorRGBA(r=15, g=50, b=255))
            led_ring.snake([15, 50, 255], 1, 100, True)
            led_ring.snake(ColorRGBA(r=15, g=50, b=255), iterations=20, wait=True)

        :param color: Led color in a list of size 3[R, G, B] or in an ColorRGBA object. RGB channels from 0 to 255.
        :type color: list[float] or ColorRGBA
        :param period: Execution time for a pattern in seconds. If 0, the default duration will be used.
        :type period: float
        :param iterations: Number of consecutive turns around the Led Ring. If 0, the animation
            continues endlessly.
        :type iterations: int
        :param wait: The service wait for the animation to finish or not to answer. If iterations
                is 0, the service answers immediately.
        :type wait: bool
        :return: status, message
        :rtype: (int, str)
        """
        color_rgba = [color if isinstance(color, ColorRGBA) else ColorRGBA(*(color[:3] + [0]))]
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.SNAKE), wait_end=wait,
                                          colors=color_rgba, period=period, iterations=iterations)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    @check_ned2_version
    def custom(self, led_colors):
        """
        Sends a colour command to all LEDs of the LED ring.
        The function expects a list of colours for the 30 LEDs  of the robot.

        Example: ::

            led_list = [[i / 30. * 255 , 0, 255 - i / 30.] for i in range(30)]
            led_ring.custom(led_list)

        :param led_colors: List of size 30 of led color in a list of size 3[R, G, B] or in an ColorRGBA object.
                RGB channels from 0 to 255.
        :type led_colors: list[list[float] or ColorRGBA]
        :return: status, message
        :rtype: (int, str)
        """
        color_rgba_list = [c if isinstance(c, ColorRGBA) else ColorRGBA(*(c[:3] + [0])) for c in led_colors]
        user_led_request = LedUserRequest(animation_mode=LedRingAnimation(LedRingAnimation.CUSTOM), wait_end=True,
                                          colors=color_rgba_list)
        result = self.__call_service('/niryo_robot_led_ring/set_user_animation', LedUser, user_led_request)
        return self.__classic_return_w_check(result)

    # --- Functions interface
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
            raise LedRingRosWrapperException(e)

        # Call service
        try:
            service = rospy.ServiceProxy(service_name, service_msg_type)
            response = service(*args)
            return response
        except rospy.ServiceException as e:
            raise LedRingRosWrapperException(e)

    def __classic_return_w_check(self, result):
        self.__check_result_status(result)
        return result.status, result.message

    @staticmethod
    def __check_result_status(result):
        if result.status < 0:
            raise LedRingRosWrapperException("Error Code : {}\nMessage : {}".format(result.status, result.message))
