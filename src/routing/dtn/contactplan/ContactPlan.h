#ifndef __FLORA_ROUTING_CONTACTPLAN_H_
#define __FLORA_ROUTING_CONTACTPLAN_H_

#include "routing/dtn/contactplan/Contact.h"
#include <omnetpp.h>
#include <algorithm>
#include <vector>
#include <fstream>
#include <queue>
#include <iostream>
#include "inet/common/INETDefs.h"

using namespace omnetpp;

namespace flora {

class ContactPlan : public cSimpleModule {

    public:

        virtual void initialize(int stage) override;
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void finish() override;

        // Contact plan population functions
        int addContact(double start, double end, int sourceEid, int destinationEid, double dataRate, double confidence);
        int addContact(double start, double end, int sourceEid, int destinationEid, double dataRate, double confidence, double failureProbability);
        int addDiscoveredContact(double start, double end, int sourceEid, int destinationEid, double dataRate, double confidence);
        int addDiscoveredContact(double start, double end, int sourceEid, int destinationEid, double dataRate, double confidence, double failureProbability);
        int addPredictedContact(double start, double end, int sourceEid, int destinationEid, double dataRate, double confidence);
        void addRange(double start, double end, int sourceEid, int destinationEid, double range, double confidence);
        void addCurrentNeighbor(int eid);

        // Contact plan exploration functions
        Contact *getContactById(int id);
        vector<Contact> * getContacts();
        vector<Contact> * getRanges();
        vector<Contact> getContactsBySrc(int Src);
        vector<int> * getContactIdsBySrc(int Src);
        vector<Contact> getContactsByDst(int Dst);
        vector<Contact> getContactsBySrcDst(int Src, int Dst);
        vector<Contact> getDiscoveredContacts();
        vector<int> getCurrentNeighbors();
        Contact* getContactBySrcDstStart(int sourceEid, int destinationEid, double start);
        double getRangeBySrcDst(int Src, int Dst);
        void parseContactPlanFile(string fileName, int nodesNumber, int mode, double failureProb);
        void parseOpportunisticContactPlanFile(string fileName, int nodesNumber, int mode, double failureProb);
        void setContactsFile(string contactsFile);
        const string& getContactsFile() const;
        simtime_t getLastEditTime();
        int getHighestId();

        // delete contact function
        vector<Contact>::iterator deleteContactById(int contactId);
        Contact removePredictedContactForSourceDestination(int sourceEid, int destinationEid);
        Contact removeDiscoveredContact(int sourceEid, int destinationEid);
        void removeCurrentNeighbor(int eid);
        void deleteOldContacts();

        // debug function
        void printContactPlan();
        void exportContactPlan(string filename);

        //Checked when discovered/predicted contacts are inserted.
        bool overlapsWithContact(int sourceEid, int destinationEid, double start, double end);
        bool hasPredictedContact(int sourceEid, int destinationEid);
        bool hasDiscoveredContact(int sourceEid, int destinationEid);
        void updateContactRanges();
        void sortContactIdsBySrcByStartTime();


    private:
        static const int DELETED_CONTACT = -1;
        int nextContactId = 1;
        vector<int> currentNeighbors_; //The current neighbors of a node
        map<double, vector<int>> contactDeleteTimes_; //The times at which a contact needs to be deleted
        vector<Contact> contacts_;
        vector<Contact> ranges_;
        vector<vector<int>> contactIdsBySrc_;
        vector<int> contactIdShift_ = {0}; // create dummy element for limbo contact
        simtime_t lastEditTime;
        string contactsFile_;
        int satelliteCount;
        int groundStationCount;
        string fileName;
        string exportFileName;
};

}

#endif /* __FLORA_ROUTING_CONTACTPLAN_H_ */
