
import __GameObject = require("./GameObject")
export import GameObject = __GameObject.GameObject;

import __Component = require("./Component")
export import Component = __Component.Component;

import __Color = require("./Color")
export import Color = __Color.Color;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

export class CollectionComponent extends Component
{
  public static GetTypeNameHash(): number { return 1132955664; }
  get Collection(): string { return __CPP_ComponentProperty_get(this, 1945626123); }
  set Collection(value: string) { __CPP_ComponentProperty_set(this, 1945626123, value); }
}

export class EventMessageHandlerComponent extends Component
{
  public static GetTypeNameHash(): number { return 155900694; }
}

export class SettingsComponent extends Component
{
  public static GetTypeNameHash(): number { return 1869761800; }
}

export class RenderComponent extends Component
{
  public static GetTypeNameHash(): number { return 1723868812; }
}

export class VisualizeSkeletonComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2088113316; }
  get Skeleton(): string { return __CPP_ComponentProperty_get(this, 3075801802); }
  set Skeleton(value: string) { __CPP_ComponentProperty_set(this, 3075801802, value); }
}

export class AlwaysVisibleComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2730761550; }
}

export class CameraComponent extends Component
{
  public static GetTypeNameHash(): number { return 1499446064; }
  get EditorShortcut(): number { return __CPP_ComponentProperty_get(this, 3139070241); }
  set EditorShortcut(value: number) { __CPP_ComponentProperty_set(this, 3139070241, value); }
  get RenderTarget(): string { return __CPP_ComponentProperty_get(this, 270813693); }
  set RenderTarget(value: string) { __CPP_ComponentProperty_set(this, 270813693, value); }
  get NearPlane(): number { return __CPP_ComponentProperty_get(this, 2247242689); }
  set NearPlane(value: number) { __CPP_ComponentProperty_set(this, 2247242689, value); }
  get FarPlane(): number { return __CPP_ComponentProperty_get(this, 687229388); }
  set FarPlane(value: number) { __CPP_ComponentProperty_set(this, 687229388, value); }
  get FOV(): number { return __CPP_ComponentProperty_get(this, 3400383453); }
  set FOV(value: number) { __CPP_ComponentProperty_set(this, 3400383453, value); }
  get Dimensions(): number { return __CPP_ComponentProperty_get(this, 4116027742); }
  set Dimensions(value: number) { __CPP_ComponentProperty_set(this, 4116027742, value); }
  get CameraRenderPipeline(): string { return __CPP_ComponentProperty_get(this, 3172946370); }
  set CameraRenderPipeline(value: string) { __CPP_ComponentProperty_set(this, 3172946370, value); }
  get Aperture(): number { return __CPP_ComponentProperty_get(this, 1274063898); }
  set Aperture(value: number) { __CPP_ComponentProperty_set(this, 1274063898, value); }
  get ShutterTime(): number { return __CPP_ComponentProperty_get(this, 2737420531); }
  set ShutterTime(value: number) { __CPP_ComponentProperty_set(this, 2737420531, value); }
  get ISO(): number { return __CPP_ComponentProperty_get(this, 1334830559); }
  set ISO(value: number) { __CPP_ComponentProperty_set(this, 1334830559, value); }
  get ExposureCompensation(): number { return __CPP_ComponentProperty_get(this, 1334442640); }
  set ExposureCompensation(value: number) { __CPP_ComponentProperty_set(this, 1334442640, value); }
  get ShowStats(): boolean { return __CPP_ComponentProperty_get(this, 2413753208); }
  set ShowStats(value: boolean) { __CPP_ComponentProperty_set(this, 2413753208, value); }
}

export class FogComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 3388463903; }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 888826411); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 888826411, value); }
  get Density(): number { return __CPP_ComponentProperty_get(this, 3731994520); }
  set Density(value: number) { __CPP_ComponentProperty_set(this, 3731994520, value); }
  get HeightFalloff(): number { return __CPP_ComponentProperty_get(this, 2592084796); }
  set HeightFalloff(value: number) { __CPP_ComponentProperty_set(this, 2592084796, value); }
}

export class RenderTargetActivatorComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1634187716; }
  get RenderTarget(): string { return __CPP_ComponentProperty_get(this, 2586899687); }
  set RenderTarget(value: string) { __CPP_ComponentProperty_set(this, 2586899687, value); }
}

export class SkyBoxComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1466514488; }
  get ExposureBias(): number { return __CPP_ComponentProperty_get(this, 3782775256); }
  set ExposureBias(value: number) { __CPP_ComponentProperty_set(this, 3782775256, value); }
  get InverseTonemap(): boolean { return __CPP_ComponentProperty_get(this, 3799247071); }
  set InverseTonemap(value: boolean) { __CPP_ComponentProperty_set(this, 3799247071, value); }
  get CubeMap(): string { return __CPP_ComponentProperty_get(this, 711715648); }
  set CubeMap(value: string) { __CPP_ComponentProperty_set(this, 711715648, value); }
  get UseFog(): boolean { return __CPP_ComponentProperty_get(this, 1822075681); }
  set UseFog(value: boolean) { __CPP_ComponentProperty_set(this, 1822075681, value); }
  get VirtualDistance(): number { return __CPP_ComponentProperty_get(this, 4238191478); }
  set VirtualDistance(value: number) { __CPP_ComponentProperty_set(this, 4238191478, value); }
}

export class SpriteComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 732121179; }
  get Texture(): string { return __CPP_ComponentProperty_get(this, 1138349771); }
  set Texture(value: string) { __CPP_ComponentProperty_set(this, 1138349771, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 356776964); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 356776964, value); }
  get Size(): number { return __CPP_ComponentProperty_get(this, 471628027); }
  set Size(value: number) { __CPP_ComponentProperty_set(this, 471628027, value); }
  get MaxScreenSize(): number { return __CPP_ComponentProperty_get(this, 3163264369); }
  set MaxScreenSize(value: number) { __CPP_ComponentProperty_set(this, 3163264369, value); }
  get AspectRatio(): number { return __CPP_ComponentProperty_get(this, 2944670669); }
  set AspectRatio(value: number) { __CPP_ComponentProperty_set(this, 2944670669, value); }
}

export class DebugTextComponent extends Component
{
  public static GetTypeNameHash(): number { return 1743500171; }
  get Text(): string { return __CPP_ComponentProperty_get(this, 3557454833); }
  set Text(value: string) { __CPP_ComponentProperty_set(this, 3557454833, value); }
  get Value0(): number { return __CPP_ComponentProperty_get(this, 4030806393); }
  set Value0(value: number) { __CPP_ComponentProperty_set(this, 4030806393, value); }
  get Value1(): number { return __CPP_ComponentProperty_get(this, 1651644235); }
  set Value1(value: number) { __CPP_ComponentProperty_set(this, 1651644235, value); }
  get Value2(): number { return __CPP_ComponentProperty_get(this, 4089579708); }
  set Value2(value: number) { __CPP_ComponentProperty_set(this, 4089579708, value); }
  get Value3(): number { return __CPP_ComponentProperty_get(this, 3785944271); }
  set Value3(value: number) { __CPP_ComponentProperty_set(this, 3785944271, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 3705935169); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 3705935169, value); }
}

