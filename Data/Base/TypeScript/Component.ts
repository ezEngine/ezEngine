import ez = require("./ez")

class MyComponent extends ez.Component 
{
    constructor(name: string) 
    {
        super()
        ez.Log.Info("Construct MyComponent: " + name)
        this._name = name;
    }

    Update(): void
    {
        ez.Log.Dev("MyComponent::Update: " + this._name)

        var go = this.GetOwner();

        if (go != null)
        {
            ez.Log.Dev("Setting Local Position")

            go.SetLocalPosition(0, 0, Math.random());
        }
    }

    private _name: string;
}

// called by the runtime
function __Ts_Create_MyComponent(name: string): MyComponent
{
    ez.Log.Info("_Create_MyComponent: " + name)
    return new MyComponent(name);
}