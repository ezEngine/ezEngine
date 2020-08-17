
import __GameObject = require("TypeScript/ez/GameObject")
export import GameObject = __GameObject.GameObject;

import __Component = require("TypeScript/ez/Component")
export import Component = __Component.Component;

import __Vec2 = require("TypeScript/ez/Vec2")
export import Vec2 = __Vec2.Vec2;

import __Vec3 = require("TypeScript/ez/Vec3")
export import Vec3 = __Vec3.Vec3;

import __Mat3 = require("TypeScript/ez/Mat3")
export import Mat3 = __Mat3.Mat3;

import __Mat4 = require("TypeScript/ez/Mat4")
export import Mat4 = __Mat4.Mat4;

import __Quat = require("TypeScript/ez/Quat")
export import Quat = __Quat.Quat;

import __Transform = require("TypeScript/ez/Transform")
export import Transform = __Transform.Transform;

import __Color = require("TypeScript/ez/Color")
export import Color = __Color.Color;

import __Time = require("TypeScript/ez/Time")
export import Time = __Time.Time;

import __Angle = require("TypeScript/ez/Angle")
export import Angle = __Angle.Angle;

import Enum = require("./AllEnums")
import Flags = require("./AllFlags")

declare function __CPP_ComponentProperty_get(component: Component, id: number);
declare function __CPP_ComponentProperty_set(component: Component, id: number, value);
declare function __CPP_ComponentFunction_Call(component: Component, id: number, ...args);

export class AgentSteeringComponent extends Component
{
  public static GetTypeNameHash(): number { return 2967729160; }
  SetTargetPosition(position: Vec3): void { __CPP_ComponentFunction_Call(this, 3026203107, position); }
  GetTargetPosition(): Vec3 { return __CPP_ComponentFunction_Call(this, 3311202897); }
  ClearTargetPosition(): void { __CPP_ComponentFunction_Call(this, 2722020003); }
}

export class RenderComponent extends Component
{
  public static GetTypeNameHash(): number { return 881101798; }
}

export class AlwaysVisibleComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2275643316; }
}

export class SettingsComponent extends Component
{
  public static GetTypeNameHash(): number { return 936533441; }
}

export class AmbientLightComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 4054988; }
  get TopColor(): Color { return __CPP_ComponentProperty_get(this, 1351389538); }
  set TopColor(value: Color) { __CPP_ComponentProperty_set(this, 1351389538, value); }
  get BottomColor(): Color { return __CPP_ComponentProperty_get(this, 1228578626); }
  set BottomColor(value: Color) { __CPP_ComponentProperty_set(this, 1228578626, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 2766494900); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 2766494900, value); }
}

export class MeshComponentBase extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3137569299; }
}

export class SkinnedMeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 3413380891; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 197002294); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 197002294, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2794932019); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2794932019, value); }
}

export class AnimatedMeshComponent extends SkinnedMeshComponent
{
  public static GetTypeNameHash(): number { return 1354920640; }
  get AnimationClip(): string { return __CPP_ComponentProperty_get(this, 1370598076); }
  set AnimationClip(value: string) { __CPP_ComponentProperty_set(this, 1370598076, value); }
  get Loop(): boolean { return __CPP_ComponentProperty_get(this, 1941506410); }
  set Loop(value: boolean) { __CPP_ComponentProperty_set(this, 1941506410, value); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 3166086532); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 3166086532, value); }
  get ApplyRootMotion(): boolean { return __CPP_ComponentProperty_get(this, 1680768189); }
  set ApplyRootMotion(value: boolean) { __CPP_ComponentProperty_set(this, 1680768189, value); }
  get VisualizeSkeleton(): boolean { return __CPP_ComponentProperty_get(this, 1571306042); }
  set VisualizeSkeleton(value: boolean) { __CPP_ComponentProperty_set(this, 1571306042, value); }
}

export class AreaDamageComponent extends Component
{
  public static GetTypeNameHash(): number { return 2662298415; }
  ApplyAreaDamage(): void { __CPP_ComponentFunction_Call(this, 2571520832); }
  get OnCreation(): boolean { return __CPP_ComponentProperty_get(this, 1698655414); }
  set OnCreation(value: boolean) { __CPP_ComponentProperty_set(this, 1698655414, value); }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 2367783163); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 2367783163, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 3568831381); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 3568831381, value); }
  get Damage(): number { return __CPP_ComponentProperty_get(this, 1461188382); }
  set Damage(value: number) { __CPP_ComponentProperty_set(this, 1461188382, value); }
  get Impulse(): number { return __CPP_ComponentProperty_get(this, 243909542); }
  set Impulse(value: number) { __CPP_ComponentProperty_set(this, 243909542, value); }
}

export class BeamComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 758405427; }
  get TargetObject(): string { return __CPP_ComponentProperty_get(this, 3127794430); }
  set TargetObject(value: string) { __CPP_ComponentProperty_set(this, 3127794430, value); }
  get Material(): string { return __CPP_ComponentProperty_get(this, 4210689235); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 4210689235, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2066413132); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2066413132, value); }
  get Width(): number { return __CPP_ComponentProperty_get(this, 768587780); }
  set Width(value: number) { __CPP_ComponentProperty_set(this, 768587780, value); }
  get UVUnitsPerWorldUnit(): number { return __CPP_ComponentProperty_get(this, 2623740164); }
  set UVUnitsPerWorldUnit(value: number) { __CPP_ComponentProperty_set(this, 2623740164, value); }
}

export class BreakableSheetComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3356995265; }
  IsBroken(): boolean { return __CPP_ComponentFunction_Call(this, 2813674560); }
  Break(): void { __CPP_ComponentFunction_Call(this, 793224861); }
  get Material(): string { return __CPP_ComponentProperty_get(this, 2472923882); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 2472923882, value); }
  get BrokenMaterial(): string { return __CPP_ComponentProperty_get(this, 4198830320); }
  set BrokenMaterial(value: string) { __CPP_ComponentProperty_set(this, 4198830320, value); }
  get Width(): number { return __CPP_ComponentProperty_get(this, 467002817); }
  set Width(value: number) { __CPP_ComponentProperty_set(this, 467002817, value); }
  get Height(): number { return __CPP_ComponentProperty_get(this, 2801619110); }
  set Height(value: number) { __CPP_ComponentProperty_set(this, 2801619110, value); }
  get Thickness(): number { return __CPP_ComponentProperty_get(this, 1072721174); }
  set Thickness(value: number) { __CPP_ComponentProperty_set(this, 1072721174, value); }
  get Density(): number { return __CPP_ComponentProperty_get(this, 1958138542); }
  set Density(value: number) { __CPP_ComponentProperty_set(this, 1958138542, value); }
  get NumPieces(): number { return __CPP_ComponentProperty_get(this, 1564409143); }
  set NumPieces(value: number) { __CPP_ComponentProperty_set(this, 1564409143, value); }
  get DisappearTimeout(): number { return __CPP_ComponentProperty_get(this, 811126576); }
  set DisappearTimeout(value: number) { __CPP_ComponentProperty_set(this, 811126576, value); }
  get BreakImpulseStrength(): number { return __CPP_ComponentProperty_get(this, 3898564077); }
  set BreakImpulseStrength(value: number) { __CPP_ComponentProperty_set(this, 3898564077, value); }
  get FixedBorder(): boolean { return __CPP_ComponentProperty_get(this, 2310197823); }
  set FixedBorder(value: boolean) { __CPP_ComponentProperty_set(this, 2310197823, value); }
  get FixedRandomSeed(): number { return __CPP_ComponentProperty_get(this, 2509413398); }
  set FixedRandomSeed(value: number) { __CPP_ComponentProperty_set(this, 2509413398, value); }
  get CollisionLayerUnbroken(): number { return __CPP_ComponentProperty_get(this, 3760567162); }
  set CollisionLayerUnbroken(value: number) { __CPP_ComponentProperty_set(this, 3760567162, value); }
  get CollisionLayerBrokenPieces(): number { return __CPP_ComponentProperty_get(this, 2209233006); }
  set CollisionLayerBrokenPieces(value: number) { __CPP_ComponentProperty_set(this, 2209233006, value); }
  get IncludeInNavmesh(): boolean { return __CPP_ComponentProperty_get(this, 1527961883); }
  set IncludeInNavmesh(value: boolean) { __CPP_ComponentProperty_set(this, 1527961883, value); }
}

