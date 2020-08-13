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
export import EventMessage = __Message.EventMessage;

import __Component = require("./Component")
export import Component = __Component.Component;
export import TypescriptComponent = __Component.TypescriptComponent;

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

declare function __CPP_GameObject_SetActiveFlag(_this: GameObject, active: boolean): void;
declare function __CPP_GameObject_GetActiveFlag(_this: GameObject): boolean;

declare function __CPP_GameObject_IsActive(_this: GameObject): boolean;

declare function __CPP_GameObject_SetName(_this: GameObject, value: string): void;
declare function __CPP_GameObject_GetName(_this: GameObject): string;
declare function __CPP_GameObject_SetGlobalKey(_this: GameObject, value: string): void;
declare function __CPP_GameObject_GetGlobalKey(_this: GameObject): string;

declare function __CPP_GameObject_SetTeamID(_this: GameObject, value: number): void;
declare function __CPP_GameObject_GetTeamID(_this: GameObject): number;

declare function __CPP_GameObject_FindChildByName(_this: GameObject, name: string, recursive: boolean): GameObject;
declare function __CPP_GameObject_FindChildByPath(_this: GameObject, path: string): GameObject;
declare function __CPP_GameObject_SearchForChildByNameSequence(_this: GameObject, objectSequence: string, componentNameHash: number): GameObject;

declare function __CPP_GameObject_TryGetComponentOfBaseTypeName(_this: GameObject, typeName: string): any;
declare function __CPP_GameObject_TryGetComponentOfBaseTypeNameHash(_this: GameObject, nameHash: number): any;
declare function __CPP_GameObject_TryGetScriptComponent(_this: GameObject, typeName: string): any;

declare function __CPP_GameObject_SendMessage(_this: GameObject, typeNameHash: number, msg: Message, recursive: boolean, expectMsgResult: boolean): void;
declare function __CPP_GameObject_PostMessage(_this: GameObject, typeNameHash: number, msg: Message, recursive: boolean, delay: number): void;

declare function __CPP_GameObject_SendEventMessage(_this: GameObject, typeNameHash: number, msg: EventMessage, sender: TypescriptComponent, expectMsgResult: boolean): void;
declare function __CPP_GameObject_PostEventMessage(_this: GameObject, typeNameHash: number, msg: EventMessage, sender: TypescriptComponent, delay: number): void;

declare function __CPP_GameObject_SetTags(_this: GameObject, ...tags: string[]): void;
declare function __CPP_GameObject_AddTags(_this: GameObject, ...tags: string[]): void;
declare function __CPP_GameObject_RemoveTags(_this: GameObject, ...tags: string[]): void;
declare function __CPP_GameObject_HasAnyTags(_this: GameObject, ...tags: string[]): boolean;
declare function __CPP_GameObject_HasAllTags(_this: GameObject, ...tags: string[]): boolean;

declare function __CPP_GameObject_GetParent(_this: GameObject): GameObject;
declare function __CPP_GameObject_SetParent(_this: GameObject, parent: GameObject, preserveGlobalTransform: boolean): void;
declare function __CPP_GameObject_AddChild(_this: GameObject, child: GameObject, preserveGlobalTransform: boolean): void;
declare function __CPP_GameObject_DetachChild(_this: GameObject, child: GameObject, preserveGlobalTransform: boolean): void;
declare function __CPP_GameObject_GetChildCount(_this: GameObject): number;
declare function __CPP_GameObject_GetChildren(__this: GameObject): GameObject[];

/**
 * Represents a C++ ezGameObject on the TypeScript side.
 * 
 * This class acts like a weak pointer to the C++ instance. In between game updates, the C++ side object may get deleted.
 * Therefore it is vital to call 'IsValid()' on a GameObject before doing anything else with it. If IsValid returns false,
 * the C++ side ezGameObject has been deleted and the TypeScript instance cannot be used anymore either.
 * 
 * Be aware that functions that return GameObjects will typically return null objects, in case of failure. They will not return
 * 'invalid' GameObject instances.
 */
export class GameObject {

    // TODO:
    // GetComponents
    // GetChildIterator

    /**
     * If the GameObject is not null, it may still be 'dead' on the C++ side. This function checks whether that is the case.
     * 
     * If the object is valid, all other functions can be called, otherwise it is an error to do anything with the GameObject.
     * GameObjects will always stay valid throughout a single game update (end of frame), so it is not necessary to call this
     * more than once per frame.
     */
    IsValid(): boolean { // [tested]
        return __CPP_GameObject_IsValid(this);
    }

