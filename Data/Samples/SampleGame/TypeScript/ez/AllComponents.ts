
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

export class DebugRenderComponent extends Component
{
  public static GetTypeNameHash(): number { return 2150378396; }
  SetRandomColor(): void { __CPP_ComponentFunction_Call(this, 1246795232); }
  get Size(): number { return __CPP_ComponentProperty_get(this, 602693538); }
  set Size(value: number) { __CPP_ComponentProperty_set(this, 602693538, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 1975722182); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 1975722182, value); }
  get Texture(): string { return __CPP_ComponentProperty_get(this, 3130406697); }
  set Texture(value: string) { __CPP_ComponentProperty_set(this, 3130406697, value); }
  get Render(): Flags.DebugRenderComponentMask { return __CPP_ComponentProperty_get(this, 1810477408); }
  set Render(value: Flags.DebugRenderComponentMask) { __CPP_ComponentProperty_set(this, 1810477408, value); }
  get CustomData(): string { return __CPP_ComponentProperty_get(this, 3191784852); }
  set CustomData(value: string) { __CPP_ComponentProperty_set(this, 3191784852, value); }
}

export class DemoComponent extends Component
{
  public static GetTypeNameHash(): number { return 3019096534; }
  get Amplitude(): number { return __CPP_ComponentProperty_get(this, 2514465050); }
  set Amplitude(value: number) { __CPP_ComponentProperty_set(this, 2514465050, value); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 2684019077); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 2684019077, value); }
}

export class DisplayMsgComponent extends Component
{
  public static GetTypeNameHash(): number { return 1799170045; }
}

export class SendMsgComponent extends Component
{
  public static GetTypeNameHash(): number { return 2012803944; }
}

export class AimIKComponent extends Component
{
  public static GetTypeNameHash(): number { return 4138041976; }
  get ForwardVector(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 1206508230); }
  set ForwardVector(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 1206508230, value); }
  get UpVector(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 694268853); }
  set UpVector(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 694268853, value); }
  get PoleVector(): string { return __CPP_ComponentProperty_get(this, 390414855); }
  set PoleVector(value: string) { __CPP_ComponentProperty_set(this, 390414855, value); }
  get Weight(): number { return __CPP_ComponentProperty_get(this, 4062761388); }
  set Weight(value: number) { __CPP_ComponentProperty_set(this, 4062761388, value); }
}

export class RenderComponent extends Component
{
  public static GetTypeNameHash(): number { return 1331355654; }
}

export class AlwaysVisibleComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3808449846; }
}

export class SettingsComponent extends Component
{
  public static GetTypeNameHash(): number { return 1750614835; }
}

export class AmbientLightComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 1655272966; }
  get TopColor(): Color { return __CPP_ComponentProperty_get(this, 4055584569); }
  set TopColor(value: Color) { __CPP_ComponentProperty_set(this, 4055584569, value); }
  get BottomColor(): Color { return __CPP_ComponentProperty_get(this, 3763477789); }
  set BottomColor(value: Color) { __CPP_ComponentProperty_set(this, 3763477789, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 1142599221); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 1142599221, value); }
}

export class MeshComponentBase extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3838468365; }
}

export class AnimatedMeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 1356608274; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 946325768); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 946325768, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 1787449362); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 1787449362, value); }
}

export class AnimationControllerComponent extends Component
{
  public static GetTypeNameHash(): number { return 1449604100; }
  get AnimGraph(): string { return __CPP_ComponentProperty_get(this, 2031661517); }
  set AnimGraph(value: string) { __CPP_ComponentProperty_set(this, 2031661517, value); }
  get RootMotionMode(): Enum.RootMotionMode { return __CPP_ComponentProperty_get(this, 1675822524); }
  set RootMotionMode(value: Enum.RootMotionMode) { __CPP_ComponentProperty_set(this, 1675822524, value); }
  get InvisibleUpdateRate(): Enum.AnimationInvisibleUpdateRate { return __CPP_ComponentProperty_get(this, 984328090); }
  set InvisibleUpdateRate(value: Enum.AnimationInvisibleUpdateRate) { __CPP_ComponentProperty_set(this, 984328090, value); }
  get EnableIK(): boolean { return __CPP_ComponentProperty_get(this, 259543281); }
  set EnableIK(value: boolean) { __CPP_ComponentProperty_set(this, 259543281, value); }
}

export class BakedProbesComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 1952482886; }
  get ShowDebugOverlay(): boolean { return __CPP_ComponentProperty_get(this, 3724333294); }
  set ShowDebugOverlay(value: boolean) { __CPP_ComponentProperty_set(this, 3724333294, value); }
  get ShowDebugProbes(): boolean { return __CPP_ComponentProperty_get(this, 1955017905); }
  set ShowDebugProbes(value: boolean) { __CPP_ComponentProperty_set(this, 1955017905, value); }
  get UseTestPosition(): boolean { return __CPP_ComponentProperty_get(this, 1061481306); }
  set UseTestPosition(value: boolean) { __CPP_ComponentProperty_set(this, 1061481306, value); }
  get TestPosition(): Vec3 { return __CPP_ComponentProperty_get(this, 3969573931); }
  set TestPosition(value: Vec3) { __CPP_ComponentProperty_set(this, 3969573931, value); }
}

export class BakedProbesVolumeComponent extends Component
{
  public static GetTypeNameHash(): number { return 1354454587; }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 4040384027); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 4040384027, value); }
}

export class BeamComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3054475186; }
  get TargetObject(): string { return __CPP_ComponentProperty_get(this, 4047713065); }
  set TargetObject(value: string) { __CPP_ComponentProperty_set(this, 4047713065, value); }
  get Material(): string { return __CPP_ComponentProperty_get(this, 742360141); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 742360141, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 1592367954); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 1592367954, value); }
  get Width(): number { return __CPP_ComponentProperty_get(this, 3161639904); }
  set Width(value: number) { __CPP_ComponentProperty_set(this, 3161639904, value); }
  get UVUnitsPerWorldUnit(): number { return __CPP_ComponentProperty_get(this, 676201184); }
  set UVUnitsPerWorldUnit(value: number) { __CPP_ComponentProperty_set(this, 676201184, value); }
}

export class BlackboardComponent extends Component
{
  public static GetTypeNameHash(): number { return 4004595404; }
  SetEntryValue(Name: string, Value: any): void { __CPP_ComponentFunction_Call(this, 1770691434, Name, Value); }
  GetEntryValue(Name: string): any { return __CPP_ComponentFunction_Call(this, 3444662205, Name); }
  get Template(): string { return __CPP_ComponentProperty_get(this, 1395642933); }
  set Template(value: string) { __CPP_ComponentProperty_set(this, 1395642933, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 3507638090); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 3507638090, value); }
}

export class ReflectionProbeComponentBase extends Component
{
  public static GetTypeNameHash(): number { return 1328595402; }
  get ReflectionProbeMode(): Enum.ReflectionProbeMode { return __CPP_ComponentProperty_get(this, 2861186312); }
  set ReflectionProbeMode(value: Enum.ReflectionProbeMode) { __CPP_ComponentProperty_set(this, 2861186312, value); }
  get NearPlane(): number { return __CPP_ComponentProperty_get(this, 3728348077); }
  set NearPlane(value: number) { __CPP_ComponentProperty_set(this, 3728348077, value); }
  get FarPlane(): number { return __CPP_ComponentProperty_get(this, 3749202860); }
  set FarPlane(value: number) { __CPP_ComponentProperty_set(this, 3749202860, value); }
  get CaptureOffset(): Vec3 { return __CPP_ComponentProperty_get(this, 995831503); }
  set CaptureOffset(value: Vec3) { __CPP_ComponentProperty_set(this, 995831503, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 2987052367); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 2987052367, value); }
  get ShowMipMaps(): boolean { return __CPP_ComponentProperty_get(this, 2694283106); }
  set ShowMipMaps(value: boolean) { __CPP_ComponentProperty_set(this, 2694283106, value); }
}

export class BoxReflectionProbeComponent extends ReflectionProbeComponentBase
{
  public static GetTypeNameHash(): number { return 2868354512; }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 1973454234); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 1973454234, value); }
  get InfluenceScale(): Vec3 { return __CPP_ComponentProperty_get(this, 3397494603); }
  set InfluenceScale(value: Vec3) { __CPP_ComponentProperty_set(this, 3397494603, value); }
  get InfluenceShift(): Vec3 { return __CPP_ComponentProperty_get(this, 3413820279); }
  set InfluenceShift(value: Vec3) { __CPP_ComponentProperty_set(this, 3413820279, value); }
  get PositiveFalloff(): Vec3 { return __CPP_ComponentProperty_get(this, 4154728610); }
  set PositiveFalloff(value: Vec3) { __CPP_ComponentProperty_set(this, 4154728610, value); }
  get NegativeFalloff(): Vec3 { return __CPP_ComponentProperty_get(this, 639334809); }
  set NegativeFalloff(value: Vec3) { __CPP_ComponentProperty_set(this, 639334809, value); }
  get BoxProjection(): boolean { return __CPP_ComponentProperty_get(this, 2435504746); }
  set BoxProjection(value: boolean) { __CPP_ComponentProperty_set(this, 2435504746, value); }
}