export class DecalComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 589392023; }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 734375091); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 734375091, value); }
  get SizeVariance(): number { return __CPP_ComponentProperty_get(this, 1644122058); }
  set SizeVariance(value: number) { __CPP_ComponentProperty_set(this, 1644122058, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 4010460738); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 4010460738, value); }
  get Decal(): string { return __CPP_ComponentProperty_get(this, 1185422148); }
  set Decal(value: string) { __CPP_ComponentProperty_set(this, 1185422148, value); }
  get SortOrder(): number { return __CPP_ComponentProperty_get(this, 3224004631); }
  set SortOrder(value: number) { __CPP_ComponentProperty_set(this, 3224004631, value); }
  get WrapAround(): boolean { return __CPP_ComponentProperty_get(this, 2245558943); }
  set WrapAround(value: boolean) { __CPP_ComponentProperty_set(this, 2245558943, value); }
  get InnerFadeAngle(): number { return __CPP_ComponentProperty_get(this, 3131777915); }
  set InnerFadeAngle(value: number) { __CPP_ComponentProperty_set(this, 3131777915, value); }
  get OuterFadeAngle(): number { return __CPP_ComponentProperty_get(this, 775085950); }
  set OuterFadeAngle(value: number) { __CPP_ComponentProperty_set(this, 775085950, value); }
  get FadeOutDuration(): number { return __CPP_ComponentProperty_get(this, 1614891466); }
  set FadeOutDuration(value: number) { __CPP_ComponentProperty_set(this, 1614891466, value); }
}

export class AmbientLightComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 1530412696; }
  get TopColor(): Color { return __CPP_ComponentProperty_get(this, 3842433167); }
  set TopColor(value: Color) { __CPP_ComponentProperty_set(this, 3842433167, value); }
  get BottomColor(): Color { return __CPP_ComponentProperty_get(this, 2382982970); }
  set BottomColor(value: Color) { __CPP_ComponentProperty_set(this, 2382982970, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 2387501718); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 2387501718, value); }
}

export class LightComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1663499658; }
  get LightColor(): Color { return __CPP_ComponentProperty_get(this, 1069884301); }
  set LightColor(value: Color) { __CPP_ComponentProperty_set(this, 1069884301, value); }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 1836720266); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 1836720266, value); }
  get CastShadows(): boolean { return __CPP_ComponentProperty_get(this, 1533272608); }
  set CastShadows(value: boolean) { __CPP_ComponentProperty_set(this, 1533272608, value); }
  get PenumbraSize(): number { return __CPP_ComponentProperty_get(this, 4200853647); }
  set PenumbraSize(value: number) { __CPP_ComponentProperty_set(this, 4200853647, value); }
  get SlopeBias(): number { return __CPP_ComponentProperty_get(this, 3455577884); }
  set SlopeBias(value: number) { __CPP_ComponentProperty_set(this, 3455577884, value); }
  get ConstantBias(): number { return __CPP_ComponentProperty_get(this, 187715332); }
  set ConstantBias(value: number) { __CPP_ComponentProperty_set(this, 187715332, value); }
}

export class DirectionalLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 719700229; }
  get NumCascades(): number { return __CPP_ComponentProperty_get(this, 1906265529); }
  set NumCascades(value: number) { __CPP_ComponentProperty_set(this, 1906265529, value); }
  get MinShadowRange(): number { return __CPP_ComponentProperty_get(this, 3705456995); }
  set MinShadowRange(value: number) { __CPP_ComponentProperty_set(this, 3705456995, value); }
  get FadeOutStart(): number { return __CPP_ComponentProperty_get(this, 305215886); }
  set FadeOutStart(value: number) { __CPP_ComponentProperty_set(this, 305215886, value); }
  get SplitModeWeight(): number { return __CPP_ComponentProperty_get(this, 3204459616); }
  set SplitModeWeight(value: number) { __CPP_ComponentProperty_set(this, 3204459616, value); }
  get NearPlaneOffset(): number { return __CPP_ComponentProperty_get(this, 897437338); }
  set NearPlaneOffset(value: number) { __CPP_ComponentProperty_set(this, 897437338, value); }
}

export class PointLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 3344432985; }
  get Range(): number { return __CPP_ComponentProperty_get(this, 1197251283); }
  set Range(value: number) { __CPP_ComponentProperty_set(this, 1197251283, value); }
}

export class SkyLightComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 3657161158; }
  get Intensity(): number { return __CPP_ComponentProperty_get(this, 1475596575); }
  set Intensity(value: number) { __CPP_ComponentProperty_set(this, 1475596575, value); }
  get Saturation(): number { return __CPP_ComponentProperty_get(this, 2897044653); }
  set Saturation(value: number) { __CPP_ComponentProperty_set(this, 2897044653, value); }
}

export class SpotLightComponent extends LightComponent
{
  public static GetTypeNameHash(): number { return 3807716399; }
  get Range(): number { return __CPP_ComponentProperty_get(this, 323933761); }
  set Range(value: number) { __CPP_ComponentProperty_set(this, 323933761, value); }
  get InnerSpotAngle(): number { return __CPP_ComponentProperty_get(this, 3350622430); }
  set InnerSpotAngle(value: number) { __CPP_ComponentProperty_set(this, 3350622430, value); }
  get OuterSpotAngle(): number { return __CPP_ComponentProperty_get(this, 374345587); }
  set OuterSpotAngle(value: number) { __CPP_ComponentProperty_set(this, 374345587, value); }
}

export class MeshComponentBase extends RenderComponent
{
  public static GetTypeNameHash(): number { return 4146899997; }
}

export class InstancedMeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 723704301; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 3854761503); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 3854761503, value); }
  get MainColor(): Color { return __CPP_ComponentProperty_get(this, 4286697190); }
  set MainColor(value: Color) { __CPP_ComponentProperty_set(this, 4286697190, value); }
}

export class MeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 278506245; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 774027977); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 774027977, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 2575747912); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 2575747912, value); }
}

export class SkinnedMeshComponent extends MeshComponentBase
{
  public static GetTypeNameHash(): number { return 150377785; }
  get Mesh(): string { return __CPP_ComponentProperty_get(this, 3731938509); }
  set Mesh(value: string) { __CPP_ComponentProperty_set(this, 3731938509, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 864530874); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 864530874, value); }
}

export class AgentSteeringComponent extends Component
{
  public static GetTypeNameHash(): number { return 2686193892; }
}

export class NpcComponent extends Component
{
  public static GetTypeNameHash(): number { return 25589817; }
}

