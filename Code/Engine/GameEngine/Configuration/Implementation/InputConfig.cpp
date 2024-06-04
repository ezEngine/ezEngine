#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GameEngine/Configuration/InputConfig.h>

static_assert(ezGameAppInputConfig::MaxInputSlotAlternatives == ezInputActionConfig::MaxInputSlotAlternatives, "Max values should be kept in sync");

ezGameAppInputConfig::ezGameAppInputConfig()
{
  for (ezUInt16 i = 0; i < MaxInputSlotAlternatives; ++i)
  {
    m_fInputSlotScale[i] = 1.0f;
    m_sInputSlotTrigger[i] = ezInputSlot_None;
  }
}

void ezGameAppInputConfig::Apply() const
{
  ezInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = m_bApplyTimeScaling;

  for (ezUInt32 i = 0; i < MaxInputSlotAlternatives; ++i)
  {
    cfg.m_sInputSlotTrigger[i] = m_sInputSlotTrigger[i];
    cfg.m_fInputSlotScale[i] = m_fInputSlotScale[i];
  }

  ezInputManager::SetInputActionConfig(m_sInputSet, m_sInputAction, cfg, true);
}

void ezGameAppInputConfig::WriteToDDL(ezStreamWriter& inout_stream, const ezArrayPtr<ezGameAppInputConfig>& actions)
{
  ezOpenDdlWriter writer;
  writer.SetCompactMode(false);
  writer.SetFloatPrecisionMode(ezOpenDdlWriter::FloatPrecisionMode::Readable);
  writer.SetPrimitiveTypeStringMode(ezOpenDdlWriter::TypeStringMode::Compliant);
  writer.SetOutputStream(&inout_stream);

  for (const ezGameAppInputConfig& config : actions)
  {
    config.WriteToDDL(writer);
  }
}

void ezGameAppInputConfig::WriteToDDL(ezOpenDdlWriter& ref_writer) const
{
  ref_writer.BeginObject("InputAction");
  {
    ezOpenDdlUtils::StoreString(ref_writer, m_sInputSet, "Set");
    ezOpenDdlUtils::StoreString(ref_writer, m_sInputAction, "Action");
    ezOpenDdlUtils::StoreBool(ref_writer, m_bApplyTimeScaling, "TimeScale");

    for (int i = 0; i < 3; ++i)
    {
      if (!m_sInputSlotTrigger[i].IsEmpty())
      {
        ref_writer.BeginObject("Slot");
        {
          ezOpenDdlUtils::StoreString(ref_writer, m_sInputSlotTrigger[i], "Key");
          ezOpenDdlUtils::StoreFloat(ref_writer, m_fInputSlotScale[i], "Scale");
        }
        ref_writer.EndObject();
      }
    }
  }
  ref_writer.EndObject();
}

void ezGameAppInputConfig::ReadFromDDL(ezStreamReader& inout_stream, ezHybridArray<ezGameAppInputConfig, 32>& out_actions)
{
  ezOpenDdlReader reader;

  if (reader.ParseDocument(inout_stream, 0, ezLog::GetThreadLocalLogSystem()).Failed())
    return;

  const ezOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const ezOpenDdlReaderElement* pAction = pRoot->GetFirstChild(); pAction != nullptr; pAction = pAction->GetSibling())
  {
    if (!pAction->IsCustomType("InputAction"))
      continue;

    ezGameAppInputConfig& cfg = out_actions.ExpandAndGetRef();

    cfg.ReadFromDDL(pAction);
  }
}

void ezGameAppInputConfig::ReadFromDDL(const ezOpenDdlReaderElement* pInput)
{
  const ezOpenDdlReaderElement* pSet = pInput->FindChildOfType(ezOpenDdlPrimitiveType::String, "Set");
  const ezOpenDdlReaderElement* pAction = pInput->FindChildOfType(ezOpenDdlPrimitiveType::String, "Action");
  const ezOpenDdlReaderElement* pTimeScale = pInput->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "TimeScale");


  if (pSet)
    m_sInputSet = pSet->GetPrimitivesString()[0];

  if (pAction)
    m_sInputAction = pAction->GetPrimitivesString()[0];

  if (pTimeScale)
    m_bApplyTimeScaling = pTimeScale->GetPrimitivesBool()[0];

  ezInt32 iSlot = 0;
  for (const ezOpenDdlReaderElement* pSlot = pInput->GetFirstChild(); pSlot != nullptr; pSlot = pSlot->GetSibling())
  {
    if (!pSlot->IsCustomType("Slot"))
      continue;

    const ezOpenDdlReaderElement* pKey = pSlot->FindChildOfType(ezOpenDdlPrimitiveType::String, "Key");
    const ezOpenDdlReaderElement* pScale = pSlot->FindChildOfType(ezOpenDdlPrimitiveType::Float, "Scale");

    if (pKey)
      m_sInputSlotTrigger[iSlot] = pKey->GetPrimitivesString()[0];

    if (pScale)
      m_fInputSlotScale[iSlot] = pScale->GetPrimitivesFloat()[0];

    ++iSlot;

    if (iSlot >= MaxInputSlotAlternatives)
      break;
  }
}

void ezGameAppInputConfig::ApplyAll(const ezArrayPtr<ezGameAppInputConfig>& actions)
{
  for (const ezGameAppInputConfig& config : actions)
  {
    config.Apply();
  }
}
