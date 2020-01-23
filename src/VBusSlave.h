
/* Found link to protocol pdf here:
 *
 * https://github.com/bbqkees/vbus-arduino-domoticz
 * https://github.com/bbqkees/vbus-arduino-domoticz/blob/master/Arduino-Code/ArduinoVBusDecoder.ino
 *
 *
 * Note 2 self:
 *  The maximum voltage is capped at approx. 8.2 V.
 */

#ifndef _VBUS_SLAVE_H_
#define _VBUS_SLAVE_H_

#include <Arduino.h>
#include <VBusPayload.h>

#ifndef VBUS_BUFFER_SIZE
#  define VBUS_BUFFER_SIZE  256
#endif

namespace VBus {

class Slave
{
public:
  typedef enum {
    DebugNone = 0,
    DebugNormal,
    DebugVerbose,
  } DebugLevel;

  /**
   * @param serial Hard or Software serial already initialized with baud rate
   * @param payloadDecoders Null pointer terminated list of pointers to payload decoders
   */
  Slave(Print &print, Stream &serial, PayloadDecoder **payloadDecoders)
    : m_print(print)
    , m_serial(serial)
    , m_payloadDecoders(payloadDecoders)
    , m_debugLevel(DebugNone)
    , m_pos(-1)
    , m_bufferOverruns(0)
    , m_headerCrcErr(0)
    , m_frameCrcErr(0)
  { }

  void begin(DebugLevel debugLevel=DebugNone)
  {
    m_debugLevel = debugLevel;
    m_pos = -1;
    m_bufferOverruns = 0;
    m_headerCrcErr = 0;
    m_frameCrcErr = 0;
  }

  void run()
  {
    auto c = m_serial.read();

    if (c < 0) {
      return;
    }

    // detect frame
    // lazy reception: waiting for next frame to process previous
    // could be optimized to process frame on the fly but will
    // make the code much more complicated
    if (c == 0xAA) {
      if (m_pos >= 0) {
        decode();
      }
      m_pos = 0;
    } else if (m_pos >= VBUS_BUFFER_SIZE) {
      // buffer overrun, reset frame state machine
      m_pos = -1;
      m_bufferOverruns++;
    }

    if (m_pos >= 0) {
      m_buffer[m_pos] = c;
      m_pos++;
    }
  }

  uint32_t getBufferOverruns()
  {
    auto o = m_bufferOverruns;
    m_bufferOverruns = 0;
    return o;
  }

  uint32_t getHeaderCrcErr()
  {
    auto e = m_headerCrcErr;
    m_headerCrcErr = 0;
    return e;
  }

  uint32_t getFrameCrcErr()
  {
    auto e = m_frameCrcErr;
    m_frameCrcErr = 0;
    return e;
  }

protected:
  void decode()
  {
    const uint8_t HeaderPos = 1;

    Header *header = reinterpret_cast<Header*>(m_buffer + HeaderPos);
    if (not header->init()) {
      m_headerCrcErr++;
      if (m_debugLevel > DebugNone) {
        m_print.println("vbus crc err hd");
      }
      return;
    }

    /* todo perform subsequent frame handling based on protocol version (0x10, 0x20, ...) */


    /* restore data bytes from septet encoding */
    const uint8_t PayloadPos = HeaderPos + sizeof(Header::Members);
    const uint8_t FrameSize = 6;
    const uint8_t FrameDataCount = 4;
    const uint8_t FrameChecksumPos = 5;

#if 0 // TODO: test this code
    /* check if data amount announced by header fits in the number of frames */
    if (m_pos - PayloadPos < FrameSize * header->m_members.m_frameCount) {
      // frame count announced in header has not been received!
      return;
    }
#endif

    // make sure we don't write outside the buffer due to corrupt frame count
    auto MaxFrameCount = (VBUS_BUFFER_SIZE - PayloadPos) / FrameSize;

    uint8_t *frame = m_buffer + PayloadPos;

    for (uint8_t f = 0; f < header->m_members.m_frameCount and f < MaxFrameCount; f++) {

      auto ccs = checksum(frame, 0, FrameDataCount + 1);

      if (ccs != frame[FrameChecksumPos]) {
        m_frameCrcErr++;
        if (m_debugLevel > DebugNone) {
          m_print.print("vbus crc err fr ");
          m_print.println(f);
        }
        return;
      }

      restoreSeptet(frame, 0, FrameDataCount);

      frame += FrameSize;
    }

    /* decode 0x100 command */
    if (header->m_members.m_command == 0x100) {

      // check if we have registered decoders for the given source address
      for (PayloadDecoder **d = m_payloadDecoders; *d; d++) {
        if ((*d)->sourceAddress() == header->m_members.m_sourceAddress) {
          auto good = (*d)->decode(header, m_buffer + PayloadPos, header->m_members.m_frameCount * FrameSize);
          if (good) {
            (*d)->onDecodeSuccess(header);
          } else {
            (*d)->onDecodeFailure(header);
          }
        }
      }
    }
  }
private:
  Print &m_print;
  Stream &m_serial;
  PayloadDecoder **m_payloadDecoders;

  DebugLevel m_debugLevel;

  uint8_t m_buffer[VBUS_BUFFER_SIZE];
  int16_t m_pos;
  uint32_t m_bufferOverruns;
  uint32_t m_headerCrcErr;
  uint32_t m_frameCrcErr;
};

} // namespace VBus

#endif /* _VBUS_SLAVE_H_ */