export class CameraComponent extends Component
{
  public static GetTypeNameHash(): number { return 3483318225; }
  get EditorShortcut(): number { return __CPP_ComponentProperty_get(this, 2829941392); }
  set EditorShortcut(value: number) { __CPP_ComponentProperty_set(this, 2829941392, value); }
  get UsageHint(): Enum.CameraUsageHint { return __CPP_ComponentProperty_get(this, 1166855742); }
  set UsageHint(value: Enum.CameraUsageHint) { __CPP_ComponentProperty_set(this, 1166855742, value); }
  get Mode(): Enum.CameraMode { return __CPP_ComponentProperty_get(this, 2334063300); }
  set Mode(value: Enum.CameraMode) { __CPP_ComponentProperty_set(this, 2334063300, value); }
  get RenderTarget(): string { return __CPP_ComponentProperty_get(this, 742366175); }
  set RenderTarget(value: string) { __CPP_ComponentProperty_set(this, 742366175, value); }
  get RenderTargetOffset(): Vec2 { return __CPP_ComponentProperty_get(this, 2417506932); }
  set RenderTargetOffset(value: Vec2) { __CPP_ComponentProperty_set(this, 2417506932, value); }
  get RenderTargetSize(): Vec2 { return __CPP_ComponentProperty_get(this, 3751637778); }
  set RenderTargetSize(value: Vec2) { __CPP_ComponentProperty_set(this, 3751637778, value); }
  get NearPlane(): number { return __CPP_ComponentProperty_get(this, 2813917003); }
  set NearPlane(value: number) { __CPP_ComponentProperty_set(this, 2813917003, value); }
  get FarPlane(): number { return __CPP_ComponentProperty_get(this, 3267424437); }
  set FarPlane(value: number) { __CPP_ComponentProperty_set(this, 3267424437, value); }
  get FOV(): number { return __CPP_ComponentProperty_get(this, 3414825327); }
  set FOV(value: number) { __CPP_ComponentProperty_set(this, 3414825327, value); }
  get Dimensions(): number { return __CPP_ComponentProperty_get(this, 2411717490); }
  set Dimensions(value: number) { __CPP_ComponentProperty_set(this, 2411717490, value); }
  get CameraRenderPipeline(): string { return __CPP_ComponentProperty_get(this, 2395330233); }
  set CameraRenderPipeline(value: string) { __CPP_ComponentProperty_set(this, 2395330233, value); }
  get Aperture(): number { return __CPP_ComponentProperty_get(this, 2519664510); }
  set Aperture(value: number) { __CPP_ComponentProperty_set(this, 2519664510, value); }
  get ShutterTime(): number { return __CPP_ComponentProperty_get(this, 4237802334); }
  set ShutterTime(value: number) { __CPP_ComponentProperty_set(this, 4237802334, value); }
  get ISO(): number { return __CPP_ComponentProperty_get(this, 1883775215); }
  set ISO(value: number) { __CPP_ComponentProperty_set(this, 1883775215, value); }
  get ExposureCompensation(): number { return __CPP_ComponentProperty_get(this, 3541271769); }
  set ExposureCompensation(value: number) { __CPP_ComponentProperty_set(this, 3541271769, value); }
  get ShowStats(): boolean { return __CPP_ComponentProperty_get(this, 4040228179); }
  set ShowStats(value: boolean) { __CPP_ComponentProperty_set(this, 4040228179, value); }
}

export class CharacterControllerComponent extends Component
{
  public static GetTypeNameHash(): number { return 3698536163; }
  RawMove(moveDeltaGlobal: Vec3): void { __CPP_ComponentFunction_Call(this, 2969332041, moveDeltaGlobal); }
  TeleportCharacter(globalFootPosition: Vec3): void { __CPP_ComponentFunction_Call(this, 1003615504, globalFootPosition); }
  IsDestinationUnobstructed(globalFootPosition: Vec3, characterHeight: number): boolean { return __CPP_ComponentFunction_Call(this, 1053383004, globalFootPosition, characterHeight); }
}

export class CollectionComponent extends Component
{
  public static GetTypeNameHash(): number { return 1196326241; }
  get Collection(): string { return __CPP_ComponentProperty_get(this, 1383801553); }
  set Collection(value: string) { __CPP_ComponentProperty_set(this, 1383801553, value); }
}

export class ColorAnimationComponent extends Component
{
  public static GetTypeNameHash(): number { return 744095698; }
  get Gradient(): string { return __CPP_ComponentProperty_get(this, 1126193345); }
  set Gradient(value: string) { __CPP_ComponentProperty_set(this, 1126193345, value); }
  get Duration(): number { return __CPP_ComponentProperty_get(this, 12500317); }
  set Duration(value: number) { __CPP_ComponentProperty_set(this, 12500317, value); }
  get SetColorMode(): Enum.SetColorMode { return __CPP_ComponentProperty_get(this, 3091697406); }
  set SetColorMode(value: Enum.SetColorMode) { __CPP_ComponentProperty_set(this, 3091697406, value); }
  get AnimationMode(): Enum.PropertyAnimMode { return __CPP_ComponentProperty_get(this, 2605484307); }
  set AnimationMode(value: Enum.PropertyAnimMode) { __CPP_ComponentProperty_set(this, 2605484307, value); }
  get RandomStartOffset(): boolean { return __CPP_ComponentProperty_get(this, 2192658561); }
  set RandomStartOffset(value: boolean) { __CPP_ComponentProperty_set(this, 2192658561, value); }
  get ApplyToChildren(): boolean { return __CPP_ComponentProperty_get(this, 2535260273); }
  set ApplyToChildren(value: boolean) { __CPP_ComponentProperty_set(this, 2535260273, value); }
}

export class DebugTextComponent extends Component
{
  public static GetTypeNameHash(): number { return 2116856431; }
  get Text(): string { return __CPP_ComponentProperty_get(this, 3534158952); }
  set Text(value: string) { __CPP_ComponentProperty_set(this, 3534158952, value); }
  get Value0(): number { return __CPP_ComponentProperty_get(this, 405606534); }
  set Value0(value: number) { __CPP_ComponentProperty_set(this, 405606534, value); }
  get Value1(): number { return __CPP_ComponentProperty_get(this, 1025567662); }
  set Value1(value: number) { __CPP_ComponentProperty_set(this, 1025567662, value); }
  get Value2(): number { return __CPP_ComponentProperty_get(this, 1440872565); }
  set Value2(value: number) { __CPP_ComponentProperty_set(this, 1440872565, value); }
  get Value3(): number { return __CPP_ComponentProperty_get(this, 2610934928); }
  set Value3(value: number) { __CPP_ComponentProperty_set(this, 2610934928, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2484850037); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2484850037, value); }
}

export class DecalComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2207675050; }
  get ProjectionAxis(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 1575218235); }
  set ProjectionAxis(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 1575218235, value); }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 4040048063); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 4040048063, value); }
  get SizeVariance(): number { return __CPP_ComponentProperty_get(this, 1788239091); }
  set SizeVariance(value: number) { __CPP_ComponentProperty_set(this, 1788239091, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2977975235); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2977975235, value); }
  get EmissiveColor(): Color { return __CPP_ComponentProperty_get(this, 3139545081); }
  set EmissiveColor(value: Color) { __CPP_ComponentProperty_set(this, 3139545081, value); }
  get SortOrder(): number { return __CPP_ComponentProperty_get(this, 3596759857); }
  set SortOrder(value: number) { __CPP_ComponentProperty_set(this, 3596759857, value); }
  get WrapAround(): boolean { return __CPP_ComponentProperty_get(this, 4179161639); }
  set WrapAround(value: boolean) { __CPP_ComponentProperty_set(this, 4179161639, value); }
  get MapNormalToGeometry(): boolean { return __CPP_ComponentProperty_get(this, 3122502064); }
  set MapNormalToGeometry(value: boolean) { __CPP_ComponentProperty_set(this, 3122502064, value); }
  get InnerFadeAngle(): number { return __CPP_ComponentProperty_get(this, 4221200728); }
  set InnerFadeAngle(value: number) { __CPP_ComponentProperty_set(this, 4221200728, value); }
  get OuterFadeAngle(): number { return __CPP_ComponentProperty_get(this, 3504670183); }
  set OuterFadeAngle(value: number) { __CPP_ComponentProperty_set(this, 3504670183, value); }
  get FadeOutDuration(): number { return __CPP_ComponentProperty_get(this, 2460709596); }
  set FadeOutDuration(value: number) { __CPP_ComponentProperty_set(this, 2460709596, value); }
  get OnFinishedAction(): Enum.OnComponentFinishedAction { return __CPP_ComponentProperty_get(this, 623730387); }
  set OnFinishedAction(value: Enum.OnComponentFinishedAction) { __CPP_ComponentProperty_set(this, 623730387, value); }
  get ApplyToDynamic(): string { return __CPP_ComponentProperty_get(this, 3960133726); }
  set ApplyToDynamic(value: string) { __CPP_ComponentProperty_set(this, 3960133726, value); }
}

export class DeviceTrackingComponent extends Component
{
  public static GetTypeNameHash(): number { return 2269135831; }
  get DeviceType(): Enum.XRDeviceType { return __CPP_ComponentProperty_get(this, 697471502); }
  set DeviceType(value: Enum.XRDeviceType) { __CPP_ComponentProperty_set(this, 697471502, value); }
  get PoseLocation(): Enum.XRPoseLocation { return __CPP_ComponentProperty_get(this, 1862083944); }
  set PoseLocation(value: Enum.XRPoseLocation) { __CPP_ComponentProperty_set(this, 1862083944, value); }
  get TransformSpace(): Enum.XRTransformSpace { return __CPP_ComponentProperty_get(this, 3602246610); }
  set TransformSpace(value: Enum.XRTransformSpace) { __CPP_ComponentProperty_set(this, 3602246610, value); }
}

export class LightComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2775204280; }
  get LightColor(): Color { return __CPP_ComponentProperty_get(this, 3105726516); }
  set LightColor(value: Color) { __CPP_ComponentProperty_set(this, 3105726516, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 3461084450); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 3461084450, value); }
  get CastShadows(): boolean { return __CPP_ComponentProperty_get(this, 1187423429); }
  set CastShadows(value: boolean) { __CPP_ComponentProperty_set(this, 1187423429, value); }
  get PenumbraSize(): number { return __CPP_ComponentProperty_get(this, 498289516); }
  set PenumbraSize(value: number) { __CPP_ComponentProperty_set(this, 498289516, value); }
  get SlopeBias(): number { return __CPP_ComponentProperty_get(this, 301669313); }
  set SlopeBias(value: number) { __CPP_ComponentProperty_set(this, 301669313, value); }
  get ConstantBias(): number { return __CPP_ComponentProperty_get(this, 2002327332); }
  set ConstantBias(value: number) { __CPP_ComponentProperty_set(this, 2002327332, value); }
}

