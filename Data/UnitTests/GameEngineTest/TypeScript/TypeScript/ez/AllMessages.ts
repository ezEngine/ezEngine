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
  public static GetTypeNameHash(): number { return 3874463418; }
  constructor() { super(); this.TypeNameHash = 3874463418; }
}

export class MsgAnimationReachedEnd extends EventMessage
{
  public static GetTypeNameHash(): number { return 3161817575; }
  constructor() { super(); this.TypeNameHash = 3161817575; }
}

export class MsgBreakableSheetBroke extends EventMessage
{
  public static GetTypeNameHash(): number { return 2481203103; }
  constructor() { super(); this.TypeNameHash = 2481203103; }
}

export class MsgBuildStaticMesh extends Message
{
  public static GetTypeNameHash(): number { return 3694682949; }
  constructor() { super(); this.TypeNameHash = 3694682949; }
}

export class MsgChildrenChanged extends Message
{
  public static GetTypeNameHash(): number { return 2784142053; }
  constructor() { super(); this.TypeNameHash = 2784142053; }
}

export class MsgCollision extends Message
{
  public static GetTypeNameHash(): number { return 1454903456; }
  constructor() { super(); this.TypeNameHash = 1454903456; }
}

export class MsgComponentInternalTrigger extends Message
{
  public static GetTypeNameHash(): number { return 2729883851; }
  constructor() { super(); this.TypeNameHash = 2729883851; }
  UsageStringHash: number = 0;
}

export class MsgComponentsChanged extends Message
{
  public static GetTypeNameHash(): number { return 1233680431; }
  constructor() { super(); this.TypeNameHash = 1233680431; }
}

export class MsgDamage extends EventMessage
{
  public static GetTypeNameHash(): number { return 3889610425; }
  constructor() { super(); this.TypeNameHash = 3889610425; }
  Damage: number = 0;
  HitObjectName: string;
}

export class MsgDeleteGameObject extends Message
{
  public static GetTypeNameHash(): number { return 214557405; }
  constructor() { super(); this.TypeNameHash = 214557405; }
}

export class MsgExtractGeometry extends Message
{
  public static GetTypeNameHash(): number { return 4190830039; }
  constructor() { super(); this.TypeNameHash = 4190830039; }
}

export class MsgExtractRenderData extends Message
{
  public static GetTypeNameHash(): number { return 4250635752; }
  constructor() { super(); this.TypeNameHash = 4250635752; }
}

export class MsgExtractVolumes extends Message
{
  public static GetTypeNameHash(): number { return 3796594901; }
  constructor() { super(); this.TypeNameHash = 3796594901; }
}

export class MsgFmodSoundFinished extends EventMessage
{
  public static GetTypeNameHash(): number { return 4085975113; }
  constructor() { super(); this.TypeNameHash = 4085975113; }
}

export class MsgGenericEvent extends EventMessage
{
  public static GetTypeNameHash(): number { return 1061762376; }
  constructor() { super(); this.TypeNameHash = 1061762376; }
  Message: string;
}

export class MsgInputActionTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 2990029836; }
  constructor() { super(); this.TypeNameHash = 2990029836; }
  InputActionHash: number = 0;
  KeyPressValue: number = 0;
  TriggerState: Enum.TriggerState = 0;
}

export class MsgMoveCharacterController extends Message
{
  public static GetTypeNameHash(): number { return 2731592292; }
  constructor() { super(); this.TypeNameHash = 2731592292; }
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
  public static GetTypeNameHash(): number { return 1217048705; }
  constructor() { super(); this.TypeNameHash = 1217048705; }
}

export class MsgPhysicsAddForce extends Message
{
  public static GetTypeNameHash(): number { return 3475477528; }
  constructor() { super(); this.TypeNameHash = 3475477528; }
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  Force: Vec3 = new Vec3(0, 0, 0);
}

export class MsgPhysicsAddImpulse extends Message
{
  public static GetTypeNameHash(): number { return 2618981170; }
  constructor() { super(); this.TypeNameHash = 2618981170; }
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  Impulse: Vec3 = new Vec3(0, 0, 0);
  ShapeID: number = 0;
}

export class MsgSetColor extends Message
{
  public static GetTypeNameHash(): number { return 12434892; }
  constructor() { super(); this.TypeNameHash = 12434892; }
  Color: Color = new Color(1, 1, 1, 1);
  Mode: Enum.SetColorMode = 0;
}

export class MsgSetFloatParameter extends Message
{
  public static GetTypeNameHash(): number { return 2334834113; }
  constructor() { super(); this.TypeNameHash = 2334834113; }
  Name: string;
  Value: number = 0;
}

export class MsgSetMeshMaterial extends Message
{
  public static GetTypeNameHash(): number { return 2062037541; }
  constructor() { super(); this.TypeNameHash = 2062037541; }
  Material: string;
  MaterialSlot: number = 0;
}

export class MsgSetPlaying extends Message
{
  public static GetTypeNameHash(): number { return 744214609; }
  constructor() { super(); this.TypeNameHash = 744214609; }
  Play: boolean = true;
}

export class MsgTransformChanged extends Message
{
  public static GetTypeNameHash(): number { return 769397284; }
  constructor() { super(); this.TypeNameHash = 769397284; }
}

export class MsgTriggerTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 1755267521; }
  constructor() { super(); this.TypeNameHash = 1755267521; }
  MsgStringHash: number = 0;
  TriggerState: Enum.TriggerState = 0;
}

export class MsgTypeScriptMsgProxy extends Message
{
  public static GetTypeNameHash(): number { return 344792456; }
  constructor() { super(); this.TypeNameHash = 344792456; }
}

export class MsgUpdateLocalBounds extends Message
{
  public static GetTypeNameHash(): number { return 1393097690; }
  constructor() { super(); this.TypeNameHash = 1393097690; }
}

