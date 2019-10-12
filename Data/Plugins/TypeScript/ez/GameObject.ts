import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

import __Component = require("./Component")
export import Component = __Component.Component;

declare function __CPP_GameObject_IsValid(_this: GameObject) : boolean;
declare function __CPP_GameObject_SetLocalPosition(_this: GameObject, pos: Vec3) : void;
declare function __CPP_GameObject_GetLocalPosition(_this: GameObject) : Vec3;
declare function __CPP_GameObject_SetLocalRotation(_this: GameObject, rot: Quat) : void;
declare function __CPP_GameObject_GetLocalRotation(_this: GameObject) : Quat;
declare function __CPP_GameObject_SetActive(_this: GameObject, active: boolean) : void;
declare function __CPP_GameObject_FindChildByName(_this: GameObject, name: string, recursive: boolean) : GameObject;
declare function __CPP_GameObject_FindComponentByTypeName(_this: GameObject, typeName: string);
declare function __CPP_GameObject_FindComponentByTypeNameHash(_this: GameObject, nameHash: number);

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
    
    GetLocalPosition(): Vec3
    {
        return __CPP_GameObject_GetLocalPosition(this);
    }    
    
    SetLocalRotation(rot: Quat): void
    {
        __CPP_GameObject_SetLocalRotation(this, rot);
    }   

    GetLocalRotation(): Quat
    {
        return __CPP_GameObject_GetLocalRotation(this);
    }   

    SetActive(active: boolean): void
    {
        __CPP_GameObject_SetActive(this, active);
    }

    FindChildByName(name: string, recursive: boolean = true): GameObject
    {
        return __CPP_GameObject_FindChildByName(this, name, recursive);
    }

    FindComponentByTypeName<TYPE extends Component>(typeName: string): TYPE
    {
        return __CPP_GameObject_FindComponentByTypeName(this, typeName);
    }
    
    FindComponentByTypeNameHash<TYPE extends Component>(hash: number): TYPE
    {
        return __CPP_GameObject_FindComponentByTypeNameHash(this, hash);
    }
}

export function __TS_CreateGameObject() : GameObject
{
    return new GameObject;
}