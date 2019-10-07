import __Vec3 = require("./Vec3")
import __Quat = require("./Quat")

export import Vec3 = __Vec3.Vec3;
export import Quat = __Quat.Quat;

declare function __CPP_GameObject_IsValid(_this: GameObject) : boolean;
declare function __CPP_GameObject_SetLocalPosition(_this: GameObject, pos: Vec3) : void;
declare function __CPP_GameObject_SetLocalRotation(_this: GameObject, rot: Quat) : void;
declare function __CPP_GameObject_SetActive(_this: GameObject, active: boolean) : void;
declare function __CPP_GameObject_FindChildByName(_this: GameObject, name: string, recursive: boolean) : GameObject;

export class GameObject
{
    // GetLocalPosition
    // rotation, scaling
    // Activate
    // Deactivate
    // IsActive
    // GetName
    // SetName
    // FindChildByName
    // FindChildByPath
    // SearchForChildByNameSequence
    // SearchForChildrenByNameSequence
    // SetLocalPosition / GetLocalPosition
    // SetLocalRotation / GetLocalRotation
    // SetLocalScaling / GetLocalScaling
    // SetLocalUniformScaling / GetLocalUniformScaling
    // SetGlobalPosition / GetGlobalPosition
    // SetGlobalRotation / GetGlobalRotation
    // SetGlobalScaling / GetGlobalScaling
    // SetGlobalTransform / GetGlobalTransform
    // GetGlobalDirForwards
    // GetGlobalDirRight
    // GetGlobalDirUp
    // GetVelocity
    // TryGetComponentOfBaseType
    // GetComponents
    // SendMessage
    // SendMessageRecursive
    // PostMessage
    // PostMessageRecursive
    // GetTags
    // GetTeamID

     IsValid() : boolean
     {
        return __CPP_GameObject_IsValid(this);
     }

    SetLocalPosition(pos: Vec3): void
    {
        __CPP_GameObject_SetLocalPosition(this, pos);
    }    
    
    SetLocalRotation(rot: Quat): void
    {
        __CPP_GameObject_SetLocalRotation(this, rot);
    }   

    SetActive(active: boolean): void
    {
        __CPP_GameObject_SetActive(this, active);
    }

    FindChildByName(name: string, recursive: boolean = true): GameObject
    {
        return __CPP_GameObject_FindChildByName(this, name, recursive);
    }
}

export function __TS_CreateGameObject() : GameObject
{
    return new GameObject;
}