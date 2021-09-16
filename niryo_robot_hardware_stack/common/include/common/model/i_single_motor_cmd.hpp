/*
i_single_motor_cmd.hpp
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

#ifndef I_SINGLE_MOTOR_CMD_H
#define I_SINGLE_MOTOR_CMD_H

#include "common/model/i_object.hpp"

namespace common
{
namespace model
{

class ISingleMotorCmd : public IObject
{
public:
    virtual int getCmdType() const = 0;

    //tests
    virtual bool isStepperCmd() const = 0;
    virtual bool isDxlCmd() const = 0;
};

} // namespace model
} // namespace common

#endif // I_SINGLE_MOTOR_CMD_H
