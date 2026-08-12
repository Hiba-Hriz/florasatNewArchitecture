package flora.constellation;

simple ConstellationCreator
{
    parameters:
        @class(constellation::ConstellationCreator);
        int planeCount;
        int interPlaneSpacing; // aswell required for TopologyControl
        int raanSpread; // 180 for Star, 360 for Delta
        double inclination = default(86.4);
        int altitude = default(780);
        int baseYear = default(20);
        double baseDay = default(0.8292315);
        int epochYear = default(20);
        double epochDay = default(0.8194444);
        double eccentricity = default(0.000001);
}