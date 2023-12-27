#include <string>
#include <omnetpp.h>
#include <fstream>
#include <sstream>
#include "SendingObject.h"


using namespace omnetpp;

class Client : public cSimpleModule
{
    private:
        cQueue *sending_queue = new cQueue();

    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;
};

Define_Module(Client);

void Client::initialize()
{
    std::string line;
    int len = 0;
    std::string s;
    int id = this->getId();
    std::string pos_0;
    std::string key;

    std::ifstream inputFile("input.txt", std::ios::in); // Open the input.txt file stream

    if (!inputFile.is_open()) { // Check if the file was opened successfully
        EV << "Error: Unable to open file." << endl;
    }

    while(!inputFile.eof()){
        std::getline(inputFile,s,'\n');
        len = len +1;
    }

    inputFile.seekg(0, std::ios::beg);

    for(int i=0; i<len; i++){
        std::getline(inputFile, line, '\n');

        pos_0 = line.substr(0, 1);

        std::stringstream strs;
        strs << id-2;
        std::string str_id = strs.str();

        //I read from the input file and put into the client's queue all the requests to be done (one at a time).
        if(strcmp(pos_0.data(), str_id.data())==0){ //Checks if the first element of the line corresponds to the id of the client that's reading the file.

            SendingObject *o = new SendingObject();
            key = line.substr(6,1); //takes position 2 in line, substr(position, length)
            o->setKey(key.data());
           if(line.find("g", 0)==2){ //get
               o->setType("get");
               sending_queue->insert(o);
           }else{
               o->setType("put");
               int value;
               value = stoi(line.substr(8,1));
               o->setVal(value);
               sending_queue->insert(o);
           }

        }
    }


    //Take the first element of the queue and send the message to the Coordinator

    SendingObject *obj = ((SendingObject*)sending_queue->pop());

    if(strcmp(obj->getType().data(), "get")==0){
        cMessage *msg1 = new cMessage("get");
        msg1->setName("get");
        msg1->addPar("key");
        msg1->par("key").setStringValue(obj->getKey().data());
        send(msg1, "out");
    }
    else{
        cMessage *msg2 = new cMessage("put");
        msg2->setName("put");
        msg2->addPar("key");
        msg2->addPar("value");
        msg2->par("key").setStringValue(obj->getKey().data());
        msg2->par("value").setLongValue(obj->getVal());
        send(msg2, "out");
    }

    inputFile.close(); // Close the input file stream
}

void Client::handleMessage(cMessage *msg)
{
    if (strcmp(msg->getName(),"putAck") == 0 || strcmp(msg->getName(),"getAck") == 0){//get ack or put ack message received
        EV<<"Client-- Message received, Message Name: " <<msg->getName() <<" Key: " << msg->par("key").stringValue() <<" Value: "<<msg->par("value").longValue()<<endl;
    }else{ //get not found message received
        EV<<"Client-- Message received, Message Name: " <<msg->getName()<<" Key: " << msg->par("key").stringValue()<<endl;
    }

    //Send the next request to the coordinator.
    if(!sending_queue->isEmpty()){
        SendingObject *obj = ((SendingObject*)sending_queue->pop());

        if(strcmp(obj->getType().data(), "get")==0){
            cMessage *msg1 = new cMessage("get");
            msg1->setName("get");
            msg1->addPar("key");
            msg1->par("key").setStringValue(obj->getKey().data());
            send(msg1, "out");
        }
        else{
            cMessage *msg2 = new cMessage("put");
            msg2->setName("put");
            msg2->addPar("key");
            msg2->addPar("value");
            msg2->par("key").setStringValue(obj->getKey().data());
            msg2->par("value").setLongValue(obj->getVal());
            send(msg2, "out");
        }
    }



}

