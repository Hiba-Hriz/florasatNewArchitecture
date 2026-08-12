#ifndef __LORAMAC_H
#define __LORAMAC_H

#include "inet/physicallayer/wireless/common/contract/packetlevel/IRadio.h"
#include "inet/linklayer/contract/IMacProtocol.h"
#include "inet/linklayer/base/MacProtocolBase.h"
#include "inet/common/FSMA.h"
#include "inet/queueing/contract/IPacketQueue.h"
#include "LoRaMacControlInfo_m.h"
#include "LoRaMacFrame_m.h"
#include "inet/common/Protocol.h"
#include <list>

#include "LoRaRadio.h"

namespace flora {

/**
 * Based on CSMA class
 */

class LoRaMac : public MacProtocolBase
{
  private:
    // Signal
    simsignal_t macDPAD;
    simsignal_t macDPADOwnTraffic;


  protected:
    /**
     * @name Configuration parameters
     */
    //@{
    MacAddress address;
    omnetpp::SimTime bdw;
    // Duty cycle
    bool waitingForDC = false;
    cMessage *dutyCycleTimer = nullptr;

    int payloadBytes;

    omnetpp::SimTime DPAD;

    double bitrate = NaN;
    int headerLength = -1;
    int ackLength = -1;

    bool withRetransmission;
    bool usingAck;
    int maxRetransmissions;


    bool useSlottedAloha = false;

    void scheduleNextClassSslot();

    void scheduleULslotsSlottedAloha();


    int retransmissionCount;
    int numRetransmissions;
    bool retransmissionPending;  // true si une retransmission est deja schedulee
    cMessage *retransmissionTimer;  // timer dedie aux retransmissions

    inet::Hz lastUplinkCF = inet::Hz(868.1e6); // valeur par defaut
    simtime_t ackTimeout = -1;
    simtime_t waitDelay1Time = -1;
    simtime_t listening1Time = -1;
    simtime_t waitDelay2Time = -1;
    simtime_t listening2Time = -1;

    int retryLimit = -1;
    int cwMin = -1;
    int cwMax = -1;
    int cwMulticast = -1;
    int sequenceNumber = 0;
    bool immediateSlotScheduled = false;

    bool isClassA = true;
    bool isClassB = false;
    bool isClassS = false;
    bool iGotBeacon = false;
    bool beaconGuard = false;

    simtime_t beaconStart = 1;
    simtime_t beaconGuardTime = -1;
    simtime_t beaconReservedTime = -1;
    simtime_t beaconPeriodTime = -1;

    simtime_t timeToNextSlot = -1;
    simtime_t classBslotTime = -1;
    int pingOffset = -1;

    simtime_t maxToA = -1;
    simtime_t clockThreshold = -1;
    simtime_t classSslotTime = -1;
    int targetClassSslot = 1;
    int maxClassSslots = 1;

    cOutVector slotSelectionData;


    //cOutVector slotBeginTimes;

    // FSA Game parameters
    double a = 1;
    int realNodeNumber = 0;
    bool FSAGame = false;


    int loRaSF;
    Hz loRaBW;
    Hz loRaCF;
    void startDIFS();
    void startBackoff();
    void handleBackoffSlotExpiry();
    void scheduleNextBackoffSlot();
    void handleDIFSExpiry();
    bool performCAD();
    double changeChannel();
    std::vector<double> availableChannels;
    int numAvailableChannels;


    // Parametres CSMA/CAD
    bool useCSMA;
    int difsCount;              // Nombre de CADs pour DIFS (2)
    int backoffMax;             // Limite max de backoff (6)
    int numBackoff;             // Nombre actuel de slots de backoff
    int maxChanges;             // Limite de changements de canal max (6)
    int channelChanges;         //Changements de canal effectues
    int cadAttempts;            // Compteur de tentatives CAD
    int lostDueToVisibility;

    // ===== Energy accounting =====
    double supplyVoltage;

    double cadCurrent;
    double idleCurrent;

    double totalCadEnergy;
    double totalBackoffEnergy;

    double totalCadTime;
    double totalBackoffTime;

    int totalCadOperations;


    int cadSymbolNum;           // Nombre de symboles par CAD (2 par defaut): Le CAD analyse un certain nombre de symboles pour determiner si le canal est occupe.
    // Messages et etats CSMA
        cMessage *cadTimer; // Timer pour chaque CAD
        cMessage *backoffSlotTimer;       // Timer pour chaque slot de backoff

    std::list<double> estimations{7.424162031962352,
        17.67163037584245,
        28.6461281367357,
        39.72117849956681,
        51.40789858652185,
        62.767063055130954,
        73.5546355419076,
        85.18146776403111,
        96.73695714142808,
        107.37868043862703,
        119.33875575508631,
        129.71839072230995,
        140.9577463078945,
        151.2212421017844,
        161.72302113166765,
        171.7055895594676,
        182.08056890613832,
        192.43965964373857,
        201.36466465116501,
        210.93100516205615,
        221.52202574453852,
        231.77170094921988,
        243.30995940646793,
        254.68129319480485,
        262.9530967120655,
        272.1272809410178,
        283.98972018804227,
        292.7617151654865,
        301.2892713253599,
        316.11740104839333,
        319.09792392966216,
        325.62319802065707,
        352.0784712882294,
        362.2506178114338,
        353.94743100030354,
        370.8011376765634,
        388.10734169281926,
        399.618853983959,
        409.9197253787451,
        417.18363471398453,
        425.25500775973126,
        432.863832059581,
        452.5304454285594,
        459.56996936405136,
        488.10091674236116,
        469.8990859641252,
        488.10091674236116,
        497.9191014970447,
        498.18072583991125,
        499.2289057782406,
        512.2901144027647};

    //@}


