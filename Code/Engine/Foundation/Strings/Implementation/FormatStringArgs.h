#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringView.h>

class ezStringBuilder;

struct ezArgI
{
  inline ezArgI(ezInt64 value, ezUInt8 uiWidth = 1, bool bPadWithZeros = false, ezUInt8 uiBase = 10)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_uiBase(uiBase)
  {
  }

  ezInt64 m_Value;
  ezUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  ezUInt8 m_uiBase;
};

struct ezArgU
{
  inline ezArgU(ezUInt64 value, ezUInt8 uiWidth = 1, bool bPadWithZeros = false, ezUInt8 uiBase = 10, bool bUpperCase = false)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_bUpperCase(bUpperCase)
    , m_uiBase(uiBase)
  {
  }

  ezUInt64 m_Value;
  ezUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  bool m_bUpperCase;
  ezUInt8 m_uiBase;
};

struct ezArgF
{
  inline ezArgF(double value, ezInt8 iPrecision = -1, bool bScientific = false, ezUInt8 uiWidth = 1, bool bPadWithZeros = false)
    : m_Value(value)
    , m_uiWidth(uiWidth)
    , m_bPadWithZeros(bPadWithZeros)
    , m_bScientific(bScientific)
    , m_iPrecision(iPrecision)
  {
  }

  double m_Value;
  ezUInt8 m_uiWidth;
  bool m_bPadWithZeros;
  bool m_bScientific;
  ezInt8 m_iPrecision;
};

struct ezArgC
{
  inline ezArgC(char value)
    : m_Value(value)
  {
  }

  char m_Value;
};

struct ezArgP
{
  inline ezArgP(const void* value)
    : m_Value(value)
  {
  }

  const void* m_Value;
};

EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgI& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezInt64 arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezInt32 arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgU& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezUInt64 arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, ezUInt32 arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgF& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, double arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const char* arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezStringBuilder& arg);
EZ_FOUNDATION_DLL const ezStringView& BuildString(char* tmp, ezUInt32 uiLength, const ezStringView& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgC& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezArgP& arg);
