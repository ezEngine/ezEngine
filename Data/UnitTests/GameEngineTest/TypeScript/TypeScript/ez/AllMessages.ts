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


export class EventMsgSetPowerInput extends EventMessage
{
  public static GetTypeNameHash(): number { return 2697475808; }
  constructor() { super(); this.TypeNameHash = 2697475808; }
  PrevValue: number = 0;
  NewValue: number = 0;
}

export class MsgAnimationPoseGeneration extends Message
{
  public static GetTypeNameHash(): number { return 1114351692; }
  constructor() { super(); this.TypeNameHash = 1114351692; }
}

export class MsgAnimationPosePreparing extends Message
{
  public static GetTypeNameHash(): number { return 2419062254; }
  constructor() { super(); this.TypeNameHash = 2419062254; }
}

export class MsgAnimationPoseUpdated extends Message
{
  public static GetTypeNameHash(): number { return 1062497747; }
  constructor() { super(); this.TypeNameHash = 1062497747; }
}

export class MsgAnimationReachedEnd extends EventMessage
{
  public static GetTypeNameHash(): number { return 1074277864; }
  constructor() { super(); this.TypeNameHash = 1074277864; }
}

export class MsgApplyRootMotion extends Message
{
  public static GetTypeNameHash(): number { return 3983304456; }
  constructor() { super(); this.TypeNameHash = 3983304456; }
  Translation: Vec3 = new Vec3(0, 0, 0);
  RotationX: number = 0;
  RotationY: number = 0;
  RotationZ: number = 0;
}

export class MsgBlackboardEntryChanged extends EventMessage
{
  public static GetTypeNameHash(): number { return 1414854208; }
  constructor() { super(); this.TypeNameHash = 1414854208; }
  Name: string;
  OldValue: any;
  NewValue: any;
}

export class MsgBuildStaticMesh extends Message
{
  public static GetTypeNameHash(): number { return 2644487384; }
  constructor() { super(); this.TypeNameHash = 2644487384; }
}

export class MsgChildrenChanged extends Message
{
  public static GetTypeNameHash(): number { return 1649659686; }
  constructor() { super(); this.TypeNameHash = 1649659686; }
}

export class MsgCollision extends Message
{
  public static GetTypeNameHash(): number { return 2889293180; }
  constructor() { super(); this.TypeNameHash = 2889293180; }
}

export class MsgComponentInternalTrigger extends Message
{
  public static GetTypeNameHash(): number { return 2150357888; }
  constructor() { super(); this.TypeNameHash = 2150357888; }
  Message: string;
  Payload: number = 0;
}

export class MsgComponentsChanged extends Message
{
  public static GetTypeNameHash(): number { return 1350137744; }
  constructor() { super(); this.TypeNameHash = 1350137744; }
}

export class MsgDamage extends EventMessage
{
  public static GetTypeNameHash(): number { return 3803404036; }
  constructor() { super(); this.TypeNameHash = 3803404036; }
  Damage: number = 0;
  HitObjectName: string;
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  ImpactDirection: Vec3 = new Vec3(0, 0, 0);
}

export class MsgDeleteGameObject extends Message
{
  public static GetTypeNameHash(): number { return 3381391115; }
  constructor() { super(); this.TypeNameHash = 3381391115; }
}

export class MsgExtractGeometry extends Message
{
  public static GetTypeNameHash(): number { return 2851303219; }
  constructor() { super(); this.TypeNameHash = 2851303219; }
}

export class MsgExtractOccluderData extends Message
{
  public static GetTypeNameHash(): number { return 3924382644; }
  constructor() { super(); this.TypeNameHash = 3924382644; }
}

export class MsgExtractRenderData extends Message
{
  public static GetTypeNameHash(): number { return 2914273568; }
  constructor() { super(); this.TypeNameHash = 2914273568; }
}

export class MsgGenericEvent extends EventMessage
{
  public static GetTypeNameHash(): number { return 3196354340; }
  constructor() { super(); this.TypeNameHash = 3196354340; }
  Message: string;
  Value: any = 0;
}

export class MsgInputActionTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 94741345; }
  constructor() { super(); this.TypeNameHash = 94741345; }
  InputAction: string;
  KeyPressValue: number = 0;
  TriggerState: Enum.TriggerState = 0;
}

export class MsgInterruptPlaying extends Message
{
  public static GetTypeNameHash(): number { return 1319502469; }
  constructor() { super(); this.TypeNameHash = 1319502469; }
}

