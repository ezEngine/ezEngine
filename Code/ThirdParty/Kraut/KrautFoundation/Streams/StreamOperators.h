#ifndef AE_FOUNDATION_FILESYSTEM_STREAMOPERATORS_H
#define AE_FOUNDATION_FILESYSTEM_STREAMOPERATORS_H

#include "../Defines.h"
#include "../Math/Matrix.h"
#include "../Math/Plane.h"
#include "../Math/Quaternion.h"
#include "../Math/Vec3.h"
#include "Streams.h"
#include "../Strings/String.h"

namespace AE_NS_FOUNDATION
{
  // ******** Stream Out Operators ********

  // *** unsigned ints ***

  inline void operator<<(aeStreamOut& s, aeUInt8 data)
  {
    s.Write(&data, sizeof(data));
  }

  inline void operator<<(aeStreamOut& s, aeUInt16 data)
  {
    s.Write(&data, sizeof(data));
  }

  inline void operator<<(aeStreamOut& s, aeUInt32 data)
  {
    s.Write(&data, sizeof(data));
  }

  inline void operator<<(aeStreamOut& s, aeUInt64 data)
  {
    s.Write(&data, sizeof(data));
  }

  // *** signed ints ***

  inline void operator<<(aeStreamOut& s, aeInt8 data)
  {
    s.Write(&data, sizeof(data));
  }

  inline void operator<<(aeStreamOut& s, aeInt16 data)
  {

    s.Write(&data, sizeof(data));
  }

  inline void operator<<(aeStreamOut& s, aeInt32 data)
  {

    s.Write(&data, sizeof(data));
  }

  inline void operator<<(aeStreamOut& s, aeInt64 data)
  {

    s.Write(&data, sizeof(data));
  }

  // *** float / double ***

  inline void operator<<(aeStreamOut& s, float data)
  {

    s.Write(&data, sizeof(data));
  }

  inline void operator<<(aeStreamOut& s, double data)
  {

    s.Write(&data, sizeof(data));
  }

  // *** bool ***

  inline void operator<<(aeStreamOut& s, bool data)
  {
    s.Write(&data, sizeof(data));
  }


  // *** Strings ***

  inline void operator<<(aeStreamOut& s, const aeBasicString& data)
  {
    const aeUInt32 uiSize = data.length();
    s.Write(&uiSize, sizeof(aeUInt32));
    s.Write(data.c_str(), sizeof(char) * uiSize);
  }


  // *** Math Classes ***

  inline void operator<<(aeStreamOut& s, const aeVec3& data)
  {
    s.Write(&data.x, sizeof(float) * 3);
  }

  inline void operator<<(aeStreamOut& s, const aePlane& data)
  {
    s.Write(&data, sizeof(float) * 4);
  }

  inline void operator<<(aeStreamOut& s, const aeMatrix& data)
  {
    s.Write(data.m_fElements, sizeof(float) * 16);
  }

  inline void operator<<(aeStreamOut& s, const aeQuaternion& data)
  {
    s.Write(&data, sizeof(float) * 4);
  }

  // ******** Stream In Operators ********

  // *** unsigned ints ***

  inline void operator>>(aeStreamIn& s, aeUInt8& data)
  {
    s.Read(&data, sizeof(data));
  }

  inline void operator>>(aeStreamIn& s, aeUInt16& data)
  {
    s.Read(&data, sizeof(data));
  }

  inline void operator>>(aeStreamIn& s, aeUInt32& data)
  {
    s.Read(&data, sizeof(data));
  }

  inline void operator>>(aeStreamIn& s, aeUInt64& data)
  {
    s.Read(&data, sizeof(data));
  }

  // *** signed ints ***

  inline void operator>>(aeStreamIn& s, aeInt8& data)
  {
    s.Read(&data, sizeof(data));
  }

  inline void operator>>(aeStreamIn& s, aeInt16& data)
  {
    s.Read(&data, sizeof(data));
  }

  inline void operator>>(aeStreamIn& s, aeInt32& data)
  {
    s.Read(&data, sizeof(data));
  }

  inline void operator>>(aeStreamIn& s, aeInt64& data)
  {
    s.Read(&data, sizeof(data));
  }

  // *** float / double ***

  inline void operator>>(aeStreamIn& s, float& data)
  {
    s.Read(&data, sizeof(data));
  }

  inline void operator>>(aeStreamIn& s, double& data)
  {
    s.Read(&data, sizeof(data));
  }

  // *** bool ***

  inline void operator>>(aeStreamIn& s, bool& data)
  {
    s.Read(&data, sizeof(data));
  }


  // *** Strings ***

  inline void operator>>(aeStreamIn& s, aeString& data)
  {
    aeUInt32 uiSize;
    s.Read(&uiSize, sizeof(aeUInt32));

    data.resize(uiSize);

    if (uiSize > 0)
      s.Read(&data[0], sizeof(char) * uiSize);
  }


  // *** Math Classes ***

  inline void operator>>(aeStreamIn& s, aeVec3& data)
  {
    s.Read(&data.x, sizeof(float) * 3);
  }

  inline void operator>>(aeStreamIn& s, aePlane& data)
  {
    s.Read(&data, sizeof(float) * 4);
  }

  inline void operator>>(aeStreamIn& s, aeMatrix& data)
  {
    s.Read(data.m_fElements, sizeof(float) * 16);
  }

  inline void operator>>(aeStreamIn& s, aeQuaternion& data)
  {
    s.Read(&data, sizeof(float) * 4);
  }

} // namespace AE_NS_FOUNDATION

#endif
