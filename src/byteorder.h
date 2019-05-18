#ifndef _BYTEORDER_H_
#define _BYTEORDER_H_

// type generic template with extra functions like from big endian... with built in ifdefs

/** In place byte swapping
 *  
 */
template<typename T> void byteswapi(T& value)
{
  uint8_t *data= reinterpret_cast<uint8_t*>(&value);
  for (uint8_t i = 0, j = sizeof(T)-1; i < sizeof(T)/2; i++, j--) {
    uint8_t tmp = data[i];
     data[i] = data[j];
     data[j] = tmp;
  }
}
template<typename T> T byteswap(T value)
{
  byteswap(value);
  return value;
}

#endif // _BYTEORDER_H_

