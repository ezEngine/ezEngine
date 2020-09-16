// AUTO-GENERATED FILE

import __Message = require("TypeScript/ez/Message")
export import Message = __Message.Message;
export import EventMessage = __Message.EventMessage;

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


export class MsgAnimationPoseUpdated extends Message
{
  public static GetTypeNameHash(): number { return 760653206; }
  constructor() { super(); this.TypeNameHash = 760653206; }
}

export class MsgAnimationReachedEnd extends EventMessage
{
  public static GetTypeNameHash(): number { return 3024948166; }
  constructor() { super(); this.TypeNameHash = 3024948166; }
}

export class MsgBreakableSheetBroke extends EventMessage
{
  public static GetTypeNameHash(): number { return 3499974897; }
  constructor() { super(); this.TypeNameHash = 3499974897; }
}

export class MsgBuildStaticMesh extends Message
{
  public static GetTypeNameHash(): number { return 3599673213; }
  constructor() { super(); this.TypeNameHash = 3599673213; }
}

export class MsgChildrenChanged extends Message
{
  public static GetTypeNameHash(): number { return 505857305; }
  constructor() { super(); this.TypeNameHash = 505857305; }
}

export class MsgCollision extends Message
{
  public static GetTypeNameHash(): number { return 3339340565; }
  constructor() { super(); this.TypeNameHash = 3339340565; }
}

export class MsgComponentInternalTrigger extends Message
{
  public static GetTypeNameHash(): number { return 268593481; }
  constructor() { super(); this.TypeNameHash = 268593481; }
  UsageStringHash: number = 0;
}

export class MsgComponentsChanged extends Message
{
  public static GetTypeNameHash(): number { return 936654107; }
  constructor() { super(); this.TypeNameHash = 936654107; }
}

export class MsgDamage extends EventMessage
{
  public static GetTypeNameHash(): number { return 146526392; }
  constructor() { super(); this.TypeNameHash = 146526392; }
  Damage: number = 0;
  HitObjectName: string;
}

export class MsgDeleteGameObject extends Message
{
  public static GetTypeNameHash(): number { return 1893671047; }
  constructor() { super(); this.TypeNameHash = 1893671047; }
}

export class MsgExtractGeometry extends Message
{
  public static GetTypeNameHash(): number { return 2613444800; }
  constructor() { super(); this.TypeNameHash = 2613444800; }
}

export class MsgExtractRenderData extends Message
{
  public static GetTypeNameHash(): number { return 2595853294; }
  constructor() { super(); this.TypeNameHash = 2595853294; }
}

export class MsgExtractVolumes extends Message
{
  public static GetTypeNameHash(): number { return 1727764704; }
  constructor() { super(); this.TypeNameHash = 1727764704; }
}

export class MsgFmodSoundFinished extends EventMessage
{
  public static GetTypeNameHash(): number { return 877453568; }
  constructor() { super(); this.TypeNameHash = 877453568; }
}

export class MsgGenericEvent extends EventMessage
{
  public static GetTypeNameHash(): number { return 226332170; }
  constructor() { super(); this.TypeNameHash = 226332170; }
  Message: string;
}

export class MsgInputActionTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 83925379; }
  constructor() { super(); this.TypeNameHash = 83925379; }
  InputActionHash: number = 0;
  KeyPressValue: number = 0;
  TriggerState: Enum.TriggerState = 0;
}

export class MsgMoveCharacterController extends Message
{
  public static GetTypeNameHash(): number { return 1703149706; }
  constructor() { super(); this.TypeNameHash = 1703149706; }
  MoveForwards: number = 0;
  MoveBackwards: number = 0;
  StrafeLeft: number = 0;
  StrafeRight: number = 0;
  RotateLeft: number = 0;
  RotateRight: number = 0;
  Run: boolean = false;
  Jump: boolean = false;
  Crouch: boolean = false;
}

export class MsgOnlyApplyToObject extends Message
{
  public static GetTypeNameHash(): number { return 3025894004; }
  constructor() { super(); this.TypeNameHash = 3025894004; }
}

export class MsgPhysicsAddForce extends Message
{
  public static GetTypeNameHash(): number { return 617921652; }
  constructor() { super(); this.TypeNameHash = 617921652; }
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  Force: Vec3 = new Vec3(0, 0, 0);
}

export class MsgPhysicsAddImpulse extends Message
{
  public static GetTypeNameHash(): number { return 3193218110; }
  constructor() { super(); this.TypeNameHash = 3193218110; }
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  Impulse: Vec3 = new Vec3(0, 0, 0);
  ShapeID: number = 0;
}

export class MsgPhysicsJointBroke extends EventMessage
{
  public static GetTypeNameHash(): number { return 4089742341; }
  constructor() { super(); this.TypeNameHash = 4089742341; }
}

export class MsgQueryAnimationSkeleton extends Message
{
  public static GetTypeNameHash(): number { return 1559971141; }
  constructor() { super(); this.TypeNameHash = 1559971141; }
}

export class MsgRmlUiReload extends Message
{
  public static GetTypeNameHash(): number { return 2540580649; }
  constructor() { super(); this.TypeNameHash = 2540580649; }
}

export class MsgSetColor extends Message
{
  public static GetTypeNameHash(): number { return 1685882411; }
  constructor() { super(); this.TypeNameHash = 1685882411; }
  Color: Color = new Color(1, 1, 1, 1);
  Mode: Enum.SetColorMode = 0;
}

export class MsgSetFloatParameter extends Message
{
  public static GetTypeNameHash(): number { return 1720453306; }
  constructor() { super(); this.TypeNameHash = 1720453306; }
  Name: string;
  Value: number = 0;
}

export class MsgSetMeshMaterial extends Message
{
  public static GetTypeNameHash(): number { return 719232908; }
  constructor() { super(); this.TypeNameHash = 719232908; }
  Material: string;
  MaterialSlot: number = 0;
}

export class MsgSetPlaying extends Message
{
  public static GetTypeNameHash(): number { return 419138500; }
  constructor() { super(); this.TypeNameHash = 419138500; }
  Play: boolean = true;
}

export class MsgTransformChanged extends Message
{
  public static GetTypeNameHash(): number { return 4141964096; }
  constructor() { super(); this.TypeNameHash = 4141964096; }
}

export class MsgTriggerTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 1043285181; }
  constructor() { super(); this.TypeNameHash = 1043285181; }
  MsgStringHash: number = 0;
  TriggerState: Enum.TriggerState = 0;
}

export class MsgTypeScriptMsgProxy extends Message
{
  public static GetTypeNameHash(): number { return 1384422179; }
  constructor() { super(); this.TypeNameHash = 1384422179; }
}

export class MsgUpdateLocalBounds extends Message
{
  public static GetTypeNameHash(): number { return 2547703269; }
  constructor() { super(); this.TypeNameHash = 2547703269; }
}

