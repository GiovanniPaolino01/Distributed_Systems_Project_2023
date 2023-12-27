#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

class SendingObject : public cObject
{
    private:
        std::string type;
        std::string key;
        int val;

    public:
        void setKey(std::string k){
            key = k;
        }
        void setVal(int va){
            val = va;
        }
        void setType(std::string t){
            type = t;
        }

        std::string getKey(){
            return key;
        }
        int getVal(){
            return val;
        }
        std::string getType(){
            return type;
        }


};
