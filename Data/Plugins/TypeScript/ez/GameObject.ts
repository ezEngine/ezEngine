import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

import __Angle = require("./Angle")
export import Angle = __Angle.Angle;

import __Time = require("./Time")
export import Time = __Time.Time;

import __Message = require("./Message")
export import Message = __Message.Message;

import __Component = require("./Component")
export import Component = __Component.Component;

declare function __CPP_GameObject_IsValid(_this: GameObject): boolean;

declare function __CPP_GameObject_SetLocalPosition(_this: GameObject, pos: Vec3): void;
declare function __CPP_GameObject_GetLocalPosition(_this: GameObject): Vec3;
declare function __CPP_GameObject_SetLocalScaling(_this: GameObject, scale: Vec3): void;
declare function __CPP_GameObject_GetLocalScaling(_this: GameObject): Vec3;
declare function __CPP_GameObject_SetLocalUniformScaling(_this: GameObject, scale: number): void;
declare function __CPP_GameObject_GetLocalUniformScaling(_this: GameObject): number;
declare function __CPP_GameObject_SetLocalRotation(_this: GameObject, rot: Quat): void;
declare function __CPP_GameObject_GetLocalRotation(_this: GameObject): Quat;

declare function __CPP_GameObject_SetGlobalPosition(_this: GameObject, pos: Vec3): void;
declare function __CPP_GameObject_GetGlobalPosition(_this: GameObject): Vec3;
declare function __CPP_GameObject_SetGlobalScaling(_this: GameObject, scale: Vec3): void;
declare function __CPP_GameObject_GetGlobalScaling(_this: GameObject): Vec3;
declare function __CPP_GameObject_SetGlobalRotation(_this: GameObject, rot: Quat): void;
declare function __CPP_GameObject_GetGlobalRotation(_this: GameObject): Quat;

declare function __CPP_GameObject_GetGlobalDirForwards(_this: GameObject): Vec3;
declare function __CPP_GameObject_GetGlobalDirRight(_this: GameObject): Vec3;
declare function __CPP_GameObject_GetGlobalDirUp(_this: GameObject): Vec3;

declare function __CPP_GameObject_SetVelocity(_this: GameObject, value: Vec3): void;
declare function __CPP_GameObject_GetVelocity(_this: GameObject): Vec3;

declare function __CPP_GameObject_SetActive(_this: GameObject, active: boolean): void;
declare function __CPP_GameObject_IsActive(_this: GameObject): boolean;

declare function __CPP_GameObject_SetName(_this: GameObject, value: string): void;
declare function __CPP_GameObject_GetName(_this: GameObject): string;

declare function __CPP_GameObject_SetTeamID(_this: GameObject, value: number): void;
declare function __CPP_GameObject_GetTeamID(_this: GameObject): number;

declare function __CPP_GameObject_FindChildByName(_this: GameObject, name: string, recursive: boolean): GameObject;
declare function __CPP_GameObject_FindChildByPath(_this: GameObject, path: string): GameObject;
declare function __CPP_GameObject_SearchForChildByNameSequence(_this: GameObject, objectSequence: string, componentNameHash: number): GameObject;

declare function __CPP_GameObject_TryGetComponentOfBaseTypeName(_this: GameObject, typeName: string);
declare function __CPP_GameObject_TryGetComponentOfBaseTypeNameHash(_this: GameObject, nameHash: number);

declare function __CPP_GameObject_SendMessage(_this: GameObject, typeNameHash: number, msg: Message, recursive: boolean): void;
declare function __CPP_GameObject_PostMessage(_this: GameObject, typeNameHash: number, msg: Message, recursive: boolean, delay: number): void;

declare function __CPP_GameObject_SetTags(_this: GameObject, ...tags:string[]): void;
declare function __CPP_GameObject_AddTags(_this: GameObject, ...tags:string[]): void;
declare function __CPP_GameObject_RemoveTags(_this: GameObject, ...tags:string[]): void;
declare function __CPP_GameObject_HasAnyTags(_this: GameObject, ...tags:string[]): boolean;
declare function __CPP_GameObject_HasAllTags(_this: GameObject, ...tags:string[]): boolean;

export class GameObject {

    // TODO:
    // SetGlobalTransform / GetGlobalTransform
    // GetComponents
    // GetTags

    IsValid(): boolean {
        return __CPP_GameObject_IsValid(this);
    }

    SetName(name: string): void {
        __CPP_GameObject_SetName(this, name);
    }

    GetName(): string {
        return __CPP_GameObject_GetName(this);
    }

    SetActive(active: boolean): void {
        __CPP_GameObject_SetActive(this, active);
    }

    IsActive(): boolean {
        return __CPP_GameObject_IsActive(this);
    }

    SetLocalPosition(pos: Vec3): void {
        __CPP_GameObject_SetLocalPosition(this, pos);
    }

