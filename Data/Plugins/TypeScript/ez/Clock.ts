declare function __CPP_Clock_SetSpeed(speed: number): void;
declare function __CPP_Clock_GetSpeed(): number;
declare function __CPP_Clock_GetTimeDiff(): number;
declare function __CPP_Clock_GetAccumulatedTime(): number;

/**
 * Functions to work with the game clock.
 */
export namespace Clock {

    /**
     * Changes the speed at which the clock's time advances. Default is 1.0.
     * 
     * @param speed The speed of time. 1.0 for real time, smaller values for 'slow motion', higher values for 'fast forward'.
     */
    export function SetClockSpeed(speed: number): void {
        __CPP_Clock_SetSpeed(speed);
    }

    /**
     * Returns the current speed of the clock. See SetClockSpeed().
     */
    export function GetClockSpeed(): number {
        return __CPP_Clock_GetSpeed();
    }

    /**
     * Returns the elapsed time in seconds since the last game step.
     */
    export function GetTimeDiff(): number {
        return __CPP_Clock_GetTimeDiff();
    }

    /**
     * Returns the accumulated time in seconds since the game simulation started.
     */
    export function GetAccumulatedTime(): number {
        return __CPP_Clock_GetAccumulatedTime();
    }
}
