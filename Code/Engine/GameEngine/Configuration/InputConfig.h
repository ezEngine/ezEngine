#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

class ezOpenDdlWriter;
class ezOpenDdlReaderElement;

class EZ_GAMEENGINE_DLL ezGameAppInputConfig
{
public:
  constexpr static ezUInt32 MaxInputSlotAlternatives = 3;

  ezGameAppInputConfig();

  void Apply() const;
  void WriteToDDL(ezOpenDdlWriter& writer) const;
  void ReadFromDDL(const ezOpenDdlReaderElement* pAction);

  static void ApplyAll(const ezArrayPtr<ezGameAppInputConfig>& actions);
  static void WriteToDDL(ezStreamWriter& stream, const ezArrayPtr<ezGameAppInputConfig>& actions);
  static void ReadFromDDL(ezStreamReader& stream, ezHybridArray<ezGameAppInputConfig, 32>& out_actions);

  ezString m_sInputSet;
  ezString m_sInputAction;

  ezString m_sInputSlotTrigger[MaxInputSlotAlternatives];

  float m_fInputSlotScale[MaxInputSlotAlternatives];

  bool m_bApplyTimeScaling = true;
};
