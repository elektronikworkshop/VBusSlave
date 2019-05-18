#pragma once

#include <Arduino.h>
#include <VBusHeader.h>

namespace VBus {

typedef uint16_t VBusAddress;
typedef uint16_t VBusCommand;


class PayloadDecoder
{
public:
  PayloadDecoder(VBusAddress sourceAddress)
    : m_sourceAddress(sourceAddress)
  {}
  VBusAddress sourceAddress() const
  {
    return m_sourceAddress;
  }
  virtual bool decode(const Header *header, uint8_t *buffer, size_t size) = 0;
  virtual void onDecodeSuccess(const Header *) {}
  virtual void onDecodeFailure(const Header *) {}
private:
  VBusAddress m_sourceAddress;
};

/*
 * # DeltaSol C 0x4212
 * #  Offset
 * #  |  Size
 * #  |  |  Mask
 * #  |  |  |  Name
 * #  |  |  |  |                  Factor Unit  unpack code
 * #  0  2     Temperature S1     0.1    C     h
 * #  2  2     Temperature S2     0.1    C     h
 * #  4  2     Temperature S3     0.1    C     h
 * #  6  2     Temperature S4     0.1    C     h
 * #  8  1     Pump speed R1        1    %     B
 * #  9  1     Pump speed R2        1    %     B
 * # 10  1     Error mask           1          B
 * # 11  1     Variant              1          B
 * # 12  2     Operating hours R1   1    h     H
 * # 14  2     Operating hours R2   1    h     H
 * # 16  2     Heat quantity        1    Wh    H
 * # 18  2     Heat quantity     1000    Wh    H
 * # 20  2     Heat quantity  1000000    Wh    H
 * # 22  2     System time          1          H
 */
class PayloadDeltaSolC
  : public PayloadDecoder
{
public:
  PayloadDeltaSolC()
    : PayloadDecoder(0x4212)
  {}
  union DeltaSolCPayload
  {
  public:
    typedef struct __attribute((packed)) {
      uint8_t m_septet;
      uint8_t m_checksum;
    } FrameMetaData;
    struct __attribute((packed)) Members {
      int16_t m_tempS1;
      int16_t m_tempS2;
      FrameMetaData m_meta0;
      int16_t m_tempS3;
      int16_t m_tempS4;
      FrameMetaData m_meta1;
      uint8_t m_pumpSpeedR1;
      uint8_t m_pumpSpeedR2;
      uint8_t m_errorMask;
      uint8_t m_variant;
      FrameMetaData m_meta2;
      uint16_t m_operatingHoursR1;
      uint16_t m_operatingHoursR2;
      FrameMetaData m_meta3;
      uint16_t m_heatQuantity1;
      uint16_t m_heatQuantity1k;
      FrameMetaData m_meta4;
      uint16_t m_heatQuantity1M;
      uint16_t m_systemTime;
      FrameMetaData m_meta5;
      uint8_t m_reserved[4];
      FrameMetaData m_meta6;
      bool init()
      {
        /*
        m_tempS1 = byteswap(m_tempS1);
        m_tempS2 = byteswap(m_tempS2);
        m_tempS3 = byteswap(m_tempS3);
        m_tempS4 = byteswap(m_tempS4);
        m_operatingHoursR1 = byteswap(m_operatingHoursR1);
        m_operatingHoursR2 = byteswap(m_operatingHoursR2);
        m_heatQuantity1 = byteswap(m_heatQuantity1);
        m_heatQuantity1k = byteswap(m_heatQuantity1k);
        m_heatQuantity1M = byteswap(m_heatQuantity1M);
        m_systemTime = byteswap(m_systemTime);
        */
        return true;
      }
    } m_members;
    uint8_t m_buffer[sizeof(Members)];
    bool init()
    {
      return m_members.init();
    }
  };
  virtual bool decode(const Header * /*header*/,
                      uint8_t *buffer,
                      size_t size) final
  {
    if (size > sizeof(DeltaSolCPayload)) {
      return false;
    }

    m_tmpPayload = reinterpret_cast<DeltaSolCPayload*>(buffer);

    return m_tmpPayload->init();
  }
protected:
  const DeltaSolCPayload * getPayload() const
  {
    return m_tmpPayload;
  }
private:
  DeltaSolCPayload *m_tmpPayload;
};

} // namespace VBus