    /**
     * Changes the name of the GameObject.
     */
    SetName(name: string): void { // [tested]
        __CPP_GameObject_SetName(this, name);
    }

    /**
     * Returns the name of the GameObject.
     */
    GetName(): string { // [tested]
        return __CPP_GameObject_GetName(this);
    }

    /**
     * Sets the active flag for a game object, which affects the object's final 'active state'.
     * When a GameObject does not have the active flag, it and all its children get deactivated, as well as all attached components.
     */
    SetActiveFlag(enabled: boolean): void { // [tested]
        __CPP_GameObject_SetActiveFlag(this, enabled);
    }

    /**
     * Returns whether the GameObject is has the active flag set.
     */
    GetActiveFlag(): boolean { // [tested]
        return __CPP_GameObject_GetActiveFlag(this);
    }

    /**
     * Returns whether the GameObject is active (it and all its parents have the active flag).
     */
    IsActive(): boolean { // [tested]
        return __CPP_GameObject_IsActive(this);
    }

    /**
     * Sets the position of the object relative to its parent object.
     * If the object has no parent, this is the same as the global position.
     * The global transform is updated at the end of the frame, so this change is not reflected in the global transform
     * until the next frame.
     */
    SetLocalPosition(pos: Vec3): void { // [tested]
        __CPP_GameObject_SetLocalPosition(this, pos);
    }

    /**
     * Returns the position relative to the parent object.
     * If the object has no parent, this is the same as the global position.
     */
    GetLocalPosition(): Vec3 { // [tested]
        return __CPP_GameObject_GetLocalPosition(this);
    }

    /**
     * Sets the rotation of the object relative to its parent object.
     * If the object has no parent, this is the same as the global rotation.
     * The global transform is updated at the end of the frame, so this change is not reflected in the global transform
     * until the next frame.
     */
    SetLocalRotation(rot: Quat): void { // [tested]
        __CPP_GameObject_SetLocalRotation(this, rot);
    }

    /**
     * Returns the rotation relative to the parent object.
     * If the object has no parent, this is the same as the global rotation.
     */
    GetLocalRotation(): Quat { // [tested]
        return __CPP_GameObject_GetLocalRotation(this);
    }

    /**
     * Sets the scaling of the object relative to its parent object.
     * If the object has no parent, this is the same as the global scale.
     * The global transform is updated at the end of the frame, so this change is not reflected in the global transform
     * until the next frame.
     */
    SetLocalScaling(scaling: Vec3): void { // [tested]
        __CPP_GameObject_SetLocalScaling(this, scaling);
    }

    /**
     * Returns the scaling relative to the parent object.
     * If the object has no parent, this is the same as the global scaling.
     */
    GetLocalScaling(): Vec3 { // [tested]
        return __CPP_GameObject_GetLocalScaling(this);
    }

    /**
     * Sets the uniform scaling of the object relative to its parent object.
     * If the object has no parent, this is the same as the global uniform scale.
     * The global transform is updated at the end of the frame, so this change is not reflected in the global transform
     * until the next frame.
     */
    SetLocalUniformScaling(scaling: number): void { // [tested]
        __CPP_GameObject_SetLocalUniformScaling(this, scaling);
    }

    /**
     * Returns the uniform scaling relative to the parent object.
     * If the object has no parent, this is the same as the global uniform scaling.
     */
    GetLocalUniformScaling(): number { // [tested]
        return __CPP_GameObject_GetLocalUniformScaling(this);
    }

    /**
     * Sets the object's global position.
     * Internally this will set the local position such that the desired global position is reached.
     */
    SetGlobalPosition(pos: Vec3): void { // [tested]
        __CPP_GameObject_SetGlobalPosition(this, pos);
    }

    /**
     * Returns the current global position as computed from the local transforms.
     */
    GetGlobalPosition(): Vec3 { // [tested]
        return __CPP_GameObject_GetGlobalPosition(this);
    }

    /**
     * Sets the object's global rotation.
     * Internally this will set the local rotation such that the desired global rotation is reached.
     */
    SetGlobalRotation(rot: Quat): void { // [tested]
        __CPP_GameObject_SetGlobalRotation(this, rot);
    }

