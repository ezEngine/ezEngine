#pragma once

#include <Foundation/Memory/MemoryUtils.h>

/// A simple variant type that can hold an int, bool, const char*, void* or float.
class ezVariant
{
public:

  struct Type
  {
    enum Enum
    {
      Invalid,
      Int,
      Bool,
      ConstCharPtr,
      VoidPtr,
      Float,
    };
  };

  ezVariant()                       { m_Type = Type::Invalid;                               }
  ezVariant(ezInt32 val)            { m_Type = Type::Int;            m_Int           = val; }
  ezVariant(bool val)               { m_Type = Type::Bool;           m_Bool          = val; }
  ezVariant(const char* val)        { m_Type = Type::ConstCharPtr;   m_ConstCharPtr  = val; }
  ezVariant(void* val)              { m_Type = Type::VoidPtr;        m_VoidPtr       = val; }
  ezVariant(float val)              { m_Type = Type::Float;          m_Float         = val; }

  void operator= (ezInt32 val)      { m_Type = Type::Int;            m_Int           = val; }
  void operator= (bool val)         { m_Type = Type::Bool;           m_Bool          = val; }
  void operator= (const char* val)  { m_Type = Type::ConstCharPtr;   m_ConstCharPtr  = val; }
  void operator= (void* val)        { m_Type = Type::VoidPtr;        m_VoidPtr       = val; }
  void operator= (float val)        { m_Type = Type::Float;          m_Float         = val; }

  Type::Enum GetType() const { return m_Type; }

  ezInt32 GetInt() const              { EZ_ASSERT(m_Type == Type::Int,          "Incorrect Variant Type."); return m_Int;           }
  bool GetBool() const                { EZ_ASSERT(m_Type == Type::Bool,         "Incorrect Variant Type."); return m_Bool;          }
  const char* GetConstCharPtr() const { EZ_ASSERT(m_Type == Type::ConstCharPtr, "Incorrect Variant Type."); return m_ConstCharPtr;  }
  void* GetVoidPtr() const            { EZ_ASSERT(m_Type == Type::VoidPtr,      "Incorrect Variant Type."); return m_VoidPtr;       }
  float GetFloat() const              { EZ_ASSERT(m_Type == Type::Float,        "Incorrect Variant Type."); return m_Float;         }

private:

  Type::Enum m_Type;

  union
  {
    ezInt32 m_Int;
    bool m_Bool;
    const char* m_ConstCharPtr;
    void* m_VoidPtr;
    float m_Float;
  };
};



