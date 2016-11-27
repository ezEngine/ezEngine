#pragma once

#include <GameFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>

class ezOpenDdlWriter;
class ezOpenDdlReaderElement;

class EZ_GAMEFOUNDATION_DLL ezGameAppInputConfig
{
public:
  enum { MaxInputSlotAlternatives = 3 };

  ezGameAppInputConfig();

  void Apply() const;
  void WriteToDDL(ezOpenDdlWriter &writer) const;
  void ReadFromDDL(const ezOpenDdlReaderElement* pAction);

  static void ApplyAll(const ezArrayPtr<ezGameAppInputConfig>& actions);
  static void WriteToDDL(ezStreamWriter& stream, const ezArrayPtr<ezGameAppInputConfig>& actions);
  static void ReadFromDDL(ezStreamReader& stream, ezHybridArray<ezGameAppInputConfig, 32>& out_actions);



  ezString m_sInputSet;
  ezString m_sInputAction;

  ezString m_sInputSlotTrigger[MaxInputSlotAlternatives];

  float m_fInputSlotScale[MaxInputSlotAlternatives];

  bool m_bApplyTimeScaling;
};
