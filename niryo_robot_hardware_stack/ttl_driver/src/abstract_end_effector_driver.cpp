/*
    abstract_end_effector_driver.cpp
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

#include "ttl_driver/abstract_end_effector_driver.hpp"

#include <sstream>
#include <utility>
#include <vector>
#include <string>

using ::std::shared_ptr;
using ::std::string;
using ::std::ostringstream;

namespace ttl_driver
{

/**
 * @brief AbstractEndEffectorDriver::AbstractEndEffectorDriver
 * @param portHandler
 * @param packetHandler
 */
AbstractEndEffectorDriver::AbstractEndEffectorDriver(shared_ptr<dynamixel::PortHandler> portHandler,
                                                     shared_ptr<dynamixel::PacketHandler> packetHandler) :
  AbstractTtlDriver(std::move(portHandler),
                    std::move(packetHandler))
{
}

/**
 * @brief AbstractEndEffectorDriver::str : build a string describing the object. For debug purpose only
 * @return
 */
std::string AbstractEndEffectorDriver::str() const
{
  return "AbstractEndEffectorDriver (" + AbstractTtlDriver::str() + ")";
}

/**
 * @brief MockEndEffectorDriver::interpreteErrorState
 * @return
 * TODO(CC) to be implemented
 */
std::string AbstractEndEffectorDriver::interpreteErrorState(uint32_t /*hw_state*/) const
{
    return "";
}

/**
 * @brief MockEndEffectorDriver::interpreteFirwmareVersion
 * @return
 */
std::string AbstractEndEffectorDriver::interpreteFirmwareVersion(uint32_t fw_version) const
{
    auto v_major = static_cast<uint8_t>(fw_version >> 24);
    auto v_minor = static_cast<uint16_t>(fw_version >> 8);
    auto v_patch = static_cast<uint8_t>(fw_version >> 0);

    std::ostringstream ss;
    ss << std::to_string(v_major) << "."
       << std::to_string(v_minor) << "."
       << std::to_string(v_patch);
    std::string version = ss.str();

    return version;
}

/**
 * @brief MockEndEffectorDriver::interpreteActionValue
 * @param value
 * @return
 */
common::model::EActionType
AbstractEndEffectorDriver::interpreteActionValue(uint32_t value) const
{
  common::model::EActionType action = common::model::EActionType::NO_ACTION;

  // HANDLE HELD en premier car c'est le seul cas ou il peut etre actif en meme temps qu'une autre action (long push)

  if (value & 1<<0)    // 0b00000001
  {
    action = common::model::EActionType::SINGLE_PUSH_ACTION;
  }
  else if (value & 1<<1)    // 0b00000010
  {
    action = common::model::EActionType::DOUBLE_PUSH_ACTION;
  }
  else if (value & 1<<2)    // 0b0000100
  {
    action = common::model::EActionType::LONG_PUSH_ACTION;
  }
  else if (value & 1<<3)    // 0b00001000
  {
    action = common::model::EActionType::HANDLE_HELD_ACTION;
  }
  return action;
}

/**
 * @brief MockEndEffectorDriver::writeSingleCmd
 * @param cmd
 * @return
 */
int AbstractEndEffectorDriver::writeSingleCmd(std::shared_ptr<common::model::AbstractTtlSingleMotorCmd> && cmd)
{
  if (cmd && cmd->isValid())
  {
    switch (common::model::EEndEffectorCommandType(cmd->getCmdType()))
    {
        case common::model::EEndEffectorCommandType::CMD_TYPE_DIGITAL_OUTPUT:
            writeDigitalOutput(cmd->getId(), cmd->getParam());
            break;
        case common::model::EEndEffectorCommandType::CMD_TYPE_PING:
            ping(cmd->getId());
            break;
        default:
            std::cout << "Command not implemented" << std::endl;
    }
  }

  return 0;
}

/**
 * @brief MockEndEffectorDriver::writeSyncCmd
 * @return
 */
int AbstractEndEffectorDriver::writeSyncCmd(int /*type*/, const std::vector<uint8_t>& /*ids*/, const std::vector<uint32_t>& /*params*/)
{
  std::cout << "Synchronized cmd not implemented for end effector" << std::endl;

  return 0;
}

}  // namespace ttl_driver