    GetLocalPosition(): Vec3 {
        return __CPP_GameObject_GetLocalPosition(this);
    }

    SetLocalRotation(rot: Quat): void {
        __CPP_GameObject_SetLocalRotation(this, rot);
    }

    GetLocalRotation(): Quat {
        return __CPP_GameObject_GetLocalRotation(this);
    }

    SetLocalScaling(scaling: Vec3): void {
        __CPP_GameObject_SetLocalScaling(this, scaling);
    }

    GetLocalScaling(): Vec3 {
        return __CPP_GameObject_GetLocalScaling(this);
    }

    SetLocalUniformScaling(scaling: number): void {
        __CPP_GameObject_SetLocalUniformScaling(this, scaling);
    }

    GetLocalUniformScaling(): number {
        return __CPP_GameObject_GetLocalUniformScaling(this);
    }

    SetGlobalPosition(pos: Vec3): void {
        __CPP_GameObject_SetGlobalPosition(this, pos);
    }

    GetGlobalPosition(): Vec3 {
        return __CPP_GameObject_GetGlobalPosition(this);
    }

    SetGlobalRotation(rot: Quat): void {
        __CPP_GameObject_SetGlobalRotation(this, rot);
    }

    GetGlobalRotation(): Quat {
        return __CPP_GameObject_GetGlobalRotation(this);
    }

    SetGlobalScaling(scaling: Vec3): void {
        __CPP_GameObject_SetGlobalScaling(this, scaling);
    }

    GetGlobalScaling(): Vec3 {
        return __CPP_GameObject_GetGlobalScaling(this);
    }

    GetGlobalDirForwards(): Vec3 {
        return __CPP_GameObject_GetGlobalDirForwards(this);
    }

    GetGlobalDirRight(): Vec3 {
        return __CPP_GameObject_GetGlobalDirRight(this);
    }

    GetGlobalDirUp(): Vec3 {
        return __CPP_GameObject_GetGlobalDirUp(this);
    }

    SetVelocity(velocity: Vec3): void {
        __CPP_GameObject_SetVelocity(this, velocity);
    }

    GetVelocity(): Vec3 {
        return __CPP_GameObject_GetVelocity(this);
    }

    SetTeamID(id: number): void {
        __CPP_GameObject_SetTeamID(this, id);
    }

    GetTeamID(): number {
        return __CPP_GameObject_GetTeamID(this);
    }

    FindChildByName(name: string, recursive: boolean = true): GameObject {
        return __CPP_GameObject_FindChildByName(this, name, recursive);
    }

    FindChildByPath(path: string): GameObject {
        return __CPP_GameObject_FindChildByPath(this, path);
    }

    SearchForChildByNameSequence(objectSequence: string): GameObject {
        return __CPP_GameObject_SearchForChildByNameSequence(this, objectSequence, 0);
    }

    SearchForChildWithComponentByNameSequence<TYPE extends Component>(objectSequence: string, typeClass: new () => TYPE): GameObject {
        return __CPP_GameObject_SearchForChildByNameSequence(this, objectSequence, typeClass.GetTypeNameHash());
    }

    TryGetComponentOfBaseTypeName<TYPE extends Component>(typeName: string): TYPE {
        return __CPP_GameObject_TryGetComponentOfBaseTypeName(this, typeName);
    }

    TryGetComponentOfBaseType<TYPE extends Component>(typeClass: new () => TYPE): TYPE {
        return __CPP_GameObject_TryGetComponentOfBaseTypeNameHash(this, typeClass.GetTypeNameHash());
    }

    SendMessage(msg: Message): void {
        __CPP_GameObject_SendMessage(this, msg.TypeNameHash, msg, false);
    }

    SendMessageRecursive(msg: Message): void {
        __CPP_GameObject_SendMessage(this, msg.TypeNameHash, msg, true);
    }

    PostMessage(msg: Message, delay: number = Time.Zero()): void {
        __CPP_GameObject_PostMessage(this, msg.TypeNameHash, msg, false, delay);
    }

    PostMessageRecursive(msg: Message, delay: number = Time.Zero()): void {
        __CPP_GameObject_PostMessage(this, msg.TypeNameHash, msg, true, delay);
    }

    SetTags(...tags:string[]): void {
        __CPP_GameObject_SetTags(this, ...tags);
    }

    AddTags(...tags:string[]): void {
        __CPP_GameObject_AddTags(this, ...tags);
    }
    
    RemoveTags(...tags:string[]): void {
        __CPP_GameObject_RemoveTags(this, ...tags);
    }

    HasAnyTags(...tags:string[]): boolean {
        return __CPP_GameObject_HasAnyTags(this, ...tags);
    }

    HasAllTags(...tags:string[]): boolean {
        return __CPP_GameObject_HasAllTags(this, ...tags);
    }
}