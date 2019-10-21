import ez = require("<PATH-TO-EZ-TS>")

class NewComponent extends ez.TypescriptComponent {
    constructor() {
        super()
    }

    Update(): void {
        ez.Log.Info("NewComponent.Update()")
    }
}

