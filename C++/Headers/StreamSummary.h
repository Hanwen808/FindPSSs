#ifndef STREAMSUMMARY_H
#define STREAMSUMMARY_H
#include <cstdint>
#include <cmath>
#include <unordered_map>

struct SSKeyNode;

struct SSValueNode {
    float counter;
    SSValueNode *next;
    SSValueNode *pre;
    SSKeyNode *firstKeyNode;
};

struct SSKeyNode {
    uint32_t key;
    float err;
    SSValueNode *parent;
    SSKeyNode *next;
};

class StreamSummary {
private:
    uint32_t size = 0;
    SSValueNode *firstValueNode = nullptr;
    std::unordered_map<uint32_t, SSKeyNode *> hashTable;
    uint32_t SS_MAX_SIZE;
public:
    StreamSummary(uint32_t x) {
        SS_MAX_SIZE = x;
    }
    ~StreamSummary(){
        SSValueNode *ssValueNode = firstValueNode;
        while (ssValueNode != nullptr) {
            SSKeyNode *ssKeyNode = ssValueNode->firstKeyNode;
            do {
                SSKeyNode *tempKeyNode = ssKeyNode;
                ssKeyNode = ssKeyNode->next;
                free(tempKeyNode);
            } while (ssKeyNode != ssValueNode->firstKeyNode);

            SSValueNode *tempValueNode = ssValueNode;
            ssValueNode = ssValueNode->next;
            free(tempValueNode);
        }
    }

    uint32_t getSize() const{
        return size;
    }
    float getMinVal() const{
        if (size != 0) {
            return firstValueNode->counter;
        } else {
            return -1;
        }
    }

    SSValueNode* getFirstValueNode() const{
        return firstValueNode;
    }

    SSKeyNode* getMinKeyNode() const{
        return firstValueNode->firstKeyNode;
    }
    SSKeyNode* findKey(uint32_t key) const{
        auto iter = hashTable.find(key);
        if (iter == hashTable.end()) {
            return nullptr;
        } else {
            return iter->second;
        }
    }