    /**
     * @name CsmaCaMac state variables
     * Various state information checked and modified according to the state machine.
     */
    //@{
    enum State {
        IDLE,
        WAIT_DIFS,
        TRANSMIT,
        BEACON_RECEPTION,
        WAIT_DELAY,
        PING_SLOT,
        RECEIVING_BEACON,
        RECEIVING,
        WAIT_DELAY_1,
        LISTENING_1,
        RECEIVING_1,
        WAIT_DELAY_2,
        LISTENING_2,
        RECEIVING_2,
        UPLINK_SLOT,
    };

    IRadio *radio = nullptr;
    IRadio::TransmissionState transmissionState = IRadio::TRANSMISSION_STATE_UNDEFINED;
    IRadio::ReceptionState receptionState = IRadio::RECEPTION_STATE_UNDEFINED;

    cFSM fsm;

    /** Remaining backoff period in seconds */
    simtime_t backoffPeriod = -1;


    /** Number of frame retransmission attempts. */
    int retryCounter = -1;

    /** Messages received from upper layer and to be transmitted later */
    cPacketQueue transmissionQueue;

    /** Passive queue module to request messages from */
    cPacketQueue *queueModule = nullptr;
    //@}

    /** @name Timer messages */
    //@{
    /** End of the Short Inter-Frame Time period */
    cMessage *endSifs = nullptr;

    /** End of the Data Inter-Frame Time period */
    cMessage *endDifs = nullptr;

    /** End of the backoff period */
    cMessage *endBackoff = nullptr;

    /** End of the ack timeout */
    cMessage *endAckTimeout = nullptr;

    /** Timeout after the transmission of a Data frame */
    cMessage *endTransmission = nullptr;

    /** Timeout after the reception of a Data frame */
    cMessage *endReception = nullptr;

    /** Timeout after the reception of a Data frame */
    cMessage *droppedPacket = nullptr;

    /** End of the Delay_1 */
    cMessage *endDelay_1 = nullptr;

    /** End of the Listening_1 */
    cMessage *endListening_1 = nullptr;

    /** End of the Delay_2 */
    cMessage *endDelay_2 = nullptr;

    /** End of the Listening_2 */
    cMessage *endListening_2 = nullptr;

    /** Radio state change self message. Currently this is optimized away and sent directly */
    cMessage *mediumStateChange = nullptr;

    /** End of the ping slot period. Start new downlink listening slot */
    cMessage *pingPeriod = nullptr;

    /** End of downlink listening slot */
    cMessage *endPingSlot = nullptr;

    /** End of the beacon period. Start new beacon listening slot */
    cMessage *beaconPeriod = nullptr;

    /** End of the beacon listening slot */
    cMessage *beaconReservedEnd = nullptr;

    /** Start of the beacon guard period */
    cMessage *beaconGuardStart = nullptr;

    /** End of the beacon guard period */
    cMessage *beaconGuardEnd = nullptr;

    /** Start of uplink transmission slot */
    cMessage *beginTXslot= nullptr;
    //@}

    /** @name Statistics */
    //@{
    long numRetry;
    long numSentWithoutRetry;
    long numGivenUp;
    long numCollision;
    long numSent;
    long numReceived;
    long numSentBroadcast;
    long numReceivedBroadcast;
    long numReceivedBeacons;
    //@}

    const char beaconReceivedText[17] = "Beacon received!";


  public:
    /**
     * @name Construction functions
     */
    //@{
    virtual ~LoRaMac();
    //@}
    virtual MacAddress getAddress();
    bool isInBeaconReception() const {
            return fsm.getState() == BEACON_RECEPTION || fsm.getState() == RECEIVING_BEACON;
        }


  protected:
    /**
     * @name Initialization functions
     */
    //@{
    /** @brief Initialization of the module and its variables */
    virtual void initialize(int stage) override;
    virtual void finish() override;
    virtual void configureNetworkInterface() override;
    //@}

    /**
     * @name Message handing functions
     * @brief Functions called from other classes to notify about state changes and to handle messages.
     */
    //@{
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void handleUpperMessage(cMessage *msg) override;
    virtual void handleLowerMessage(cMessage *msg) override;
    virtual void handleWithFsm(cMessage *msg);

    virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;

    virtual Packet *encapsulate(Packet *msg);
    virtual Packet *decapsulate(Packet *frame);
    //@}


    /**
     * @name Frame transmission functions
     */
    //@{
    virtual void sendDataFrame(Packet *frameToSend);
    virtual void sendAckFrame();
    //virtual void sendJoinFrame();
    //@}

    /**
     * @name Utility functions
     */
    //@{
    virtual void finishCurrentTransmission();
    virtual Packet *getCurrentTransmission();

    virtual bool isReceiving();
    virtual bool isAck(const Ptr<const LoRaMacFrame> &frame);
    virtual bool isBroadcast(const Ptr<const LoRaMacFrame> & msg);
    virtual bool isForUs(const Ptr<const LoRaMacFrame> &msg);

    void turnOnReceiver(void);
    void turnOffReceiver(void);
    simtime_t lastBeaconReceptionTime;


    virtual void calculatePingPeriod(const Ptr<const LoRaMacFrame> &frame);
    virtual void schedulePingPeriod();
    virtual int aesEncrypt(unsigned char *message, int message_len, unsigned char *key, unsigned char *cipher);

    virtual bool isBeacon(const Ptr<const LoRaMacFrame> &frame);
    virtual bool isDownlink(const Ptr<const LoRaMacFrame> &frame);

    virtual bool timeToTrasmit();
    virtual void scheduleULslots();

    virtual void beaconScheduling();
    virtual void increaseBeaconTime();
    //virtual Packet *getCurrentReception();

    //@}
};

} // namespace inet

#endif // ifndef __LORAMAC_H