export class ColorAnimationComponent extends Component
{
  public static GetTypeNameHash(): number { return 1742746679; }
  get Gradient(): string { return __CPP_ComponentProperty_get(this, 2021884940); }
  set Gradient(value: string) { __CPP_ComponentProperty_set(this, 2021884940, value); }
  get Duration(): number { return __CPP_ComponentProperty_get(this, 169740918); }
  set Duration(value: number) { __CPP_ComponentProperty_set(this, 169740918, value); }
}

export class MaterialAnimComponent extends Component
{
  public static GetTypeNameHash(): number { return 1269318208; }
  get Material(): string { return __CPP_ComponentProperty_get(this, 2681800903); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 2681800903, value); }
  get Animation(): string { return __CPP_ComponentProperty_get(this, 2121710415); }
  set Animation(value: string) { __CPP_ComponentProperty_set(this, 2121710415, value); }
}

export class PropertyAnimComponent extends Component
{
  public static GetTypeNameHash(): number { return 2178372556; }
  get Animation(): string { return __CPP_ComponentProperty_get(this, 2525170109); }
  set Animation(value: string) { __CPP_ComponentProperty_set(this, 2525170109, value); }
  get Playing(): boolean { return __CPP_ComponentProperty_get(this, 3757895802); }
  set Playing(value: boolean) { __CPP_ComponentProperty_set(this, 3757895802, value); }
  get RandomOffset(): number { return __CPP_ComponentProperty_get(this, 3000366948); }
  set RandomOffset(value: number) { __CPP_ComponentProperty_set(this, 3000366948, value); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 1068692544); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 1068692544, value); }
  get RangeLow(): number { return __CPP_ComponentProperty_get(this, 226977689); }
  set RangeLow(value: number) { __CPP_ComponentProperty_set(this, 226977689, value); }
  get RangeHigh(): number { return __CPP_ComponentProperty_get(this, 3226374128); }
  set RangeHigh(value: number) { __CPP_ComponentProperty_set(this, 3226374128, value); }
}

export class TransformComponent extends Component
{
  public static GetTypeNameHash(): number { return 551484762; }
  SetDirectionForwards(Forwards: boolean): void { __CPP_ComponentFunction_Call(this, 86192402, Forwards); }
  IsDirectionForwards(): boolean { return __CPP_ComponentFunction_Call(this, 2002710701); }
  ToggleDirection(): void { __CPP_ComponentFunction_Call(this, 1099092709); }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 3560178539); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 3560178539, value); }
  get Running(): boolean { return __CPP_ComponentProperty_get(this, 2500412541); }
  set Running(value: boolean) { __CPP_ComponentProperty_set(this, 2500412541, value); }
  get ReverseAtEnd(): boolean { return __CPP_ComponentProperty_get(this, 3727677945); }
  set ReverseAtEnd(value: boolean) { __CPP_ComponentProperty_set(this, 3727677945, value); }
  get ReverseAtStart(): boolean { return __CPP_ComponentProperty_get(this, 4085723490); }
  set ReverseAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 4085723490, value); }
}

export class RotorComponent extends TransformComponent
{
  public static GetTypeNameHash(): number { return 1335116622; }
  get AxisDeviation(): number { return __CPP_ComponentProperty_get(this, 225344491); }
  set AxisDeviation(value: number) { __CPP_ComponentProperty_set(this, 225344491, value); }
  get DegreesToRotate(): number { return __CPP_ComponentProperty_get(this, 3035449786); }
  set DegreesToRotate(value: number) { __CPP_ComponentProperty_set(this, 3035449786, value); }
  get Acceleration(): number { return __CPP_ComponentProperty_get(this, 3771073929); }
  set Acceleration(value: number) { __CPP_ComponentProperty_set(this, 3771073929, value); }
  get Deceleration(): number { return __CPP_ComponentProperty_get(this, 3074476972); }
  set Deceleration(value: number) { __CPP_ComponentProperty_set(this, 3074476972, value); }
}

export class SliderComponent extends TransformComponent
{
  public static GetTypeNameHash(): number { return 2203654552; }
  get Distance(): number { return __CPP_ComponentProperty_get(this, 3931258583); }
  set Distance(value: number) { __CPP_ComponentProperty_set(this, 3931258583, value); }
  get Acceleration(): number { return __CPP_ComponentProperty_get(this, 536360816); }
  set Acceleration(value: number) { __CPP_ComponentProperty_set(this, 536360816, value); }
  get Deceleration(): number { return __CPP_ComponentProperty_get(this, 158982535); }
  set Deceleration(value: number) { __CPP_ComponentProperty_set(this, 158982535, value); }
  get RandomStart(): number { return __CPP_ComponentProperty_get(this, 4145002481); }
  set RandomStart(value: number) { __CPP_ComponentProperty_set(this, 4145002481, value); }
}

export class JointAttachmentComponent extends Component
{
  public static GetTypeNameHash(): number { return 315480184; }
  get JointName(): string { return __CPP_ComponentProperty_get(this, 1509896923); }
  set JointName(value: string) { __CPP_ComponentProperty_set(this, 1509896923, value); }
}

export class LineToComponent extends Component
{
  public static GetTypeNameHash(): number { return 3732008703; }
  get Target(): string { return __CPP_ComponentProperty_get(this, 396242644); }
  set Target(value: string) { __CPP_ComponentProperty_set(this, 396242644, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 711928565); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 711928565, value); }
}

export class SimpleWindComponent extends Component
{
  public static GetTypeNameHash(): number { return 1375451247; }
  get MinStrength(): number { return __CPP_ComponentProperty_get(this, 685243556); }
  set MinStrength(value: number) { __CPP_ComponentProperty_set(this, 685243556, value); }
  get MaxStrength(): number { return __CPP_ComponentProperty_get(this, 243361492); }
  set MaxStrength(value: number) { __CPP_ComponentProperty_set(this, 243361492, value); }
  get Deviation(): number { return __CPP_ComponentProperty_get(this, 3687271029); }
  set Deviation(value: number) { __CPP_ComponentProperty_set(this, 3687271029, value); }
}

export class AreaDamageComponent extends Component
{
  public static GetTypeNameHash(): number { return 2607626610; }
  get OnCreation(): boolean { return __CPP_ComponentProperty_get(this, 507184412); }
  set OnCreation(value: boolean) { __CPP_ComponentProperty_set(this, 507184412, value); }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 690652859); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 690652859, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 3501594183); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 3501594183, value); }
  get Damage(): number { return __CPP_ComponentProperty_get(this, 2211884615); }
  set Damage(value: number) { __CPP_ComponentProperty_set(this, 2211884615, value); }
  get Impulse(): number { return __CPP_ComponentProperty_get(this, 1590335605); }
  set Impulse(value: number) { __CPP_ComponentProperty_set(this, 1590335605, value); }
}