export class CameraComponent extends Component
{
  public static GetTypeNameHash(): number { return 399384073; }
  get EditorShortcut(): number { return __CPP_ComponentProperty_get(this, 3983516621); }
  set EditorShortcut(value: number) { __CPP_ComponentProperty_set(this, 3983516621, value); }
  get UsageHint(): Enum.CameraUsageHint { return __CPP_ComponentProperty_get(this, 377758853); }
  set UsageHint(value: Enum.CameraUsageHint) { __CPP_ComponentProperty_set(this, 377758853, value); }
  get Mode(): Enum.CameraMode { return __CPP_ComponentProperty_get(this, 2647822527); }
  set Mode(value: Enum.CameraMode) { __CPP_ComponentProperty_set(this, 2647822527, value); }
  get RenderTarget(): string { return __CPP_ComponentProperty_get(this, 2058514322); }
  set RenderTarget(value: string) { __CPP_ComponentProperty_set(this, 2058514322, value); }
  get RenderTargetOffset(): Vec2 { return __CPP_ComponentProperty_get(this, 217065525); }
  set RenderTargetOffset(value: Vec2) { __CPP_ComponentProperty_set(this, 217065525, value); }
  get RenderTargetSize(): Vec2 { return __CPP_ComponentProperty_get(this, 3840538309); }
  set RenderTargetSize(value: Vec2) { __CPP_ComponentProperty_set(this, 3840538309, value); }
  get NearPlane(): number { return __CPP_ComponentProperty_get(this, 2333990868); }
  set NearPlane(value: number) { __CPP_ComponentProperty_set(this, 2333990868, value); }
  get FarPlane(): number { return __CPP_ComponentProperty_get(this, 380471032); }
  set FarPlane(value: number) { __CPP_ComponentProperty_set(this, 380471032, value); }
  get FOV(): number { return __CPP_ComponentProperty_get(this, 159882140); }
  set FOV(value: number) { __CPP_ComponentProperty_set(this, 159882140, value); }
  get Dimensions(): number { return __CPP_ComponentProperty_get(this, 3619040875); }
  set Dimensions(value: number) { __CPP_ComponentProperty_set(this, 3619040875, value); }
  get CameraRenderPipeline(): string { return __CPP_ComponentProperty_get(this, 1461435618); }
  set CameraRenderPipeline(value: string) { __CPP_ComponentProperty_set(this, 1461435618, value); }
  get Aperture(): number { return __CPP_ComponentProperty_get(this, 2434424955); }
  set Aperture(value: number) { __CPP_ComponentProperty_set(this, 2434424955, value); }
  get ShutterTime(): number { return __CPP_ComponentProperty_get(this, 1229310217); }
  set ShutterTime(value: number) { __CPP_ComponentProperty_set(this, 1229310217, value); }
  get ISO(): number { return __CPP_ComponentProperty_get(this, 2015108855); }
  set ISO(value: number) { __CPP_ComponentProperty_set(this, 2015108855, value); }
  get ExposureCompensation(): number { return __CPP_ComponentProperty_get(this, 2490173364); }
  set ExposureCompensation(value: number) { __CPP_ComponentProperty_set(this, 2490173364, value); }
  get ShowStats(): boolean { return __CPP_ComponentProperty_get(this, 602803161); }
  set ShowStats(value: boolean) { __CPP_ComponentProperty_set(this, 602803161, value); }
}

export class CharacterControllerComponent extends Component
{
  public static GetTypeNameHash(): number { return 3855358184; }
  RawMove(moveDeltaGlobal: Vec3): void { __CPP_ComponentFunction_Call(this, 2989659834, moveDeltaGlobal); }
  TeleportCharacter(globalFootPosition: Vec3): void { __CPP_ComponentFunction_Call(this, 3942687383, globalFootPosition); }
  IsDestinationUnobstructed(globalFootPosition: Vec3, characterHeight: number): boolean { return __CPP_ComponentFunction_Call(this, 1759721276, globalFootPosition, characterHeight); }
  IsTouchingGround(): boolean { return __CPP_ComponentFunction_Call(this, 199969770); }
  IsCrouching(): boolean { return __CPP_ComponentFunction_Call(this, 2211445420); }
}

export class CollectionComponent extends Component
{
  public static GetTypeNameHash(): number { return 1842091837; }
  get Collection(): string { return __CPP_ComponentProperty_get(this, 1385393910); }
  set Collection(value: string) { __CPP_ComponentProperty_set(this, 1385393910, value); }
  get RegisterNames(): boolean { return __CPP_ComponentProperty_get(this, 3356805730); }
  set RegisterNames(value: boolean) { __CPP_ComponentProperty_set(this, 3356805730, value); }
}

export class ColorAnimationComponent extends Component
{
  public static GetTypeNameHash(): number { return 3982813566; }
  get Gradient(): string { return __CPP_ComponentProperty_get(this, 1405509525); }
  set Gradient(value: string) { __CPP_ComponentProperty_set(this, 1405509525, value); }
  get Duration(): number { return __CPP_ComponentProperty_get(this, 329619611); }
  set Duration(value: number) { __CPP_ComponentProperty_set(this, 329619611, value); }
  get SetColorMode(): Enum.SetColorMode { return __CPP_ComponentProperty_get(this, 3365169082); }
  set SetColorMode(value: Enum.SetColorMode) { __CPP_ComponentProperty_set(this, 3365169082, value); }
  get AnimationMode(): Enum.PropertyAnimMode { return __CPP_ComponentProperty_get(this, 850528577); }
  set AnimationMode(value: Enum.PropertyAnimMode) { __CPP_ComponentProperty_set(this, 850528577, value); }
  get RandomStartOffset(): boolean { return __CPP_ComponentProperty_get(this, 282178386); }
  set RandomStartOffset(value: boolean) { __CPP_ComponentProperty_set(this, 282178386, value); }
  get ApplyToChildren(): boolean { return __CPP_ComponentProperty_get(this, 1543662045); }
  set ApplyToChildren(value: boolean) { __CPP_ComponentProperty_set(this, 1543662045, value); }
}

export class CommentComponent extends Component
{
  public static GetTypeNameHash(): number { return 3498221260; }
  get Comment(): string { return __CPP_ComponentProperty_get(this, 3941988709); }
  set Comment(value: string) { __CPP_ComponentProperty_set(this, 3941988709, value); }
}

export class CustomMeshComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1082120609; }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 3679749726); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 3679749726, value); }
  get Material(): string { return __CPP_ComponentProperty_get(this, 3774417817); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 3774417817, value); }
}

export class DebugTextComponent extends Component
{
  public static GetTypeNameHash(): number { return 752500620; }
  get Text(): string { return __CPP_ComponentProperty_get(this, 676134654); }
  set Text(value: string) { __CPP_ComponentProperty_set(this, 676134654, value); }
  get Value0(): number { return __CPP_ComponentProperty_get(this, 869502225); }
  set Value0(value: number) { __CPP_ComponentProperty_set(this, 869502225, value); }
  get Value1(): number { return __CPP_ComponentProperty_get(this, 775705132); }
  set Value1(value: number) { __CPP_ComponentProperty_set(this, 775705132, value); }
  get Value2(): number { return __CPP_ComponentProperty_get(this, 1768733809); }
  set Value2(value: number) { __CPP_ComponentProperty_set(this, 1768733809, value); }
  get Value3(): number { return __CPP_ComponentProperty_get(this, 1816166542); }
  set Value3(value: number) { __CPP_ComponentProperty_set(this, 1816166542, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 1881354469); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 1881354469, value); }
  get MaxDistance(): number { return __CPP_ComponentProperty_get(this, 1272349630); }
  set MaxDistance(value: number) { __CPP_ComponentProperty_set(this, 1272349630, value); }
}

export class DecalComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2317758174; }
  get ProjectionAxis(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 2835729844); }
  set ProjectionAxis(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 2835729844, value); }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 1588442994); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 1588442994, value); }
  get SizeVariance(): number { return __CPP_ComponentProperty_get(this, 2095302921); }
  set SizeVariance(value: number) { __CPP_ComponentProperty_set(this, 2095302921, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2286083006); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2286083006, value); }
  get EmissiveColor(): Color { return __CPP_ComponentProperty_get(this, 2763898085); }
  set EmissiveColor(value: Color) { __CPP_ComponentProperty_set(this, 2763898085, value); }
  get SortOrder(): number { return __CPP_ComponentProperty_get(this, 2393756233); }
  set SortOrder(value: number) { __CPP_ComponentProperty_set(this, 2393756233, value); }
  get WrapAround(): boolean { return __CPP_ComponentProperty_get(this, 3404680148); }
  set WrapAround(value: boolean) { __CPP_ComponentProperty_set(this, 3404680148, value); }
  get MapNormalToGeometry(): boolean { return __CPP_ComponentProperty_get(this, 2511527943); }
  set MapNormalToGeometry(value: boolean) { __CPP_ComponentProperty_set(this, 2511527943, value); }
  get InnerFadeAngle(): number { return __CPP_ComponentProperty_get(this, 4277744763); }
  set InnerFadeAngle(value: number) { __CPP_ComponentProperty_set(this, 4277744763, value); }
  get OuterFadeAngle(): number { return __CPP_ComponentProperty_get(this, 2290285157); }
  set OuterFadeAngle(value: number) { __CPP_ComponentProperty_set(this, 2290285157, value); }
  get FadeOutDuration(): number { return __CPP_ComponentProperty_get(this, 4012256443); }
  set FadeOutDuration(value: number) { __CPP_ComponentProperty_set(this, 4012256443, value); }
  get OnFinishedAction(): Enum.OnComponentFinishedAction { return __CPP_ComponentProperty_get(this, 428775208); }
  set OnFinishedAction(value: Enum.OnComponentFinishedAction) { __CPP_ComponentProperty_set(this, 428775208, value); }
  get ApplyToDynamic(): string { return __CPP_ComponentProperty_get(this, 2082908993); }
  set ApplyToDynamic(value: string) { __CPP_ComponentProperty_set(this, 2082908993, value); }
}

export class DeviceTrackingComponent extends Component
{
  public static GetTypeNameHash(): number { return 3500774039; }
  get DeviceType(): Enum.XRDeviceType { return __CPP_ComponentProperty_get(this, 1370408087); }
  set DeviceType(value: Enum.XRDeviceType) { __CPP_ComponentProperty_set(this, 1370408087, value); }
  get PoseLocation(): Enum.XRPoseLocation { return __CPP_ComponentProperty_get(this, 90502617); }
  set PoseLocation(value: Enum.XRPoseLocation) { __CPP_ComponentProperty_set(this, 90502617, value); }
  get TransformSpace(): Enum.XRTransformSpace { return __CPP_ComponentProperty_get(this, 123171866); }
  set TransformSpace(value: Enum.XRTransformSpace) { __CPP_ComponentProperty_set(this, 123171866, value); }
  get Rotation(): boolean { return __CPP_ComponentProperty_get(this, 731647032); }
  set Rotation(value: boolean) { __CPP_ComponentProperty_set(this, 731647032, value); }
  get Scale(): boolean { return __CPP_ComponentProperty_get(this, 1034704341); }
  set Scale(value: boolean) { __CPP_ComponentProperty_set(this, 1034704341, value); }
}

