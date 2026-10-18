#include "io.hpp"

namespace dns {

void WriteUint8(std::stringstream &ss, uint8_t num) {
  ss.write(reinterpret_cast<char *>(&num), 1);
}

void WriteUint16(std::stringstream &ss, uint16_t num) {
  WriteUint8(ss, num >> 8);
  WriteUint8(ss, num & 0xFF);
}

void WriteUint32(std::stringstream &ss, uint32_t num) {
  WriteUint8(ss, (num >> 24) & 0xFF);
  WriteUint8(ss, (num >> 16) & 0xFF);
  WriteUint8(ss, (num >> 8) & 0xFF);
  WriteUint8(ss, num & 0xFF);
}

} // namespace dns
