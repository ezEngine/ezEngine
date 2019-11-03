import ez = require("<PATH-TO-EZ-TS>")

export class NewComponent extends ez.TypescriptComponent {
    constructor() {
        super()
    }

    Update(): void {
        ez.Log.Info("NewComponent.Update()")
    }
}

