#include <GameFoundation/PCH.h>
#include <GameFoundation/GameApplication/InputConfig.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Logging/Log.h>
#include <Core/Input/InputManager.h>

EZ_CHECK_AT_COMPILETIME_MSG(ezGameAppInputConfig::MaxInputSlotAlternatives == ezInputActionConfig::MaxInputSlotAlternatives, "Max values should be kept in sync");

ezGameAppInputConfig::ezGameAppInputConfig()
{
  for (ezUInt16 i = 0; i < MaxInputSlotAlternatives; ++i)
  {
    m_fInputSlotScale[i] = 1.0f;
    m_sInputSlotTrigger[i] = ezInputSlot_None;
  }

  m_bApplyTimeScaling = true;
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

  ezInputManager::SetInputActionConfig( m_sInputSet, m_sInputAction, cfg, true );
}

void ezGameAppInputConfig::WriteToJson(ezStreamWriter& stream, const ezArrayPtr<ezGameAppInputConfig>& actions)
{
  ezStandardJSONWriter json;
  json.SetOutputStream(&stream);

  json.BeginObject();
  json.BeginArray("InputActions");

  for (const ezGameAppInputConfig& config : actions)
  {
	  config.WriteToJson( json );
  }

  json.EndArray();
  json.EndObject();
}


void ezGameAppInputConfig::WriteToJson( ezStandardJSONWriter& json ) const
{
	json.BeginObject();
	{
		json.AddVariableString( "Set", m_sInputSet );
		json.AddVariableString( "Action", m_sInputAction );
		json.AddVariableBool( "TimeScale", m_bApplyTimeScaling );
		json.BeginArray( "Slots" );
		{
			for ( int i = 0; i < 3; ++i )
			{
				if ( !m_sInputSlotTrigger[i].IsEmpty() )
				{
					json.BeginObject();
					{
						json.AddVariableString( "Key", m_sInputSlotTrigger[i] );
						json.AddVariableFloat( "Scale", m_fInputSlotScale[i] );
					}
					json.EndObject();
				}
			}
		}
		json.EndArray();
	}
	json.EndObject();
}

void ezGameAppInputConfig::ReadFromJson( ezStreamReader& stream, ezHybridArray<ezGameAppInputConfig, 32>& out_actions )
{
  ezJSONReader json;
  json.SetLogInterface(ezGlobalLog::GetInstance());
  if (json.Parse(stream).Failed())
    return;

  const ezVariantDictionary& root = json.GetTopLevelObject();

  ezVariant* inputActions;
  if (!root.TryGetValue("InputActions", inputActions))
    return;

  if (!inputActions->IsA<ezVariantArray>())
    return;

  const ezVariantArray& inputActionsArray = inputActions->Get<ezVariantArray>();

  for (ezUInt32 i = 0; i < inputActionsArray.GetCount(); ++i)
  {
    if (!inputActionsArray[i].IsA<ezVariantDictionary>())
      continue;

	const ezVariantDictionary& action = inputActionsArray[i].Get<ezVariantDictionary>();

	ezGameAppInputConfig& cfg = out_actions.ExpandAndGetRef();

	cfg.ReadFromJson( action );
  }
}

void ezGameAppInputConfig::ReadFromJson( const ezVariantDictionary& action )
{
	ezVariant* pVar;

	if ( action.TryGetValue( "Set", pVar ) && pVar->IsA<ezString>() )
		m_sInputSet = pVar->ConvertTo<ezString>();

	if ( action.TryGetValue( "Action", pVar ) && pVar->IsA<ezString>() )
		m_sInputAction = pVar->ConvertTo<ezString>();

	if ( action.TryGetValue( "TimeScale", pVar ) && pVar->IsA<bool>() )
		m_bApplyTimeScaling = pVar->ConvertTo<bool>();

	if ( !action.TryGetValue( "Slots", pVar ) || !pVar->IsA<ezVariantArray>() )
		return;

	const ezVariantArray& slotsArray = pVar->Get<ezVariantArray>();

	for ( ezUInt32 slot = 0; slot < slotsArray.GetCount() && slot < MaxInputSlotAlternatives; ++slot )
	{
		if ( !slotsArray[slot].IsA<ezVariantDictionary>() )
			continue;

		const ezVariantDictionary& slotValue = slotsArray[slot].Get<ezVariantDictionary>();

		if ( slotValue.TryGetValue( "Key", pVar ) && pVar->IsA<ezString>() )
			m_sInputSlotTrigger[slot] = pVar->ConvertTo<ezString>();

		if ( slotValue.TryGetValue( "Scale", pVar ) && pVar->IsA<float>() )
			m_fInputSlotScale[slot] = pVar->ConvertTo<float>();
	}
}

void ezGameAppInputConfig::ApplyAll( const ezArrayPtr<ezGameAppInputConfig>& actions )
{
	for ( const ezGameAppInputConfig& config : actions )
	{
		config.Apply();
	}
}