export class LightComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3732905662; }
  get EffectiveColor(): Color { return __CPP_ComponentProperty_get(this, 1413990146); }
  set EffectiveColor(value: Color) { __CPP_ComponentProperty_set(this, 1413990146, value); }
  get UseColorTemperature(): boolean { return __CPP_ComponentProperty_get(this, 3390607811); }
  set UseColorTemperature(value: boolean) { __CPP_ComponentProperty_set(this, 3390607811, value); }
  get LightColor(): Color { return __CPP_ComponentProperty_get(this, 1934568030); }
  set LightColor(value: Color) { __CPP_ComponentProperty_set(this, 1934568030, value); }
  get Temperature(): number { return __CPP_ComponentProperty_get(this, 941002825); }
  set Temperature(value: number) { __CPP_ComponentProperty_set(this, 941002825, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 2874814740); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 2874814740, value); }
  get SpecularMultiplier(): number { return __CPP_ComponentProperty_get(this, 4242523270); }
  set SpecularMultiplier(value: number) { __CPP_ComponentProperty_set(this, 4242523270, value); }
  get CastShadows(): boolean { return __CPP_ComponentProperty_get(this, 250551917); }
  set CastShadows(value: boolean) { __CPP_ComponentProperty_set(this, 250551917, value); }
  get TransparentShadows(): boolean { return __CPP_ComponentProperty_get(this, 667984368); }
  set TransparentShadows(value: boolean) { __CPP_ComponentProperty_set(this, 667984368, value); }
  get PenumbraSize(): number { return __CPP_ComponentProperty_get(this, 3718860233); }
  set PenumbraSize(value: number) { __CPP_ComponentProperty_set(this, 3718860233, value); }
  get SlopeBias(): number { return __CPP_ComponentProperty_get(this, 3444063500); }
  set SlopeBias(value: number) { __CPP_ComponentProperty_set(this, 3444063500, value); }
  get ConstantBias(): number { return __CPP_ComponentProperty_get(this, 3565918648); }
  set ConstantBias(value: number) { __CPP_ComponentProperty_set(this, 3565918648, value); }
}

export class DirectionalLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 3687235078; }
  get NumCascades(): number { return __CPP_ComponentProperty_get(this, 4241707254); }
  set NumCascades(value: number) { __CPP_ComponentProperty_set(this, 4241707254, value); }
  get MinShadowRange(): number { return __CPP_ComponentProperty_get(this, 4063057648); }
  set MinShadowRange(value: number) { __CPP_ComponentProperty_set(this, 4063057648, value); }
  get FadeOutStart(): number { return __CPP_ComponentProperty_get(this, 1106469026); }
  set FadeOutStart(value: number) { __CPP_ComponentProperty_set(this, 1106469026, value); }
  get SplitModeWeight(): number { return __CPP_ComponentProperty_get(this, 2604859805); }
  set SplitModeWeight(value: number) { __CPP_ComponentProperty_set(this, 2604859805, value); }
  get NearPlaneOffset(): number { return __CPP_ComponentProperty_get(this, 1922790091); }
  set NearPlaneOffset(value: number) { __CPP_ComponentProperty_set(this, 1922790091, value); }
}

export class EventMessageHandlerComponent extends Component
{
  public static GetTypeNameHash(): number { return 2410982864; }
  get HandleGlobalEvents(): boolean { return __CPP_ComponentProperty_get(this, 363574155); }
  set HandleGlobalEvents(value: boolean) { __CPP_ComponentProperty_set(this, 363574155, value); }
  get PassThroughUnhandledEvents(): boolean { return __CPP_ComponentProperty_get(this, 527316798); }
  set PassThroughUnhandledEvents(value: boolean) { __CPP_ComponentProperty_set(this, 527316798, value); }
}

export class FillLightComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2966820638; }
  get EffectiveColor(): Color { return __CPP_ComponentProperty_get(this, 3568522479); }
  set EffectiveColor(value: Color) { __CPP_ComponentProperty_set(this, 3568522479, value); }
  get LightMode(): Enum.FillLightMode { return __CPP_ComponentProperty_get(this, 3384563433); }
  set LightMode(value: Enum.FillLightMode) { __CPP_ComponentProperty_set(this, 3384563433, value); }
  get UseColorTemperature(): boolean { return __CPP_ComponentProperty_get(this, 2218298512); }
  set UseColorTemperature(value: boolean) { __CPP_ComponentProperty_set(this, 2218298512, value); }
  get LightColor(): Color { return __CPP_ComponentProperty_get(this, 3175804825); }
  set LightColor(value: Color) { __CPP_ComponentProperty_set(this, 3175804825, value); }
  get Temperature(): number { return __CPP_ComponentProperty_get(this, 27945615); }
  set Temperature(value: number) { __CPP_ComponentProperty_set(this, 27945615, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 2509171759); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 2509171759, value); }
  get Range(): number { return __CPP_ComponentProperty_get(this, 198847836); }
  set Range(value: number) { __CPP_ComponentProperty_set(this, 198847836, value); }
  get FalloffExponent(): number { return __CPP_ComponentProperty_get(this, 243012702); }
  set FalloffExponent(value: number) { __CPP_ComponentProperty_set(this, 243012702, value); }
  get Directionality(): number { return __CPP_ComponentProperty_get(this, 3106718082); }
  set Directionality(value: number) { __CPP_ComponentProperty_set(this, 3106718082, value); }
}

export class FmodComponent extends Component
{
  public static GetTypeNameHash(): number { return 3900472244; }
}

export class FmodEventComponent extends FmodComponent
{
  public static GetTypeNameHash(): number { return 2814833380; }
  Restart(): void { __CPP_ComponentFunction_Call(this, 958670764); }
  StartOneShot(): void { __CPP_ComponentFunction_Call(this, 1161139504); }
  StopSound(Immediate: boolean): void { __CPP_ComponentFunction_Call(this, 502299734, Immediate); }
  SoundCue(): void { __CPP_ComponentFunction_Call(this, 366471496); }
  SetEventParameter(ParamName: string, Value: number): void { __CPP_ComponentFunction_Call(this, 2121876374, ParamName, Value); }
  get Paused(): boolean { return __CPP_ComponentProperty_get(this, 3424531128); }
  set Paused(value: boolean) { __CPP_ComponentProperty_set(this, 3424531128, value); }
  get Volume(): number { return __CPP_ComponentProperty_get(this, 4162464906); }
  set Volume(value: number) { __CPP_ComponentProperty_set(this, 4162464906, value); }
  get Pitch(): number { return __CPP_ComponentProperty_get(this, 3779070326); }
  set Pitch(value: number) { __CPP_ComponentProperty_set(this, 3779070326, value); }
  get NoGlobalPitch(): boolean { return __CPP_ComponentProperty_get(this, 2479668282); }
  set NoGlobalPitch(value: boolean) { __CPP_ComponentProperty_set(this, 2479668282, value); }
  get SoundEvent(): string { return __CPP_ComponentProperty_get(this, 2550082732); }
  set SoundEvent(value: string) { __CPP_ComponentProperty_set(this, 2550082732, value); }
  get UseOcclusion(): boolean { return __CPP_ComponentProperty_get(this, 1709596494); }
  set UseOcclusion(value: boolean) { __CPP_ComponentProperty_set(this, 1709596494, value); }
  get OcclusionThreshold(): number { return __CPP_ComponentProperty_get(this, 1819793160); }
  set OcclusionThreshold(value: number) { __CPP_ComponentProperty_set(this, 1819793160, value); }
  get OcclusionCollisionLayer(): number { return __CPP_ComponentProperty_get(this, 372633314); }
  set OcclusionCollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 372633314, value); }
  get OnFinishedAction(): Enum.OnComponentFinishedAction { return __CPP_ComponentProperty_get(this, 2674070426); }
  set OnFinishedAction(value: Enum.OnComponentFinishedAction) { __CPP_ComponentProperty_set(this, 2674070426, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 2784458228); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 2784458228, value); }
}

export class FmodListenerComponent extends FmodComponent
{
  public static GetTypeNameHash(): number { return 98225585; }
  get ListenerIndex(): number { return __CPP_ComponentProperty_get(this, 826440037); }
  set ListenerIndex(value: number) { __CPP_ComponentProperty_set(this, 826440037, value); }
}

export class FogComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 3095518117; }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 3730513872); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 3730513872, value); }
  get Density(): number { return __CPP_ComponentProperty_get(this, 3233698728); }
  set Density(value: number) { __CPP_ComponentProperty_set(this, 3233698728, value); }
  get HeightFalloff(): number { return __CPP_ComponentProperty_get(this, 1854481097); }
  set HeightFalloff(value: number) { __CPP_ComponentProperty_set(this, 1854481097, value); }
  get ModulateWithSkyColor(): boolean { return __CPP_ComponentProperty_get(this, 992867426); }
  set ModulateWithSkyColor(value: boolean) { __CPP_ComponentProperty_set(this, 992867426, value); }
  get SkyDistance(): number { return __CPP_ComponentProperty_get(this, 645510939); }
  set SkyDistance(value: number) { __CPP_ComponentProperty_set(this, 645510939, value); }
}

export class FollowPathComponent extends Component
{
  public static GetTypeNameHash(): number { return 3651741630; }
  SetDirectionForwards(Forwards: boolean): void { __CPP_ComponentFunction_Call(this, 1994925173, Forwards); }
  IsDirectionForwards(): boolean { return __CPP_ComponentFunction_Call(this, 2347550617); }
  ToggleDirection(): void { __CPP_ComponentFunction_Call(this, 1974632230); }
  get Path(): string { return __CPP_ComponentProperty_get(this, 372999738); }
  set Path(value: string) { __CPP_ComponentProperty_set(this, 372999738, value); }
  get StartDistance(): number { return __CPP_ComponentProperty_get(this, 837848361); }
  set StartDistance(value: number) { __CPP_ComponentProperty_set(this, 837848361, value); }
  get Running(): boolean { return __CPP_ComponentProperty_get(this, 623714660); }
  set Running(value: boolean) { __CPP_ComponentProperty_set(this, 623714660, value); }
  get Mode(): Enum.PropertyAnimMode { return __CPP_ComponentProperty_get(this, 3007335921); }
  set Mode(value: Enum.PropertyAnimMode) { __CPP_ComponentProperty_set(this, 3007335921, value); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 3476271465); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 3476271465, value); }
  get LookAhead(): number { return __CPP_ComponentProperty_get(this, 3077407718); }
  set LookAhead(value: number) { __CPP_ComponentProperty_set(this, 3077407718, value); }
  get Smoothing(): number { return __CPP_ComponentProperty_get(this, 3812626917); }
  set Smoothing(value: number) { __CPP_ComponentProperty_set(this, 3812626917, value); }
  get FollowMode(): Enum.FollowPathMode { return __CPP_ComponentProperty_get(this, 2961192936); }
  set FollowMode(value: Enum.FollowPathMode) { __CPP_ComponentProperty_set(this, 2961192936, value); }
  get TiltAmount(): number { return __CPP_ComponentProperty_get(this, 541978106); }
  set TiltAmount(value: number) { __CPP_ComponentProperty_set(this, 541978106, value); }
  get MaxTilt(): number { return __CPP_ComponentProperty_get(this, 783404318); }
  set MaxTilt(value: number) { __CPP_ComponentProperty_set(this, 783404318, value); }
}

