#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Reflection/Reflection.h>
#include <CoreUtils/Basics.h>

struct ezNodePin
{
  EZ_DECLARE_POD_TYPE();

  struct Type
  {
    typedef ezUInt8 StorageType;

    enum Enum
    {
      Unknown,
      Input,
      Output,
      PassThrough,

      Default = Unknown
    };
  };

  EZ_FORCE_INLINE ezNodePin()
  {
    m_bIsConnected = false;
    m_uiInputIndex = -1;
    m_uiOutputIndex = -1;
  }

  ezEnum<Type> m_Type;
  bool m_bIsConnected;
  ezUInt8 m_uiInputIndex;
  ezUInt8 m_uiOutputIndex;
};

struct ezInputNodePin : public ezNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_FORCE_INLINE ezInputNodePin()
  {
    m_Type = Type::Input;
  }
};

struct ezOutputNodePin : public ezNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_FORCE_INLINE ezOutputNodePin()
  {
    m_Type = Type::Output;
  }
};

struct ezPassThroughNodePin : public ezNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_FORCE_INLINE ezPassThroughNodePin()
  {
    m_Type = Type::PassThrough;
  }
};

class EZ_COREUTILS_DLL ezNode : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezNode);

public:
  virtual ~ezNode() { }

  void InitializePins();

  const ezNodePin* GetPinByName(const char* szName) const;

private:
  ezDynamicArray<const ezNodePin*> m_InputPins;
  ezDynamicArray<const ezNodePin*> m_OutputPins;
  ezHashTable<ezHashedString, const ezNodePin*> m_NameToPin;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_COREUTILS_DLL, ezNodePin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_COREUTILS_DLL, ezInputNodePin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_COREUTILS_DLL, ezOutputNodePin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_COREUTILS_DLL, ezPassThroughNodePin);
