/*
    motor_state.cpp
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

#include "model/abstract_motor_state.hpp"
#include <sstream>

using namespace std;

namespace common {
    namespace model
    {

        AbstractMotorState::AbstractMotorState() :
            _type(EMotorType::MOTOR_TYPE_UNKNOWN)
        {
            reset();
        }

        AbstractMotorState::AbstractMotorState(uint8_t id, EMotorType type) :
              _type(type),
              _id(id),
              _position_state(0),
              _temperature_state(0),
              _voltage_state(0),
              _hw_error_state(0),
              _hw_error_message_state("")
        {
        }

        AbstractMotorState::~AbstractMotorState()
        {

        }

        void AbstractMotorState::reset()
        {
            _id = 0;
            _position_state = 0;
            _temperature_state = 0;
            _voltage_state = 0;
            _hw_error_state = 0;
            _hw_error_message_state.clear();
        }

        bool AbstractMotorState::operator==(const AbstractMotorState &m)
        {
            return (this->_id == m._id);
        }

        string AbstractMotorState::str() const
        {
            ostringstream ss;

            ss << "AbstractMotorState (" << static_cast<int>(_id) << ")" << "\n";

            ss << "position " << _position_state << "\n"
               << "temperature " << _temperature_state << "\n"
               << "voltage " << _voltage_state << "\n"
               << "hw_error " << _hw_error_state << "\n"
               << "hw_error_message \"" << _hw_error_message_state << "\"";
            ss << "\n";

            return ss.str();
        }

        void AbstractMotorState::setPositionState(uint32_t pos)
        {
            _position_state = pos;
        }

        void AbstractMotorState::setTemperatureState(uint32_t temp)
        {
            _temperature_state = temp;
        }

        void AbstractMotorState::setVoltageState(uint32_t volt)
        {
            _voltage_state = volt;
        }

        void AbstractMotorState::setHardwareError(uint32_t hw_error)
        {
            _hw_error_state = hw_error;
        }

        void AbstractMotorState::setHardwareError(string hw_error_msg)
        {
            _hw_error_message_state = hw_error_msg;
        }

    } // namespace model
} // namespace common
