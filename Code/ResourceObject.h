#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class ResourceObject : public cObject
{
    private:
        std::string key;
        int val;
        int ver;
        int gateIndex;
    public:
        void setKey(std::string k){
            this->key = k;
        }
        void setVal(int va){
            this->val = va;
        }
        void setVer(int ve){
            this->ver = ve;
        }
        void setGateIndex(int index){
            this->gateIndex = index;
        }
        std::string getKey(){
            return key;
        }
        int getVal(){
            return val;
        }
        int getVer(){
            return ver;
        }
        int getGateIndex(){
            return gateIndex;
        }

};
