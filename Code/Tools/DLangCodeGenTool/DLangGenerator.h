#pragma once

#include "CppStructure.h"
#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

class ezFileWriter;
class CppStructure;

enum class TargetType
{
  None,
  Value,
  Reference,
  Union,
  InterfaceStruct,
};

class DLangGenerator
{
public:
  DLangGenerator();
  ~DLangGenerator();

  void SetStructure(const CppStructure& structure);

  void ClearOutput();
  const ezStringView GetOutput() const { return m_sOutput.GetView(); }

  ezResult GenerateStructure(const char* szClassName, TargetType targetType, bool bWithSurroundings = true);
  ezResult GenerateEnum(const char* szEnumName);

  void BeginStructure(const CE_Structure& ce, TargetType targetType, bool bWithSurroundings);
  void EndStructure(const CE_Structure& ce, bool bWithSurroundings);

  ezResult WriteMembers(const CE_Structure& ce);
  ezResult WriteMethod(const CE_Method& ce);
  ezResult WriteConstructor(const CE_Constructor& ce);
  ezResult WriteField(const CE_Field& ce);
  ezResult WriteEnum(const CE_Enumeration& ce);
  ezResult WriteVariable(const CE_Variable& ce);
  ezResult WriteTypedef(const CE_Typedef& ce);

  ezResult BuildType(ezStringBuilder& out, const ezString& sTypeID, bool* bIsValueType = nullptr) const;

  void WriteVisibility(CE_Visibility vis);

  CE_Visibility m_lastVisibility = CE_Visibility::None;

  void Indent() { m_iIndentation += 1; }
  void Unindent() { m_iIndentation -= 1; }

  void WriteIndentation(ezInt32 iIndentOffset = 0);
  void EndLine();
  void WriteIndentedLine(const ezFormatString& fmt, ezInt32 iIndentOffset = 0);

  void WhitelistType(const char* szTypeName, TargetType type);
  bool IsTypeWhitelisted(const ezString& type, bool* bIsValueType) const;

  static bool IsAllowedDefaultArg(ezString& arg);

protected:
  const CppStructure* m_pStructure = nullptr;

  ezStringBuilder m_sOutput;
  int m_iIndentation = 0;
  int m_iSubStructure = 0;

  TargetType m_TargetType = TargetType::None;
  ezMap<ezString, TargetType> m_WhitelistedTypes;
};