export class DirectionalLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 4190390155; }
  get NumCascades(): number { return __CPP_ComponentProperty_get(this, 2340627423); }
  set NumCascades(value: number) { __CPP_ComponentProperty_set(this, 2340627423, value); }
  get MinShadowRange(): number { return __CPP_ComponentProperty_get(this, 262172733); }
  set MinShadowRange(value: number) { __CPP_ComponentProperty_set(this, 262172733, value); }
  get FadeOutStart(): number { return __CPP_ComponentProperty_get(this, 1239001328); }
  set FadeOutStart(value: number) { __CPP_ComponentProperty_set(this, 1239001328, value); }
  get SplitModeWeight(): number { return __CPP_ComponentProperty_get(this, 45569243); }
  set SplitModeWeight(value: number) { __CPP_ComponentProperty_set(this, 45569243, value); }
  get NearPlaneOffset(): number { return __CPP_ComponentProperty_get(this, 125251581); }
  set NearPlaneOffset(value: number) { __CPP_ComponentProperty_set(this, 125251581, value); }
}

export class EventMessageHandlerComponent extends Component
{
  public static GetTypeNameHash(): number { return 1246896404; }
  get HandleGlobalEvents(): boolean { return __CPP_ComponentProperty_get(this, 1785533810); }
  set HandleGlobalEvents(value: boolean) { __CPP_ComponentProperty_set(this, 1785533810, value); }
}

export class FmodComponent extends Component
{
  public static GetTypeNameHash(): number { return 807370411; }
}

export class FmodEventComponent extends FmodComponent
{
  public static GetTypeNameHash(): number { return 3913395172; }
  Restart(): void { __CPP_ComponentFunction_Call(this, 3030922480); }
  StartOneShot(): void { __CPP_ComponentFunction_Call(this, 2999620077); }
  StopSound(Immediate: boolean): void { __CPP_ComponentFunction_Call(this, 1358061666, Immediate); }
  SoundCue(): void { __CPP_ComponentFunction_Call(this, 895127516); }
  SetEventParameter(ParamName: string, Value: number): void { __CPP_ComponentFunction_Call(this, 2707260683, ParamName, Value); }
  get Paused(): boolean { return __CPP_ComponentProperty_get(this, 559620417); }
  set Paused(value: boolean) { __CPP_ComponentProperty_set(this, 559620417, value); }
  get Volume(): number { return __CPP_ComponentProperty_get(this, 2438766047); }
  set Volume(value: number) { __CPP_ComponentProperty_set(this, 2438766047, value); }
  get Pitch(): number { return __CPP_ComponentProperty_get(this, 1746009170); }
  set Pitch(value: number) { __CPP_ComponentProperty_set(this, 1746009170, value); }
  get SoundEvent(): string { return __CPP_ComponentProperty_get(this, 2751019528); }
  set SoundEvent(value: string) { __CPP_ComponentProperty_set(this, 2751019528, value); }
  get UseOcclusion(): boolean { return __CPP_ComponentProperty_get(this, 1476141879); }
  set UseOcclusion(value: boolean) { __CPP_ComponentProperty_set(this, 1476141879, value); }
  get OcclusionThreshold(): number { return __CPP_ComponentProperty_get(this, 407424135); }
  set OcclusionThreshold(value: number) { __CPP_ComponentProperty_set(this, 407424135, value); }
  get OcclusionCollisionLayer(): number { return __CPP_ComponentProperty_get(this, 685309626); }
  set OcclusionCollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 685309626, value); }
  get OnFinishedAction(): Enum.OnComponentFinishedAction { return __CPP_ComponentProperty_get(this, 98621763); }
  set OnFinishedAction(value: Enum.OnComponentFinishedAction) { __CPP_ComponentProperty_set(this, 98621763, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 180981950); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 180981950, value); }
}

export class FmodListenerComponent extends FmodComponent
{
  public static GetTypeNameHash(): number { return 312734920; }
  get ListenerIndex(): number { return __CPP_ComponentProperty_get(this, 1259282162); }
  set ListenerIndex(value: number) { __CPP_ComponentProperty_set(this, 1259282162, value); }
}

export class FogComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 1312197427; }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 944336001); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 944336001, value); }
  get Density(): number { return __CPP_ComponentProperty_get(this, 1772920610); }
  set Density(value: number) { __CPP_ComponentProperty_set(this, 1772920610, value); }
  get HeightFalloff(): number { return __CPP_ComponentProperty_get(this, 3470037436); }
  set HeightFalloff(value: number) { __CPP_ComponentProperty_set(this, 3470037436, value); }
}

export class MeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 397412448; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 437259449); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 437259449, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 216143134); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 216143134, value); }
}

export class GizmoComponent extends MeshComponent
{
  public static GetTypeNameHash(): number { return 4084255410; }
}

export class GrabbableItemComponent extends Component
{
  public static GetTypeNameHash(): number { return 640040242; }
}

export class GreyBoxComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3220742941; }
  get Shape(): Enum.GreyBoxShape { return __CPP_ComponentProperty_get(this, 380354102); }
  set Shape(value: Enum.GreyBoxShape) { __CPP_ComponentProperty_set(this, 380354102, value); }
  get Material(): string { return __CPP_ComponentProperty_get(this, 1397464602); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 1397464602, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 3988942159); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 3988942159, value); }
  get SizeNegX(): number { return __CPP_ComponentProperty_get(this, 3366117003); }
  set SizeNegX(value: number) { __CPP_ComponentProperty_set(this, 3366117003, value); }
  get SizePosX(): number { return __CPP_ComponentProperty_get(this, 3910851833); }
  set SizePosX(value: number) { __CPP_ComponentProperty_set(this, 3910851833, value); }
  get SizeNegY(): number { return __CPP_ComponentProperty_get(this, 909521445); }
  set SizeNegY(value: number) { __CPP_ComponentProperty_set(this, 909521445, value); }
  get SizePosY(): number { return __CPP_ComponentProperty_get(this, 3387141325); }
  set SizePosY(value: number) { __CPP_ComponentProperty_set(this, 3387141325, value); }
  get SizeNegZ(): number { return __CPP_ComponentProperty_get(this, 2733354278); }
  set SizeNegZ(value: number) { __CPP_ComponentProperty_set(this, 2733354278, value); }
  get SizePosZ(): number { return __CPP_ComponentProperty_get(this, 2829676430); }
  set SizePosZ(value: number) { __CPP_ComponentProperty_set(this, 2829676430, value); }
  get Detail(): number { return __CPP_ComponentProperty_get(this, 3448756101); }
  set Detail(value: number) { __CPP_ComponentProperty_set(this, 3448756101, value); }
  get Curvature(): number { return __CPP_ComponentProperty_get(this, 2035743766); }
  set Curvature(value: number) { __CPP_ComponentProperty_set(this, 2035743766, value); }
  get Thickness(): number { return __CPP_ComponentProperty_get(this, 1638929319); }
  set Thickness(value: number) { __CPP_ComponentProperty_set(this, 1638929319, value); }
  get SlopedTop(): boolean { return __CPP_ComponentProperty_get(this, 537886510); }
  set SlopedTop(value: boolean) { __CPP_ComponentProperty_set(this, 537886510, value); }
  get SlopedBottom(): boolean { return __CPP_ComponentProperty_get(this, 283185279); }
  set SlopedBottom(value: boolean) { __CPP_ComponentProperty_set(this, 283185279, value); }
}

export class HeadBoneComponent extends Component
{
  public static GetTypeNameHash(): number { return 2949993843; }
  SetVerticalRotation(Radians: number): void { __CPP_ComponentFunction_Call(this, 2976403157, Radians); }
  ChangeVerticalRotation(Radians: number): void { __CPP_ComponentFunction_Call(this, 2309737157, Radians); }
  get VerticalRotation(): number { return __CPP_ComponentProperty_get(this, 3248526879); }
  set VerticalRotation(value: number) { __CPP_ComponentProperty_set(this, 3248526879, value); }
}

export class InputComponent extends Component
{
  public static GetTypeNameHash(): number { return 1460567787; }
  GetCurrentInputState(InputAction: string, OnlyKeyPressed: boolean): number { return __CPP_ComponentFunction_Call(this, 481201980, InputAction, OnlyKeyPressed); }
  get InputSet(): string { return __CPP_ComponentProperty_get(this, 1680374769); }
  set InputSet(value: string) { __CPP_ComponentProperty_set(this, 1680374769, value); }
  get Granularity(): Enum.InputMessageGranularity { return __CPP_ComponentProperty_get(this, 3472635708); }
  set Granularity(value: Enum.InputMessageGranularity) { __CPP_ComponentProperty_set(this, 3472635708, value); }
}

export class InstancedMeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 3579715970; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 2719579728); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 2719579728, value); }
  get MainColor(): Color { return __CPP_ComponentProperty_get(this, 3129005358); }
  set MainColor(value: Color) { __CPP_ComponentProperty_set(this, 3129005358, value); }
}

export class JointAttachmentComponent extends Component
{
  public static GetTypeNameHash(): number { return 4059070551; }
  get JointName(): string { return __CPP_ComponentProperty_get(this, 1338087530); }
  set JointName(value: string) { __CPP_ComponentProperty_set(this, 1338087530, value); }
}

export class KrautTreeComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 835144953; }
  get KrautTree(): string { return __CPP_ComponentProperty_get(this, 908664750); }
  set KrautTree(value: string) { __CPP_ComponentProperty_set(this, 908664750, value); }
}

export class LineToComponent extends Component
{
  public static GetTypeNameHash(): number { return 440374311; }
  get Target(): string { return __CPP_ComponentProperty_get(this, 1777367805); }
  set Target(value: string) { __CPP_ComponentProperty_set(this, 1777367805, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 4196159176); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 4196159176, value); }
}

