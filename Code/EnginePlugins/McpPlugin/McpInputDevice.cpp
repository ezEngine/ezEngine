#include <McpPlugin/McpPluginPCH.h>

#include <McpPlugin/McpInputDevice.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpInputDevice, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMcpInputDevice* ezMcpInputDevice::s_pInstance = nullptr;

ezMcpInputDevice::ezMcpInputDevice()
{
  // ezInputDevice is an ezEnumerable, so constructing this is all it takes for the input manager to
  // start asking it for values. There is no registration call.
  s_pInstance = this;

  // An agent moving the mouse to a specific spot (e.g. to click a UI element) means that position
  // literally, not 'whichever is bigger, mine or the real cursor's' - see m_bOverridesAbsoluteInput.
  m_bOverridesAbsoluteInput = true;
}

ezMcpInputDevice::~ezMcpInputDevice()
{
  if (s_pInstance == this)
  {
    s_pInstance = nullptr;
  }
}

void ezMcpInputDevice::SetSlotValue(ezStringView sSlot, float fValue, ezUInt32 uiFrames)
{
  const ezString sKey = sSlot;

  m_InputSlotValues[sKey] = fValue;

  if (uiFrames > 0)
  {
    m_RemainingFrames[sKey] = uiFrames;
  }
  else
  {
    // switching a timed hold back to an indefinite one
    m_RemainingFrames.Remove(sKey);
  }
}

void ezMcpInputDevice::ClearSlot(ezStringView sSlot)
{
  const ezString sKey = sSlot;

  // Erased rather than set to zero: a zero written by this device still takes part in the Max merge,
  // which is harmless, but leaving it in would report the slot as held by GetHeldSlots().
  m_InputSlotValues.Remove(sKey);
  m_RemainingFrames.Remove(sKey);
}

void ezMcpInputDevice::ClearAllSlots()
{
  m_InputSlotValues.Clear();
  m_RemainingFrames.Clear();
}

void ezMcpInputDevice::QueueText(ezStringView sText)
{
  m_sLastCharacters.Append(sText);
}

void ezMcpInputDevice::GetHeldSlots(ezDynamicArray<HeldSlot>& out_slots) const
{
  for (auto it = m_InputSlotValues.GetIterator(); it.IsValid(); ++it)
  {
    HeldSlot& slot = out_slots.ExpandAndGetRef();
    slot.m_sSlot = it.Key();
    slot.m_fValue = it.Value();

    if (const ezUInt32* pFrames = m_RemainingFrames.GetValue(it.Key()))
    {
      slot.m_uiFramesRemaining = *pFrames;
    }
  }
}

void ezMcpInputDevice::UpdateInputSlotValues()
{
  // Called once per input update, which is what makes it the place to count frames down. The values
  // themselves need no work - they are already in m_InputSlotValues and stay there until changed.
  for (auto it = m_RemainingFrames.GetIterator(); it.IsValid();)
  {
    ezUInt32& uiRemaining = it.Value();

    if (uiRemaining > 1)
    {
      --uiRemaining;
      ++it;
      continue;
    }

    // The last frame of the hold has now been delivered, so stop writing the slot. Removing during
    // iteration is why the iterator is advanced by Remove() rather than by ++it here.
    m_InputSlotValues.Remove(it.Key());
    it = m_RemainingFrames.Remove(it);
  }
}
