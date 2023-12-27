#include <string.h>
#include <omnetpp.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include "ResourceObject.h"

using namespace omnetpp;

class Node : public cSimpleModule
{
    private:
       cMessage *handled_message;
       cQueue *request_queue;
       cQueue *resources_queue;
       cOutVector vecX;
       cOutVector vecY;
       cOutVector vecZ;
       cOutVector vecK;
       cOutVector vecW;

    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;

    public:
        cMessage *handleGet();
        cMessage *handlePut();
        static int MyCompareFunc (cObject *a, cObject *b);

};

Define_Module(Node);

void Node::initialize()
{
    char c = 'c';
    request_queue = new cQueue(&c, MyCompareFunc);
    resources_queue = new cQueue();
    handled_message = new cMessage("null"); //if it is a new message, it is initialized to "null"


    //RESOURCE INITIALIZATION (FROM FILE)
    std::string rigaLetta;
    std::ifstream File("InizializzazioneNodo.txt");
    while (getline(File, rigaLetta))
    {
        std::string delimiter = " ";
        std::string nodeNumberInFile = rigaLetta.substr(0, rigaLetta.find(delimiter));
        int n = this->getId()-(this->gateCount()/2)-2; //Retrieves node number
        std::stringstream ss;
        ss<<n;
        std::string nodeNumber = ss.str();

        // In nodeNumberInFile there's only the initisl number of that row. The node checks if this is his row, and if this is it, it starts elaborating informations
        if(strcmp(nodeNumberInFile.data(), nodeNumber.data())==0){

            std::regex regex{R"([\s]+)"}; //split on space
            std::regex regex2{R"([\s(,)]+)"}; // split on space, comma and parenthesis
            //std::regex regex{R"([\s,]+)"}; // split on space and comma
            std::sregex_token_iterator it{rigaLetta.begin(), rigaLetta.end(), regex, -1};
            std::vector<std::string> splitted{it, {}}; //splits in strings the row basing on spaces and creates the vector (called splitted) of those strings.
            for(int i=1; i<splitted.size(); i++){ //I start from 1 and not from 0 to avoid the number of the node, I'm interested only in resources' initialization

                //For each resource initialization (key,val,ver) I split basing on commas and parenthesis, and I create the vector splitted2 which will be: splitted2[0]=key, splitted2[1]=val, splitted2[2]=ver
                std::stringstream ss1;
                ss1 << splitted[i];
                std::string myRow;
                myRow = ss1.str();
                std::sregex_token_iterator ite{myRow.begin()+1, myRow.end(), regex2, -1};
                std::vector<std::string> splitted2{ite, {}};

                //This can be used to let the code print out the initializations.
                /*for(int i=0; i<splitted2.size(); i++){
                    EV << splitted2[i] << endl;
                }*/

                //Inserts resources into the queue.
                ResourceObject *resource = new ResourceObject();
                std::stringstream ss2;
                ss2 << splitted2[0];
                std::string key = ss2.str();
                resource->setKey(key);
                std::stringstream ss3;
                ss3 << splitted2[1];
                std::string value = ss3.str();
                int val = stoi(value);
                resource->setVal(val);
                std::stringstream ss4;
                ss4 << splitted2[2];
                std::string version = ss4.str();
                int ver = stoi(version);
                resource->setVer(ver);
                resources_queue->insert(resource);
            }
            break;
        }

    }
    File.close();

    //Results part
        vecX.setName("Resource-X");
        vecY.setName("Resource-Y");
        vecZ.setName("Resource-Z");
        vecK.setName("Resource-K");
        vecW.setName("Resource-W");

        int val;
        for(int j=0; j<resources_queue->getLength(); j++){

            if(((ResourceObject*)(resources_queue->get(j)))->getKey() == "x"){
                val = ((ResourceObject*)(resources_queue->get(j)))->getVal();
                vecX.record(val);
            }

            else if(((ResourceObject*)(resources_queue->get(j)))->getKey() == "y"){
                val = ((ResourceObject*)(resources_queue->get(j)))->getVal();
                vecY.record(val);
            }

            else if(((ResourceObject*)(resources_queue->get(j)))->getKey() == "w"){
                val = ((ResourceObject*)(resources_queue->get(j)))->getVal();
                vecW.record(val);
            }

            else if(((ResourceObject*)(resources_queue->get(j)))->getKey() == "z"){
                val = ((ResourceObject*)(resources_queue->get(j)))->getVal();
                vecZ.record(val);
            }

            else if(((ResourceObject*)(resources_queue->get(j)))->getKey() == "k"){
                val = ((ResourceObject*)(resources_queue->get(j)))->getVal();
                vecK.record(val);
            }
        }
        //Results part end
}