export class MarkerComponent extends Component
{
  public static GetTypeNameHash(): number { return 2240330531; }
  get Marker(): string { return __CPP_ComponentProperty_get(this, 3679528792); }
  set Marker(value: string) { __CPP_ComponentProperty_set(this, 3679528792, value); }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 169547257); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 169547257, value); }
}

export class MotionMatchingComponent extends SkinnedMeshComponent
{
  public static GetTypeNameHash(): number { return 3085935728; }
}

export class MoveToComponent extends Component
{
  public static GetTypeNameHash(): number { return 3156487545; }
  SetTargetPosition(position: Vec3): void { __CPP_ComponentFunction_Call(this, 3378325860, position); }
  get Running(): boolean { return __CPP_ComponentProperty_get(this, 2512484554); }
  set Running(value: boolean) { __CPP_ComponentProperty_set(this, 2512484554, value); }
  get TranslationSpeed(): number { return __CPP_ComponentProperty_get(this, 1054583698); }
  set TranslationSpeed(value: number) { __CPP_ComponentProperty_set(this, 1054583698, value); }
  get TranslationAcceleration(): number { return __CPP_ComponentProperty_get(this, 2205023531); }
  set TranslationAcceleration(value: number) { __CPP_ComponentProperty_set(this, 2205023531, value); }
  get TranslationDeceleration(): number { return __CPP_ComponentProperty_get(this, 1189201957); }
  set TranslationDeceleration(value: number) { __CPP_ComponentProperty_set(this, 1189201957, value); }
}

export class NpcComponent extends Component
{
  public static GetTypeNameHash(): number { return 372422185; }
}

export class ParticleComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2729030598; }
  StartEffect(): boolean { return __CPP_ComponentFunction_Call(this, 1657903094); }
  StopEffect(): void { __CPP_ComponentFunction_Call(this, 2833468889); }
  InterruptEffect(): void { __CPP_ComponentFunction_Call(this, 1202074159); }
  IsEffectActive(): boolean { return __CPP_ComponentFunction_Call(this, 1435956595); }
  get Effect(): string { return __CPP_ComponentProperty_get(this, 482044043); }
  set Effect(value: string) { __CPP_ComponentProperty_set(this, 482044043, value); }
  get SpawnAtStart(): boolean { return __CPP_ComponentProperty_get(this, 3005128404); }
  set SpawnAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 3005128404, value); }
  get OnFinishedAction(): Enum.OnComponentFinishedAction2 { return __CPP_ComponentProperty_get(this, 1452429251); }
  set OnFinishedAction(value: Enum.OnComponentFinishedAction2) { __CPP_ComponentProperty_set(this, 1452429251, value); }
  get MinRestartDelay(): number { return __CPP_ComponentProperty_get(this, 2966264592); }
  set MinRestartDelay(value: number) { __CPP_ComponentProperty_set(this, 2966264592, value); }
  get RestartDelayRange(): number { return __CPP_ComponentProperty_get(this, 3744843482); }
  set RestartDelayRange(value: number) { __CPP_ComponentProperty_set(this, 3744843482, value); }
  get RandomSeed(): number { return __CPP_ComponentProperty_get(this, 3568875264); }
  set RandomSeed(value: number) { __CPP_ComponentProperty_set(this, 3568875264, value); }
  get SpawnDirection(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 734857198); }
  set SpawnDirection(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 734857198, value); }
  get IgnoreOwnerRotation(): boolean { return __CPP_ComponentProperty_get(this, 3802357005); }
  set IgnoreOwnerRotation(value: boolean) { __CPP_ComponentProperty_set(this, 3802357005, value); }
  get SharedInstanceName(): string { return __CPP_ComponentProperty_get(this, 3998862172); }
  set SharedInstanceName(value: string) { __CPP_ComponentProperty_set(this, 3998862172, value); }
}

export class ParticleFinisherComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1583315087; }
}

export class PlayerStartPointComponent extends Component
{
  public static GetTypeNameHash(): number { return 4071975843; }
  get PlayerPrefab(): string { return __CPP_ComponentProperty_get(this, 3944299230); }
  set PlayerPrefab(value: string) { __CPP_ComponentProperty_set(this, 3944299230, value); }
}

export class PointLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 3870819112; }
  get Range(): number { return __CPP_ComponentProperty_get(this, 2353710059); }
  set Range(value: number) { __CPP_ComponentProperty_set(this, 2353710059, value); }
}

export class PrefabReferenceComponent extends Component
{
  public static GetTypeNameHash(): number { return 3089077846; }
  get Prefab(): string { return __CPP_ComponentProperty_get(this, 3674679045); }
  set Prefab(value: string) { __CPP_ComponentProperty_set(this, 3674679045, value); }
}

export class ProcPlacementComponent extends Component
{
  public static GetTypeNameHash(): number { return 1231039093; }
  get Resource(): string { return __CPP_ComponentProperty_get(this, 3055015184); }
  set Resource(value: string) { __CPP_ComponentProperty_set(this, 3055015184, value); }
}

export class ProcVertexColorComponent extends MeshComponent
{
  public static GetTypeNameHash(): number { return 2605948033; }
  get Resource(): string { return __CPP_ComponentProperty_get(this, 3204422965); }
  set Resource(value: string) { __CPP_ComponentProperty_set(this, 3204422965, value); }
}

export class ProcVolumeComponent extends Component
{
  public static GetTypeNameHash(): number { return 1310446349; }
  get Value(): number { return __CPP_ComponentProperty_get(this, 498974889); }
  set Value(value: number) { __CPP_ComponentProperty_set(this, 498974889, value); }
  get SortOrder(): number { return __CPP_ComponentProperty_get(this, 16519358); }
  set SortOrder(value: number) { __CPP_ComponentProperty_set(this, 16519358, value); }
  get BlendMode(): Enum.ProcGenBlendMode { return __CPP_ComponentProperty_get(this, 4048739530); }
  set BlendMode(value: Enum.ProcGenBlendMode) { __CPP_ComponentProperty_set(this, 4048739530, value); }
}

export class ProcVolumeBoxComponent extends ProcVolumeComponent
{
  public static GetTypeNameHash(): number { return 1203168513; }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 2955699855); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 2955699855, value); }
  get FadeOutStart(): Vec3 { return __CPP_ComponentProperty_get(this, 232649030); }
  set FadeOutStart(value: Vec3) { __CPP_ComponentProperty_set(this, 232649030, value); }
}

export class ProcVolumeSphereComponent extends ProcVolumeComponent
{
  public static GetTypeNameHash(): number { return 3558691999; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 859654706); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 859654706, value); }
  get FadeOutStart(): number { return __CPP_ComponentProperty_get(this, 1499968187); }
  set FadeOutStart(value: number) { __CPP_ComponentProperty_set(this, 1499968187, value); }
}

export class ProjectileComponent extends Component
{
  public static GetTypeNameHash(): number { return 2031292246; }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 2930622135); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 2930622135, value); }
  get GravityMultiplier(): number { return __CPP_ComponentProperty_get(this, 3468554650); }
  set GravityMultiplier(value: number) { __CPP_ComponentProperty_set(this, 3468554650, value); }
  get MaxLifetime(): number { return __CPP_ComponentProperty_get(this, 2474472027); }
  set MaxLifetime(value: number) { __CPP_ComponentProperty_set(this, 2474472027, value); }
  get OnTimeoutSpawn(): string { return __CPP_ComponentProperty_get(this, 980639630); }
  set OnTimeoutSpawn(value: string) { __CPP_ComponentProperty_set(this, 980639630, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 3312343554); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 3312343554, value); }
  get FallbackSurface(): string { return __CPP_ComponentProperty_get(this, 573591754); }
  set FallbackSurface(value: string) { __CPP_ComponentProperty_set(this, 573591754, value); }
}

export class PropertyAnimComponent extends Component
{
  public static GetTypeNameHash(): number { return 2917871434; }
  PlayAnimationRange(RangeLow: number, RangeHigh: number): void { __CPP_ComponentFunction_Call(this, 1952704030, RangeLow, RangeHigh); }
  get Animation(): string { return __CPP_ComponentProperty_get(this, 2525048732); }
  set Animation(value: string) { __CPP_ComponentProperty_set(this, 2525048732, value); }
  get Playing(): boolean { return __CPP_ComponentProperty_get(this, 397405593); }
  set Playing(value: boolean) { __CPP_ComponentProperty_set(this, 397405593, value); }
  get Mode(): Enum.PropertyAnimMode { return __CPP_ComponentProperty_get(this, 1282053399); }
  set Mode(value: Enum.PropertyAnimMode) { __CPP_ComponentProperty_set(this, 1282053399, value); }
  get RandomOffset(): number { return __CPP_ComponentProperty_get(this, 2483535309); }
  set RandomOffset(value: number) { __CPP_ComponentProperty_set(this, 2483535309, value); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 2595264145); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 2595264145, value); }
  get RangeLow(): number { return __CPP_ComponentProperty_get(this, 3709402260); }
  set RangeLow(value: number) { __CPP_ComponentProperty_set(this, 3709402260, value); }
  get RangeHigh(): number { return __CPP_ComponentProperty_get(this, 2010125141); }
  set RangeHigh(value: number) { __CPP_ComponentProperty_set(this, 2010125141, value); }
}

export class PxComponent extends Component
{
  public static GetTypeNameHash(): number { return 1847683745; }
}