export class GreyBoxComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 1980800360; }
  get Material(): string { return __CPP_ComponentProperty_get(this, 2240130518); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 2240130518, value); }
  get Color(): Color { return __CPP_ComponentProperty_get(this, 417945998); }
  set Color(value: Color) { __CPP_ComponentProperty_set(this, 417945998, value); }
  get SizeNegX(): number { return __CPP_ComponentProperty_get(this, 3386746212); }
  set SizeNegX(value: number) { __CPP_ComponentProperty_set(this, 3386746212, value); }
  get SizePosX(): number { return __CPP_ComponentProperty_get(this, 2691708435); }
  set SizePosX(value: number) { __CPP_ComponentProperty_set(this, 2691708435, value); }
  get SizeNegY(): number { return __CPP_ComponentProperty_get(this, 3935814400); }
  set SizeNegY(value: number) { __CPP_ComponentProperty_set(this, 3935814400, value); }
  get SizePosY(): number { return __CPP_ComponentProperty_get(this, 2209223924); }
  set SizePosY(value: number) { __CPP_ComponentProperty_set(this, 2209223924, value); }
  get SizeNegZ(): number { return __CPP_ComponentProperty_get(this, 1292841833); }
  set SizeNegZ(value: number) { __CPP_ComponentProperty_set(this, 1292841833, value); }
  get SizePosZ(): number { return __CPP_ComponentProperty_get(this, 2924894958); }
  set SizePosZ(value: number) { __CPP_ComponentProperty_set(this, 2924894958, value); }
  get Detail(): number { return __CPP_ComponentProperty_get(this, 3482731274); }
  set Detail(value: number) { __CPP_ComponentProperty_set(this, 3482731274, value); }
  get Curvature(): number { return __CPP_ComponentProperty_get(this, 3803551679); }
  set Curvature(value: number) { __CPP_ComponentProperty_set(this, 3803551679, value); }
  get Thickness(): number { return __CPP_ComponentProperty_get(this, 1898412680); }
  set Thickness(value: number) { __CPP_ComponentProperty_set(this, 1898412680, value); }
  get SlopedTop(): boolean { return __CPP_ComponentProperty_get(this, 826460154); }
  set SlopedTop(value: boolean) { __CPP_ComponentProperty_set(this, 826460154, value); }
  get SlopedBottom(): boolean { return __CPP_ComponentProperty_get(this, 2280834043); }
  set SlopedBottom(value: boolean) { __CPP_ComponentProperty_set(this, 2280834043, value); }
}

export class HeadBoneComponent extends Component
{
  public static GetTypeNameHash(): number { return 1052789136; }
  SetVerticalRotation(Radians: number): void { __CPP_ComponentFunction_Call(this, 3898267856, Radians); }
  ChangeVerticalRotation(Radians: number): void { __CPP_ComponentFunction_Call(this, 2186350086, Radians); }
  get VerticalRotation(): number { return __CPP_ComponentProperty_get(this, 2268548106); }
  set VerticalRotation(value: number) { __CPP_ComponentProperty_set(this, 2268548106, value); }
}

export class InputComponent extends Component
{
  public static GetTypeNameHash(): number { return 3934804548; }
  get InputSet(): string { return __CPP_ComponentProperty_get(this, 1015242655); }
  set InputSet(value: string) { __CPP_ComponentProperty_set(this, 1015242655, value); }
}

export class PlayerStartPointComponent extends Component
{
  public static GetTypeNameHash(): number { return 1578782549; }
  get PlayerPrefab(): string { return __CPP_ComponentProperty_get(this, 212458917); }
  set PlayerPrefab(value: string) { __CPP_ComponentProperty_set(this, 212458917, value); }
}

export class ProjectileComponent extends Component
{
  public static GetTypeNameHash(): number { return 3433729677; }
  get Speed(): number { return __CPP_ComponentProperty_get(this, 3890857686); }
  set Speed(value: number) { __CPP_ComponentProperty_set(this, 3890857686, value); }
  get GravityMultiplier(): number { return __CPP_ComponentProperty_get(this, 4001279176); }
  set GravityMultiplier(value: number) { __CPP_ComponentProperty_set(this, 4001279176, value); }
  get MaxLifetime(): number { return __CPP_ComponentProperty_get(this, 3643167971); }
  set MaxLifetime(value: number) { __CPP_ComponentProperty_set(this, 3643167971, value); }
  get OnTimeoutSpawn(): string { return __CPP_ComponentProperty_get(this, 2143059840); }
  set OnTimeoutSpawn(value: string) { __CPP_ComponentProperty_set(this, 2143059840, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 33838955); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 33838955, value); }
  get FallbackSurface(): string { return __CPP_ComponentProperty_get(this, 757117991); }
  set FallbackSurface(value: string) { __CPP_ComponentProperty_set(this, 757117991, value); }
}

export class TimedDeathComponent extends Component
{
  public static GetTypeNameHash(): number { return 2583125537; }
  get MinDelay(): number { return __CPP_ComponentProperty_get(this, 1166025649); }
  set MinDelay(value: number) { __CPP_ComponentProperty_set(this, 1166025649, value); }
  get DelayRange(): number { return __CPP_ComponentProperty_get(this, 172876544); }
  set DelayRange(value: number) { __CPP_ComponentProperty_set(this, 172876544, value); }
  get TimeoutPrefab(): string { return __CPP_ComponentProperty_get(this, 3682858630); }
  set TimeoutPrefab(value: string) { __CPP_ComponentProperty_set(this, 3682858630, value); }
}

export class SpatialAnchorComponent extends Component
{
  public static GetTypeNameHash(): number { return 1032182987; }
  get PersistentName(): string { return __CPP_ComponentProperty_get(this, 802074756); }
  set PersistentName(value: string) { __CPP_ComponentProperty_set(this, 802074756, value); }
}

export class SrmRenderComponent extends Component
{
  public static GetTypeNameHash(): number { return 3575162138; }
  get Material(): string { return __CPP_ComponentProperty_get(this, 2679596013); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 2679596013, value); }
}

export class CharacterControllerComponent extends Component
{
  public static GetTypeNameHash(): number { return 3480762257; }
}

export class PrefabReferenceComponent extends Component
{
  public static GetTypeNameHash(): number { return 1865057140; }
  get Prefab(): string { return __CPP_ComponentProperty_get(this, 1183151522); }
  set Prefab(value: string) { __CPP_ComponentProperty_set(this, 1183151522, value); }
}

