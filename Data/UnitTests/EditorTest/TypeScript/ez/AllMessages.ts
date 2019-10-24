// AUTO-GENERATED FILE

import __Message = require("./Message")
export import Message = __Message.Message;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

import __Color = require("./Color")
export import Color = __Color.Color;

import __Time = require("./Time")
export import Time = __Time.Time;

import __Angle = require("./Angle")
export import Angle = __Angle.Angle;

export class EventMessage extends Message
{
  public static GetTypeNameHash(): number { return 758823069; }
  constructor() { super(); this.TypeNameHash = 758823069; }
}

export class MsgGenericEvent extends EventMessage
{
  public static GetTypeNameHash(): number { return 1061762376; }
  constructor() { super(); this.TypeNameHash = 1061762376; }
  Message: string;
}

export class MsgCollision extends Message
{
  public static GetTypeNameHash(): number { return 1454903456; }
  constructor() { super(); this.TypeNameHash = 1454903456; }
}

export class MsgDeleteGameObject extends Message
{
  public static GetTypeNameHash(): number { return 214557405; }
  constructor() { super(); this.TypeNameHash = 214557405; }
}

export class MsgComponentInternalTrigger extends Message
{
  public static GetTypeNameHash(): number { return 2729883851; }
  constructor() { super(); this.TypeNameHash = 2729883851; }
}

export class MsgUpdateLocalBounds extends Message
{
  public static GetTypeNameHash(): number { return 1393097690; }
  constructor() { super(); this.TypeNameHash = 1393097690; }
}

export class MsgSetPlaying extends Message
{
  public static GetTypeNameHash(): number { return 744214609; }
  constructor() { super(); this.TypeNameHash = 744214609; }
  Play: boolean = true;
}

export class MsgChildrenChanged extends Message
{
  public static GetTypeNameHash(): number { return 2784142053; }
  constructor() { super(); this.TypeNameHash = 2784142053; }
}

export class MsgComponentsChanged extends Message
{
  public static GetTypeNameHash(): number { return 1233680431; }
  constructor() { super(); this.TypeNameHash = 1233680431; }
}

export class MsgTransformChanged extends Message
{
  public static GetTypeNameHash(): number { return 769397284; }
  constructor() { super(); this.TypeNameHash = 769397284; }
}

export class MsgSetFloatParameter extends Message
{
  public static GetTypeNameHash(): number { return 2334834113; }
  constructor() { super(); this.TypeNameHash = 2334834113; }
  Name: string;
  Value: number = 0;
}

export class MsgExtractGeometry extends Message
{
  public static GetTypeNameHash(): number { return 4190830039; }
  constructor() { super(); this.TypeNameHash = 4190830039; }
}

export class MsgAnimationPoseUpdated extends Message
{
  public static GetTypeNameHash(): number { return 3874463418; }
  constructor() { super(); this.TypeNameHash = 3874463418; }
}

export class MsgSetMeshMaterial extends Message
{
  public static GetTypeNameHash(): number { return 2062037541; }
  constructor() { super(); this.TypeNameHash = 2062037541; }
  Material: string;
  MaterialSlot: number = 0;
}

export class MsgOnlyApplyToObject extends Message
{
  public static GetTypeNameHash(): number { return 1217048705; }
  constructor() { super(); this.TypeNameHash = 1217048705; }
}

export class MsgSetColor extends Message
{
  public static GetTypeNameHash(): number { return 12434892; }
  constructor() { super(); this.TypeNameHash = 12434892; }
  Color: Color = new Color(1, 1, 1, 1);
}

export class MsgExtractRenderData extends Message
{
  public static GetTypeNameHash(): number { return 4250635752; }
  constructor() { super(); this.TypeNameHash = 4250635752; }
}

export class MsgPropertyAnimationEndReached extends EventMessage
{
  public static GetTypeNameHash(): number { return 3741310044; }
  constructor() { super(); this.TypeNameHash = 3741310044; }
}

export class MsgPropertyAnimationPlayRange extends Message
{
  public static GetTypeNameHash(): number { return 4041140774; }
  constructor() { super(); this.TypeNameHash = 4041140774; }
  RangeLow: number = N/A;
  RangeHigh: number = N/A;
}

export class MsgInputActionTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 2990029836; }
  constructor() { super(); this.TypeNameHash = 2990029836; }
}

export class MsgPhysicsAddImpulse extends Message
{
  public static GetTypeNameHash(): number { return 2618981170; }
  constructor() { super(); this.TypeNameHash = 2618981170; }
}

export class MsgPhysicsAddForce extends Message
{
  public static GetTypeNameHash(): number { return 3475477528; }
  constructor() { super(); this.TypeNameHash = 3475477528; }
}

export class MsgBuildStaticMesh extends Message
{
  public static GetTypeNameHash(): number { return 3694682949; }
  constructor() { super(); this.TypeNameHash = 3694682949; }
}

export class MsgDamage extends Message
{
  public static GetTypeNameHash(): number { return 3889610425; }
  constructor() { super(); this.TypeNameHash = 3889610425; }
  Damage: number = 0;
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

export class MsgFmodRestartSound extends Message
{
  public static GetTypeNameHash(): number { return 3559542584; }
  constructor() { super(); this.TypeNameHash = 3559542584; }
  OneShot: boolean = true;
}

export class MsgFmodStopSound extends Message
{
  public static GetTypeNameHash(): number { return 990442021; }
  constructor() { super(); this.TypeNameHash = 990442021; }
  Immediate: boolean = false;
}

export class MsgFmodSoundFinished extends EventMessage
{
  public static GetTypeNameHash(): number { return 4085975113; }
  constructor() { super(); this.TypeNameHash = 4085975113; }
}

export class MsgFmodAddSoundCue extends Message
{
  public static GetTypeNameHash(): number { return 48382456; }
  constructor() { super(); this.TypeNameHash = 48382456; }
}

export class MsgBreakableSheetBroke extends EventMessage
{
  public static GetTypeNameHash(): number { return 2481203103; }
  constructor() { super(); this.TypeNameHash = 2481203103; }
}

export class MsgTriggerRaycastInteractionComponent extends Message
{
  public static GetTypeNameHash(): number { return 3760994013; }
  constructor() { super(); this.TypeNameHash = 3760994013; }
}

export class MsgPxTriggerTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 3270153082; }
  constructor() { super(); this.TypeNameHash = 3270153082; }
}