export class PxJointComponent extends PxComponent
{
  public static GetTypeNameHash(): number { return 2622441505; }
  get BreakForce(): number { return __CPP_ComponentProperty_get(this, 1528846925); }
  set BreakForce(value: number) { __CPP_ComponentProperty_set(this, 1528846925, value); }
  get BreakTorque(): number { return __CPP_ComponentProperty_get(this, 3445777535); }
  set BreakTorque(value: number) { __CPP_ComponentProperty_set(this, 3445777535, value); }
  get PairCollision(): boolean { return __CPP_ComponentProperty_get(this, 4216644245); }
  set PairCollision(value: boolean) { __CPP_ComponentProperty_set(this, 4216644245, value); }
  get ParentActor(): string { return __CPP_ComponentProperty_get(this, 863846737); }
  set ParentActor(value: string) { __CPP_ComponentProperty_set(this, 863846737, value); }
  get ChildActor(): string { return __CPP_ComponentProperty_get(this, 2144601122); }
  set ChildActor(value: string) { __CPP_ComponentProperty_set(this, 2144601122, value); }
  get ChildActorAnchor(): string { return __CPP_ComponentProperty_get(this, 1189131464); }
  set ChildActorAnchor(value: string) { __CPP_ComponentProperty_set(this, 1189131464, value); }
}

export class Px6DOFJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 3162081425; }
  get FreeLinearAxis(): Flags.PxAxis { return __CPP_ComponentProperty_get(this, 2597744060); }
  set FreeLinearAxis(value: Flags.PxAxis) { __CPP_ComponentProperty_set(this, 2597744060, value); }
  get LinearLimitMode(): Enum.PxJointLimitMode { return __CPP_ComponentProperty_get(this, 3178215844); }
  set LinearLimitMode(value: Enum.PxJointLimitMode) { __CPP_ComponentProperty_set(this, 3178215844, value); }
  get LinearRangeX(): Vec2 { return __CPP_ComponentProperty_get(this, 2612508715); }
  set LinearRangeX(value: Vec2) { __CPP_ComponentProperty_set(this, 2612508715, value); }
  get LinearRangeY(): Vec2 { return __CPP_ComponentProperty_get(this, 1104231376); }
  set LinearRangeY(value: Vec2) { __CPP_ComponentProperty_set(this, 1104231376, value); }
  get LinearRangeZ(): Vec2 { return __CPP_ComponentProperty_get(this, 3284996328); }
  set LinearRangeZ(value: Vec2) { __CPP_ComponentProperty_set(this, 3284996328, value); }
  get LinearStiffness(): number { return __CPP_ComponentProperty_get(this, 2783723137); }
  set LinearStiffness(value: number) { __CPP_ComponentProperty_set(this, 2783723137, value); }
  get LinearDamping(): number { return __CPP_ComponentProperty_get(this, 3049323952); }
  set LinearDamping(value: number) { __CPP_ComponentProperty_set(this, 3049323952, value); }
  get FreeAngularAxis(): Flags.PxAxis { return __CPP_ComponentProperty_get(this, 4053482092); }
  set FreeAngularAxis(value: Flags.PxAxis) { __CPP_ComponentProperty_set(this, 4053482092, value); }
  get SwingLimitMode(): Enum.PxJointLimitMode { return __CPP_ComponentProperty_get(this, 643857727); }
  set SwingLimitMode(value: Enum.PxJointLimitMode) { __CPP_ComponentProperty_set(this, 643857727, value); }
  get SwingLimit(): number { return __CPP_ComponentProperty_get(this, 839016035); }
  set SwingLimit(value: number) { __CPP_ComponentProperty_set(this, 839016035, value); }
  get SwingStiffness(): number { return __CPP_ComponentProperty_get(this, 273798875); }
  set SwingStiffness(value: number) { __CPP_ComponentProperty_set(this, 273798875, value); }
  get SwingDamping(): number { return __CPP_ComponentProperty_get(this, 1017502676); }
  set SwingDamping(value: number) { __CPP_ComponentProperty_set(this, 1017502676, value); }
  get TwistLimitMode(): Enum.PxJointLimitMode { return __CPP_ComponentProperty_get(this, 1929151164); }
  set TwistLimitMode(value: Enum.PxJointLimitMode) { __CPP_ComponentProperty_set(this, 1929151164, value); }
  get LowerTwistLimit(): number { return __CPP_ComponentProperty_get(this, 3069009615); }
  set LowerTwistLimit(value: number) { __CPP_ComponentProperty_set(this, 3069009615, value); }
  get UpperTwistLimit(): number { return __CPP_ComponentProperty_get(this, 1026105598); }
  set UpperTwistLimit(value: number) { __CPP_ComponentProperty_set(this, 1026105598, value); }
  get TwistStiffness(): number { return __CPP_ComponentProperty_get(this, 1631603378); }
  set TwistStiffness(value: number) { __CPP_ComponentProperty_set(this, 1631603378, value); }
  get TwistDamping(): number { return __CPP_ComponentProperty_get(this, 2687804960); }
  set TwistDamping(value: number) { __CPP_ComponentProperty_set(this, 2687804960, value); }
}

export class PxActorComponent extends PxComponent
{
  public static GetTypeNameHash(): number { return 3483224225; }
}

export class PxCenterOfMassComponent extends PxComponent
{
  public static GetTypeNameHash(): number { return 3286487091; }
}

export class PxCharacterShapeComponent extends PxComponent
{
  public static GetTypeNameHash(): number { return 1343544267; }
  GetCollisionFlags(): Flags.PxCharacterShapeCollisionFlags { return __CPP_ComponentFunction_Call(this, 3330305547); }
  IsTouchingGround(): boolean { return __CPP_ComponentFunction_Call(this, 3610679602); }
  GetShapeId(): number { return __CPP_ComponentFunction_Call(this, 3217763362); }
  GetCurrentHeightValue(): number { return __CPP_ComponentFunction_Call(this, 430286784); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 1549417725); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 1549417725, value); }
  get Mass(): number { return __CPP_ComponentProperty_get(this, 2216264431); }
  set Mass(value: number) { __CPP_ComponentProperty_set(this, 2216264431, value); }
  get MaxStepHeight(): number { return __CPP_ComponentProperty_get(this, 1324024427); }
  set MaxStepHeight(value: number) { __CPP_ComponentProperty_set(this, 1324024427, value); }
  get MaxSlopeAngle(): number { return __CPP_ComponentProperty_get(this, 1704527751); }
  set MaxSlopeAngle(value: number) { __CPP_ComponentProperty_set(this, 1704527751, value); }
  get ForceSlopeSliding(): boolean { return __CPP_ComponentProperty_get(this, 2060790178); }
  set ForceSlopeSliding(value: boolean) { __CPP_ComponentProperty_set(this, 2060790178, value); }
  get ConstrainedClimbMode(): boolean { return __CPP_ComponentProperty_get(this, 1234087231); }
  set ConstrainedClimbMode(value: boolean) { __CPP_ComponentProperty_set(this, 1234087231, value); }
}

export class PxCharacterCapsuleShapeComponent extends PxCharacterShapeComponent
{
  public static GetTypeNameHash(): number { return 1477791513; }
  get CapsuleHeight(): number { return __CPP_ComponentProperty_get(this, 2532689984); }
  set CapsuleHeight(value: number) { __CPP_ComponentProperty_set(this, 2532689984, value); }
  get CapsuleRadius(): number { return __CPP_ComponentProperty_get(this, 4265961598); }
  set CapsuleRadius(value: number) { __CPP_ComponentProperty_set(this, 4265961598, value); }
}

export class PxCharacterControllerComponent extends CharacterControllerComponent
{
  public static GetTypeNameHash(): number { return 2853557721; }
  get RotateSpeed(): number { return __CPP_ComponentProperty_get(this, 1102556006); }
  set RotateSpeed(value: number) { __CPP_ComponentProperty_set(this, 1102556006, value); }
  get WalkSpeed(): number { return __CPP_ComponentProperty_get(this, 234260316); }
  set WalkSpeed(value: number) { __CPP_ComponentProperty_set(this, 234260316, value); }
  get RunSpeed(): number { return __CPP_ComponentProperty_get(this, 591224616); }
  set RunSpeed(value: number) { __CPP_ComponentProperty_set(this, 591224616, value); }
  get AirSpeed(): number { return __CPP_ComponentProperty_get(this, 2840707501); }
  set AirSpeed(value: number) { __CPP_ComponentProperty_set(this, 2840707501, value); }
  get AirFriction(): number { return __CPP_ComponentProperty_get(this, 3801722069); }
  set AirFriction(value: number) { __CPP_ComponentProperty_set(this, 3801722069, value); }
  get CrouchHeight(): number { return __CPP_ComponentProperty_get(this, 4009952502); }
  set CrouchHeight(value: number) { __CPP_ComponentProperty_set(this, 4009952502, value); }
  get CrouchSpeed(): number { return __CPP_ComponentProperty_get(this, 1197055873); }
  set CrouchSpeed(value: number) { __CPP_ComponentProperty_set(this, 1197055873, value); }
  get JumpImpulse(): number { return __CPP_ComponentProperty_get(this, 1195185450); }
  set JumpImpulse(value: number) { __CPP_ComponentProperty_set(this, 1195185450, value); }
  get PushingForce(): number { return __CPP_ComponentProperty_get(this, 56035031); }
  set PushingForce(value: number) { __CPP_ComponentProperty_set(this, 56035031, value); }
  get WalkSurfaceInteraction(): string { return __CPP_ComponentProperty_get(this, 2991904957); }
  set WalkSurfaceInteraction(value: string) { __CPP_ComponentProperty_set(this, 2991904957, value); }
  get WalkInteractionDistance(): number { return __CPP_ComponentProperty_get(this, 184574390); }
  set WalkInteractionDistance(value: number) { __CPP_ComponentProperty_set(this, 184574390, value); }
  get RunInteractionDistance(): number { return __CPP_ComponentProperty_get(this, 2247468653); }
  set RunInteractionDistance(value: number) { __CPP_ComponentProperty_set(this, 2247468653, value); }
  get FallbackWalkSurface(): string { return __CPP_ComponentProperty_get(this, 750633846); }
  set FallbackWalkSurface(value: string) { __CPP_ComponentProperty_set(this, 750633846, value); }
  get HeadObject(): string { return __CPP_ComponentProperty_get(this, 1526065600); }
  set HeadObject(value: string) { __CPP_ComponentProperty_set(this, 1526065600, value); }
}