void Node::handleMessage(cMessage *msg)
{

    cMessage *response_message = new cMessage("Response"); //Declaration of the message to be sent to the Coordinator, the name will be set to getResponse or putResponse depending on the type of response I need to send
    //1
    if (strcmp(msg->getName(),"put") == 0 || strcmp(msg->getName(),"get") == 0){

        //If I'm not handling any request and I don't have any request into the queue, I hanldle this request now.
        //2
        if(strcmp(handled_message->getName(),"null")==0 && request_queue->isEmpty()){
            *handled_message = *msg;
            handled_message->setName(msg->getName());
            //3
            if (strcmp(handled_message->getName(),"get") == 0){
               response_message = handleGet();
            }else{                                                              //case in which I'm handling a PUT
                                                                                //The node has to propose his version to the Coordinator, so
                                                                                //it searches for the resource into the resourceQueue and
                                                                                //it sends to the Coordinator a message containing
                                                                                //version = versionOfTheResourceOnTheNode + 1
                                                                                //If it cannot find the resource, it sends version = 1
                response_message = handlePut();
            }//3

            // I send the response message to the Coordinator with the answer to the request it has done (put or get)
            for(int i=0; i<this->gateCount()/2; i++){ //number of coordinator
                if(handled_message->getArrivalGateId() == findGate("in", i)){
                    send(response_message, "out", i);
                }
            }

            //If it was a PUT, I have to wait for the COMMIT message before finishing to handle the request;
            //If it was a GET, instead, I've finished handling the request.
            if(strcmp(handled_message->getName(), "get") == 0){
                    handled_message->setName("null");
            }


        }//2

        // If I'm already handling a request, I have to check if the one that just arrived is "younger" or "older" than the one I'm handling.
        // If it's older, I insert it into the queue, ordering it; if it's younger, instead, I send a KILL message to the coordinator.
        //2
        else if (strcmp(handled_message->getName(), "null")!=0){
            //3
            if((strcmp(msg->getName(), "get") == 0 || (((msg->par("LAMPORTCLOCK").doubleValue() - (int) msg->par("LAMPORTCLOCK").doubleValue()) < (handled_message->par("LAMPORTCLOCK").doubleValue() - (int) handled_message->par("LAMPORTCLOCK").doubleValue()))) || ( ((msg->par("LAMPORTCLOCK").doubleValue() - (int) msg->par("LAMPORTCLOCK").doubleValue()) == (handled_message->par("LAMPORTCLOCK").doubleValue() - (int) handled_message->par("LAMPORTCLOCK").doubleValue())) &&  ((int) msg->par("LAMPORTCLOCK").doubleValue() <  (int) handled_message->par("LAMPORTCLOCK").doubleValue()) ))){
                request_queue->insert(msg);                                                                     //Viene "automaticamente" inserito in ordine di par("sendingTime")
            }
            //3
            else{       //case kill: The request is younger (has a higher Lamport Clock) than the one I'm handling, I have to answer to the Coordinator with a KILL message.
                //---------------------------------------------------------------------------------------------
                for(int i=0; i<this->gateCount()/2; i++){ //number of Coordinator
                   cMessage *m = new cMessage("kill");
                   m->addPar("type");
                   m->addPar("key");
                   m->par("type").setStringValue(msg->getName());
                   m->par("key").setStringValue(msg->par("key").stringValue());
                   if(strcmp(m->par("type"), "put") == 0){
                       m->addPar("value");
                       m->par("value").setLongValue(msg->par("value").longValue());
                   }
                   if(msg->getArrivalGateId() == findGate("in", i)){
                       send(m, "out", i);
                   }
               }

               EV << "Node-- I send a KILL message to the Coordinator, this request ("<< msg->getName() << " " << msg->par("key").stringValue() << " " << msg->par("value").longValue() << ") is younger than the one I'm already handling" <<endl;

            }//3

        }
        //2
        else{//case in which handled_message==null but there are requests into the queue
            //I insert the message into the queue, in order, and then I pop the first message from the queue (the one with the lower Lamport Clock) and I handle it.
            request_queue->insert(msg); //It's "automatically" inserted in order of par("LAMPORTCLOCK").
            handled_message = check_and_cast<cMessage*>(request_queue->pop());

        }//2


        EV << "Node-- I've received a request:" << endl;
        EV << msg->getName() <<endl;
        EV << msg->par("key").stringValue() <<endl;
        if(strcmp(msg->getName(),"put")==0){
            EV << msg->par("value").longValue() <<endl;
        }


    //1
    }else{ //Case kill or commit
        if(strcmp(msg->getName(),"commit") == 0){ // case commit

            EV << "Node-- I've received a COMMIT messsage from the coordinator. put" << " " << msg->par("key").stringValue() << " " << msg->par("value").longValue() << " Version: "<< msg->par("version").longValue()<< endl;

            bool exists = false;
            //scan the queue until I find the resource to be updated and overwrite the value and the version fields
            for(int i=0; i<resources_queue->getLength(); i++){
                if(strcmp(((ResourceObject*)(resources_queue->get(i)))->getKey().data(), msg->par("key")) == 0){
                    ((ResourceObject*)(resources_queue->get(i)))->setVal(msg->par("value").longValue());
                    ((ResourceObject*)(resources_queue->get(i)))->setVer(msg->par("version").longValue());
                    exists = true;
                }
            }
            if(!exists){
                ResourceObject* r = new ResourceObject();
                r->setKey(msg->par("key").stringValue());
                r->setVal(msg->par("value").longValue());
                r->setVer(msg->par("version").longValue());
                resources_queue->insert(r);

            }

            for(int i=0; i<resources_queue->getLength(); i++){
                EV <<"key " << ((ResourceObject*)(resources_queue->get(i)))->getKey() << " val " << ((ResourceObject*)(resources_queue->get(i)))->getVal() << " ver " << ((ResourceObject*)(resources_queue->get(i)))->getVer() <<endl;
            }
            handled_message->setName("null");

            //results part 2
            int val;
            for(int j=0; j<resources_queue->getLength(); j++){

                if(((ResourceObject*)(resources_queue->get(j)))->getKey() == ((std::string)msg->par("key"))){
                    val = ((ResourceObject*)(resources_queue->get(j)))->getVal();
                    if(((std::string)msg->par("key"))=="x") vecX.record(val);
                    else if(((std::string)msg->par("key"))=="y") vecY.record(val);
                    else if(((std::string)msg->par("key"))=="z") vecZ.record(val);
                    else if(((std::string)msg->par("key"))=="k") vecK.record(val);
                    else if(((std::string)msg->par("key"))=="w") vecW.record(val);
                    break;
                }

            }
            //results part 2 end

        }else{ //case kill
            EV << "Node-- I've received a KILL message from the Coordinator"<<endl;
            EV << "KILLLLLLLLLLLLLLLLLLLLLLLLLLL " << msg->par("type").stringValue() << " " << msg->par("key").stringValue() << endl;

            if(handled_message->getSenderModuleId() == msg->par("sender").longValue()){
                EV << "NOME HANDLE" << handled_message->getName() << "TIPO " << msg->par("type").stringValue() << "KEY HANDLE " <<handled_message->par("key").stringValue() << "KEY " <<  msg->par("key").stringValue();
                if(strcmp(handled_message->getName(), msg->par("type").stringValue()) == 0 && strcmp(handled_message->par("key").stringValue(), msg->par("key").stringValue()) == 0){
                    handled_message->setName("null");
                }
            }else{
                for(int i=0; i<request_queue->getLength(); i++){
                    cMessage *m = check_and_cast<cMessage*>(request_queue->get(i));
                    if(m->getSenderModuleId() == msg->par("sender").longValue() && strcmp(m->getName(), msg->par("type").stringValue()) == 0 && strcmp(m->par("key").stringValue(), msg->par("key").stringValue()) == 0){
                        request_queue->remove(request_queue->get(i));

                    }
                }
            }
        }
    }//1

    while (strcmp(handled_message->getName(), "null")==0 && !request_queue->isEmpty()){
        handled_message = check_and_cast<cMessage*>(request_queue->pop());


        if (strcmp(handled_message->getName(),"get") == 0){
            response_message = handleGet();
       }else{                                                              //case in which I'm handling a PUT
                                                                           //The Node has to propose it's version to the Coordinator, so
                                                                           //it searches in the resource queue the resource with that key and
                                                                           //sends to the coordinator a message containing version = versionOfTheResourceOnNode + 1
                                                                           //if it doesn't find the resource, it sends version = 1
           response_message = handlePut();
       }//3

        // I send the response message to the Coordinator with the answer to the request it has done (put or get)
       for(int i=0; i<this->gateCount()/2; i++){ //number of coordinator
           if(handled_message->getArrivalGateId() == findGate("in", i)){
               send(response_message, "out", i);
           }
       }

       //If it was a PUT, I have to wait for the COMMIT message before finishing to handle the request;
       //If it was a GET, instead, I've finished handling the request.
       if(strcmp(handled_message->getName(), "get") == 0){
               handled_message->setName("null");
       }
    }
}


