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

  /// The bindings that the project ships with.
  static constexpr const ezStringView s_sConfigFile = ":project/RuntimeConfigs/InputConfig.ddl"_ezsv;

  /// The bindings that the user changed at runtime, see ezInputRebinding.
  ///
  /// Only exists once something was rebound, and may only contain some actions, so it is applied on top of s_sConfigFile.
  static constexpr const ezStringView s_sUserConfigFile = ":appdata/RuntimeConfigs/InputConfig.ddl"_ezsv;

  ezGameAppInputConfig();

  void Apply() const;
  void WriteToDDL(ezOpenDdlWriter& ref_writer) const;
  void ReadFromDDL(const ezOpenDdlReaderElement* pAction);

  static void ApplyAll(const ezArrayPtr<ezGameAppInputConfig>& actions);

  /// Reads the actions from the given file and applies them to ezInputManager.
  ///
  /// Returns EZ_FAILURE if the file can't be opened, which is expected for s_sUserConfigFile when nothing was rebound yet.
  static ezResult ApplyFile(ezStringView sFile);
  static void WriteToDDL(ezStreamWriter& inout_stream, const ezArrayPtr<ezGameAppInputConfig>& actions);
  static void ReadFromDDL(ezStreamReader& inout_stream, ezDynamicArray<ezGameAppInputConfig>& out_actions);

  ezString m_sInputSet;
  ezString m_sInputAction;

  ezString m_sInputSlotTrigger[MaxInputSlotAlternatives];

  float m_fInputSlotScale[MaxInputSlotAlternatives];

  bool m_bApplyTimeScaling = true;
};
