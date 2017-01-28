#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Basics.h>

class ezNode;

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
    m_uiInputIndex = 0xFF;
    m_uiOutputIndex = 0xFF;
    m_pParent = nullptr;
  }

  ezEnum<Type> m_Type;
  ezUInt8 m_uiInputIndex;
  ezUInt8 m_uiOutputIndex;
  ezNode* m_pParent;
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

class EZ_FOUNDATION_DLL ezNode : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezNode, ezReflectedClass);

public:
  virtual ~ezNode() { }

  void InitializePins();

  ezHashedString GetPinName(const ezNodePin* pPin) const;
  const ezNodePin* GetPinByName(const char* szName) const;
  const ezNodePin* GetPinByName(ezHashedString sName) const;
  const ezArrayPtr<const ezNodePin* const> GetInputPins() const { return m_InputPins; }
  const ezArrayPtr<const ezNodePin* const> GetOutputPins() const { return m_OutputPins; }

private:
  ezDynamicArray<const ezNodePin*> m_InputPins;
  ezDynamicArray<const ezNodePin*> m_OutputPins;
  ezHashTable<ezHashedString, const ezNodePin*> m_NameToPin;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezNodePin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezInputNodePin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezOutputNodePin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezPassThroughNodePin);