int Node::MyCompareFunc (cObject *a, cObject *b) {
    if ((((check_and_cast<cMessage*>(a))->par("LAMPORTCLOCK").doubleValue() - (int) (check_and_cast<cMessage*>(a))->par("LAMPORTCLOCK").doubleValue())) < (((check_and_cast<cMessage*>(b))->par("LAMPORTCLOCK").doubleValue() - (int) (check_and_cast<cMessage*>(b))->par("LAMPORTCLOCK").doubleValue()))){
        return -1;
    }else if ((((check_and_cast<cMessage*>(a))->par("LAMPORTCLOCK").doubleValue() - (int) (check_and_cast<cMessage*>(a))->par("LAMPORTCLOCK").doubleValue())) == (((check_and_cast<cMessage*>(b))->par("LAMPORTCLOCK").doubleValue() - (int) (check_and_cast<cMessage*>(b))->par("LAMPORTCLOCK").doubleValue()))){
        return 0;
    }else return 1;
}

cMessage* Node::handleGet(){
    cMessage *response_message = new cMessage("Response");
    ResourceObject *r = new ResourceObject(); //resource I'm looking for
    bool found = false;
    int i=0;
    while(i<resources_queue->getLength() && found == false){ //search for key into the queue
           r = (ResourceObject*)resources_queue->get(i);
           if(strcmp(r->getKey().data(), handled_message->par("key").stringValue())==0){ //Check if the object into the queue is the one with the key I'm looking for
               found = true;
           }
           i++;
    }
       //Creation of the response message for the Coordinator
    response_message->setName("getResponse");
    if(found == true){
       response_message->addPar("key");
       response_message->addPar("value");
       response_message->addPar("version");
       response_message->par("key").setStringValue(r->getKey().data());
       response_message->par("value").setLongValue(r->getVal());
       response_message->par("version").setLongValue(r->getVer());
    }
    else{ //Il nodo non ha la risorsa
       response_message->setName("getNotFound");
       response_message->addPar("key");
       response_message->par("key").setStringValue(handled_message->par("key").stringValue());
    }

    return response_message;
}

cMessage* Node::handlePut(){
    cMessage *response_message = new cMessage("Response");
    ResourceObject *r = new ResourceObject(); //resource I'm looking for
    bool found = false;
    int i=0;
    while(i<resources_queue->getLength() && found == false){ //Search for key of the resource into the queue
        r = (ResourceObject*)resources_queue->get(i);
        if(strcmp(r->getKey().data(), handled_message->par("key").stringValue())==0){ //Check if the object into the queue is the one with the key I'm looking for
            found = true;
        }
        i++;
    }
    //Creation of the response message for the Coordinator, I send my version proposal.
    //(The coordinator will then evaluate all the responses with the version proposals, choose the version (highest version) and send a COMMIT message to the nodes with the definitive version)
    response_message->setName("putResponse");
    response_message->addPar("key");
    response_message->addPar("value");
    response_message->addPar("version");
    response_message->par("key").setStringValue(handled_message->par("key").stringValue());
    response_message->par("value").setLongValue(handled_message->par("value").longValue());
    if(found == true){
        response_message->par("version").setLongValue(r->getVer()+1);
    }
    else{ //The node hasn't the resource
        response_message->par("version").setLongValue(1);
    }

    return response_message;
}

