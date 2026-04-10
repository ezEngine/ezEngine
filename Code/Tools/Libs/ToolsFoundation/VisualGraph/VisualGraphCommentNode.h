#pragma once

#include <Foundation/Math/Vec2.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// A comment box that can be placed in any visual graph to annotate groups of nodes.
///
/// Comments are treated as special nodes by ezVisualGraphObjectManager: they participate in
/// position tracking and serialization but have no pins and cannot be connected.
/// Their size and color are document properties editable through the property panel.
class EZ_TOOLSFOUNDATION_DLL ezVisualGraphComment : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualGraphComment, ezReflectedClass);

public:
  ezString m_sComment = "Comment";
  ezVec2 m_vSize = ezVec2(300, 200);
  ezColorGammaUB m_Color = ezColorGammaUB(70, 70, 70, 200);
};
