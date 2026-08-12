package flora.routing.dtn.com;

simple Com
{
    parameters:
        @class(routing::Com);
        @display("i=block/wtx");
        int numGroundLinks = default(40);
        double packetLoss = default(0.0);
                      
    gates:
        output dtnTransportOut;
        input dtnTransportIn;
        inout satelliteLink[numGroundLinks] @labels(satelliteLink);
}
