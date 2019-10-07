
import __GameObject = require("./GameObject")
export import GameObject = __GameObject.GameObject;

import __Component = require("./Component")
export import Component = __Component.Component;

export class CollectionComponent extends Component
{
}

function __TS_Create_CollectionComponent(): CollectionComponent
{
  return new CollectionComponent();
}


export class EventMessageHandlerComponent extends Component
{
}

function __TS_Create_EventMessageHandlerComponent(): EventMessageHandlerComponent
{
  return new EventMessageHandlerComponent();
}


export class SettingsComponent extends Component
{
}

function __TS_Create_SettingsComponent(): SettingsComponent
{
  return new SettingsComponent();
}


export class RenderComponent extends Component
{
}

function __TS_Create_RenderComponent(): RenderComponent
{
  return new RenderComponent();
}


export class VisualizeSkeletonComponent extends RenderComponent
{
}

function __TS_Create_VisualizeSkeletonComponent(): VisualizeSkeletonComponent
{
  return new VisualizeSkeletonComponent();
}


export class AlwaysVisibleComponent extends RenderComponent
{
}

function __TS_Create_AlwaysVisibleComponent(): AlwaysVisibleComponent
{
  return new AlwaysVisibleComponent();
}


export class CameraComponent extends Component
{
}

function __TS_Create_CameraComponent(): CameraComponent
{
  return new CameraComponent();
}


export class FogComponent extends SettingsComponent
{
}

function __TS_Create_FogComponent(): FogComponent
{
  return new FogComponent();
}


export class RenderTargetActivatorComponent extends RenderComponent
{
}

function __TS_Create_RenderTargetActivatorComponent(): RenderTargetActivatorComponent
{
  return new RenderTargetActivatorComponent();
}


export class SkyBoxComponent extends RenderComponent
{
}

function __TS_Create_SkyBoxComponent(): SkyBoxComponent
{
  return new SkyBoxComponent();
}


export class SpriteComponent extends RenderComponent
{
}

function __TS_Create_SpriteComponent(): SpriteComponent
{
  return new SpriteComponent();
}


export class DebugTextComponent extends Component
{
}

function __TS_Create_DebugTextComponent(): DebugTextComponent
{
  return new DebugTextComponent();
}


export class DecalComponent extends RenderComponent
{
}

function __TS_Create_DecalComponent(): DecalComponent
{
  return new DecalComponent();
}


export class AmbientLightComponent extends SettingsComponent
{
}

function __TS_Create_AmbientLightComponent(): AmbientLightComponent
{
  return new AmbientLightComponent();
}


export class LightComponent extends RenderComponent
{
}

function __TS_Create_LightComponent(): LightComponent
{
  return new LightComponent();
}


export class DirectionalLightComponent extends LightComponent
{
}

function __TS_Create_DirectionalLightComponent(): DirectionalLightComponent
{
  return new DirectionalLightComponent();
}


export class PointLightComponent extends LightComponent
{
}

function __TS_Create_PointLightComponent(): PointLightComponent
{
  return new PointLightComponent();
}


export class SkyLightComponent extends SettingsComponent
{
}

function __TS_Create_SkyLightComponent(): SkyLightComponent
{
  return new SkyLightComponent();
}


export class SpotLightComponent extends LightComponent
{
}

function __TS_Create_SpotLightComponent(): SpotLightComponent
{
  return new SpotLightComponent();
}


export class MeshComponentBase extends RenderComponent
{
}

function __TS_Create_MeshComponentBase(): MeshComponentBase
{
  return new MeshComponentBase();
}


export class InstancedMeshComponent extends MeshComponentBase
{
}

function __TS_Create_InstancedMeshComponent(): InstancedMeshComponent
{
  return new InstancedMeshComponent();
}


export class MeshComponent extends MeshComponentBase
{
}

function __TS_Create_MeshComponent(): MeshComponent
{
  return new MeshComponent();
}


export class SkinnedMeshComponent extends MeshComponentBase
{
}

