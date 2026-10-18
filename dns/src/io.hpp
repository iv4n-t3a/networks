#pragma once

#include <cstdint>
#include <sstream>

namespace dns {

void WriteUint8(std::stringstream &ss, uint8_t num);
void WriteUint16(std::stringstream &ss, uint16_t num);
void WriteUint32(std::stringstream &ss, uint32_t num);

} // namespace dns
