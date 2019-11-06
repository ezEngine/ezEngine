export class Angle {
    static DegreeToRadian(degree: number): number {
        return degree * Math.PI / 180.0;
    }

    static RadianToDegree(radian: number): number {
        return radian * 180.0 / Math.PI;
    }

    static AngleBetween(radianA: number, radianB: number) {
        return Math.PI - Math.abs(Math.abs(radianA - radianB) - Math.PI);
    }

    static IsEqualSimple(radianLhs: number, radianRhs: number, epsilon: number): boolean {
        const diff = Angle.AngleBetween(radianLhs, radianRhs);

        return (diff >= -epsilon) && (diff <= epsilon);
    }
}