function __TS_Create_SkinnedMeshComponent(): SkinnedMeshComponent
{
  return new SkinnedMeshComponent();
}


export class AgentSteeringComponent extends Component
{
}

function __TS_Create_AgentSteeringComponent(): AgentSteeringComponent
{
  return new AgentSteeringComponent();
}


export class NpcComponent extends Component
{
}

function __TS_Create_NpcComponent(): NpcComponent
{
  return new NpcComponent();
}


export class ColorAnimationComponent extends Component
{
}

function __TS_Create_ColorAnimationComponent(): ColorAnimationComponent
{
  return new ColorAnimationComponent();
}


export class MaterialAnimComponent extends Component
{
}

function __TS_Create_MaterialAnimComponent(): MaterialAnimComponent
{
  return new MaterialAnimComponent();
}


export class PropertyAnimComponent extends Component
{
}

function __TS_Create_PropertyAnimComponent(): PropertyAnimComponent
{
  return new PropertyAnimComponent();
}


export class TransformComponent extends Component
{
}

function __TS_Create_TransformComponent(): TransformComponent
{
  return new TransformComponent();
}


export class RotorComponent extends TransformComponent
{
}

function __TS_Create_RotorComponent(): RotorComponent
{
  return new RotorComponent();
}


export class SliderComponent extends TransformComponent
{
}

function __TS_Create_SliderComponent(): SliderComponent
{
  return new SliderComponent();
}


export class JointAttachmentComponent extends Component
{
}

function __TS_Create_JointAttachmentComponent(): JointAttachmentComponent
{
  return new JointAttachmentComponent();
}


export class LineToComponent extends Component
{
}

function __TS_Create_LineToComponent(): LineToComponent
{
  return new LineToComponent();
}


export class SimpleWindComponent extends Component
{
}

function __TS_Create_SimpleWindComponent(): SimpleWindComponent
{
  return new SimpleWindComponent();
}


export class AreaDamageComponent extends Component
{
}

function __TS_Create_AreaDamageComponent(): AreaDamageComponent
{
  return new AreaDamageComponent();
}


export class GreyBoxComponent extends RenderComponent
{
}

function __TS_Create_GreyBoxComponent(): GreyBoxComponent
{
  return new GreyBoxComponent();
}


export class HeadBoneComponent extends Component
{
}

function __TS_Create_HeadBoneComponent(): HeadBoneComponent
{
  return new HeadBoneComponent();
}


export class InputComponent extends Component
{
}

function __TS_Create_InputComponent(): InputComponent
{
  return new InputComponent();
}


export class PlayerStartPointComponent extends Component
{
}

function __TS_Create_PlayerStartPointComponent(): PlayerStartPointComponent
{
  return new PlayerStartPointComponent();
}


export class ProjectileComponent extends Component
{
}

function __TS_Create_ProjectileComponent(): ProjectileComponent
{
  return new ProjectileComponent();
}


export class TimedDeathComponent extends Component
{
}

function __TS_Create_TimedDeathComponent(): TimedDeathComponent
{
  return new TimedDeathComponent();
}


export class SpatialAnchorComponent extends Component
{
}

function __TS_Create_SpatialAnchorComponent(): SpatialAnchorComponent
{
  return new SpatialAnchorComponent();
}


export class SrmRenderComponent extends Component
{
}

function __TS_Create_SrmRenderComponent(): SrmRenderComponent
{
  return new SrmRenderComponent();
}


export class CharacterControllerComponent extends Component
{
}

function __TS_Create_CharacterControllerComponent(): CharacterControllerComponent
{
  return new CharacterControllerComponent();
}


export class PrefabReferenceComponent extends Component
{
}

function __TS_Create_PrefabReferenceComponent(): PrefabReferenceComponent
{
  return new PrefabReferenceComponent();
}


export class SpawnComponent extends Component
{
}

function __TS_Create_SpawnComponent(): SpawnComponent
{
  return new SpawnComponent();
}


export class DeviceTrackingComponent extends Component
{
}