export class PxDistanceJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 1215632847; }
  get MinDistance(): number { return __CPP_ComponentProperty_get(this, 3870659725); }
  set MinDistance(value: number) { __CPP_ComponentProperty_set(this, 3870659725, value); }
  get MaxDistance(): number { return __CPP_ComponentProperty_get(this, 2621264901); }
  set MaxDistance(value: number) { __CPP_ComponentProperty_set(this, 2621264901, value); }
  get SpringStiffness(): number { return __CPP_ComponentProperty_get(this, 1436087603); }
  set SpringStiffness(value: number) { __CPP_ComponentProperty_set(this, 1436087603, value); }
  get SpringDamping(): number { return __CPP_ComponentProperty_get(this, 2168272315); }
  set SpringDamping(value: number) { __CPP_ComponentProperty_set(this, 2168272315, value); }
  get SpringTolerance(): number { return __CPP_ComponentProperty_get(this, 2690036870); }
  set SpringTolerance(value: number) { __CPP_ComponentProperty_set(this, 2690036870, value); }
}

export class PxDynamicActorComponent extends PxActorComponent
{
  public static GetTypeNameHash(): number { return 1966010478; }
  AddLinearForce(vForce: Vec3): void { __CPP_ComponentFunction_Call(this, 2540848727, vForce); }
  AddLinearImpulse(vImpulse: Vec3): void { __CPP_ComponentFunction_Call(this, 2649785483, vImpulse); }
  AddAngularForce(vForce: Vec3): void { __CPP_ComponentFunction_Call(this, 1959571468, vForce); }
  AddAngularImpulse(vImpulse: Vec3): void { __CPP_ComponentFunction_Call(this, 1030459233, vImpulse); }
  GetLocalCenterOfMass(): Vec3 { return __CPP_ComponentFunction_Call(this, 1669959262); }
  GetGlobalCenterOfMass(): Vec3 { return __CPP_ComponentFunction_Call(this, 578104259); }
  get Kinematic(): boolean { return __CPP_ComponentProperty_get(this, 1795006776); }
  set Kinematic(value: boolean) { __CPP_ComponentProperty_set(this, 1795006776, value); }
  get Mass(): number { return __CPP_ComponentProperty_get(this, 3787959933); }
  set Mass(value: number) { __CPP_ComponentProperty_set(this, 3787959933, value); }
  get Density(): number { return __CPP_ComponentProperty_get(this, 2687085476); }
  set Density(value: number) { __CPP_ComponentProperty_set(this, 2687085476, value); }
  get DisableGravity(): boolean { return __CPP_ComponentProperty_get(this, 269482962); }
  set DisableGravity(value: boolean) { __CPP_ComponentProperty_set(this, 269482962, value); }
  get LinearDamping(): number { return __CPP_ComponentProperty_get(this, 3759843815); }
  set LinearDamping(value: number) { __CPP_ComponentProperty_set(this, 3759843815, value); }
  get AngularDamping(): number { return __CPP_ComponentProperty_get(this, 1997839320); }
  set AngularDamping(value: number) { __CPP_ComponentProperty_set(this, 1997839320, value); }
  get MaxContactImpulse(): number { return __CPP_ComponentProperty_get(this, 1214436601); }
  set MaxContactImpulse(value: number) { __CPP_ComponentProperty_set(this, 1214436601, value); }
  get ContinuousCollisionDetection(): boolean { return __CPP_ComponentProperty_get(this, 633091624); }
  set ContinuousCollisionDetection(value: boolean) { __CPP_ComponentProperty_set(this, 633091624, value); }
}

export class PxFixedJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 1139022332; }
}

export class PxGrabObjectComponent extends PxComponent
{
  public static GetTypeNameHash(): number { return 4233636810; }
  GrabNearbyObject(): boolean { return __CPP_ComponentFunction_Call(this, 3090747394); }
  HasObjectGrabbed(): boolean { return __CPP_ComponentFunction_Call(this, 2173623409); }
  DropGrabbedObject(): void { __CPP_ComponentFunction_Call(this, 3518624938); }
  ThrowGrabbedObject(Direction: Vec3): void { __CPP_ComponentFunction_Call(this, 717754895, Direction); }
  BreakObjectGrab(): void { __CPP_ComponentFunction_Call(this, 3435321019); }
  get MaxGrabPointDistance(): number { return __CPP_ComponentProperty_get(this, 1601822916); }
  set MaxGrabPointDistance(value: number) { __CPP_ComponentProperty_set(this, 1601822916, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 4284917885); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 4284917885, value); }
  get SpringStiffness(): number { return __CPP_ComponentProperty_get(this, 3611446090); }
  set SpringStiffness(value: number) { __CPP_ComponentProperty_set(this, 3611446090, value); }
  get SpringDamping(): number { return __CPP_ComponentProperty_get(this, 2631266268); }
  set SpringDamping(value: number) { __CPP_ComponentProperty_set(this, 2631266268, value); }
  get BreakDistance(): number { return __CPP_ComponentProperty_get(this, 1925322734); }
  set BreakDistance(value: number) { __CPP_ComponentProperty_set(this, 1925322734, value); }
  get AttachTo(): string { return __CPP_ComponentProperty_get(this, 3611666549); }
  set AttachTo(value: string) { __CPP_ComponentProperty_set(this, 3611666549, value); }
  get GrabAnyObjectWithSize(): number { return __CPP_ComponentProperty_get(this, 798805724); }
  set GrabAnyObjectWithSize(value: number) { __CPP_ComponentProperty_set(this, 798805724, value); }
}

export class PxPrismaticJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 1626170948; }
  get LimitMode(): Enum.PxJointLimitMode { return __CPP_ComponentProperty_get(this, 2540655324); }
  set LimitMode(value: Enum.PxJointLimitMode) { __CPP_ComponentProperty_set(this, 2540655324, value); }
  get LowerLimit(): number { return __CPP_ComponentProperty_get(this, 2382209319); }
  set LowerLimit(value: number) { __CPP_ComponentProperty_set(this, 2382209319, value); }
  get UpperLimit(): number { return __CPP_ComponentProperty_get(this, 804209440); }
  set UpperLimit(value: number) { __CPP_ComponentProperty_set(this, 804209440, value); }
  get SpringStiffness(): number { return __CPP_ComponentProperty_get(this, 1734299754); }
  set SpringStiffness(value: number) { __CPP_ComponentProperty_set(this, 1734299754, value); }
  get SpringDamping(): number { return __CPP_ComponentProperty_get(this, 2882559108); }
  set SpringDamping(value: number) { __CPP_ComponentProperty_set(this, 2882559108, value); }
}

export class PxRevoluteJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 2180974280; }
  get LimitMode(): Enum.PxJointLimitMode { return __CPP_ComponentProperty_get(this, 725199849); }
  set LimitMode(value: Enum.PxJointLimitMode) { __CPP_ComponentProperty_set(this, 725199849, value); }
  get LowerLimit(): number { return __CPP_ComponentProperty_get(this, 2109587495); }
  set LowerLimit(value: number) { __CPP_ComponentProperty_set(this, 2109587495, value); }
  get UpperLimit(): number { return __CPP_ComponentProperty_get(this, 477029198); }
  set UpperLimit(value: number) { __CPP_ComponentProperty_set(this, 477029198, value); }
  get SpringStiffness(): number { return __CPP_ComponentProperty_get(this, 1637387936); }
  set SpringStiffness(value: number) { __CPP_ComponentProperty_set(this, 1637387936, value); }
  get SpringDamping(): number { return __CPP_ComponentProperty_get(this, 457950255); }
  set SpringDamping(value: number) { __CPP_ComponentProperty_set(this, 457950255, value); }
  get DriveMode(): Enum.PxJointDriveMode { return __CPP_ComponentProperty_get(this, 1745672202); }
  set DriveMode(value: Enum.PxJointDriveMode) { __CPP_ComponentProperty_set(this, 1745672202, value); }
  get DriveVelocity(): number { return __CPP_ComponentProperty_get(this, 1918282165); }
  set DriveVelocity(value: number) { __CPP_ComponentProperty_set(this, 1918282165, value); }
  get MaxDriveTorque(): number { return __CPP_ComponentProperty_get(this, 1838188652); }
  set MaxDriveTorque(value: number) { __CPP_ComponentProperty_set(this, 1838188652, value); }
}

export class PxSettingsComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 2755465365; }
  get ObjectGravity(): Vec3 { return __CPP_ComponentProperty_get(this, 1260658321); }
  set ObjectGravity(value: Vec3) { __CPP_ComponentProperty_set(this, 1260658321, value); }
  get CharacterGravity(): Vec3 { return __CPP_ComponentProperty_get(this, 2023339447); }
  set CharacterGravity(value: Vec3) { __CPP_ComponentProperty_set(this, 2023339447, value); }
  get MaxDepenetrationVelocity(): number { return __CPP_ComponentProperty_get(this, 3509969829); }
  set MaxDepenetrationVelocity(value: number) { __CPP_ComponentProperty_set(this, 3509969829, value); }
  get SteppingMode(): Enum.PxSteppingMode { return __CPP_ComponentProperty_get(this, 3942726414); }
  set SteppingMode(value: Enum.PxSteppingMode) { __CPP_ComponentProperty_set(this, 3942726414, value); }
  get FixedFrameRate(): number { return __CPP_ComponentProperty_get(this, 4112920660); }
  set FixedFrameRate(value: number) { __CPP_ComponentProperty_set(this, 4112920660, value); }
  get MaxSubSteps(): number { return __CPP_ComponentProperty_get(this, 2182652505); }
  set MaxSubSteps(value: number) { __CPP_ComponentProperty_set(this, 2182652505, value); }
}

