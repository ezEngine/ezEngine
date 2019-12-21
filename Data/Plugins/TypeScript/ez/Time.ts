declare function __CPP_Time_Now(): number;

/**
 * Utility functions to work with time values.
 * Time is genreally measured in seconds, this class provides functions to convert between different units.
 */
export class Time {

    /**
     * Returns the current time.
     */
    static Now(): number {
        return __CPP_Time_Now();
    }

    /**
     * Converts nanoseconds to seconds
     */
    static Nanoseconds(fNanoseconds: number): number {
        return fNanoseconds * 0.000000001;
    }

    /**
     * Converts microseconds to seconds
     */
    static Microseconds(fMicroseconds: number): number {
        return fMicroseconds * 0.000001;
    }

    /**
     * Converts milliseconds to seconds
     */
    static Milliseconds(fMilliseconds: number): number {
        return fMilliseconds * 0.001;
    }

    /**
     * Converts seconds to seconds
     */
    static Seconds(fSeconds: number): number {
        return fSeconds;
    }

    /**
     * Converts minutes to seconds
     */
    static Minutes(fMinutes: number): number {
        return fMinutes * 60;
    }

    /**
     * Converts hours to seconds
     */
    static Hours(fHours: number): number {
        return fHours * 60 * 60;
    }

    /**
     * Returns a zero time value
     */
    static Zero(): number {
        return 0;
    }

    /**
     * Converts seconds to nanoseconds
     */
    static GetNanoseconds(Time: number): number {
        return Time * 1000000000.0;
    }

    /**
     * Converts seconds to microseconds
     */
    static GetMicroseconds(Time: number): number {
        return Time * 1000000.0;
    }

    /**
     * Converts seconds to milliseconds
     */
    static GetMilliseconds(Time: number): number {
        return Time * 1000.0;
    }

    /**
     * Converts seconds to seconds
     */
    static GetSeconds(Time: number): number {
        return Time;
    }

    /**
     * Converts seconds to minutes
     */
    static GetMinutes(Time: number): number {
        return Time / 60.0;
    }

    /**
     * Converts seconds to hours
     */
    static GetHours(Time: number): number {
        return Time / (60.0 * 60.0);
    }
}