export class MsgMoveCharacterController extends Message
{
  public static GetTypeNameHash(): number { return 3376977277; }
  constructor() { super(); this.TypeNameHash = 3376977277; }
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

export class MsgObjectGrabbed extends Message
{
  public static GetTypeNameHash(): number { return 1848455959; }
  constructor() { super(); this.TypeNameHash = 1848455959; }
  GotGrabbed: boolean = false;
}

export class MsgOnlyApplyToObject extends Message
{
  public static GetTypeNameHash(): number { return 2494349142; }
  constructor() { super(); this.TypeNameHash = 2494349142; }
}

export class MsgParentChanged extends Message
{
  public static GetTypeNameHash(): number { return 2163082146; }
  constructor() { super(); this.TypeNameHash = 2163082146; }
}

export class MsgPathChanged extends EventMessage
{
  public static GetTypeNameHash(): number { return 1539147651; }
  constructor() { super(); this.TypeNameHash = 1539147651; }
}

export class MsgPhysicsAddForce extends Message
{
  public static GetTypeNameHash(): number { return 555661941; }
  constructor() { super(); this.TypeNameHash = 555661941; }
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  Force: Vec3 = new Vec3(0, 0, 0);
}

export class MsgPhysicsAddImpulse extends Message
{
  public static GetTypeNameHash(): number { return 2566646672; }
  constructor() { super(); this.TypeNameHash = 2566646672; }
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  Impulse: Vec3 = new Vec3(0, 0, 0);
  ObjectFilterID: number = 0;
}

export class MsgPhysicsJointBroke extends EventMessage
{
  public static GetTypeNameHash(): number { return 1311784990; }
  constructor() { super(); this.TypeNameHash = 1311784990; }
}

export class MsgQueryAnimationSkeleton extends Message
{
  public static GetTypeNameHash(): number { return 4103519367; }
  constructor() { super(); this.TypeNameHash = 4103519367; }
}

export class MsgReleaseObjectGrab extends Message
{
  public static GetTypeNameHash(): number { return 1937223203; }
  constructor() { super(); this.TypeNameHash = 1937223203; }
}

export class MsgRetrieveBoneState extends Message
{
  public static GetTypeNameHash(): number { return 2483229000; }
  constructor() { super(); this.TypeNameHash = 2483229000; }
}

export class MsgRopePoseUpdated extends Message
{
  public static GetTypeNameHash(): number { return 2063668489; }
  constructor() { super(); this.TypeNameHash = 2063668489; }
}

export class MsgSensorDetectedObjectsChanged extends EventMessage
{
  public static GetTypeNameHash(): number { return 3745087146; }
  constructor() { super(); this.TypeNameHash = 3745087146; }
}

export class MsgSetColor extends Message
{
  public static GetTypeNameHash(): number { return 1623708695; }
  constructor() { super(); this.TypeNameHash = 1623708695; }
  Color: Color = new Color(1, 1, 1, 1);
  Mode: Enum.SetColorMode = 0;
}

export class MsgSetCustomData extends Message
{
  public static GetTypeNameHash(): number { return 3715064801; }
  constructor() { super(); this.TypeNameHash = 3715064801; }
  Data0: number = 0;
  Data1: number = 0;
  Data2: number = 0;
  Data3: number = 0;
}

export class MsgSetFloatParameter extends Message
{
  public static GetTypeNameHash(): number { return 2829824120; }
  constructor() { super(); this.TypeNameHash = 2829824120; }
  Name: string;
  Value: number = 0;
}

export class MsgSetMeshMaterial extends Message
{
  public static GetTypeNameHash(): number { return 4237338344; }
  constructor() { super(); this.TypeNameHash = 4237338344; }
  Material: string;
  MaterialSlot: number = 0;
}

export class MsgSetPlaying extends Message
{
  public static GetTypeNameHash(): number { return 3392464038; }
  constructor() { super(); this.TypeNameHash = 3392464038; }
  Play: boolean = true;
}

export class MsgStateMachineStateChanged extends EventMessage
{
  public static GetTypeNameHash(): number { return 3508545104; }
  constructor() { super(); this.TypeNameHash = 3508545104; }
  OldStateName: string;
  NewStateName: string;
}

export class MsgTransformChanged extends Message
{
  public static GetTypeNameHash(): number { return 3642376348; }
  constructor() { super(); this.TypeNameHash = 3642376348; }
}

export class MsgTriggerTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 3303636443; }
  constructor() { super(); this.TypeNameHash = 3303636443; }
  Message: string;
  TriggerState: Enum.TriggerState = 0;
}

export class MsgTypeScriptMsgProxy extends Message
{
  public static GetTypeNameHash(): number { return 2451549063; }
  constructor() { super(); this.TypeNameHash = 2451549063; }
}

export class MsgUpdateLocalBounds extends Message
{
  public static GetTypeNameHash(): number { return 3747139038; }
  constructor() { super(); this.TypeNameHash = 3747139038; }
}