export class SpawnComponent extends Component
{
  public static GetTypeNameHash(): number { return 3190985319; }
  TriggerManualSpawn(): boolean { return __CPP_ComponentFunction_Call(this, 988788857); }
  ScheduleSpawn(): void { __CPP_ComponentFunction_Call(this, 4073221374); }
  get Prefab(): string { return __CPP_ComponentProperty_get(this, 4180171245); }
  set Prefab(value: string) { __CPP_ComponentProperty_set(this, 4180171245, value); }
  get AttachAsChild(): boolean { return __CPP_ComponentProperty_get(this, 1695659419); }
  set AttachAsChild(value: boolean) { __CPP_ComponentProperty_set(this, 1695659419, value); }
  get SpawnAtStart(): boolean { return __CPP_ComponentProperty_get(this, 3342778494); }
  set SpawnAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 3342778494, value); }
  get SpawnContinuously(): boolean { return __CPP_ComponentProperty_get(this, 365863953); }
  set SpawnContinuously(value: boolean) { __CPP_ComponentProperty_set(this, 365863953, value); }
  get MinDelay(): number { return __CPP_ComponentProperty_get(this, 3733536849); }
  set MinDelay(value: number) { __CPP_ComponentProperty_set(this, 3733536849, value); }
  get DelayRange(): number { return __CPP_ComponentProperty_get(this, 2123774873); }
  set DelayRange(value: number) { __CPP_ComponentProperty_set(this, 2123774873, value); }
  get Deviation(): number { return __CPP_ComponentProperty_get(this, 140918596); }
  set Deviation(value: number) { __CPP_ComponentProperty_set(this, 140918596, value); }
}

export class DeviceTrackingComponent extends Component
{
  public static GetTypeNameHash(): number { return 139499111; }
}

export class StageSpaceComponent extends Component
{
  public static GetTypeNameHash(): number { return 958903712; }
}

export class VisualScriptComponent extends EventMessageHandlerComponent
{
  public static GetTypeNameHash(): number { return 2690074736; }
  get Script(): string { return __CPP_ComponentProperty_get(this, 3651176802); }
  set Script(value: string) { __CPP_ComponentProperty_set(this, 3651176802, value); }
  get HandleGlobalEvents(): boolean { return __CPP_ComponentProperty_get(this, 1848150404); }
  set HandleGlobalEvents(value: boolean) { __CPP_ComponentProperty_set(this, 1848150404, value); }
}

export class GizmoComponent extends MeshComponent
{
  public static GetTypeNameHash(): number { return 377001128; }
}

export class FmodComponent extends Component
{
  public static GetTypeNameHash(): number { return 2042709018; }
}

export class FmodEventComponent extends FmodComponent
{
  public static GetTypeNameHash(): number { return 558378035; }
  get Paused(): boolean { return __CPP_ComponentProperty_get(this, 1656273756); }
  set Paused(value: boolean) { __CPP_ComponentProperty_set(this, 1656273756, value); }
  get Volume(): number { return __CPP_ComponentProperty_get(this, 1025867988); }
  set Volume(value: number) { __CPP_ComponentProperty_set(this, 1025867988, value); }
  get Pitch(): number { return __CPP_ComponentProperty_get(this, 3452951144); }
  set Pitch(value: number) { __CPP_ComponentProperty_set(this, 3452951144, value); }
  get SoundEvent(): string { return __CPP_ComponentProperty_get(this, 3003562859); }
  set SoundEvent(value: string) { __CPP_ComponentProperty_set(this, 3003562859, value); }
  get UseOcclusion(): boolean { return __CPP_ComponentProperty_get(this, 2037892002); }
  set UseOcclusion(value: boolean) { __CPP_ComponentProperty_set(this, 2037892002, value); }
  get OcclusionThreshold(): number { return __CPP_ComponentProperty_get(this, 7389752); }
  set OcclusionThreshold(value: number) { __CPP_ComponentProperty_set(this, 7389752, value); }
  get OcclusionCollisionLayer(): number { return __CPP_ComponentProperty_get(this, 1847228176); }
  set OcclusionCollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 1847228176, value); }
  get ShowDebugInfo(): boolean { return __CPP_ComponentProperty_get(this, 2228978601); }
  set ShowDebugInfo(value: boolean) { __CPP_ComponentProperty_set(this, 2228978601, value); }
}

export class FmodListenerComponent extends FmodComponent
{
  public static GetTypeNameHash(): number { return 1825899409; }
  get ListenerIndex(): number { return __CPP_ComponentProperty_get(this, 4006526050); }
  set ListenerIndex(value: number) { __CPP_ComponentProperty_set(this, 4006526050, value); }
}

export class KrautTreeComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 3858119102; }
  get KrautTree(): string { return __CPP_ComponentProperty_get(this, 3713490805); }
  set KrautTree(value: string) { __CPP_ComponentProperty_set(this, 3713490805, value); }
}

export class ParticleComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 2240296493; }
  get Effect(): string { return __CPP_ComponentProperty_get(this, 2934020498); }
  set Effect(value: string) { __CPP_ComponentProperty_set(this, 2934020498, value); }
  get SpawnAtStart(): boolean { return __CPP_ComponentProperty_get(this, 159119901); }
  set SpawnAtStart(value: boolean) { __CPP_ComponentProperty_set(this, 159119901, value); }
  get MinRestartDelay(): number { return __CPP_ComponentProperty_get(this, 2648740691); }
  set MinRestartDelay(value: number) { __CPP_ComponentProperty_set(this, 2648740691, value); }
  get RestartDelayRange(): number { return __CPP_ComponentProperty_get(this, 1970963815); }
  set RestartDelayRange(value: number) { __CPP_ComponentProperty_set(this, 1970963815, value); }
  get RandomSeed(): number { return __CPP_ComponentProperty_get(this, 2905873272); }
  set RandomSeed(value: number) { __CPP_ComponentProperty_set(this, 2905873272, value); }
  get SharedInstanceName(): string { return __CPP_ComponentProperty_get(this, 2929262704); }
  set SharedInstanceName(value: string) { __CPP_ComponentProperty_set(this, 2929262704, value); }
}

export class ParticleFinisherComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 953462438; }
}

export class BreakableSheetComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 464038314; }
  get Material(): string { return __CPP_ComponentProperty_get(this, 2807885632); }
  set Material(value: string) { __CPP_ComponentProperty_set(this, 2807885632, value); }
  get BrokenMaterial(): string { return __CPP_ComponentProperty_get(this, 2065678432); }
  set BrokenMaterial(value: string) { __CPP_ComponentProperty_set(this, 2065678432, value); }
  get Width(): number { return __CPP_ComponentProperty_get(this, 2677769377); }
  set Width(value: number) { __CPP_ComponentProperty_set(this, 2677769377, value); }
  get Height(): number { return __CPP_ComponentProperty_get(this, 1979251224); }
  set Height(value: number) { __CPP_ComponentProperty_set(this, 1979251224, value); }
  get Thickness(): number { return __CPP_ComponentProperty_get(this, 363855313); }
  set Thickness(value: number) { __CPP_ComponentProperty_set(this, 363855313, value); }
  get Density(): number { return __CPP_ComponentProperty_get(this, 1338567896); }
  set Density(value: number) { __CPP_ComponentProperty_set(this, 1338567896, value); }
  get NumPieces(): number { return __CPP_ComponentProperty_get(this, 3342476185); }
  set NumPieces(value: number) { __CPP_ComponentProperty_set(this, 3342476185, value); }
  get DisappearTimeout(): number { return __CPP_ComponentProperty_get(this, 2830655529); }
  set DisappearTimeout(value: number) { __CPP_ComponentProperty_set(this, 2830655529, value); }
  get BreakImpulseStrength(): number { return __CPP_ComponentProperty_get(this, 1942972086); }
  set BreakImpulseStrength(value: number) { __CPP_ComponentProperty_set(this, 1942972086, value); }
  get FixedBorder(): boolean { return __CPP_ComponentProperty_get(this, 145453452); }
  set FixedBorder(value: boolean) { __CPP_ComponentProperty_set(this, 145453452, value); }
  get FixedRandomSeed(): number { return __CPP_ComponentProperty_get(this, 329097487); }
  set FixedRandomSeed(value: number) { __CPP_ComponentProperty_set(this, 329097487, value); }
  get CollisionLayerUnbroken(): number { return __CPP_ComponentProperty_get(this, 680976267); }
  set CollisionLayerUnbroken(value: number) { __CPP_ComponentProperty_set(this, 680976267, value); }
  get CollisionLayerBrokenPieces(): number { return __CPP_ComponentProperty_get(this, 2400150256); }
  set CollisionLayerBrokenPieces(value: number) { __CPP_ComponentProperty_set(this, 2400150256, value); }
  get IncludeInNavmesh(): boolean { return __CPP_ComponentProperty_get(this, 2576117800); }
  set IncludeInNavmesh(value: boolean) { __CPP_ComponentProperty_set(this, 2576117800, value); }
}