export class ForwardEventsToGameStateComponent extends EventMessageHandlerComponent
{
  public static GetTypeNameHash(): number { return 1132316266; }
}

export class MeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 481758555; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 1864394775); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 1864394775, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 1079163548); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 1079163548, value); }
  get SortingDepthOffset(): number { return __CPP_ComponentProperty_get(this, 3099356804); }
  set SortingDepthOffset(value: number) { __CPP_ComponentProperty_set(this, 3099356804, value); }
}

export class GizmoComponent extends MeshComponent
{
  public static GetTypeNameHash(): number { return 1286849521; }
}

export class GlobalBlackboardComponent extends BlackboardComponent
{
  public static GetTypeNameHash(): number { return 3781981925; }
  get BlackboardName(): string { return __CPP_ComponentProperty_get(this, 2603695606); }
  set BlackboardName(value: string) { __CPP_ComponentProperty_set(this, 2603695606, value); }
  get InitMode(): Enum.GlobalBlackboardInitMode { return __CPP_ComponentProperty_get(this, 3699819868); }
  set InitMode(value: Enum.GlobalBlackboardInitMode) { __CPP_ComponentProperty_set(this, 3699819868, value); }
}

export class GrabbableItemComponent extends Component
{
  public static GetTypeNameHash(): number { return 3683886134; }
  get DebugShowPoints(): boolean { return __CPP_ComponentProperty_get(this, 1730674546); }
  set DebugShowPoints(value: boolean) { __CPP_ComponentProperty_set(this, 1730674546, value); }
}

export class GreyBoxComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1977995121; }
  get Shape(): Enum.GreyBoxShape { return __CPP_ComponentProperty_get(this, 861097748); }
  set Shape(value: Enum.GreyBoxShape) { __CPP_ComponentProperty_set(this, 861097748, value); }
  get Material(): string { return __CPP_ComponentProperty_get(this, 3493112051); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 3493112051, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 457155504); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 457155504, value); }
  get SizeNegX(): number { return __CPP_ComponentProperty_get(this, 1628214514); }
  set SizeNegX(value: number) { __CPP_ComponentProperty_set(this, 1628214514, value); }
  get SizePosX(): number { return __CPP_ComponentProperty_get(this, 608676239); }
  set SizePosX(value: number) { __CPP_ComponentProperty_set(this, 608676239, value); }
  get SizeNegY(): number { return __CPP_ComponentProperty_get(this, 1931762144); }
  set SizeNegY(value: number) { __CPP_ComponentProperty_set(this, 1931762144, value); }
  get SizePosY(): number { return __CPP_ComponentProperty_get(this, 3393461640); }
  set SizePosY(value: number) { __CPP_ComponentProperty_set(this, 3393461640, value); }
  get SizeNegZ(): number { return __CPP_ComponentProperty_get(this, 2278470836); }
  set SizeNegZ(value: number) { __CPP_ComponentProperty_set(this, 2278470836, value); }
  get SizePosZ(): number { return __CPP_ComponentProperty_get(this, 3664653916); }
  set SizePosZ(value: number) { __CPP_ComponentProperty_set(this, 3664653916, value); }
  get Detail(): number { return __CPP_ComponentProperty_get(this, 509925246); }
  set Detail(value: number) { __CPP_ComponentProperty_set(this, 509925246, value); }
  get Curvature(): number { return __CPP_ComponentProperty_get(this, 3496336964); }
  set Curvature(value: number) { __CPP_ComponentProperty_set(this, 3496336964, value); }
  get Thickness(): number { return __CPP_ComponentProperty_get(this, 2680750755); }
  set Thickness(value: number) { __CPP_ComponentProperty_set(this, 2680750755, value); }
  get SlopedTop(): boolean { return __CPP_ComponentProperty_get(this, 321690466); }
  set SlopedTop(value: boolean) { __CPP_ComponentProperty_set(this, 321690466, value); }
  get SlopedBottom(): boolean { return __CPP_ComponentProperty_get(this, 1032913302); }
  set SlopedBottom(value: boolean) { __CPP_ComponentProperty_set(this, 1032913302, value); }
  get GenerateCollision(): boolean { return __CPP_ComponentProperty_get(this, 3417576651); }
  set GenerateCollision(value: boolean) { __CPP_ComponentProperty_set(this, 3417576651, value); }
  get IncludeInNavmesh(): boolean { return __CPP_ComponentProperty_get(this, 850252985); }
  set IncludeInNavmesh(value: boolean) { __CPP_ComponentProperty_set(this, 850252985, value); }
  get UseAsOccluder(): boolean { return __CPP_ComponentProperty_get(this, 1048772127); }
  set UseAsOccluder(value: boolean) { __CPP_ComponentProperty_set(this, 1048772127, value); }
}

export class InputComponent extends Component
{
  public static GetTypeNameHash(): number { return 3356365986; }
  GetCurrentInputState(InputAction: string, OnlyKeyPressed: boolean): number { return __CPP_ComponentFunction_Call(this, 203962772, InputAction, OnlyKeyPressed); }
  get InputSet(): string { return __CPP_ComponentProperty_get(this, 3122856233); }
  set InputSet(value: string) { __CPP_ComponentProperty_set(this, 3122856233, value); }
  get Granularity(): Enum.InputMessageGranularity { return __CPP_ComponentProperty_get(this, 4138318112); }
  set Granularity(value: Enum.InputMessageGranularity) { __CPP_ComponentProperty_set(this, 4138318112, value); }
  get ForwardToBlackboard(): boolean { return __CPP_ComponentProperty_get(this, 37629567); }
  set ForwardToBlackboard(value: boolean) { __CPP_ComponentProperty_set(this, 37629567, value); }
}

export class InstancedMeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 3725395076; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 1897303241); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 1897303241, value); }
  get MainColor(): Color { return __CPP_ComponentProperty_get(this, 494953498); }
  set MainColor(value: Color) { __CPP_ComponentProperty_set(this, 494953498, value); }
}

export class JointAttachmentComponent extends Component
{
  public static GetTypeNameHash(): number { return 2994552942; }
  get JointName(): string { return __CPP_ComponentProperty_get(this, 3084471528); }
  set JointName(value: string) { __CPP_ComponentProperty_set(this, 3084471528, value); }
  get PositionOffset(): Vec3 { return __CPP_ComponentProperty_get(this, 513472167); }
  set PositionOffset(value: Vec3) { __CPP_ComponentProperty_set(this, 513472167, value); }
  get RotationOffset(): Quat { return __CPP_ComponentProperty_get(this, 1197051065); }
  set RotationOffset(value: Quat) { __CPP_ComponentProperty_set(this, 1197051065, value); }
}

export class JointOverrideComponent extends Component
{
  public static GetTypeNameHash(): number { return 1611350560; }
  get JointName(): string { return __CPP_ComponentProperty_get(this, 2457458788); }
  set JointName(value: string) { __CPP_ComponentProperty_set(this, 2457458788, value); }
  get OverridePosition(): boolean { return __CPP_ComponentProperty_get(this, 1635931413); }
  set OverridePosition(value: boolean) { __CPP_ComponentProperty_set(this, 1635931413, value); }
  get OverrideRotation(): boolean { return __CPP_ComponentProperty_get(this, 2171188702); }
  set OverrideRotation(value: boolean) { __CPP_ComponentProperty_set(this, 2171188702, value); }
  get OverrideScale(): boolean { return __CPP_ComponentProperty_get(this, 1486447761); }
  set OverrideScale(value: boolean) { __CPP_ComponentProperty_set(this, 1486447761, value); }
}

export class LensFlareComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 4014299360; }
  get LinkToLightShape(): boolean { return __CPP_ComponentProperty_get(this, 1082018071); }
  set LinkToLightShape(value: boolean) { __CPP_ComponentProperty_set(this, 1082018071, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 597467394); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 597467394, value); }
  get OcclusionSampleRadius(): number { return __CPP_ComponentProperty_get(this, 2885796196); }
  set OcclusionSampleRadius(value: number) { __CPP_ComponentProperty_set(this, 2885796196, value); }
  get OcclusionSampleSpread(): number { return __CPP_ComponentProperty_get(this, 111443103); }
  set OcclusionSampleSpread(value: number) { __CPP_ComponentProperty_set(this, 111443103, value); }
  get OcclusionDepthOffset(): number { return __CPP_ComponentProperty_get(this, 2753257405); }
  set OcclusionDepthOffset(value: number) { __CPP_ComponentProperty_set(this, 2753257405, value); }
  get ApplyFog(): boolean { return __CPP_ComponentProperty_get(this, 2998285808); }
  set ApplyFog(value: boolean) { __CPP_ComponentProperty_set(this, 2998285808, value); }
}

export class LocalBlackboardComponent extends BlackboardComponent
{
  public static GetTypeNameHash(): number { return 3594989640; }
  get BlackboardName(): string { return __CPP_ComponentProperty_get(this, 3727366016); }
  set BlackboardName(value: string) { __CPP_ComponentProperty_set(this, 3727366016, value); }
  get SendEntryChangedMessage(): boolean { return __CPP_ComponentProperty_get(this, 994580380); }
  set SendEntryChangedMessage(value: boolean) { __CPP_ComponentProperty_set(this, 994580380, value); }
}

export class LodAnimatedMeshComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2943567369; }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 3098523586); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 3098523586, value); }
  get SortingDepthOffset(): number { return __CPP_ComponentProperty_get(this, 2768559189); }
  set SortingDepthOffset(value: number) { __CPP_ComponentProperty_set(this, 2768559189, value); }
  get BoundsOffset(): Vec3 { return __CPP_ComponentProperty_get(this, 1186810419); }
  set BoundsOffset(value: Vec3) { __CPP_ComponentProperty_set(this, 1186810419, value); }
  get BoundsRadius(): number { return __CPP_ComponentProperty_get(this, 2633191913); }
  set BoundsRadius(value: number) { __CPP_ComponentProperty_set(this, 2633191913, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 3913565407); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 3913565407, value); }
  get OverlapRanges(): boolean { return __CPP_ComponentProperty_get(this, 3024161584); }
  set OverlapRanges(value: boolean) { __CPP_ComponentProperty_set(this, 3024161584, value); }
}