function __TS_Create_DeviceTrackingComponent(): DeviceTrackingComponent
{
  return new DeviceTrackingComponent();
}


export class StageSpaceComponent extends Component
{
}

function __TS_Create_StageSpaceComponent(): StageSpaceComponent
{
  return new StageSpaceComponent();
}


export class VisualScriptComponent extends EventMessageHandlerComponent
{
}

function __TS_Create_VisualScriptComponent(): VisualScriptComponent
{
  return new VisualScriptComponent();
}


export class GizmoComponent extends MeshComponent
{
}

function __TS_Create_GizmoComponent(): GizmoComponent
{
  return new GizmoComponent();
}


export class KrautTreeComponent extends RenderComponent
{
}

function __TS_Create_KrautTreeComponent(): KrautTreeComponent
{
  return new KrautTreeComponent();
}


export class ParticleComponent extends RenderComponent
{
}

function __TS_Create_ParticleComponent(): ParticleComponent
{
  return new ParticleComponent();
}


export class ParticleFinisherComponent extends RenderComponent
{
}

function __TS_Create_ParticleFinisherComponent(): ParticleFinisherComponent
{
  return new ParticleFinisherComponent();
}


export class BreakableSheetComponent extends RenderComponent
{
}

function __TS_Create_BreakableSheetComponent(): BreakableSheetComponent
{
  return new BreakableSheetComponent();
}


export class PxComponent extends Component
{
}

function __TS_Create_PxComponent(): PxComponent
{
  return new PxComponent();
}


export class PxActorComponent extends PxComponent
{
}

function __TS_Create_PxActorComponent(): PxActorComponent
{
  return new PxActorComponent();
}


export class PxCenterOfMassComponent extends PxComponent
{
}

function __TS_Create_PxCenterOfMassComponent(): PxCenterOfMassComponent
{
  return new PxCenterOfMassComponent();
}


export class PxCharacterControllerComponent extends CharacterControllerComponent
{
}

function __TS_Create_PxCharacterControllerComponent(): PxCharacterControllerComponent
{
  return new PxCharacterControllerComponent();
}


export class PxCharacterProxyComponent extends PxComponent
{
}

function __TS_Create_PxCharacterProxyComponent(): PxCharacterProxyComponent
{
  return new PxCharacterProxyComponent();
}


export class PxDynamicActorComponent extends PxActorComponent
{
}

function __TS_Create_PxDynamicActorComponent(): PxDynamicActorComponent
{
  return new PxDynamicActorComponent();
}


export class PxRaycastInteractComponent extends Component
{
}

function __TS_Create_PxRaycastInteractComponent(): PxRaycastInteractComponent
{
  return new PxRaycastInteractComponent();
}


export class PxSettingsComponent extends SettingsComponent
{
}

function __TS_Create_PxSettingsComponent(): PxSettingsComponent
{
  return new PxSettingsComponent();
}


export class PxStaticActorComponent extends PxActorComponent
{
}

function __TS_Create_PxStaticActorComponent(): PxStaticActorComponent
{
  return new PxStaticActorComponent();
}


export class PxTriggerComponent extends PxActorComponent
{
}

function __TS_Create_PxTriggerComponent(): PxTriggerComponent
{
  return new PxTriggerComponent();
}


export class PxVisColMeshComponent extends RenderComponent
{
}

function __TS_Create_PxVisColMeshComponent(): PxVisColMeshComponent
{
  return new PxVisColMeshComponent();
}


export class PxJointComponent extends PxComponent
{
}

function __TS_Create_PxJointComponent(): PxJointComponent
{
  return new PxJointComponent();
}


export class Px6DOFJointComponent extends PxJointComponent
{
}

function __TS_Create_Px6DOFJointComponent(): Px6DOFJointComponent
{
  return new Px6DOFJointComponent();
}


export class PxDistanceJointComponent extends PxJointComponent
{
}

function __TS_Create_PxDistanceJointComponent(): PxDistanceJointComponent
{
  return new PxDistanceJointComponent();
}