export class PxShapeComponent extends PxComponent
{
  public static GetTypeNameHash(): number { return 1749932415; }
  GetShapeId(): number { return __CPP_ComponentFunction_Call(this, 1164675636); }
  get Surface(): string { return __CPP_ComponentProperty_get(this, 2010635400); }
  set Surface(value: string) { __CPP_ComponentProperty_set(this, 2010635400, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 3242337277); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 3242337277, value); }
  get OnContact(): Flags.OnPhysXContact { return __CPP_ComponentProperty_get(this, 938330827); }
  set OnContact(value: Flags.OnPhysXContact) { __CPP_ComponentProperty_set(this, 938330827, value); }
}

export class PxShapeBoxComponent extends PxShapeComponent
{
  public static GetTypeNameHash(): number { return 1071573402; }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 1934199532); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 1934199532, value); }
}

export class PxShapeCapsuleComponent extends PxShapeComponent
{
  public static GetTypeNameHash(): number { return 4207776107; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 896423814); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 896423814, value); }
  get Height(): number { return __CPP_ComponentProperty_get(this, 1596242740); }
  set Height(value: number) { __CPP_ComponentProperty_set(this, 1596242740, value); }
}

export class PxShapeConvexComponent extends PxShapeComponent
{
  public static GetTypeNameHash(): number { return 1004389012; }
  get CollisionMesh(): string { return __CPP_ComponentProperty_get(this, 783392928); }
  set CollisionMesh(value: string) { __CPP_ComponentProperty_set(this, 783392928, value); }
}

export class PxShapeSphereComponent extends PxShapeComponent
{
  public static GetTypeNameHash(): number { return 2668079410; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 3724012172); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 3724012172, value); }
}

export class PxSphericalJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 1759322555; }
  get LimitMode(): Enum.PxJointLimitMode { return __CPP_ComponentProperty_get(this, 266412360); }
  set LimitMode(value: Enum.PxJointLimitMode) { __CPP_ComponentProperty_set(this, 266412360, value); }
  get ConeLimitY(): number { return __CPP_ComponentProperty_get(this, 712976609); }
  set ConeLimitY(value: number) { __CPP_ComponentProperty_set(this, 712976609, value); }
  get ConeLimitZ(): number { return __CPP_ComponentProperty_get(this, 1470971936); }
  set ConeLimitZ(value: number) { __CPP_ComponentProperty_set(this, 1470971936, value); }
  get SpringStiffness(): number { return __CPP_ComponentProperty_get(this, 916902239); }
  set SpringStiffness(value: number) { __CPP_ComponentProperty_set(this, 916902239, value); }
  get SpringDamping(): number { return __CPP_ComponentProperty_get(this, 1545087794); }
  set SpringDamping(value: number) { __CPP_ComponentProperty_set(this, 1545087794, value); }
}

export class PxStaticActorComponent extends PxActorComponent
{
  public static GetTypeNameHash(): number { return 1585245047; }
  GetShapeId(): number { return __CPP_ComponentFunction_Call(this, 2757948943); }
  get CollisionMesh(): string { return __CPP_ComponentProperty_get(this, 1358073866); }
  set CollisionMesh(value: string) { __CPP_ComponentProperty_set(this, 1358073866, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 1511485863); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 1511485863, value); }
  get IncludeInNavmesh(): boolean { return __CPP_ComponentProperty_get(this, 1790475479); }
  set IncludeInNavmesh(value: boolean) { __CPP_ComponentProperty_set(this, 1790475479, value); }
  get PullSurfacesFromGraphicsMesh(): boolean { return __CPP_ComponentProperty_get(this, 3294651339); }
  set PullSurfacesFromGraphicsMesh(value: boolean) { __CPP_ComponentProperty_set(this, 3294651339, value); }
}

export class PxTriggerComponent extends PxActorComponent
{
  public static GetTypeNameHash(): number { return 228526554; }
  get TriggerMessage(): string { return __CPP_ComponentProperty_get(this, 269552928); }
  set TriggerMessage(value: string) { __CPP_ComponentProperty_set(this, 269552928, value); }
}

export class PxVisColMeshComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3324320749; }
  get CollisionMesh(): string { return __CPP_ComponentProperty_get(this, 3956325505); }
  set CollisionMesh(value: string) { __CPP_ComponentProperty_set(this, 3956325505, value); }
}

export class RaycastComponent extends Component
{
  public static GetTypeNameHash(): number { return 3981854131; }
  get MaxDistance(): number { return __CPP_ComponentProperty_get(this, 3129640220); }
  set MaxDistance(value: number) { __CPP_ComponentProperty_set(this, 3129640220, value); }
  get DisableTargetObjectOnNoHit(): boolean { return __CPP_ComponentProperty_get(this, 3509569366); }
  set DisableTargetObjectOnNoHit(value: boolean) { __CPP_ComponentProperty_set(this, 3509569366, value); }
  get RaycastEndObject(): string { return __CPP_ComponentProperty_get(this, 256955472); }
  set RaycastEndObject(value: string) { __CPP_ComponentProperty_set(this, 256955472, value); }
  get ForceTargetParentless(): boolean { return __CPP_ComponentProperty_get(this, 3038098170); }
  set ForceTargetParentless(value: boolean) { __CPP_ComponentProperty_set(this, 3038098170, value); }
  get CollisionLayerEndPoint(): number { return __CPP_ComponentProperty_get(this, 2701207315); }
  set CollisionLayerEndPoint(value: number) { __CPP_ComponentProperty_set(this, 2701207315, value); }
  get CollisionLayerTrigger(): number { return __CPP_ComponentProperty_get(this, 3347131213); }
  set CollisionLayerTrigger(value: number) { __CPP_ComponentProperty_set(this, 3347131213, value); }
  get TriggerMessage(): string { return __CPP_ComponentProperty_get(this, 1847371601); }
  set TriggerMessage(value: string) { __CPP_ComponentProperty_set(this, 1847371601, value); }
}

export class RcAgentComponent extends AgentSteeringComponent
{
  public static GetTypeNameHash(): number { return 1633686969; }
  get WalkSpeed(): number { return __CPP_ComponentProperty_get(this, 2630372737); }
  set WalkSpeed(value: number) { __CPP_ComponentProperty_set(this, 2630372737, value); }
}

export class RcComponent extends Component
{
  public static GetTypeNameHash(): number { return 57241557; }
}

export class RcMarkPoiVisibleComponent extends RcComponent
{
  public static GetTypeNameHash(): number { return 4027749092; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 548239723); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 548239723, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 326049735); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 326049735, value); }
}

export class RcNavMeshComponent extends RcComponent
{
  public static GetTypeNameHash(): number { return 4272789835; }
  get ShowNavMesh(): boolean { return __CPP_ComponentProperty_get(this, 1380155385); }
  set ShowNavMesh(value: boolean) { __CPP_ComponentProperty_set(this, 1380155385, value); }
}

export class RenderTargetActivatorComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1107985400; }
  get RenderTarget(): string { return __CPP_ComponentProperty_get(this, 3381518269); }
  set RenderTarget(value: string) { __CPP_ComponentProperty_set(this, 3381518269, value); }
}

export class RmlUiCanvas2DComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1631281122; }
  get RmlFile(): string { return __CPP_ComponentProperty_get(this, 1744881067); }
  set RmlFile(value: string) { __CPP_ComponentProperty_set(this, 1744881067, value); }
  get AnchorPoint(): Vec2 { return __CPP_ComponentProperty_get(this, 2942900279); }
  set AnchorPoint(value: Vec2) { __CPP_ComponentProperty_set(this, 2942900279, value); }
  get Size(): Vec2 { return __CPP_ComponentProperty_get(this, 3509800902); }
  set Size(value: Vec2) { __CPP_ComponentProperty_set(this, 3509800902, value); }
  get Offset(): Vec2 { return __CPP_ComponentProperty_get(this, 2695024109); }
  set Offset(value: Vec2) { __CPP_ComponentProperty_set(this, 2695024109, value); }
  get PassInput(): boolean { return __CPP_ComponentProperty_get(this, 2027822126); }
  set PassInput(value: boolean) { __CPP_ComponentProperty_set(this, 2027822126, value); }
}

export class TransformComponent extends Component
{
  public static GetTypeNameHash(): number { return 2805885102; }
  SetDirectionForwards(Forwards: boolean): void { __CPP_ComponentFunction_Call(this, 1515239784, Forwards); }
  IsDirectionForwards(): boolean { return __CPP_ComponentFunction_Call(this, 4123231190); }
  ToggleDirection(): void { __CPP_ComponentFunction_Call(this, 2939572249); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 2767270139); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 2767270139, value); }
  get Running(): boolean { return __CPP_ComponentProperty_get(this, 637946864); }
  set Running(value: boolean) { __CPP_ComponentProperty_set(this, 637946864, value); }
  get ReverseAtEnd(): boolean { return __CPP_ComponentProperty_get(this, 370496401); }
  set ReverseAtEnd(value: boolean) { __CPP_ComponentProperty_set(this, 370496401, value); }
  get ReverseAtStart(): boolean { return __CPP_ComponentProperty_get(this, 942895673); }
  set ReverseAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 942895673, value); }
}