    /**
     * Returns the current global rotation as computed from the local transforms.
     */
    GetGlobalRotation(): Quat { // [tested]
        return __CPP_GameObject_GetGlobalRotation(this);
    }

    /**
     * Sets the object's scaling position.
     * Internally this will set the local scaling such that the desired global scaling is reached.
     */
    SetGlobalScaling(scaling: Vec3): void { // [tested]
        __CPP_GameObject_SetGlobalScaling(this, scaling);
    }

    /**
     * Returns the current global scaling as computed from the local transforms.
     * Note that there is no global uniform scaling as the local uniform scaling and non-uniform scaling are
     * combined into the global scaling.
     */
    GetGlobalScaling(): Vec3 { // [tested]
        return __CPP_GameObject_GetGlobalScaling(this);
    }

    /**
     * Returns the vector representing the logical 'forward' direction of the GameObject in global space.
     */
    GetGlobalDirForwards(): Vec3 { // [tested]
        return __CPP_GameObject_GetGlobalDirForwards(this);
    }

    /**
     * Returns the vector representing the logical 'right' direction of the GameObject in global space.
     */
    GetGlobalDirRight(): Vec3 { // [tested]
        return __CPP_GameObject_GetGlobalDirRight(this);
    }

    /**
     * Returns the vector representing the logical 'up' direction of the GameObject in global space.
     */
    GetGlobalDirUp(): Vec3 { // [tested]
        return __CPP_GameObject_GetGlobalDirUp(this);
    }

    /**
     * Sets the velocity value of the GameObject.
     * The velocity value only has an effect in some scenarios. It does not affect the GameObject's position.
     * E.g. velocity is used by sound-sources to compute Doppler effects.
     * By default this value is computed out of position changes.
     */
    SetVelocity(velocity: Vec3): void { // [tested]
        __CPP_GameObject_SetVelocity(this, velocity);
    }

    /**
     * Returns the velocity of the object over the last two world updates.
     */
    GetVelocity(): Vec3 { // [tested]
        return __CPP_GameObject_GetVelocity(this);
    }

    /**
     * Sets the team ID of this GameObject.
     * 
     * The team ID can be used to identify to which team or player an object belongs to.
     * The team ID is inherited to other GameObjects, e.g. when spawning new objects.
     * Thus for instance a projectile inherits the team ID of the weapon or player that spawned it.
     * 
     * @param id The team ID must be in range [0; 65535] (uint16).
     */
    SetTeamID(id: number): void { // [tested]
        __CPP_GameObject_SetTeamID(this, id);
    }

    /**
     * Returns the object's team ID.
     */
    GetTeamID(): number { // [tested]
        return __CPP_GameObject_GetTeamID(this);
    }

    /**
     * Searches for a child GameObject with the given name.
     * 
     * @param name The expected exact name of the child object.
     * @param recursive If false, only direct children are inspected. Otherwise recursively all children are inspected.
     */
    FindChildByName(name: string, recursive: boolean = true): GameObject { // [tested]
        return __CPP_GameObject_FindChildByName(this, name, recursive);
    }

    /** 
     * Searches for a child using a path. Every path segment represents a child with a given name.
     * 
     * Paths are separated with single slashes: /
     * When an empty path is given, 'this' is returned.
     * When on any part of the path the next child cannot be found, null is returned.
     * This function expects an exact path to the destination. It does not search the full hierarchy for
     * the next child, as SearchChildByNameSequence() does.
     */
    FindChildByPath(path: string): GameObject { // [tested]
        return __CPP_GameObject_FindChildByPath(this, path);
    }

    /**
     * Searches for a child similar to FindChildByName() but allows to search for multiple names in a sequence.
     *
     * The names in the sequence are separated with slashes.
     * For example, calling this with "a/b" will first search the entire hierarchy below this object for a child
     * named "a". If that is found, the search continues from there for a child called "b".
     */
    SearchForChildByNameSequence(objectSequence: string): GameObject { // [tested]
        return __CPP_GameObject_SearchForChildByNameSequence(this, objectSequence, 0);
    }

