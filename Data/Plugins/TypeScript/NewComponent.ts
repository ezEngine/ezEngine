import ez = require("<PATH-TO-EZ-TS>")

export class NewComponent extends ez.TypescriptComponent {
    constructor() {
        super()
    }

    Update(): void {
        ez.Log.Info("NewComponent.Update()")
    }

    /*
    Initialize(): void {
    }

    Deinitialize(): void {
    }

    OnActivated(): void {
    }

    OnDeactivated(): void {
    }

    OnSimulationStarted(): void {
    }
    */
}