export class LodComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3310849595; }
  get BoundsOffset(): Vec3 { return __CPP_ComponentProperty_get(this, 383236148); }
  set BoundsOffset(value: Vec3) { __CPP_ComponentProperty_set(this, 383236148, value); }
  get BoundsRadius(): number { return __CPP_ComponentProperty_get(this, 2685930657); }
  set BoundsRadius(value: number) { __CPP_ComponentProperty_set(this, 2685930657, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 2212156245); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 2212156245, value); }
  get OverlapRanges(): boolean { return __CPP_ComponentProperty_get(this, 3582593088); }
  set OverlapRanges(value: boolean) { __CPP_ComponentProperty_set(this, 3582593088, value); }
}

export class LodMeshComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 981632222; }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 3125296401); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 3125296401, value); }
  get SortingDepthOffset(): number { return __CPP_ComponentProperty_get(this, 2646970251); }
  set SortingDepthOffset(value: number) { __CPP_ComponentProperty_set(this, 2646970251, value); }
  get BoundsOffset(): Vec3 { return __CPP_ComponentProperty_get(this, 3002206611); }
  set BoundsOffset(value: Vec3) { __CPP_ComponentProperty_set(this, 3002206611, value); }
  get BoundsRadius(): number { return __CPP_ComponentProperty_get(this, 403042845); }
  set BoundsRadius(value: number) { __CPP_ComponentProperty_set(this, 403042845, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 4085459334); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 4085459334, value); }
  get OverlapRanges(): boolean { return __CPP_ComponentProperty_get(this, 791400127); }
  set OverlapRanges(value: boolean) { __CPP_ComponentProperty_set(this, 791400127, value); }
}

export class MarkerComponent extends Component
{
  public static GetTypeNameHash(): number { return 1601973565; }
  get Marker(): string { return __CPP_ComponentProperty_get(this, 930410843); }
  set Marker(value: string) { __CPP_ComponentProperty_set(this, 930410843, value); }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 999589493); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 999589493, value); }
}

export class OccluderComponent extends Component
{
  public static GetTypeNameHash(): number { return 88153051; }
  get Type(): Enum.OccluderType { return __CPP_ComponentProperty_get(this, 2414574304); }
  set Type(value: Enum.OccluderType) { __CPP_ComponentProperty_set(this, 2414574304, value); }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 1943536156); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 1943536156, value); }
}

export class ParticleComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2686675370; }
  StartEffect(): boolean { return __CPP_ComponentFunction_Call(this, 2033083937); }
  StopEffect(): void { __CPP_ComponentFunction_Call(this, 3349038782); }
  InterruptEffect(): void { __CPP_ComponentFunction_Call(this, 3574566604); }
  IsEffectActive(): boolean { return __CPP_ComponentFunction_Call(this, 1214294675); }
  get Effect(): string { return __CPP_ComponentProperty_get(this, 3484195627); }
  set Effect(value: string) { __CPP_ComponentProperty_set(this, 3484195627, value); }
  get SpawnAtStart(): boolean { return __CPP_ComponentProperty_get(this, 1150464054); }
  set SpawnAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 1150464054, value); }
  get OnFinishedAction(): Enum.OnComponentFinishedAction2 { return __CPP_ComponentProperty_get(this, 1953865125); }
  set OnFinishedAction(value: Enum.OnComponentFinishedAction2) { __CPP_ComponentProperty_set(this, 1953865125, value); }
  get MinRestartDelay(): number { return __CPP_ComponentProperty_get(this, 4025812929); }
  set MinRestartDelay(value: number) { __CPP_ComponentProperty_set(this, 4025812929, value); }
  get RestartDelayRange(): number { return __CPP_ComponentProperty_get(this, 1168028766); }
  set RestartDelayRange(value: number) { __CPP_ComponentProperty_set(this, 1168028766, value); }
  get RandomSeed(): number { return __CPP_ComponentProperty_get(this, 3044836071); }
  set RandomSeed(value: number) { __CPP_ComponentProperty_set(this, 3044836071, value); }
  get SpawnDirection(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 3051994722); }
  set SpawnDirection(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 3051994722, value); }
  get IgnoreOwnerRotation(): boolean { return __CPP_ComponentProperty_get(this, 532070852); }
  set IgnoreOwnerRotation(value: boolean) { __CPP_ComponentProperty_set(this, 532070852, value); }
  get SharedInstanceName(): string { return __CPP_ComponentProperty_get(this, 2480396375); }
  set SharedInstanceName(value: string) { __CPP_ComponentProperty_set(this, 2480396375, value); }
}

export class ParticleFinisherComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1971050762; }
}

export class PathComponent extends Component
{
  public static GetTypeNameHash(): number { return 2501094966; }
  get Flags(): Flags.PathComponentFlags { return __CPP_ComponentProperty_get(this, 1539400503); }
  set Flags(value: Flags.PathComponentFlags) { __CPP_ComponentProperty_set(this, 1539400503, value); }
  get Closed(): boolean { return __CPP_ComponentProperty_get(this, 2456626931); }
  set Closed(value: boolean) { __CPP_ComponentProperty_set(this, 2456626931, value); }
  get Detail(): number { return __CPP_ComponentProperty_get(this, 2372432154); }
  set Detail(value: number) { __CPP_ComponentProperty_set(this, 2372432154, value); }
}

export class PathNodeComponent extends Component
{
  public static GetTypeNameHash(): number { return 3685933877; }
  get Roll(): number { return __CPP_ComponentProperty_get(this, 3005320873); }
  set Roll(value: number) { __CPP_ComponentProperty_set(this, 3005320873, value); }
  get Tangent1(): Enum.PathNodeTangentMode { return __CPP_ComponentProperty_get(this, 1721434648); }
  set Tangent1(value: Enum.PathNodeTangentMode) { __CPP_ComponentProperty_set(this, 1721434648, value); }
  get Tangent2(): Enum.PathNodeTangentMode { return __CPP_ComponentProperty_get(this, 2460014285); }
  set Tangent2(value: Enum.PathNodeTangentMode) { __CPP_ComponentProperty_set(this, 2460014285, value); }
}

export class PlayerStartPointComponent extends Component
{
  public static GetTypeNameHash(): number { return 420623154; }
  get PlayerPrefab(): string { return __CPP_ComponentProperty_get(this, 586140687); }
  set PlayerPrefab(value: string) { __CPP_ComponentProperty_set(this, 586140687, value); }
}

export class PointLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 2694457879; }
  get Range(): number { return __CPP_ComponentProperty_get(this, 2070313016); }
  set Range(value: number) { __CPP_ComponentProperty_set(this, 2070313016, value); }
  get ShadowFadeOutRange(): number { return __CPP_ComponentProperty_get(this, 929605317); }
  set ShadowFadeOutRange(value: number) { __CPP_ComponentProperty_set(this, 929605317, value); }
}

export class PostProcessingComponent extends Component
{
  public static GetTypeNameHash(): number { return 355889753; }
  get VolumeType(): string { return __CPP_ComponentProperty_get(this, 2875458730); }
  set VolumeType(value: string) { __CPP_ComponentProperty_set(this, 2875458730, value); }
}

export class PrefabReferenceComponent extends Component
{
  public static GetTypeNameHash(): number { return 2790782988; }
  get Prefab(): string { return __CPP_ComponentProperty_get(this, 3332257502); }
  set Prefab(value: string) { __CPP_ComponentProperty_set(this, 3332257502, value); }
}

export class PropertyAnimComponent extends Component
{
  public static GetTypeNameHash(): number { return 3464210963; }
  PlayAnimationRange(RangeLow: number, RangeHigh: number): void { __CPP_ComponentFunction_Call(this, 3789596221, RangeLow, RangeHigh); }
  get Animation(): string { return __CPP_ComponentProperty_get(this, 3369422858); }
  set Animation(value: string) { __CPP_ComponentProperty_set(this, 3369422858, value); }
  get Playing(): boolean { return __CPP_ComponentProperty_get(this, 166357422); }
  set Playing(value: boolean) { __CPP_ComponentProperty_set(this, 166357422, value); }
  get Mode(): Enum.PropertyAnimMode { return __CPP_ComponentProperty_get(this, 1609689093); }
  set Mode(value: Enum.PropertyAnimMode) { __CPP_ComponentProperty_set(this, 1609689093, value); }
  get RandomOffset(): number { return __CPP_ComponentProperty_get(this, 3367951183); }
  set RandomOffset(value: number) { __CPP_ComponentProperty_set(this, 3367951183, value); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 459274022); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 459274022, value); }
  get RangeLow(): number { return __CPP_ComponentProperty_get(this, 4204810908); }
  set RangeLow(value: number) { __CPP_ComponentProperty_set(this, 4204810908, value); }
  get RangeHigh(): number { return __CPP_ComponentProperty_get(this, 1788390653); }
  set RangeHigh(value: number) { __CPP_ComponentProperty_set(this, 1788390653, value); }
}

export class RenderTargetActivatorComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2562992528; }
  get RenderTarget(): string { return __CPP_ComponentProperty_get(this, 3221140373); }
  set RenderTarget(value: string) { __CPP_ComponentProperty_set(this, 3221140373, value); }
}