export class PxComponent extends Component
{
  public static GetTypeNameHash(): number { return 2589707972; }
}

export class PxActorComponent extends PxComponent
{
  public static GetTypeNameHash(): number { return 2683447660; }
}

export class PxCenterOfMassComponent extends PxComponent
{
  public static GetTypeNameHash(): number { return 2045644465; }
}

export class PxCharacterControllerComponent extends CharacterControllerComponent
{
  public static GetTypeNameHash(): number { return 1223718870; }
  get JumpImpulse(): number { return __CPP_ComponentProperty_get(this, 3752915912); }
  set JumpImpulse(value: number) { __CPP_ComponentProperty_set(this, 3752915912, value); }
  get WalkSpeed(): number { return __CPP_ComponentProperty_get(this, 597881097); }
  set WalkSpeed(value: number) { __CPP_ComponentProperty_set(this, 597881097, value); }
  get RunSpeed(): number { return __CPP_ComponentProperty_get(this, 630476761); }
  set RunSpeed(value: number) { __CPP_ComponentProperty_set(this, 630476761, value); }
  get CrouchSpeed(): number { return __CPP_ComponentProperty_get(this, 4179228608); }
  set CrouchSpeed(value: number) { __CPP_ComponentProperty_set(this, 4179228608, value); }
  get AirSpeed(): number { return __CPP_ComponentProperty_get(this, 873146125); }
  set AirSpeed(value: number) { __CPP_ComponentProperty_set(this, 873146125, value); }
  get AirFriction(): number { return __CPP_ComponentProperty_get(this, 1988656084); }
  set AirFriction(value: number) { __CPP_ComponentProperty_set(this, 1988656084, value); }
  get RotateSpeed(): number { return __CPP_ComponentProperty_get(this, 3412720572); }
  set RotateSpeed(value: number) { __CPP_ComponentProperty_set(this, 3412720572, value); }
  get PushingForce(): number { return __CPP_ComponentProperty_get(this, 1842823331); }
  set PushingForce(value: number) { __CPP_ComponentProperty_set(this, 1842823331, value); }
  get WalkSurfaceInteraction(): string { return __CPP_ComponentProperty_get(this, 3276791160); }
  set WalkSurfaceInteraction(value: string) { __CPP_ComponentProperty_set(this, 3276791160, value); }
  get WalkInteractionDistance(): number { return __CPP_ComponentProperty_get(this, 1567471521); }
  set WalkInteractionDistance(value: number) { __CPP_ComponentProperty_set(this, 1567471521, value); }
  get RunInteractionDistance(): number { return __CPP_ComponentProperty_get(this, 3658331408); }
  set RunInteractionDistance(value: number) { __CPP_ComponentProperty_set(this, 3658331408, value); }
  get FallbackWalkSurface(): string { return __CPP_ComponentProperty_get(this, 1199624698); }
  set FallbackWalkSurface(value: string) { __CPP_ComponentProperty_set(this, 1199624698, value); }
}

export class PxCharacterProxyComponent extends PxComponent
{
  public static GetTypeNameHash(): number { return 3984055946; }
  get CapsuleHeight(): number { return __CPP_ComponentProperty_get(this, 149776140); }
  set CapsuleHeight(value: number) { __CPP_ComponentProperty_set(this, 149776140, value); }
  get CapsuleCrouchHeight(): number { return __CPP_ComponentProperty_get(this, 2342783111); }
  set CapsuleCrouchHeight(value: number) { __CPP_ComponentProperty_set(this, 2342783111, value); }
  get CapsuleRadius(): number { return __CPP_ComponentProperty_get(this, 3485498318); }
  set CapsuleRadius(value: number) { __CPP_ComponentProperty_set(this, 3485498318, value); }
  get Mass(): number { return __CPP_ComponentProperty_get(this, 1246279374); }
  set Mass(value: number) { __CPP_ComponentProperty_set(this, 1246279374, value); }
  get MaxStepHeight(): number { return __CPP_ComponentProperty_get(this, 3002720666); }
  set MaxStepHeight(value: number) { __CPP_ComponentProperty_set(this, 3002720666, value); }
  get MaxSlopeAngle(): number { return __CPP_ComponentProperty_get(this, 1645339142); }
  set MaxSlopeAngle(value: number) { __CPP_ComponentProperty_set(this, 1645339142, value); }
  get ForceSlopeSliding(): boolean { return __CPP_ComponentProperty_get(this, 478230514); }
  set ForceSlopeSliding(value: boolean) { __CPP_ComponentProperty_set(this, 478230514, value); }
  get ConstrainedClimbMode(): boolean { return __CPP_ComponentProperty_get(this, 2896057121); }
  set ConstrainedClimbMode(value: boolean) { __CPP_ComponentProperty_set(this, 2896057121, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 2890519820); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 2890519820, value); }
}

