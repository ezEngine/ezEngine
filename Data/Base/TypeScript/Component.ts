namespace ezLog
{
    export declare function Error(text : string) : void;
    export declare function SeriousWarning(text : string) : void;
    export declare function Warning(text : string) : void;
    export declare function Success(text : string) : void;
    export declare function Info(text : string) : void;
    export declare function Dev(text : string) : void;
    export declare function Debug(text : string) : void;
};

interface ezTsComponent {
    Update(): void;
    // GetOwner() : ezTsGameObject
    // Activate / Deactivate / SetActive
    // GetWorld
    // GetHandle
    // Initialize / Deinitialize / OnActivate / OnDeactivate / OnSimulationStarted  
}

class MyComponent implements ezTsComponent {
    constructor(name: string) {
        ezLog.Info("Construct MyComponent: " + name)
        this._name = name;
    }

    Update(): void
    {
        ezLog.Dev("MyComponent::Update: " + this._name)
    }

    private _name: string;
}

// called by the runtime
function _Create_MyComponent(name: string): MyComponent
{
    ezLog.Info("_Create_MyComponent: " + name)
    return new MyComponent(name);
}