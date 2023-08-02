
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Types/Status.h>

class ezLogInterface;
class ezStreamReader;

/// \brief Simple implementation to read Adobe CUBE LUT files
///
/// Currently only reads 3D LUTs as this is the data we need for our lookup textures in the tone mapping step.
class ezAdobeCUBEReader
{
public:
  ezAdobeCUBEReader();
  ~ezAdobeCUBEReader();

  ezStatus ParseFile(ezStreamReader& inout_stream, ezLogInterface* pLog = nullptr);

  ezVec3 GetDomainMin() const;
  ezVec3 GetDomainMax() const;

  ezUInt32 GetLUTSize() const;
  const ezString& GetTitle() const;

  ezVec3 GetLUTEntry(ezUInt32 r, ezUInt32 g, ezUInt32 b) const;

protected:
  ezUInt32 m_uiLUTSize = 0;
  ezString m_sTitle = "<UNTITLED>";

  ezVec3 m_vDomainMin = ezVec3::ZeroVector();
  ezVec3 m_vDomainMax = ezVec3(1.0f);

  ezDynamicArray<ezVec3> m_LUTValues;

  ezUInt32 GetLUTIndex(ezUInt32 r, ezUInt32 g, ezUInt32 b) const;
};