export class PxDynamicActorComponent extends PxActorComponent
{
  public static GetTypeNameHash(): number { return 330874413; }
  get Kinematic(): boolean { return __CPP_ComponentProperty_get(this, 3605752388); }
  set Kinematic(value: boolean) { __CPP_ComponentProperty_set(this, 3605752388, value); }
  get Mass(): number { return __CPP_ComponentProperty_get(this, 2596983535); }
  set Mass(value: number) { __CPP_ComponentProperty_set(this, 2596983535, value); }
  get Density(): number { return __CPP_ComponentProperty_get(this, 3863634452); }
  set Density(value: number) { __CPP_ComponentProperty_set(this, 3863634452, value); }
  get DisableGravity(): boolean { return __CPP_ComponentProperty_get(this, 4122991066); }
  set DisableGravity(value: boolean) { __CPP_ComponentProperty_set(this, 4122991066, value); }
  get LinearDamping(): number { return __CPP_ComponentProperty_get(this, 2006494667); }
  set LinearDamping(value: number) { __CPP_ComponentProperty_set(this, 2006494667, value); }
  get AngularDamping(): number { return __CPP_ComponentProperty_get(this, 1634398123); }
  set AngularDamping(value: number) { __CPP_ComponentProperty_set(this, 1634398123, value); }
  get MaxContactImpulse(): number { return __CPP_ComponentProperty_get(this, 3817615028); }
  set MaxContactImpulse(value: number) { __CPP_ComponentProperty_set(this, 3817615028, value); }
  get ContinuousCollisionDetection(): boolean { return __CPP_ComponentProperty_get(this, 4031185784); }
  set ContinuousCollisionDetection(value: boolean) { __CPP_ComponentProperty_set(this, 4031185784, value); }
}

export class PxRaycastInteractComponent extends Component
{
  public static GetTypeNameHash(): number { return 347650539; }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 1680035716); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 1680035716, value); }
  get MaxDistance(): number { return __CPP_ComponentProperty_get(this, 4274457726); }
  set MaxDistance(value: number) { __CPP_ComponentProperty_set(this, 4274457726, value); }
  get UserMessage(): string { return __CPP_ComponentProperty_get(this, 1381685518); }
  set UserMessage(value: string) { __CPP_ComponentProperty_set(this, 1381685518, value); }
}

export class PxSettingsComponent extends SettingsComponent
{
  public static GetTypeNameHash(): number { return 1345701569; }
  get ObjectGravity(): Vec3 { return __CPP_ComponentProperty_get(this, 1684378819); }
  set ObjectGravity(value: Vec3) { __CPP_ComponentProperty_set(this, 1684378819, value); }
  get CharacterGravity(): Vec3 { return __CPP_ComponentProperty_get(this, 1959650009); }
  set CharacterGravity(value: Vec3) { __CPP_ComponentProperty_set(this, 1959650009, value); }
  get MaxDepenetrationVelocity(): number { return __CPP_ComponentProperty_get(this, 3087567019); }
  set MaxDepenetrationVelocity(value: number) { __CPP_ComponentProperty_set(this, 3087567019, value); }
  get FixedFrameRate(): number { return __CPP_ComponentProperty_get(this, 2753189343); }
  set FixedFrameRate(value: number) { __CPP_ComponentProperty_set(this, 2753189343, value); }
  get MaxSubSteps(): number { return __CPP_ComponentProperty_get(this, 1800354618); }
  set MaxSubSteps(value: number) { __CPP_ComponentProperty_set(this, 1800354618, value); }
}

export class PxStaticActorComponent extends PxActorComponent
{
  public static GetTypeNameHash(): number { return 1704525155; }
  get CollisionMesh(): string { return __CPP_ComponentProperty_get(this, 1834963500); }
  set CollisionMesh(value: string) { __CPP_ComponentProperty_set(this, 1834963500, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 4086752060); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 4086752060, value); }
  get IncludeInNavmesh(): boolean { return __CPP_ComponentProperty_get(this, 253099092); }
  set IncludeInNavmesh(value: boolean) { __CPP_ComponentProperty_set(this, 253099092, value); }
}

export class PxTriggerComponent extends PxActorComponent
{
  public static GetTypeNameHash(): number { return 2614527918; }
  get Kinematic(): boolean { return __CPP_ComponentProperty_get(this, 734053562); }
  set Kinematic(value: boolean) { __CPP_ComponentProperty_set(this, 734053562, value); }
  get TriggerMessage(): string { return __CPP_ComponentProperty_get(this, 2743525242); }
  set TriggerMessage(value: string) { __CPP_ComponentProperty_set(this, 2743525242, value); }
}

export class PxVisColMeshComponent extends RenderComponent
{
  public static GetTypeNameHash(): number { return 944888949; }
  get CollisionMesh(): string { return __CPP_ComponentProperty_get(this, 1815135183); }
  set CollisionMesh(value: string) { __CPP_ComponentProperty_set(this, 1815135183, value); }
}

export class PxJointComponent extends PxComponent
{
  public static GetTypeNameHash(): number { return 2886076577; }
  get BreakForce(): number { return __CPP_ComponentProperty_get(this, 2543536925); }
  set BreakForce(value: number) { __CPP_ComponentProperty_set(this, 2543536925, value); }
  get BreakTorque(): number { return __CPP_ComponentProperty_get(this, 192834439); }
  set BreakTorque(value: number) { __CPP_ComponentProperty_set(this, 192834439, value); }
  get PairCollision(): boolean { return __CPP_ComponentProperty_get(this, 2301394125); }
  set PairCollision(value: boolean) { __CPP_ComponentProperty_set(this, 2301394125, value); }
  get ParentActor(): string { return __CPP_ComponentProperty_get(this, 2784251693); }
  set ParentActor(value: string) { __CPP_ComponentProperty_set(this, 2784251693, value); }
  get ChildActor(): string { return __CPP_ComponentProperty_get(this, 2253256299); }
  set ChildActor(value: string) { __CPP_ComponentProperty_set(this, 2253256299, value); }
}

export class Px6DOFJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 4232843366; }
  get LinearStiffness(): number { return __CPP_ComponentProperty_get(this, 936192555); }
  set LinearStiffness(value: number) { __CPP_ComponentProperty_set(this, 936192555, value); }
  get LinearDamping(): number { return __CPP_ComponentProperty_get(this, 2279764867); }
  set LinearDamping(value: number) { __CPP_ComponentProperty_set(this, 2279764867, value); }
  get AngularStiffness(): number { return __CPP_ComponentProperty_get(this, 439117069); }
  set AngularStiffness(value: number) { __CPP_ComponentProperty_set(this, 439117069, value); }
  get AngularDamping(): number { return __CPP_ComponentProperty_get(this, 927888496); }
  set AngularDamping(value: number) { __CPP_ComponentProperty_set(this, 927888496, value); }
}

export class PxDistanceJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 1331924690; }
  get MinDistance(): number { return __CPP_ComponentProperty_get(this, 1376210225); }
  set MinDistance(value: number) { __CPP_ComponentProperty_set(this, 1376210225, value); }
  get MaxDistance(): number { return __CPP_ComponentProperty_get(this, 3056334569); }
  set MaxDistance(value: number) { __CPP_ComponentProperty_set(this, 3056334569, value); }
  get SpringStiffness(): number { return __CPP_ComponentProperty_get(this, 3118952870); }
  set SpringStiffness(value: number) { __CPP_ComponentProperty_set(this, 3118952870, value); }
  get SpringDamping(): number { return __CPP_ComponentProperty_get(this, 544415573); }
  set SpringDamping(value: number) { __CPP_ComponentProperty_set(this, 544415573, value); }
  get SpringTolerance(): number { return __CPP_ComponentProperty_get(this, 1467670947); }
  set SpringTolerance(value: number) { __CPP_ComponentProperty_set(this, 1467670947, value); }
}