export class PxFixedJointComponent extends PxJointComponent
{
}

function __TS_Create_PxFixedJointComponent(): PxFixedJointComponent
{
  return new PxFixedJointComponent();
}


export class PxPrismaticJointComponent extends PxJointComponent
{
}

function __TS_Create_PxPrismaticJointComponent(): PxPrismaticJointComponent
{
  return new PxPrismaticJointComponent();
}


export class PxRevoluteJointComponent extends PxJointComponent
{
}

function __TS_Create_PxRevoluteJointComponent(): PxRevoluteJointComponent
{
  return new PxRevoluteJointComponent();
}


export class PxSphericalJointComponent extends PxJointComponent
{
}

function __TS_Create_PxSphericalJointComponent(): PxSphericalJointComponent
{
  return new PxSphericalJointComponent();
}


export class PxShapeComponent extends PxComponent
{
}

function __TS_Create_PxShapeComponent(): PxShapeComponent
{
  return new PxShapeComponent();
}


export class PxShapeBoxComponent extends PxShapeComponent
{
}

function __TS_Create_PxShapeBoxComponent(): PxShapeBoxComponent
{
  return new PxShapeBoxComponent();
}


export class PxShapeCapsuleComponent extends PxShapeComponent
{
}

function __TS_Create_PxShapeCapsuleComponent(): PxShapeCapsuleComponent
{
  return new PxShapeCapsuleComponent();
}


export class PxShapeConvexComponent extends PxShapeComponent
{
}

function __TS_Create_PxShapeConvexComponent(): PxShapeConvexComponent
{
  return new PxShapeConvexComponent();
}


export class PxShapeSphereComponent extends PxShapeComponent
{
}

function __TS_Create_PxShapeSphereComponent(): PxShapeSphereComponent
{
  return new PxShapeSphereComponent();
}


export class RcComponent extends Component
{
}

function __TS_Create_RcComponent(): RcComponent
{
  return new RcComponent();
}


export class RcMarkPoiVisibleComponent extends RcComponent
{
}

function __TS_Create_RcMarkPoiVisibleComponent(): RcMarkPoiVisibleComponent
{
  return new RcMarkPoiVisibleComponent();
}


export class RcAgentComponent extends AgentSteeringComponent
{
}

function __TS_Create_RcAgentComponent(): RcAgentComponent
{
  return new RcAgentComponent();
}


export class RcNavMeshComponent extends RcComponent
{
}

function __TS_Create_RcNavMeshComponent(): RcNavMeshComponent
{
  return new RcNavMeshComponent();
}


export class SoldierComponent extends NpcComponent
{
}

function __TS_Create_SoldierComponent(): SoldierComponent
{
  return new SoldierComponent();
}


export class ShapeIconComponent extends Component
{
}

function __TS_Create_ShapeIconComponent(): ShapeIconComponent
{
  return new ShapeIconComponent();
}


export class FmodComponent extends Component
{
}

function __TS_Create_FmodComponent(): FmodComponent
{
  return new FmodComponent();
}


export class FmodEventComponent extends FmodComponent
{
}

function __TS_Create_FmodEventComponent(): FmodEventComponent
{
  return new FmodEventComponent();
}


export class FmodListenerComponent extends FmodComponent
{
}

function __TS_Create_FmodListenerComponent(): FmodListenerComponent
{
  return new FmodListenerComponent();
}


export class ProcPlacementComponent extends Component
{
}

function __TS_Create_ProcPlacementComponent(): ProcPlacementComponent
{
  return new ProcPlacementComponent();
}


export class ProcVertexColorComponent extends MeshComponent
{
}

function __TS_Create_ProcVertexColorComponent(): ProcVertexColorComponent
{
  return new ProcVertexColorComponent();
}


export class TypeScriptComponent extends Component
{
}

function __TS_Create_TypeScriptComponent(): TypeScriptComponent
{
  return new TypeScriptComponent();
}


