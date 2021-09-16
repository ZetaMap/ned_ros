/*
joint_state.hpp
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

#ifndef JOINT_STATE_H
#define JOINT_STATE_H

#include "abstract_motor_state.hpp"
#include "hardware_type_enum.hpp"
#include "component_type_enum.hpp"

#include <stdint.h>
#include <string>


namespace common
{
namespace model
{

/**
 * @brief The JointState class
 */
class JointState : public AbstractMotorState
{
    
public:
    JointState();
    JointState(std::string name, EHardwareType type,
               EComponentType component_type,
               EBusProtocol bus_proto, uint8_t id);
    JointState(const JointState& state);

    virtual ~JointState() override;

    void setName(std::string &name);
    void setOffsetPosition(double offset_position);
    void setDirection(int direction);

    std::string getName() const;
    double getOffsetPosition() const;
    int getDirection() const;

    virtual bool operator==(const JointState &other) const;

    virtual int to_motor_pos(double pos_rad) { return 0; }
    virtual double to_rad_pos(int position_dxl) { return 0; }

    // AbstractMotorState interface
    virtual void reset() override;
    virtual bool isValid() const override;
    virtual std::string str() const override;

public:
    double pos{0.0};
    double cmd{0.0};
    double vel{0.0};
    double eff{0.0};

protected:
    std::string _name;
    double _offset_position{0.0};
    bool _need_calibration{false};
    int _direction{1};

};

/**
 * @brief JointState::getName
 * @return
 */
inline
std::string JointState::getName() const
{
    return _name;
}


/**
 * @brief JointState::getOffsetPosition
 * @return
 */
inline
double JointState::getOffsetPosition() const
{
    return _offset_position;
}

/**
 * @brief JointState::getDirection
 * @return
 */
inline
int JointState::getDirection() const
{
    return _direction;
}

} // namespace model
} // namespace common

#endif // JOINT_STATE_H
