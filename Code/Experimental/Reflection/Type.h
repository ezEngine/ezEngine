#pragma once

#include <Foundation/Basics.h>



class ezTypeRTTI
{
public:
  enum TypeMode
  {
    Struct,
    Class
  };

  ezTypeRTTI(const char* szTypeName, const ezTypeRTTI* pParentType, TypeMode mode);

  const char* GetTypeName() const { return m_szTypeName; }
  ezInt32 GetTypeID() const { return m_iTypeID; }
  const ezTypeRTTI* GetParentRTTI() const { return m_pParentType; }
  TypeMode GetTypeMode() const { return m_TypeMode; }

  static ezInt32 GetRTTIClassID(const char* szClassName);

  template<class DERIVED>
  bool IsDerivedFrom() const
  {
    ezInt32 iTypeID = DERIVED::GetStaticRTTI()->GetTypeID();

    const ezTypeRTTI* pCur = this;

    // ID ranges, I know...
    while (pCur)
    {
      if (pCur->m_iTypeID == iTypeID)
        return true;

      pCur = pCur->GetParentRTTI();
    }

    return false;
  }

private:
  TypeMode m_TypeMode;
  const char* m_szTypeName;
  ezInt32 m_iTypeID;
  const ezTypeRTTI* m_pParentType;
};