export class RotorComponent extends TransformComponent
{
  public static GetTypeNameHash(): number { return 3532285892; }
  get Axis(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 2940618167); }
  set Axis(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 2940618167, value); }
  get AxisDeviation(): number { return __CPP_ComponentProperty_get(this, 2309013532); }
  set AxisDeviation(value: number) { __CPP_ComponentProperty_set(this, 2309013532, value); }
  get DegreesToRotate(): number { return __CPP_ComponentProperty_get(this, 749788053); }
  set DegreesToRotate(value: number) { __CPP_ComponentProperty_set(this, 749788053, value); }
  get Acceleration(): number { return __CPP_ComponentProperty_get(this, 3848631741); }
  set Acceleration(value: number) { __CPP_ComponentProperty_set(this, 3848631741, value); }
  get Deceleration(): number { return __CPP_ComponentProperty_get(this, 2175680877); }
  set Deceleration(value: number) { __CPP_ComponentProperty_set(this, 2175680877, value); }
}

export class ShapeIconComponent extends Component
{
  public static GetTypeNameHash(): number { return 2471278007; }
}

export class SimpleWindComponent extends Component
{
  public static GetTypeNameHash(): number { return 3333981950; }
  get MinStrength(): number { return __CPP_ComponentProperty_get(this, 3645054854); }
  set MinStrength(value: number) { __CPP_ComponentProperty_set(this, 3645054854, value); }
  get MaxStrength(): number { return __CPP_ComponentProperty_get(this, 888413558); }
  set MaxStrength(value: number) { __CPP_ComponentProperty_set(this, 888413558, value); }
  get Deviation(): number { return __CPP_ComponentProperty_get(this, 3227694086); }
  set Deviation(value: number) { __CPP_ComponentProperty_set(this, 3227694086, value); }
}

export class SkyBoxComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 583885377; }
  get CubeMap(): string { return __CPP_ComponentProperty_get(this, 500355463); }
  set CubeMap(value: string) { __CPP_ComponentProperty_set(this, 500355463, value); }
  get ExposureBias(): number { return __CPP_ComponentProperty_get(this, 3387669692); }
  set ExposureBias(value: number) { __CPP_ComponentProperty_set(this, 3387669692, value); }
  get InverseTonemap(): boolean { return __CPP_ComponentProperty_get(this, 270595196); }
  set InverseTonemap(value: boolean) { __CPP_ComponentProperty_set(this, 270595196, value); }
  get UseFog(): boolean { return __CPP_ComponentProperty_get(this, 3763144241); }
  set UseFog(value: boolean) { __CPP_ComponentProperty_set(this, 3763144241, value); }
  get VirtualDistance(): number { return __CPP_ComponentProperty_get(this, 109965805); }
  set VirtualDistance(value: number) { __CPP_ComponentProperty_set(this, 109965805, value); }
}

export class SkyLightComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 3893160818; }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 173401106); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 173401106, value); }
  get Saturation(): number { return __CPP_ComponentProperty_get(this, 3724971240); }
  set Saturation(value: number) { __CPP_ComponentProperty_set(this, 3724971240, value); }
}

export class SliderComponent extends TransformComponent
{
  public static GetTypeNameHash(): number { return 3631651564; }
  get Axis(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 1660009473); }
  set Axis(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 1660009473, value); }
  get Distance(): number { return __CPP_ComponentProperty_get(this, 2730708928); }
  set Distance(value: number) { __CPP_ComponentProperty_set(this, 2730708928, value); }
  get Acceleration(): number { return __CPP_ComponentProperty_get(this, 3134755983); }
  set Acceleration(value: number) { __CPP_ComponentProperty_set(this, 3134755983, value); }
  get Deceleration(): number { return __CPP_ComponentProperty_get(this, 196327998); }
  set Deceleration(value: number) { __CPP_ComponentProperty_set(this, 196327998, value); }
  get RandomStart(): number { return __CPP_ComponentProperty_get(this, 3580817675); }
  set RandomStart(value: number) { __CPP_ComponentProperty_set(this, 3580817675, value); }
}

export class SoldierComponent extends NpcComponent
{
  public static GetTypeNameHash(): number { return 2976010003; }
}

export class SpatialAnchorComponent extends Component
{
  public static GetTypeNameHash(): number { return 1981633881; }
}

export class SpawnComponent extends Component
{
  public static GetTypeNameHash(): number { return 3634599688; }
  CanTriggerManualSpawn(): boolean { return __CPP_ComponentFunction_Call(this, 1549970299); }
  TriggerManualSpawn(IgnoreSpawnDelay: boolean, LocalOffset: Vec3): boolean { return __CPP_ComponentFunction_Call(this, 2121412534, IgnoreSpawnDelay, LocalOffset); }
  ScheduleSpawn(): void { __CPP_ComponentFunction_Call(this, 2352409150); }
  get Prefab(): string { return __CPP_ComponentProperty_get(this, 711830455); }
  set Prefab(value: string) { __CPP_ComponentProperty_set(this, 711830455, value); }
  get AttachAsChild(): boolean { return __CPP_ComponentProperty_get(this, 3780515770); }
  set AttachAsChild(value: boolean) { __CPP_ComponentProperty_set(this, 3780515770, value); }
  get SpawnAtStart(): boolean { return __CPP_ComponentProperty_get(this, 1858674518); }
  set SpawnAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 1858674518, value); }
  get SpawnContinuously(): boolean { return __CPP_ComponentProperty_get(this, 1553005908); }
  set SpawnContinuously(value: boolean) { __CPP_ComponentProperty_set(this, 1553005908, value); }
  get MinDelay(): number { return __CPP_ComponentProperty_get(this, 1823020091); }
  set MinDelay(value: number) { __CPP_ComponentProperty_set(this, 1823020091, value); }
  get DelayRange(): number { return __CPP_ComponentProperty_get(this, 54640091); }
  set DelayRange(value: number) { __CPP_ComponentProperty_set(this, 54640091, value); }
  get Deviation(): number { return __CPP_ComponentProperty_get(this, 2605521762); }
  set Deviation(value: number) { __CPP_ComponentProperty_set(this, 2605521762, value); }
}

export class SpotLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 287001999; }
  get Range(): number { return __CPP_ComponentProperty_get(this, 3291111878); }
  set Range(value: number) { __CPP_ComponentProperty_set(this, 3291111878, value); }
  get InnerSpotAngle(): number { return __CPP_ComponentProperty_get(this, 2326027726); }
  set InnerSpotAngle(value: number) { __CPP_ComponentProperty_set(this, 2326027726, value); }
  get OuterSpotAngle(): number { return __CPP_ComponentProperty_get(this, 2074056982); }
  set OuterSpotAngle(value: number) { __CPP_ComponentProperty_set(this, 2074056982, value); }
}

export class SpriteComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1509537617; }
  get Texture(): string { return __CPP_ComponentProperty_get(this, 2205201189); }
  set Texture(value: string) { __CPP_ComponentProperty_set(this, 2205201189, value); }
  get BlendMode(): Enum.SpriteBlendMode { return __CPP_ComponentProperty_get(this, 4053088110); }
  set BlendMode(value: Enum.SpriteBlendMode) { __CPP_ComponentProperty_set(this, 4053088110, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 3338557884); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 3338557884, value); }
  get Size(): number { return __CPP_ComponentProperty_get(this, 3309575041); }
  set Size(value: number) { __CPP_ComponentProperty_set(this, 3309575041, value); }
  get MaxScreenSize(): number { return __CPP_ComponentProperty_get(this, 3036624617); }
  set MaxScreenSize(value: number) { __CPP_ComponentProperty_set(this, 3036624617, value); }
  get AspectRatio(): number { return __CPP_ComponentProperty_get(this, 2925551062); }
  set AspectRatio(value: number) { __CPP_ComponentProperty_set(this, 2925551062, value); }
}

export class SrmRenderComponent extends Component
{
  public static GetTypeNameHash(): number { return 3450893762; }
  get Material(): string { return __CPP_ComponentProperty_get(this, 2802000997); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 2802000997, value); }
}

export class StageSpaceComponent extends Component
{
  public static GetTypeNameHash(): number { return 3115872394; }
  get StageSpace(): Enum.XRStageSpace { return __CPP_ComponentProperty_get(this, 3642981443); }
  set StageSpace(value: Enum.XRStageSpace) { __CPP_ComponentProperty_set(this, 3642981443, value); }
}

export class TimedDeathComponent extends Component
{
  public static GetTypeNameHash(): number { return 1522552562; }
  get MinDelay(): number { return __CPP_ComponentProperty_get(this, 2164537704); }
  set MinDelay(value: number) { __CPP_ComponentProperty_set(this, 2164537704, value); }
  get DelayRange(): number { return __CPP_ComponentProperty_get(this, 2783730664); }
  set DelayRange(value: number) { __CPP_ComponentProperty_set(this, 2783730664, value); }
  get TimeoutPrefab(): string { return __CPP_ComponentProperty_get(this, 3580428685); }
  set TimeoutPrefab(value: string) { __CPP_ComponentProperty_set(this, 3580428685, value); }
}

export class VisualScriptComponent extends EventMessageHandlerComponent
{
  public static GetTypeNameHash(): number { return 1506397838; }
  get Script(): string { return __CPP_ComponentProperty_get(this, 4204456658); }
  set Script(value: string) { __CPP_ComponentProperty_set(this, 4204456658, value); }
}

export class VisualizeHandComponent extends Component
{
  public static GetTypeNameHash(): number { return 606529222; }
}

export class VisualizeSkeletonComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 4224311822; }
  get Skeleton(): string { return __CPP_ComponentProperty_get(this, 2496540722); }
  set Skeleton(value: string) { __CPP_ComponentProperty_set(this, 2496540722, value); }
}

