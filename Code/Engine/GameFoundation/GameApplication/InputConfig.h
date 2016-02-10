#pragma once

#include <GameFoundation/Basics.h>
#include <Foundation/Strings/String.h>

class ezStandardJSONWriter;

class EZ_GAMEFOUNDATION_DLL ezGameAppInputConfig
{
public:
  enum { MaxInputSlotAlternatives = 3 };

  ezGameAppInputConfig();

  void Apply() const;
  void WriteToJson( ezStandardJSONWriter &json ) const;
  void ReadFromJson( const ezVariantDictionary& action );

  static void ApplyAll( const ezArrayPtr<ezGameAppInputConfig>& actions );
  static void WriteToJson(ezStreamWriter& stream, const ezArrayPtr<ezGameAppInputConfig>& actions);
  static void ReadFromJson( ezStreamReader& stream, ezHybridArray<ezGameAppInputConfig, 32>& out_actions );



  ezString m_sInputSet;
  ezString m_sInputAction;

  ezString m_sInputSlotTrigger[MaxInputSlotAlternatives];

  float m_fInputSlotScale[MaxInputSlotAlternatives];

  bool m_bApplyTimeScaling;
};