export class ResetTransformComponent extends Component
{
  public static GetTypeNameHash(): number { return 4104782307; }
  get ResetPositionX(): boolean { return __CPP_ComponentProperty_get(this, 669990680); }
  set ResetPositionX(value: boolean) { __CPP_ComponentProperty_set(this, 669990680, value); }
  get ResetPositionY(): boolean { return __CPP_ComponentProperty_get(this, 1517988336); }
  set ResetPositionY(value: boolean) { __CPP_ComponentProperty_set(this, 1517988336, value); }
  get ResetPositionZ(): boolean { return __CPP_ComponentProperty_get(this, 1479054757); }
  set ResetPositionZ(value: boolean) { __CPP_ComponentProperty_set(this, 1479054757, value); }
  get LocalPosition(): Vec3 { return __CPP_ComponentProperty_get(this, 948266900); }
  set LocalPosition(value: Vec3) { __CPP_ComponentProperty_set(this, 948266900, value); }
  get ResetRotation(): boolean { return __CPP_ComponentProperty_get(this, 4043987685); }
  set ResetRotation(value: boolean) { __CPP_ComponentProperty_set(this, 4043987685, value); }
  get LocalRotation(): Quat { return __CPP_ComponentProperty_get(this, 1983181777); }
  set LocalRotation(value: Quat) { __CPP_ComponentProperty_set(this, 1983181777, value); }
  get ResetScaling(): boolean { return __CPP_ComponentProperty_get(this, 4130213123); }
  set ResetScaling(value: boolean) { __CPP_ComponentProperty_set(this, 4130213123, value); }
  get LocalScaling(): Vec3 { return __CPP_ComponentProperty_get(this, 3727479380); }
  set LocalScaling(value: Vec3) { __CPP_ComponentProperty_set(this, 3727479380, value); }
  get LocalUniformScaling(): number { return __CPP_ComponentProperty_get(this, 1329766042); }
  set LocalUniformScaling(value: number) { __CPP_ComponentProperty_set(this, 1329766042, value); }
}

export class RopeRenderComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2531851482; }
  get Material(): string { return __CPP_ComponentProperty_get(this, 1666641405); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 1666641405, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2685822600); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2685822600, value); }
  get Thickness(): number { return __CPP_ComponentProperty_get(this, 1484383793); }
  set Thickness(value: number) { __CPP_ComponentProperty_set(this, 1484383793, value); }
  get Detail(): number { return __CPP_ComponentProperty_get(this, 1046542646); }
  set Detail(value: number) { __CPP_ComponentProperty_set(this, 1046542646, value); }
  get Subdivide(): boolean { return __CPP_ComponentProperty_get(this, 3992940747); }
  set Subdivide(value: boolean) { __CPP_ComponentProperty_set(this, 3992940747, value); }
  get UScale(): number { return __CPP_ComponentProperty_get(this, 4199831591); }
  set UScale(value: number) { __CPP_ComponentProperty_set(this, 4199831591, value); }
}

export class TransformComponent extends Component
{
  public static GetTypeNameHash(): number { return 1680961727; }
  SetDirectionForwards(Forwards: boolean): void { __CPP_ComponentFunction_Call(this, 1772757026, Forwards); }
  IsDirectionForwards(): boolean { return __CPP_ComponentFunction_Call(this, 535634261); }
  ToggleDirection(): void { __CPP_ComponentFunction_Call(this, 1750973492); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 1320097369); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 1320097369, value); }
  get Running(): boolean { return __CPP_ComponentProperty_get(this, 2998653796); }
  set Running(value: boolean) { __CPP_ComponentProperty_set(this, 2998653796, value); }
  get ReverseAtEnd(): boolean { return __CPP_ComponentProperty_get(this, 1093294572); }
  set ReverseAtEnd(value: boolean) { __CPP_ComponentProperty_set(this, 1093294572, value); }
  get ReverseAtStart(): boolean { return __CPP_ComponentProperty_get(this, 1892362220); }
  set ReverseAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 1892362220, value); }
}

export class RotorComponent extends TransformComponent
{
  public static GetTypeNameHash(): number { return 3070814480; }
  get Axis(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 2360317893); }
  set Axis(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 2360317893, value); }
  get AxisDeviation(): number { return __CPP_ComponentProperty_get(this, 1861770418); }
  set AxisDeviation(value: number) { __CPP_ComponentProperty_set(this, 1861770418, value); }
  get DegreesToRotate(): number { return __CPP_ComponentProperty_get(this, 2477849409); }
  set DegreesToRotate(value: number) { __CPP_ComponentProperty_set(this, 2477849409, value); }
  get Acceleration(): number { return __CPP_ComponentProperty_get(this, 982214252); }
  set Acceleration(value: number) { __CPP_ComponentProperty_set(this, 982214252, value); }
  get Deceleration(): number { return __CPP_ComponentProperty_get(this, 3503289027); }
  set Deceleration(value: number) { __CPP_ComponentProperty_set(this, 3503289027, value); }
}

export class SceneTransitionComponent extends Component
{
  public static GetTypeNameHash(): number { return 811802936; }
  StartTransition(PositionOffset: Vec3, RotationOffset: Quat): void { __CPP_ComponentFunction_Call(this, 779411698, PositionOffset, RotationOffset); }
  StartTransitionWithOffsetTo(GlobalPosition: Vec3, GlobalRotation: Quat): void { __CPP_ComponentFunction_Call(this, 735934107, GlobalPosition, GlobalRotation); }
  StartPreload(): void { __CPP_ComponentFunction_Call(this, 280523740); }
  CancelPreload(): void { __CPP_ComponentFunction_Call(this, 1205761980); }
  get Mode(): Enum.SceneLoadMode { return __CPP_ComponentProperty_get(this, 2847435955); }
  set Mode(value: Enum.SceneLoadMode) { __CPP_ComponentProperty_set(this, 2847435955, value); }
  get TargetScene(): string { return __CPP_ComponentProperty_get(this, 4108749278); }
  set TargetScene(value: string) { __CPP_ComponentProperty_set(this, 4108749278, value); }
  get PreloadCollection(): string { return __CPP_ComponentProperty_get(this, 2336716771); }
  set PreloadCollection(value: string) { __CPP_ComponentProperty_set(this, 2336716771, value); }
  get SpawnPoint(): string { return __CPP_ComponentProperty_get(this, 1303462238); }
  set SpawnPoint(value: string) { __CPP_ComponentProperty_set(this, 1303462238, value); }
  get RelativeSpawnPosition(): boolean { return __CPP_ComponentProperty_get(this, 1843002242); }
  set RelativeSpawnPosition(value: boolean) { __CPP_ComponentProperty_set(this, 1843002242, value); }
}

export class ScriptComponent extends EventMessageHandlerComponent
{
  public static GetTypeNameHash(): number { return 1614462437; }
  SetScriptVariable(Name: string, Value: any): void { __CPP_ComponentFunction_Call(this, 1554318557, Name, Value); }
  GetScriptVariable(Name: string): any { return __CPP_ComponentFunction_Call(this, 4178349120, Name); }
  SetUpdateInterval(interval: number): void { __CPP_ComponentFunction_Call(this, 2103510945, interval); }
  get UpdateInterval(): number { return __CPP_ComponentProperty_get(this, 3597189607); }
  set UpdateInterval(value: number) { __CPP_ComponentProperty_set(this, 3597189607, value); }
  get ScriptClass(): string { return __CPP_ComponentProperty_get(this, 1184048898); }
  set ScriptClass(value: string) { __CPP_ComponentProperty_set(this, 1184048898, value); }
}

export class SensorComponent extends Component
{
  public static GetTypeNameHash(): number { return 1950355900; }
  get UpdateRate(): Enum.UpdateRate { return __CPP_ComponentProperty_get(this, 408363047); }
  set UpdateRate(value: Enum.UpdateRate) { __CPP_ComponentProperty_set(this, 408363047, value); }
  get SpatialCategory(): string { return __CPP_ComponentProperty_get(this, 391596057); }
  set SpatialCategory(value: string) { __CPP_ComponentProperty_set(this, 391596057, value); }
  get TestVisibility(): boolean { return __CPP_ComponentProperty_get(this, 1117377997); }
  set TestVisibility(value: boolean) { __CPP_ComponentProperty_set(this, 1117377997, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 168630636); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 168630636, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 2468499328); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 2468499328, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 480095682); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 480095682, value); }
}

export class SensorConeComponent extends SensorComponent
{
  public static GetTypeNameHash(): number { return 3668756514; }
  get NearDistance(): number { return __CPP_ComponentProperty_get(this, 4156364868); }
  set NearDistance(value: number) { __CPP_ComponentProperty_set(this, 4156364868, value); }
  get FarDistance(): number { return __CPP_ComponentProperty_get(this, 421069967); }
  set FarDistance(value: number) { __CPP_ComponentProperty_set(this, 421069967, value); }
  get Angle(): number { return __CPP_ComponentProperty_get(this, 2857622228); }
  set Angle(value: number) { __CPP_ComponentProperty_set(this, 2857622228, value); }
}

export class SensorCylinderComponent extends SensorComponent
{
  public static GetTypeNameHash(): number { return 89262471; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 3201847377); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 3201847377, value); }
  get Height(): number { return __CPP_ComponentProperty_get(this, 376275050); }
  set Height(value: number) { __CPP_ComponentProperty_set(this, 376275050, value); }
}

export class SensorSphereComponent extends SensorComponent
{
  public static GetTypeNameHash(): number { return 3985283984; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 3153608822); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 3153608822, value); }
}

export class ShapeIconComponent extends Component
{
  public static GetTypeNameHash(): number { return 2751457557; }
}

export class SimpleAnimationComponent extends Component
{
  public static GetTypeNameHash(): number { return 3773119732; }
  get AnimationClip(): string { return __CPP_ComponentProperty_get(this, 4114851481); }
  set AnimationClip(value: string) { __CPP_ComponentProperty_set(this, 4114851481, value); }
  get AnimationMode(): Enum.PropertyAnimMode { return __CPP_ComponentProperty_get(this, 2854264169); }
  set AnimationMode(value: Enum.PropertyAnimMode) { __CPP_ComponentProperty_set(this, 2854264169, value); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 2330469972); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 2330469972, value); }
  get RootMotionMode(): Enum.RootMotionMode { return __CPP_ComponentProperty_get(this, 3939232860); }
  set RootMotionMode(value: Enum.RootMotionMode) { __CPP_ComponentProperty_set(this, 3939232860, value); }
  get InvisibleUpdateRate(): Enum.AnimationInvisibleUpdateRate { return __CPP_ComponentProperty_get(this, 515439340); }
  set InvisibleUpdateRate(value: Enum.AnimationInvisibleUpdateRate) { __CPP_ComponentProperty_set(this, 515439340, value); }
  get EnableIK(): boolean { return __CPP_ComponentProperty_get(this, 1673832118); }
  set EnableIK(value: boolean) { __CPP_ComponentProperty_set(this, 1673832118, value); }
}