export class PxFixedJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 2039308803; }
}

export class PxPrismaticJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 1339125375; }
  get LowerLimit(): number { return __CPP_ComponentProperty_get(this, 552741316); }
  set LowerLimit(value: number) { __CPP_ComponentProperty_set(this, 552741316, value); }
  get UpperLimit(): number { return __CPP_ComponentProperty_get(this, 254409485); }
  set UpperLimit(value: number) { __CPP_ComponentProperty_set(this, 254409485, value); }
  get Stiffness(): number { return __CPP_ComponentProperty_get(this, 4188360833); }
  set Stiffness(value: number) { __CPP_ComponentProperty_set(this, 4188360833, value); }
  get Damping(): number { return __CPP_ComponentProperty_get(this, 3708473064); }
  set Damping(value: number) { __CPP_ComponentProperty_set(this, 3708473064, value); }
}

export class PxRevoluteJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 4255433392; }
  get LimitRotation(): boolean { return __CPP_ComponentProperty_get(this, 3553229283); }
  set LimitRotation(value: boolean) { __CPP_ComponentProperty_set(this, 3553229283, value); }
  get LowerLimit(): number { return __CPP_ComponentProperty_get(this, 3894764947); }
  set LowerLimit(value: number) { __CPP_ComponentProperty_set(this, 3894764947, value); }
  get UpperLimit(): number { return __CPP_ComponentProperty_get(this, 457135448); }
  set UpperLimit(value: number) { __CPP_ComponentProperty_set(this, 457135448, value); }
  get EnableDrive(): boolean { return __CPP_ComponentProperty_get(this, 2443522805); }
  set EnableDrive(value: boolean) { __CPP_ComponentProperty_set(this, 2443522805, value); }
  get DriveVelocity(): number { return __CPP_ComponentProperty_get(this, 1617742204); }
  set DriveVelocity(value: number) { __CPP_ComponentProperty_set(this, 1617742204, value); }
  get DriveBraking(): boolean { return __CPP_ComponentProperty_get(this, 537795305); }
  set DriveBraking(value: boolean) { __CPP_ComponentProperty_set(this, 537795305, value); }
}

export class PxSphericalJointComponent extends PxJointComponent
{
  public static GetTypeNameHash(): number { return 3831607129; }
  get LimitRotation(): boolean { return __CPP_ComponentProperty_get(this, 2941248708); }
  set LimitRotation(value: boolean) { __CPP_ComponentProperty_set(this, 2941248708, value); }
  get ConeLimitY(): number { return __CPP_ComponentProperty_get(this, 3717977844); }
  set ConeLimitY(value: number) { __CPP_ComponentProperty_set(this, 3717977844, value); }
  get ConeLimitZ(): number { return __CPP_ComponentProperty_get(this, 3550126526); }
  set ConeLimitZ(value: number) { __CPP_ComponentProperty_set(this, 3550126526, value); }
}

export class PxShapeComponent extends PxComponent
{
  public static GetTypeNameHash(): number { return 2433889857; }
  get Surface(): string { return __CPP_ComponentProperty_get(this, 1501508833); }
  set Surface(value: string) { __CPP_ComponentProperty_set(this, 1501508833, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 2220097582); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 2220097582, value); }
}

export class PxShapeBoxComponent extends PxShapeComponent
{
  public static GetTypeNameHash(): number { return 2313036161; }
  get Extents(): Vec3 { return __CPP_ComponentProperty_get(this, 3296683976); }
  set Extents(value: Vec3) { __CPP_ComponentProperty_set(this, 3296683976, value); }
}

export class PxShapeCapsuleComponent extends PxShapeComponent
{
  public static GetTypeNameHash(): number { return 895423988; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 92680004); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 92680004, value); }
  get Height(): number { return __CPP_ComponentProperty_get(this, 107119866); }
  set Height(value: number) { __CPP_ComponentProperty_set(this, 107119866, value); }
}

export class PxShapeConvexComponent extends PxShapeComponent
{
  public static GetTypeNameHash(): number { return 1906805707; }
  get CollisionMesh(): string { return __CPP_ComponentProperty_get(this, 152232487); }
  set CollisionMesh(value: string) { __CPP_ComponentProperty_set(this, 152232487, value); }
}

export class PxShapeSphereComponent extends PxShapeComponent
{
  public static GetTypeNameHash(): number { return 3488897286; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 843798237); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 843798237, value); }
}

export class ProcPlacementComponent extends Component
{
  public static GetTypeNameHash(): number { return 1547738067; }
  get Resource(): string { return __CPP_ComponentProperty_get(this, 4043566494); }
  set Resource(value: string) { __CPP_ComponentProperty_set(this, 4043566494, value); }
}

export class ProcVertexColorComponent extends MeshComponent
{
  public static GetTypeNameHash(): number { return 1329603393; }
  get Resource(): string { return __CPP_ComponentProperty_get(this, 2195391453); }
  set Resource(value: string) { __CPP_ComponentProperty_set(this, 2195391453, value); }
}

export class RcComponent extends Component
{
  public static GetTypeNameHash(): number { return 3588964595; }
}

export class RcMarkPoiVisibleComponent extends RcComponent
{
  public static GetTypeNameHash(): number { return 3659328064; }
  get Radius(): number { return __CPP_ComponentProperty_get(this, 3031724320); }
  set Radius(value: number) { __CPP_ComponentProperty_set(this, 3031724320, value); }
  get CollisionLayer(): number { return __CPP_ComponentProperty_get(this, 1456454779); }
  set CollisionLayer(value: number) { __CPP_ComponentProperty_set(this, 1456454779, value); }
}

export class RcAgentComponent extends AgentSteeringComponent
{
  public static GetTypeNameHash(): number { return 3286434196; }
  get WalkSpeed(): number { return __CPP_ComponentProperty_get(this, 3486937020); }
  set WalkSpeed(value: number) { __CPP_ComponentProperty_set(this, 3486937020, value); }
}

export class RcNavMeshComponent extends RcComponent
{
  public static GetTypeNameHash(): number { return 2903450766; }
  get ShowNavMesh(): boolean { return __CPP_ComponentProperty_get(this, 882906550); }
  set ShowNavMesh(value: boolean) { __CPP_ComponentProperty_set(this, 882906550, value); }
}

export class SoldierComponent extends NpcComponent
{
  public static GetTypeNameHash(): number { return 2491962252; }
}

export class TypeScriptComponent extends Component
{
  public static GetTypeNameHash(): number { return 2634110472; }
  get Script(): string { return __CPP_ComponentProperty_get(this, 1497334971); }
  set Script(value: string) { __CPP_ComponentProperty_set(this, 1497334971, value); }
}

export class ShapeIconComponent extends Component
{
  public static GetTypeNameHash(): number { return 730772158; }
}

