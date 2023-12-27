#include <string.h>
#include <iostream>
#include <omnetpp.h>
#include "ResourceObject.h"
#include <algorithm>

using namespace omnetpp;

class Coordinator : public cSimpleModule
{
    private:
        cMessage* handled_message;
        cQueue *response_queue;
        int versionProposal;
        int responsesReceived = 0;
        int responsesReceivedP = 0;
        bool justKilling;
        double counter;
        cQueue *temp;

    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;

   public:
        static int MyCompareFunc (cObject *a, cObject *b);
};

Define_Module(Coordinator);

void Coordinator::initialize()
{
    handled_message = new cMessage();
    response_queue = new cQueue();
    versionProposal = 0;
    justKilling = false;
    temp = new cQueue();

    counter = (double) this->getId(); //Lamport Clock
}

void Coordinator::handleMessage(cMessage *msg)
{
    //1
    if(msg->getArrivalGateId() == findGate("inFromC") || msg->getSenderModuleId()==this->getId()){ //The message arrives from Client (It's a new request to handle)

        //New request from the Client => increment the decimal part of the Lamport clock
        if(msg->getArrivalGateId() == findGate("inFromC")){
            counter = counter + 0.1;
        }

        //I'm going to handle a new request, so I empty the queue of the responses, and I add the Lamport clock to the message.
        response_queue->clear();
        msg->addPar("LAMPORTCLOCK");
        msg->par("LAMPORTCLOCK").setDoubleValue(counter);
        *handled_message = *msg;
        EV << "Coordinator-- Message from Client arrived; ";

        //2
        if(strcmp(msg->getName(), "get") == 0){ //get

            EV << " Message name: " << msg->getName() << " Key: " << msg->par("key").stringValue()<<endl;

            //3
            for(int i=0; i<par("nr").intValue(); i++){ //Send the GET request to NR nodes

                cMessage *mForNodes = new cMessage("get");

                mForNodes->addPar("key");
                mForNodes->addPar("LAMPORTCLOCK");
                mForNodes->par("key").setStringValue(msg->par("key").stringValue());

                //If this is a request sent from the coordinator to itself it's a request that has been killed and has to be redone, the Lamport clock must remain the same
                if(msg->getSenderModuleId()==this->getId()){
                    mForNodes->par("LAMPORTCLOCK").setDoubleValue(handled_message->par("LAMPORTCLOCK").doubleValue());
                    justKilling = false;
                }else{
                    mForNodes->par("LAMPORTCLOCK").setDoubleValue(counter);
                }

                int random = (i+this->getId())%par("n").intValue(); //Each coordinator contacts a different subset of nodes

                send(mForNodes, "out", random);
            }
        }//2
        else if(strcmp(msg->getName(), "put")==0){     //put

            EV << " Message name: " << msg->getName() << " Key: " << msg->par("key").stringValue() << " Value: " << msg->par("value").longValue() <<endl;

            //3
            for(int i=0; i<par("nw").intValue(); i++){ //Send the PUT request to NW nodes

                cMessage *mForNodes = new cMessage("put");

                mForNodes->addPar("key");
                mForNodes->addPar("value");
                mForNodes->addPar("LAMPORTCLOCK");
                mForNodes->par("key").setStringValue(msg->par("key").stringValue());
                mForNodes->par("value").setLongValue(msg->par("value").longValue());
                if(msg->getSenderModuleId()==this->getId()){
                   mForNodes->par("LAMPORTCLOCK").setDoubleValue(handled_message->par("LAMPORTCLOCK").doubleValue());
                   justKilling = false;
               }else{
                   mForNodes->par("LAMPORTCLOCK").setDoubleValue(counter);
               }


                int random = (i+this->getId())%par("n").intValue(); //Each coordinator contacts a different subset of nodes
                ResourceObject* r = new ResourceObject();
                r->setVal(random);
                temp->insert(r); //Saves in the queue the nodes to which the Coordinator has sent the request it is handling now

                send(mForNodes, "out", random);
            }//3
        }//2
    //1
    }else{ //The message comes from a node.
           //In case of a getResponse, the node sends me version and value.
           //In case of a putResponse, the node sends me only its version proposal
           //(Coordinator now has to save all the responses and, when it has received NW answers, it has to decide which is the definitive version
           //and send, to all the nodes it has contacted before, a COMMIT message containing the version to commit).

        if(strcmp(msg->getName(), "getResponse") == 0){
            EV << "Coordinator-- getResponse received from node " << msg->getArrivalGate()->getIndex() << "; ";
            responsesReceived++;
            ResourceObject *r = new ResourceObject();
            r->setVal(msg->par("value").longValue());
            r->setVer(msg->par("version").longValue());
            response_queue->insert(r);
            EV <<"Node's response: "<< msg->par("key").stringValue() << " = " << msg->par("value").longValue()<< ", Version: " << msg->par("version").longValue() <<endl;

        }

        if(strcmp(msg->getName(), "getNotFound") == 0){
            responsesReceived++;
            EV <<"Coordinator-- Node's response: GET NOT FOUND"<< endl;

        }

        if(strcmp(msg->getName(), "putResponse") == 0 && justKilling == false){
            EV << "Coordinator-- putResponse received from node " << msg->getArrivalGate()->getIndex() << "; ";
            responsesReceivedP++;
            ResourceObject *r = new ResourceObject();
            r->setVal(msg->par("value").longValue());
            r->setVer(msg->par("version").longValue());
            r->setGateIndex(msg->getArrivalGate()->getIndex());
            response_queue->insert(r);
            EV << "Node's response: Version proposal for put " << msg->par("key").stringValue() << " " << msg->par("value").longValue() << ": " << msg->par("version").longValue() << ", r gerVer: " << r->getVer();
        }

        if(strcmp(msg->getName(), "kill") == 0 && justKilling == false){ //Kill received and kill process isn't started yet

           EV << "Coordinator-- KILL received from node " << msg->getArrivalGate()->getIndex() << " for request: " << msg->par("type").stringValue() << " ";

           //Kill process starts
           justKilling = true;
           int n = 0;
           if(strcmp(msg->par("type"), "get") == 0){
               n = par("nr").intValue();
               EV << endl;
           }else{
               n = par("nw").intValue();
               EV << msg->par("value").longValue() << endl;
           }

           //Sends the kill message to all the nodes he have contacted before for this request that has to be killed.
           for(int i=0; i<n; i++){
               cMessage *m = new cMessage("kill");
               m->addPar("type");
               m->addPar("key");
               m->addPar("sender");
               m->par("type").setStringValue(msg->par("type").stringValue());
               m->par("key").setStringValue(msg->par("key").stringValue());
               m->par("sender").setLongValue(this->getId());

               ResourceObject *r = new ResourceObject();
               r = (ResourceObject*)temp->pop();
               send(m, "out", r->getVal());
           }

           //Coordinator reschedules the message just killed and sends it to himself with the same Lamport clock as before
           cMessage *m = new cMessage();
           m->setName(msg->par("type").stringValue());
           m->addPar("key");
           m->addPar("LAMPORTCLOCK");
           m->par("key").setStringValue(msg->par("key").stringValue());
           m->par("LAMPORTCLOCK").setDoubleValue(handled_message->par("LAMPORTCLOCK"));
           if(strcmp(msg->par("type"), "put") == 0){
               m->addPar("value");
               m->par("value").setLongValue(msg->par("value").longValue());
           }


           simtime_t delay = 100;
           cancelEvent(m);
           scheduleAt(simTime()+delay, m);

           //The answers received before the kill message aren't valid anymore, so the queue is cancelled
           response_queue->clear();
           responsesReceivedP = 0;
        }



        int len=0;
        //I'm handling a GET, if I've received at least NR getResponse messages, than I have to find and send to the client the value of the last version received
        if((strcmp(msg->getName(), "getResponse")==0 || strcmp(msg->getName(), "getNotFound")==0) && responsesReceived>=par("nr").intValue()){

            if(response_queue->isEmpty()){ //Answers are all of type "getNotFound", the resource is'n present in any node
                cMessage *mforClient = new cMessage("getNotFound");
                mforClient->addPar("key");
                mforClient->par("key").setStringValue(msg->par("key").stringValue());
                send(mforClient, "outToC");

            }
            else{//Resource found somewhere
                len = response_queue->getLength();
                int valueToClient = 0;
                int max_ver = 0;

                //Find the maximum version and its value
                for(int i=0; i<len; i++){
                    if(((ResourceObject*)(response_queue->get(i)))->getVer()>max_ver){
                        max_ver = ((ResourceObject*)(response_queue->get(i)))->getVer();
                        valueToClient = ((ResourceObject*)(response_queue->get(i)))->getVal();
                    }
                }

                /*for(int i=0; i<len; i++){
                    if(((ResourceObject*)(response_queue->get(i)))->getVer() == max_ver){
                        valueToClient = ((ResourceObject*)(response_queue->get(i)))->getVal();
                    }
                }*/

                EV << "Coordinator-- GET value for client: " << msg->par("key").stringValue() << " = " << valueToClient << " max_ver: " <<max_ver<<endl;
                cMessage *mforClient = new cMessage("getAck");
                mforClient->addPar("key");
                mforClient->addPar("value");
                mforClient->par("key").setStringValue(msg->par("key").stringValue());
                mforClient->par("value").setLongValue(valueToClient);
                send(mforClient, "outToC");
                response_queue->clear();
            }

            responsesReceived = 0;

        }


        int lenP = 0;
        /* I'm handling a PUT, if I've received at least NW putResponses, then I have to look for the higher version proposal from nodes
         * and communicate to all the nodes I've contacted before, with a COMMIT message containing that version */
        if(strcmp(msg->getName(), "putResponse")==0 && (/*response_queue->getLength()*/responsesReceivedP>=par("nw").intValue())){
            lenP = response_queue->getLength();

            //Find the most recent version
            int max_verP = 0;
            for(int i=0; i<lenP; i++){
                if(((ResourceObject*)(response_queue->get(i)))->getVer() > max_verP){
                    EV << "Response queue, get(i), getVer:   " << ((ResourceObject*)(response_queue->get(i)))->getVer();
                    max_verP = ((ResourceObject*)(response_queue->get(i)))->getVer();
                }
            }

            //Send commit of that value with the most recent version just found.
            for(int i=0; i<lenP; i++){
                //Contact the same nodes (the ones you have asked for the put) and send COMMIT passing key, value, version.
                cMessage *mForNodes = new cMessage("commit");

                mForNodes->addPar("key");
                mForNodes->addPar("value");
                mForNodes->addPar("version");
                mForNodes->par("key").setStringValue(msg->par("key").stringValue());
                mForNodes->par("value").setLongValue(msg->par("value").longValue());
                mForNodes->par("version").setLongValue(max_verP);

                int gate = ((ResourceObject*)(response_queue->get(i)))->getGateIndex();
                send(mForNodes, "out", gate);

            }

            //Send to the Client the ack for the put it has asked.
            cMessage *mforClient = new cMessage("putAck");
            mforClient->addPar("key");
            mforClient->addPar("value");
            mforClient->par("key").setStringValue(msg->par("key").stringValue());
            mforClient->par("value").setLongValue(msg->par("value").longValue());
            send(mforClient, "outToC");
            response_queue->clear();
            EV << "Coordinator-- PUT "<< mforClient->par("key").stringValue()<< " " << mforClient->par("value").longValue() <<", version that will be committed: " << max_verP <<endl;

            responsesReceivedP = 0;
        }
    }//1
}


int Coordinator::MyCompareFunc (cObject *a, cObject *b) {
    if ((((check_and_cast<cMessage*>(a))->par("LAMPORTCLOCK").doubleValue() - (int) (check_and_cast<cMessage*>(a))->par("LAMPORTCLOCK").doubleValue())) < (((check_and_cast<cMessage*>(b))->par("LAMPORTCLOCK").doubleValue() - (int) (check_and_cast<cMessage*>(b))->par("LAMPORTCLOCK").doubleValue()))){
        return -1;
    }else if ((((check_and_cast<cMessage*>(a))->par("LAMPORTCLOCK").doubleValue() - (int) (check_and_cast<cMessage*>(a))->par("LAMPORTCLOCK").doubleValue())) == (((check_and_cast<cMessage*>(b))->par("LAMPORTCLOCK").doubleValue() - (int) (check_and_cast<cMessage*>(b))->par("LAMPORTCLOCK").doubleValue()))){
        return 0;
    }else return 1;
}