export class SimpleWindComponent extends Component
{
  public static GetTypeNameHash(): number { return 626606470; }
  get MinWindStrength(): Enum.WindStrength { return __CPP_ComponentProperty_get(this, 619940196); }
  set MinWindStrength(value: Enum.WindStrength) { __CPP_ComponentProperty_set(this, 619940196, value); }
  get MaxWindStrength(): Enum.WindStrength { return __CPP_ComponentProperty_get(this, 1272420470); }
  set MaxWindStrength(value: Enum.WindStrength) { __CPP_ComponentProperty_set(this, 1272420470, value); }
  get MaxDeviation(): number { return __CPP_ComponentProperty_get(this, 597539859); }
  set MaxDeviation(value: number) { __CPP_ComponentProperty_set(this, 597539859, value); }
}

export class SkeletonComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1641866982; }
  get Skeleton(): string { return __CPP_ComponentProperty_get(this, 1515066319); }
  set Skeleton(value: string) { __CPP_ComponentProperty_set(this, 1515066319, value); }
  get VisualizeSkeleton(): boolean { return __CPP_ComponentProperty_get(this, 4113161467); }
  set VisualizeSkeleton(value: boolean) { __CPP_ComponentProperty_set(this, 4113161467, value); }
  get VisualizeColliders(): boolean { return __CPP_ComponentProperty_get(this, 3787596667); }
  set VisualizeColliders(value: boolean) { __CPP_ComponentProperty_set(this, 3787596667, value); }
  get VisualizeJoints(): boolean { return __CPP_ComponentProperty_get(this, 3456629988); }
  set VisualizeJoints(value: boolean) { __CPP_ComponentProperty_set(this, 3456629988, value); }
  get VisualizeSwingLimits(): boolean { return __CPP_ComponentProperty_get(this, 3194867746); }
  set VisualizeSwingLimits(value: boolean) { __CPP_ComponentProperty_set(this, 3194867746, value); }
  get VisualizeTwistLimits(): boolean { return __CPP_ComponentProperty_get(this, 1877742448); }
  set VisualizeTwistLimits(value: boolean) { __CPP_ComponentProperty_set(this, 1877742448, value); }
  get BonesToHighlight(): string { return __CPP_ComponentProperty_get(this, 2710454113); }
  set BonesToHighlight(value: string) { __CPP_ComponentProperty_set(this, 2710454113, value); }
}

export class SkeletonPoseComponent extends Component
{
  public static GetTypeNameHash(): number { return 4060488408; }
  get Skeleton(): string { return __CPP_ComponentProperty_get(this, 2576807777); }
  set Skeleton(value: string) { __CPP_ComponentProperty_set(this, 2576807777, value); }
  get Mode(): Enum.SkeletonPoseMode { return __CPP_ComponentProperty_get(this, 501630531); }
  set Mode(value: Enum.SkeletonPoseMode) { __CPP_ComponentProperty_set(this, 501630531, value); }
  get EditBones(): number { return __CPP_ComponentProperty_get(this, 4116987872); }
  set EditBones(value: number) { __CPP_ComponentProperty_set(this, 4116987872, value); }
}

export class SkyBoxComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1806701530; }
  get CubeMap(): string { return __CPP_ComponentProperty_get(this, 791111828); }
  set CubeMap(value: string) { __CPP_ComponentProperty_set(this, 791111828, value); }
  get ExposureBias(): number { return __CPP_ComponentProperty_get(this, 4053428649); }
  set ExposureBias(value: number) { __CPP_ComponentProperty_set(this, 4053428649, value); }
  get InverseTonemap(): boolean { return __CPP_ComponentProperty_get(this, 562715483); }
  set InverseTonemap(value: boolean) { __CPP_ComponentProperty_set(this, 562715483, value); }
  get UseFog(): boolean { return __CPP_ComponentProperty_get(this, 2183232300); }
  set UseFog(value: boolean) { __CPP_ComponentProperty_set(this, 2183232300, value); }
  get VirtualDistance(): number { return __CPP_ComponentProperty_get(this, 1475247756); }
  set VirtualDistance(value: number) { __CPP_ComponentProperty_set(this, 1475247756, value); }
}

export class SkyLightComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 2390201003; }
  get ReflectionProbeMode(): Enum.ReflectionProbeMode { return __CPP_ComponentProperty_get(this, 852024994); }
  set ReflectionProbeMode(value: Enum.ReflectionProbeMode) { __CPP_ComponentProperty_set(this, 852024994, value); }
  get CubeMap(): string { return __CPP_ComponentProperty_get(this, 575541439); }
  set CubeMap(value: string) { __CPP_ComponentProperty_set(this, 575541439, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 821218387); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 821218387, value); }
  get Saturation(): number { return __CPP_ComponentProperty_get(this, 4010581560); }
  set Saturation(value: number) { __CPP_ComponentProperty_set(this, 4010581560, value); }
  get NearPlane(): number { return __CPP_ComponentProperty_get(this, 1331659047); }
  set NearPlane(value: number) { __CPP_ComponentProperty_set(this, 1331659047, value); }
  get FarPlane(): number { return __CPP_ComponentProperty_get(this, 268875995); }
  set FarPlane(value: number) { __CPP_ComponentProperty_set(this, 268875995, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 3344849363); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 3344849363, value); }
  get ShowMipMaps(): boolean { return __CPP_ComponentProperty_get(this, 895237282); }
  set ShowMipMaps(value: boolean) { __CPP_ComponentProperty_set(this, 895237282, value); }
}

export class SliderComponent extends TransformComponent
{
  public static GetTypeNameHash(): number { return 1295303479; }
  get Axis(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 2215777889); }
  set Axis(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 2215777889, value); }
  get Distance(): number { return __CPP_ComponentProperty_get(this, 2929805649); }
  set Distance(value: number) { __CPP_ComponentProperty_set(this, 2929805649, value); }
  get Acceleration(): number { return __CPP_ComponentProperty_get(this, 1187090096); }
  set Acceleration(value: number) { __CPP_ComponentProperty_set(this, 1187090096, value); }
  get Deceleration(): number { return __CPP_ComponentProperty_get(this, 2302800190); }
  set Deceleration(value: number) { __CPP_ComponentProperty_set(this, 2302800190, value); }
  get RandomStart(): number { return __CPP_ComponentProperty_get(this, 1439600697); }
  set RandomStart(value: number) { __CPP_ComponentProperty_set(this, 1439600697, value); }
}

export class SpatialAnchorComponent extends Component
{
  public static GetTypeNameHash(): number { return 2762556177; }
}

export class SpawnBoxComponent extends Component
{
  public static GetTypeNameHash(): number { return 381409133; }
  StartSpawning(): void { __CPP_ComponentFunction_Call(this, 3460926770); }
  get HalfExtents(): Vec3 { return __CPP_ComponentProperty_get(this, 3309010296); }
  set HalfExtents(value: Vec3) { __CPP_ComponentProperty_set(this, 3309010296, value); }
  get Prefab(): string { return __CPP_ComponentProperty_get(this, 1094356410); }
  set Prefab(value: string) { __CPP_ComponentProperty_set(this, 1094356410, value); }
  get SpawnAtStart(): boolean { return __CPP_ComponentProperty_get(this, 1227995398); }
  set SpawnAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 1227995398, value); }
  get SpawnContinuously(): boolean { return __CPP_ComponentProperty_get(this, 337530404); }
  set SpawnContinuously(value: boolean) { __CPP_ComponentProperty_set(this, 337530404, value); }
  get MinSpawnCount(): number { return __CPP_ComponentProperty_get(this, 1027167878); }
  set MinSpawnCount(value: number) { __CPP_ComponentProperty_set(this, 1027167878, value); }
  get SpawnCountRange(): number { return __CPP_ComponentProperty_get(this, 3735215613); }
  set SpawnCountRange(value: number) { __CPP_ComponentProperty_set(this, 3735215613, value); }
  get Duration(): number { return __CPP_ComponentProperty_get(this, 910458143); }
  set Duration(value: number) { __CPP_ComponentProperty_set(this, 910458143, value); }
  get MaxRotationZ(): number { return __CPP_ComponentProperty_get(this, 4244883874); }
  set MaxRotationZ(value: number) { __CPP_ComponentProperty_set(this, 4244883874, value); }
  get MaxTiltZ(): number { return __CPP_ComponentProperty_get(this, 1711808229); }
  set MaxTiltZ(value: number) { __CPP_ComponentProperty_set(this, 1711808229, value); }
}

export class SpawnComponent extends Component
{
  public static GetTypeNameHash(): number { return 438730474; }
  CanTriggerManualSpawn(): boolean { return __CPP_ComponentFunction_Call(this, 1182018235); }
  TriggerManualSpawn(IgnoreSpawnDelay: boolean, LocalOffset: Vec3): boolean { return __CPP_ComponentFunction_Call(this, 4253763478, IgnoreSpawnDelay, LocalOffset); }
  ScheduleSpawn(): void { __CPP_ComponentFunction_Call(this, 645050758); }
  get Prefab(): string { return __CPP_ComponentProperty_get(this, 1025383935); }
  set Prefab(value: string) { __CPP_ComponentProperty_set(this, 1025383935, value); }
  get AttachAsChild(): boolean { return __CPP_ComponentProperty_get(this, 2678119025); }
  set AttachAsChild(value: boolean) { __CPP_ComponentProperty_set(this, 2678119025, value); }
  get SpawnAtStart(): boolean { return __CPP_ComponentProperty_get(this, 4055899534); }
  set SpawnAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 4055899534, value); }
  get SpawnContinuously(): boolean { return __CPP_ComponentProperty_get(this, 2426811985); }
  set SpawnContinuously(value: boolean) { __CPP_ComponentProperty_set(this, 2426811985, value); }
  get MinDelay(): number { return __CPP_ComponentProperty_get(this, 542565958); }
  set MinDelay(value: number) { __CPP_ComponentProperty_set(this, 542565958, value); }
  get DelayRange(): number { return __CPP_ComponentProperty_get(this, 490325517); }
  set DelayRange(value: number) { __CPP_ComponentProperty_set(this, 490325517, value); }
  get Deviation(): number { return __CPP_ComponentProperty_get(this, 824059517); }
  set Deviation(value: number) { __CPP_ComponentProperty_set(this, 824059517, value); }
}

export class SphereReflectionProbeComponent extends ReflectionProbeComponentBase
{
  public static GetTypeNameHash(): number { return 681694830; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 7676970); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 7676970, value); }
  get Falloff(): number { return __CPP_ComponentProperty_get(this, 562629380); }
  set Falloff(value: number) { __CPP_ComponentProperty_set(this, 562629380, value); }
  get SphereProjection(): boolean { return __CPP_ComponentProperty_get(this, 3697861663); }
  set SphereProjection(value: boolean) { __CPP_ComponentProperty_set(this, 3697861663, value); }
}