    /**
     * Searches for a child similar to FindChildByName() but allows to search for multiple names in a sequence.
     *
     * The names in the sequence are separated with slashes.
     * For example, calling this with "a/b" will first search the entire hierarchy below this object for a child
     * named "a". If that is found, the search continues from there for a child called "b".
     * If such a child is found it is verified that the object contains a component of 'typeClass'. 
     * If it doesn't the search continues (including back-tracking).
     */
    SearchForChildWithComponentByNameSequence<TYPE extends Component>(objectSequence: string, typeClass: new () => TYPE): GameObject { // [tested]
        return __CPP_GameObject_SearchForChildByNameSequence(this, objectSequence, typeClass.GetTypeNameHash());
    }

    /**
     * Tries to find a component of type 'typeName' in the object's components list and returns the first match.
     * 
     * This function only works for C++ components, not for script components (derived from TypescriptComponent).
     * Use TryGetScriptComponent() for such use cases instead.
     */
    TryGetComponentOfBaseTypeName<TYPE extends Component>(typeName: string): TYPE { // [tested]
        return __CPP_GameObject_TryGetComponentOfBaseTypeName(this, typeName);
    }

    /**
     * Tries to find a component of type 'typeClass' in the object's components list and returns the first match.
     * 
     * This function only works for C++ components, not for script components (derived from TypescriptComponent).
     * Use TryGetScriptComponent() for such use cases instead.
     */
    TryGetComponentOfBaseType<TYPE extends Component>(typeClass: new () => TYPE): TYPE { // [tested]
        return __CPP_GameObject_TryGetComponentOfBaseTypeNameHash(this, typeClass.GetTypeNameHash());
    }

    /**
     * Similar to TryGetComponentOfBaseType() but tries to find a component type written in TypeScript.
     * 
     * See also ez.Utils.FindPrefabRootScript() for the common use case to finding the script of a prefab instance.
     * 
     * @param typeName The name of the TypeScript component to find.
     */
    TryGetScriptComponent<TYPE extends TypescriptComponent>(typeName: string): TYPE { // [tested]
        return __CPP_GameObject_TryGetScriptComponent(this, typeName);
    }

    /**
     * Sends a message to all the components on this GameObject (but not its children).
     * 
     * The message is delivered immediately.
     * 
     * @param expectResultData If set to true, the calling code assumes that the message receiver(s) may write result data
     *   back into the message and thus the caller is interested in reading that data afterwards. If set to false
     *   (the default) the state of the message is not synchronized back into the TypeScript message after the message
     *   has been delivered and thus any data written into the message by the receiver, is lost.
     */
    SendMessage(msg: Message, expectResultData: boolean = false): void { // [tested]
        __CPP_GameObject_SendMessage(this, msg.TypeNameHash, msg, false, expectResultData);
    }

    /**
     * Sends a message to all the components on this GameObject (including all its children).
     * 
     * The message is delivered immediately.
     * 
     * @param expectResultData If set to true, the calling code assumes that the message receiver(s) may write result data
     *   back into the message and thus the caller is interested in reading that data afterwards. If set to false
     *   (the default) the state of the message is not synchronized back into the TypeScript message after the message
     *   has been delivered and thus any data written into the message by the receiver, is lost.
     */
    SendMessageRecursive(msg: Message, expectResultData: boolean = false): void { // [tested]
        __CPP_GameObject_SendMessage(this, msg.TypeNameHash, msg, true, expectResultData);
    }

    /**
     * Queues a message to be sent to all the components on this GameObject (but not its children).
     * 
     * The message is delivered after the timeout.
     * If the timeout is zero, the message is delivered in the next frame.
     */
    PostMessage(msg: Message, delay: number = Time.Zero()): void { // [tested]
        __CPP_GameObject_PostMessage(this, msg.TypeNameHash, msg, false, delay);
    }

    /**
     * Queues a message to be sent to all the components on this GameObject (including all its children).
     * 
     * The message is delivered after the timeout.
     * If the timeout is zero, the message is delivered in the next frame.
     */
    PostMessageRecursive(msg: Message, delay: number = Time.Zero()): void { // [tested]
        __CPP_GameObject_PostMessage(this, msg.TypeNameHash, msg, true, delay);
    }

