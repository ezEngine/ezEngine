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

  static constexpr const ezStringView s_sConfigFile = ":project/RuntimeConfigs/InputConfig.ddl"_ezsv;

  ezGameAppInputConfig();

  void Apply() const;
  void WriteToDDL(ezOpenDdlWriter& ref_writer) const;
  void ReadFromDDL(const ezOpenDdlReaderElement* pAction);

  static void ApplyAll(const ezArrayPtr<ezGameAppInputConfig>& actions);
  static void WriteToDDL(ezStreamWriter& inout_stream, const ezArrayPtr<ezGameAppInputConfig>& actions);
  static void ReadFromDDL(ezStreamReader& inout_stream, ezDynamicArray<ezGameAppInputConfig>& out_actions);

  ezString m_sInputSet;
  ezString m_sInputAction;

  ezString m_sInputSlotTrigger[MaxInputSlotAlternatives];

  float m_fInputSlotScale[MaxInputSlotAlternatives];

  bool m_bApplyTimeScaling = true;
};