export class SpotLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 2983247510; }
  get Range(): number { return __CPP_ComponentProperty_get(this, 886765956); }
  set Range(value: number) { __CPP_ComponentProperty_set(this, 886765956, value); }
  get InnerSpotAngle(): number { return __CPP_ComponentProperty_get(this, 2388270276); }
  set InnerSpotAngle(value: number) { __CPP_ComponentProperty_set(this, 2388270276, value); }
  get OuterSpotAngle(): number { return __CPP_ComponentProperty_get(this, 1291241033); }
  set OuterSpotAngle(value: number) { __CPP_ComponentProperty_set(this, 1291241033, value); }
  get ShadowFadeOutRange(): number { return __CPP_ComponentProperty_get(this, 701712349); }
  set ShadowFadeOutRange(value: number) { __CPP_ComponentProperty_set(this, 701712349, value); }
}

export class SpriteComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 4107853740; }
  get Texture(): string { return __CPP_ComponentProperty_get(this, 1708425619); }
  set Texture(value: string) { __CPP_ComponentProperty_set(this, 1708425619, value); }
  get BlendMode(): Enum.SpriteBlendMode { return __CPP_ComponentProperty_get(this, 1484307633); }
  set BlendMode(value: Enum.SpriteBlendMode) { __CPP_ComponentProperty_set(this, 1484307633, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2584012464); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2584012464, value); }
  get Size(): number { return __CPP_ComponentProperty_get(this, 880140730); }
  set Size(value: number) { __CPP_ComponentProperty_set(this, 880140730, value); }
  get MaxScreenSize(): number { return __CPP_ComponentProperty_get(this, 3365523384); }
  set MaxScreenSize(value: number) { __CPP_ComponentProperty_set(this, 3365523384, value); }
  get AspectRatio(): number { return __CPP_ComponentProperty_get(this, 1319232297); }
  set AspectRatio(value: number) { __CPP_ComponentProperty_set(this, 1319232297, value); }
}

export class StageSpaceComponent extends Component
{
  public static GetTypeNameHash(): number { return 3814358307; }
  get StageSpace(): Enum.XRStageSpace { return __CPP_ComponentProperty_get(this, 3796859176); }
  set StageSpace(value: Enum.XRStageSpace) { __CPP_ComponentProperty_set(this, 3796859176, value); }
}

export class StateMachineComponent extends Component
{
  public static GetTypeNameHash(): number { return 71440247; }
  SetState(Name: string): boolean { return __CPP_ComponentFunction_Call(this, 3119358439, Name); }
  GetCurrentState(): string { return __CPP_ComponentFunction_Call(this, 3309425088); }
  FireTransitionEvent(Name: string): void { __CPP_ComponentFunction_Call(this, 258813224, Name); }
  get Resource(): string { return __CPP_ComponentProperty_get(this, 3553976178); }
  set Resource(value: string) { __CPP_ComponentProperty_set(this, 3553976178, value); }
  get InitialState(): string { return __CPP_ComponentProperty_get(this, 3055923916); }
  set InitialState(value: string) { __CPP_ComponentProperty_set(this, 3055923916, value); }
  get BlackboardName(): string { return __CPP_ComponentProperty_get(this, 3412889803); }
  set BlackboardName(value: string) { __CPP_ComponentProperty_set(this, 3412889803, value); }
}

export class TimedDeathComponent extends Component
{
  public static GetTypeNameHash(): number { return 1589515529; }
  get MinDelay(): number { return __CPP_ComponentProperty_get(this, 2549195916); }
  set MinDelay(value: number) { __CPP_ComponentProperty_set(this, 2549195916, value); }
  get DelayRange(): number { return __CPP_ComponentProperty_get(this, 1350176412); }
  set DelayRange(value: number) { __CPP_ComponentProperty_set(this, 1350176412, value); }
  get TimeoutPrefab(): string { return __CPP_ComponentProperty_get(this, 978098831); }
  set TimeoutPrefab(value: string) { __CPP_ComponentProperty_set(this, 978098831, value); }
}

export class TriggerDelayModifierComponent extends Component
{
  public static GetTypeNameHash(): number { return 928702445; }
  get ActivationDelay(): number { return __CPP_ComponentProperty_get(this, 3433323958); }
  set ActivationDelay(value: number) { __CPP_ComponentProperty_set(this, 3433323958, value); }
  get DeactivationDelay(): number { return __CPP_ComponentProperty_get(this, 789862730); }
  set DeactivationDelay(value: number) { __CPP_ComponentProperty_set(this, 789862730, value); }
}

export class TwoBoneIKComponent extends Component
{
  public static GetTypeNameHash(): number { return 2107968216; }
  get JointStart(): string { return __CPP_ComponentProperty_get(this, 563228101); }
  set JointStart(value: string) { __CPP_ComponentProperty_set(this, 563228101, value); }
  get JointMiddle(): string { return __CPP_ComponentProperty_get(this, 225641780); }
  set JointMiddle(value: string) { __CPP_ComponentProperty_set(this, 225641780, value); }
  get JointEnd(): string { return __CPP_ComponentProperty_get(this, 3672770765); }
  set JointEnd(value: string) { __CPP_ComponentProperty_set(this, 3672770765, value); }
  get MidAxis(): Enum.BasisAxis { return __CPP_ComponentProperty_get(this, 1163515027); }
  set MidAxis(value: Enum.BasisAxis) { __CPP_ComponentProperty_set(this, 1163515027, value); }
  get PoleVector(): string { return __CPP_ComponentProperty_get(this, 3239437795); }
  set PoleVector(value: string) { __CPP_ComponentProperty_set(this, 3239437795, value); }
  get Weight(): number { return __CPP_ComponentProperty_get(this, 613184787); }
  set Weight(value: number) { __CPP_ComponentProperty_set(this, 613184787, value); }
}

export class VisualizeHandComponent extends Component
{
  public static GetTypeNameHash(): number { return 3717625868; }
}

export class VolumeComponent extends Component
{
  public static GetTypeNameHash(): number { return 4218212731; }
  SetValue(Name: string, Value: any): void { __CPP_ComponentFunction_Call(this, 548868820, Name, Value); }
  get Type(): string { return __CPP_ComponentProperty_get(this, 514052835); }
  set Type(value: string) { __CPP_ComponentProperty_set(this, 514052835, value); }
  get SortOrder(): number { return __CPP_ComponentProperty_get(this, 1049093630); }
  set SortOrder(value: number) { __CPP_ComponentProperty_set(this, 1049093630, value); }
  get Template(): string { return __CPP_ComponentProperty_get(this, 155034580); }
  set Template(value: string) { __CPP_ComponentProperty_set(this, 155034580, value); }
}

export class VolumeBoxComponent extends VolumeComponent
{
  public static GetTypeNameHash(): number { return 4290233462; }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 3685812592); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 3685812592, value); }
  get Falloff(): Vec3 { return __CPP_ComponentProperty_get(this, 3020062844); }
  set Falloff(value: Vec3) { __CPP_ComponentProperty_set(this, 3020062844, value); }
}

export class VolumeSphereComponent extends VolumeComponent
{
  public static GetTypeNameHash(): number { return 315895710; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 2966498114); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 2966498114, value); }
  get Falloff(): number { return __CPP_ComponentProperty_get(this, 2058106743); }
  set Falloff(value: number) { __CPP_ComponentProperty_set(this, 2058106743, value); }
}

export class WindVolumeComponent extends Component
{
  public static GetTypeNameHash(): number { return 969175823; }
  get Strength(): Enum.WindStrength { return __CPP_ComponentProperty_get(this, 1408869612); }
  set Strength(value: Enum.WindStrength) { __CPP_ComponentProperty_set(this, 1408869612, value); }
  get StrengthFactor(): number { return __CPP_ComponentProperty_get(this, 338039914); }
  set StrengthFactor(value: number) { __CPP_ComponentProperty_set(this, 338039914, value); }
  get BurstDuration(): number { return __CPP_ComponentProperty_get(this, 635172675); }
  set BurstDuration(value: number) { __CPP_ComponentProperty_set(this, 635172675, value); }
  get OnFinishedAction(): Enum.OnComponentFinishedAction { return __CPP_ComponentProperty_get(this, 1277794349); }
  set OnFinishedAction(value: Enum.OnComponentFinishedAction) { __CPP_ComponentProperty_set(this, 1277794349, value); }
}

export class WindVolumeConeComponent extends WindVolumeComponent
{
  public static GetTypeNameHash(): number { return 114696516; }
  get Angle(): number { return __CPP_ComponentProperty_get(this, 329015589); }
  set Angle(value: number) { __CPP_ComponentProperty_set(this, 329015589, value); }
  get Length(): number { return __CPP_ComponentProperty_get(this, 191039892); }
  set Length(value: number) { __CPP_ComponentProperty_set(this, 191039892, value); }
}

export class WindVolumeCylinderComponent extends WindVolumeComponent
{
  public static GetTypeNameHash(): number { return 1321819725; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 258961560); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 258961560, value); }
  get RadiusFalloff(): number { return __CPP_ComponentProperty_get(this, 3847740912); }
  set RadiusFalloff(value: number) { __CPP_ComponentProperty_set(this, 3847740912, value); }
  get Length(): number { return __CPP_ComponentProperty_get(this, 4114686021); }
  set Length(value: number) { __CPP_ComponentProperty_set(this, 4114686021, value); }
  get PositiveFalloff(): number { return __CPP_ComponentProperty_get(this, 91847062); }
  set PositiveFalloff(value: number) { __CPP_ComponentProperty_set(this, 91847062, value); }
  get NegativeFalloff(): number { return __CPP_ComponentProperty_get(this, 3560202190); }
  set NegativeFalloff(value: number) { __CPP_ComponentProperty_set(this, 3560202190, value); }
  get Mode(): Enum.WindVolumeCylinderMode { return __CPP_ComponentProperty_get(this, 3800825428); }
  set Mode(value: Enum.WindVolumeCylinderMode) { __CPP_ComponentProperty_set(this, 3800825428, value); }
}

export class WindVolumeSphereComponent extends WindVolumeComponent
{
  public static GetTypeNameHash(): number { return 4046423258; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 1394920935); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 1394920935, value); }
}