    /**
     * Sends an *event message* up the object hierarchy to the closest event handler (typically another script).
     * 
     * The message is delivered immediately.
     * 
     * @param expectResultData If set to true, the calling code assumes that the message receiver(s) may write result data
     *   back into the message and thus the caller is interested in reading that data afterwards. If set to false
     *   (the default) the state of the message is not synchronized back into the TypeScript message after the message
     *   has been delivered and thus any data written into the message by the receiver, is lost.
     */
    SendEventMessage(msg: EventMessage, sender: TypescriptComponent, expectMsgResult: boolean = false): void {
        __CPP_GameObject_SendEventMessage(this, msg.TypeNameHash, msg, sender, expectMsgResult);
    }

    /**
     * Queues an *event message* to be sent up the object hierarchy to the closest event handler (typically another script).
     * 
     * The message is delivered after the timeout.
     * If the timeout is zero, the message is delivered in the next frame.
     */
    PostEventMessage(msg: EventMessage, sender: TypescriptComponent, delay: number = Time.Zero()): void {
        __CPP_GameObject_PostEventMessage(this, msg.TypeNameHash, msg, sender, delay);
    }

    /**
     * Replaces all the tags on this GameObject with the given set of tags.
     */
    SetTags(...tags: string[]): void { // [tested]
        __CPP_GameObject_SetTags(this, ...tags);
    }

    /**
     * Adds all the given tags to this GameObject.
     */
    AddTags(...tags: string[]): void { // [tested]
        __CPP_GameObject_AddTags(this, ...tags);
    }

    /**
     * Removes all the given tags from this GameObject.
     * Ignores tags that were not set on this GameObject to begin with.
     */
    RemoveTags(...tags: string[]): void { // [tested]
        __CPP_GameObject_RemoveTags(this, ...tags);
    }

    /**
     * Checks whether this object has at least on of the given tags.
     */
    HasAnyTags(...tags: string[]): boolean { // [tested]
        return __CPP_GameObject_HasAnyTags(this, ...tags);
    }

    /**
     * Checks whether this object has all the given tags.
     */
    HasAllTags(...tags: string[]): boolean { // [tested]
        return __CPP_GameObject_HasAllTags(this, ...tags);
    }

    /**
     * Changes the global key of the GameObject.
     */
    SetGlobalKey(name: string): void { // [tested]
        __CPP_GameObject_SetGlobalKey(this, name);
    }

    /**
     * Returns the global key of the GameObject.
     */
    GetGlobalKey(): string { // [tested]
        return __CPP_GameObject_GetGlobalKey(this);
    }

    /**
     * Returns the parent game object or null if this object has no parent.
     */
    GetParent(): GameObject { // [tested]
        return __CPP_GameObject_GetParent(this);
    }

    /**
     * Attaches this object to another game object as its child.
     * 
     * @param parent The object to attach this object to.
     * @param preserveGlobalTransform If true, the global transform of this is preserved and the local transform is adjusted as needed.
     *  If false, the local transform is preserved and the global transform is computed accordingly.
     */
    SetParent(parent: GameObject, preserveGlobalTransform: boolean = true): void { // [tested]
        __CPP_GameObject_SetParent(this, parent, preserveGlobalTransform);
    }

    /**
     * Attaches the given object to this object as a child.
     * 
     * @param child The object to attach to this object.
     * @param preserveGlobalTransform If true, the global transform of the child is preserved and the local transform is adjusted as needed.
     *  If false, the local transform is preserved and the global transform is computed accordingly.
     */
    AddChild(child: GameObject, preserveGlobalTransform: boolean = true): void { // [tested]
        __CPP_GameObject_AddChild(this, child, preserveGlobalTransform);
    }

    /**
     * Detaches the given child object from this object.
     * This is similar to child.SetParent(null), but only detaches the child, if it is indeed attached to this object.
     * 
     * @param child The object to detach from this object.
     * @param preserveGlobalTransform If true, the global transform of the child is preserved and the local transform is adjusted as needed.
     *  If false, the local transform is preserved and the global transform is computed accordingly.
     */
    DetachChild(child: GameObject, preserveGlobalTransform: boolean = true): void { // [tested]
        __CPP_GameObject_DetachChild(this, child, preserveGlobalTransform);
    }

    /**
     * Returns the number of objects attached to this as children.
     */
    GetChildCount(): number { // [tested]
        return __CPP_GameObject_GetChildCount(this);
    }

    /**
     * Returns all the child objects in an array.
     */
    GetChildren(): GameObject[] { // [tested]
        return __CPP_GameObject_GetChildren(this);
    }
}