    void SSPush(uint32_t key, float value,float err) {
        SSKeyNode *newKeyNode;
        SSValueNode *newValueNode= nullptr;
        if(size>=SS_MAX_SIZE){//when stream summary is full, drop a key node of the first value node (the smallest value node)
            //delete the second key node (or the first key node when the first value node only has one key node)
            SSKeyNode *ssKeyNode = firstValueNode->firstKeyNode->next;
            auto iter = hashTable.find(ssKeyNode->key);
            hashTable.erase(iter);

            if(ssKeyNode->next==ssKeyNode){//the first value node only has one key node
                newValueNode=firstValueNode;
                firstValueNode= firstValueNode->next;//may be nullptr
                firstValueNode->pre= nullptr;
            }else{
                firstValueNode->firstKeyNode->next = ssKeyNode->next;
            }
            size--;
            newKeyNode=ssKeyNode;
            newKeyNode->err=err;
        }else{
            newKeyNode = (SSKeyNode *) calloc(1, sizeof(SSKeyNode));
            newKeyNode->err=err;
        }

        SSValueNode *lastValueNode= nullptr;
        SSValueNode *curValueNode=firstValueNode;
        while(curValueNode!= nullptr and curValueNode->counter<value){//find the first value node larger than value
            lastValueNode=curValueNode;
            curValueNode=curValueNode->next;
        }

        if(curValueNode== nullptr or curValueNode->counter>value)//need to create a new value Node between lastValueNode and curValueNode
        {
            if(newValueNode== nullptr){
                newValueNode = (SSValueNode *) calloc(1, sizeof(SSValueNode));
                newValueNode->firstKeyNode = newKeyNode;
            }

            newKeyNode->key=key;
            newKeyNode->parent = newValueNode;
            newKeyNode->next = newKeyNode;

            newValueNode->counter = value;
            newValueNode->pre = lastValueNode;
            newValueNode->next=curValueNode;

            if(curValueNode!= nullptr){
                curValueNode->pre=newValueNode;
            }

            if(lastValueNode== nullptr){//means newValueNode should be the firstValueNode
                firstValueNode = newValueNode;
            }else{
                lastValueNode->next=newValueNode;
            }

            hashTable[key] = newKeyNode;
            size++;
        }else{//value==curValueNode->counter
            //add into curValueNode
            //the key nodes form a circular list
            //compared to inserting the new key node as the first key node, inserting the new key node after the first key node can avoid check whether the ori first key node's next is itself
            newKeyNode->key=key;
            newKeyNode->parent = curValueNode;
            newKeyNode->next = curValueNode->firstKeyNode->next;
            curValueNode->firstKeyNode->next = newKeyNode;

            hashTable[key] = newKeyNode;
            size++;
        }
    }
    //newVal must be larger than the old val
    void SSUpdate(SSKeyNode *ssKeyNode, float newValue) {

        SSValueNode *newValueNode= nullptr;
        SSValueNode *lastValueNode= nullptr;
        SSValueNode *curValueNode= nullptr;

        if (ssKeyNode->next == ssKeyNode) {//the value node only has one key node
            newValueNode=ssKeyNode->parent;
            lastValueNode=newValueNode->pre;
            curValueNode=newValueNode->next;

            if(lastValueNode!= nullptr){
                lastValueNode->next=curValueNode;
            }
            if(curValueNode!= nullptr){
                curValueNode->pre=lastValueNode;
            }
            if(firstValueNode==newValueNode){
                firstValueNode=newValueNode->next;//may be nullptr
                firstValueNode->pre= nullptr;
            }
        }
        else{
            lastValueNode=ssKeyNode->parent;
            curValueNode=lastValueNode->next;
            //detach the key node from old value node
            //Since the linked list is single linked, we have to traverse the list to find the pre node.
            //We choose to replace the node content instead of finding the pre node.
            SSKeyNode* nextKeyNode=ssKeyNode->next;
            uint32_t key=ssKeyNode->key;
            float err=ssKeyNode->err;
            ssKeyNode->key=nextKeyNode->key;
            nextKeyNode->key=key;
            ssKeyNode->err=nextKeyNode->err;
            nextKeyNode->err=err;

            hashTable[ssKeyNode->key]=ssKeyNode;
            hashTable[nextKeyNode->key]=nextKeyNode;

            ssKeyNode->next=nextKeyNode->next;
            if(nextKeyNode==lastValueNode->firstKeyNode){
                lastValueNode->firstKeyNode=ssKeyNode;
            }
            ssKeyNode=nextKeyNode;
        }

        while(curValueNode!= nullptr and curValueNode->counter<newValue){//find the first value node larger than newValue
            lastValueNode=curValueNode;
            curValueNode=curValueNode->next;
        }

        if(curValueNode== nullptr or curValueNode->counter>newValue)//need to create a new value Node between lastValueNode and curValueNode
        {
            if(newValueNode== nullptr){
                newValueNode = (SSValueNode *) calloc(1, sizeof(SSValueNode));
                newValueNode->firstKeyNode = ssKeyNode;
                ssKeyNode->parent = newValueNode;
                ssKeyNode->next = ssKeyNode;
            }

            newValueNode->counter = newValue;
            newValueNode->pre = lastValueNode;
            newValueNode->next=curValueNode;

            if(curValueNode!= nullptr){
                curValueNode->pre=newValueNode;
            }

            if(lastValueNode== nullptr){//means newValueNode should be the firstValueNode
                firstValueNode = newValueNode;
            }else{
                lastValueNode->next=newValueNode;
            }

        }else{//newValue==curValueNode->counter
            ssKeyNode->parent = curValueNode;
            ssKeyNode->next = curValueNode->firstKeyNode->next;
            curValueNode->firstKeyNode->next = ssKeyNode;

            if(newValueNode!= nullptr){
                free(newValueNode);
            }
        }
    }
    void getKeyVals(std::unordered_map<uint32_t,float>& keyVals) const{
        for(auto iter=hashTable.begin();iter!=hashTable.end();iter++){
            uint32_t key=iter->first;
            SSKeyNode* keyNode=iter->second;
            float value=std::fabs(keyNode->parent->counter-keyNode->err);
            keyVals[key]=value;
        }
    }
};

#endif //STREAMSUMMARY_H
