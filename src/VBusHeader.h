#pragma once

#include <Arduino.h>

namespace VBus {

inline uint8_t
checksum(const uint8_t *buffer, uint16_t offset, uint16_t length)
{
  uint8_t crc = 0x7F;
  buffer += offset;
  for (uint16_t i = 0; i < length; i++) {
    crc = (crc - *buffer++) & 0x7F;
  }
  return crc;
}

inline void
restoreSeptet(unsigned char *buffer, uint16_t offset, uint16_t length)
{
  uint8_t septet = buffer[offset + length];
  for (uint16_t i = 0; i < length; i++) {
    if (septet & (1 << i)) {
      buffer[offset + i] |= 0x80;
    }
  }
}

union Header {
  struct __attribute((packed)) Members {
    uint16_t m_destAddress;
    uint16_t m_sourceAddress;
    uint8_t  m_protocolVersion;
    uint16_t m_command;
    uint8_t  m_frameCount;
    uint8_t  m_checksum;
    void init()
    {
      /* TODO ifdef-remove for big endian platforms
       *  Note: in place byte swapping does not work on packed
       *  fields, since it members aren't aligned for referencing.
       */
       /*
      m_destAddress = byteswap(m_destAddress);
      m_sourceAddress = byteswap(m_sourceAddress);
      m_command = byteswap(m_command);
      */
    }
  } m_members;

  uint8_t m_buffer[sizeof(Members)];

  bool init()
  {
    auto ccs = checksum(m_buffer, 0, sizeof(Members)-1);

    if (m_members.m_checksum != ccs) {
#if 0
      Serial.print("checksums: ");
      Serial.print(m_members.m_checksum);
      Serial.print(" ");
      Serial.println(ccs);
#endif
      return false;
    }

    m_members.init();

    return true;
  }
};

} // namespace VBus
