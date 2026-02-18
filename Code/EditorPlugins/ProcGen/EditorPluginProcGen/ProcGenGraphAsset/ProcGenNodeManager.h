#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Connection.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>

/// Visual graph pin for procedural generation nodes.
///
/// Basic pin implementation for procedural generation graphs without additional metadata.
class ezProcGenPin : public ezVisualGraphPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenPin, ezVisualGraphPin);

public:
  using ezVisualGraphPin::ezVisualGraphPin;
};

/// Object manager for procedural generation graphs.
///
/// Manages nodes and connections for procedural generation systems, such as terrain generation or placement rules.
/// Validates connections between different types of procedural generation nodes.
class ezProcGenNodeManager : public ezVisualGraphObjectManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const override;

  virtual ezStatus InternalCanConnect(const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& out_result) const override;
};
