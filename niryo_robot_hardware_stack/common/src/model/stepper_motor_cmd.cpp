/*
    stepper_motor_cmd.cpp
    Copyright (C) 2017 Niryo
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

#include "model/stepper_motor_cmd.hpp"

#include <sstream>

using namespace std;

namespace common {
    namespace model {

        StepperMotorCmd::StepperMotorCmd() :
            _type(EStepperCommandType::CMD_TYPE_NONE)
        {
        }

        StepperMotorCmd::StepperMotorCmd(EStepperCommandType type,
                                         vector<uint8_t> motor_id,
                                         vector<int32_t> params) :
            _type(type), _motor_id_list(motor_id), _param_list(params)
        {
        }

        void StepperMotorCmd::setType(EStepperCommandType type)
        {
            _type = type;
        }

        void StepperMotorCmd::setMotorsId(vector<uint8_t> motor_id)
        {
            _motor_id_list = motor_id;
        }

        void StepperMotorCmd::setParams(vector<int32_t> params)
        {
            _param_list = params;
        }

        string StepperMotorCmd::str() const
        {
            ostringstream ss;
            ss << "Single motor cmd - ";

            ss << StepperCommandTypeEnum(_type).toString();

            return ss.str();
        }

    } // namespace model
} // namespace common
