#ifndef RGS_H
#define RGS_H
#include "Sketch.h"
// s = 256 = 2^8
// m = 8 = 2 ^ 3
// maxRho = 32 - 8 - 3 = 21
#define maxRho 21
#include <unordered_map>
#include <vector>
#include <cmath>

class RGS : public Sketch {
private:
    uint32_t d, s, m, w;
    uint32_t lam;
    uint8_t **regArray;
    uint32_t **IDs;
    float **Cards;
    std::vector<std::vector<std::pair<uint32_t,uint32_t>>> bucketArray;
    uint32_t hash_seed;
    uint32_t threshold_t;
    uint32_t hash_seed_bkt;
    uint32_t cur_time;
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> current_superspreaders;
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> flow_time_map;
    uint32_t current_packet_no;

    inline static uint32_t getRegValue(uint32_t groupStartByteIdx,uint32_t regIdxInGroup,uint8_t* regArray){
        uint32_t regStartByteIdx=regIdxInGroup*5/8;
        uint32_t regStartBitIdx=regIdxInGroup*5%8;

        return ((*(uint16_t*)(regArray+groupStartByteIdx+regStartByteIdx))>>regStartBitIdx)&0x1F;
    }

    inline static void setRegValue(uint32_t groupStartByteIdx,uint32_t regIdxInGroup,uint32_t newValue,uint8_t* regArray){
        uint32_t regStartByteIdx=regIdxInGroup*5/8;
        uint32_t regStartBitIdx=regIdxInGroup*5%8;
        uint16_t* value_ptr=(uint16_t*)(regArray+groupStartByteIdx+regStartByteIdx);
        *value_ptr=(*value_ptr)&(~(((uint16_t)0x1F)<<regStartBitIdx));
        *value_ptr=(*value_ptr)|(newValue<<regStartBitIdx);
    }

    inline bool updateBucket(uint32_t key,uint32_t eleHashValue, uint32_t bktIdx, double updateValue){
        //check if the flow has been recorded
        uint32_t minCellIdx=-1,minCellVal=-1;
        for(int cellIdx=0;cellIdx < lam; cellIdx++){
            if(bucketArray[bktIdx][cellIdx].second==0){
                uint32_t newVal=floor(updateValue);

                double temp=((double)rand())/RAND_MAX;
                if(temp<(updateValue-newVal)){
                    newVal+=1;
                }

                bucketArray[bktIdx][cellIdx].first=key;
                bucketArray[bktIdx][cellIdx].second=newVal;
                return true;
            }
            else if(bucketArray[bktIdx][cellIdx].first==key){
                uint32_t newVal=floor(updateValue);
                double temp=((double)rand())/RAND_MAX;
                if(temp<(updateValue-newVal)){
                    newVal+=1;
                }

                bucketArray[bktIdx][cellIdx].second+=newVal;
                return true;
            }
            else{
                if(minCellIdx==-1 || bucketArray[bktIdx][cellIdx].second<minCellVal){
                    minCellIdx=cellIdx;
                    minCellVal=bucketArray[bktIdx][cellIdx].second;
                }
            }
        }

        uint64_t randVal=((uint64_t)eleHashValue)*(minCellVal+1);
        if(randVal<=4294967295){
            bucketArray[bktIdx][minCellIdx].first=key;
            bucketArray[bktIdx][minCellIdx].second=minCellVal+1;
            return true;
        }
        return false;
    }
    
public:
    RGS(uint32_t d, uint32_t s, uint32_t m, uint32_t lam, uint32_t w, uint32_t);
    ~RGS() {
        delete[] regArray;
        delete[] IDs;
        delete[] Cards;
    }
    void insert(uint32_t key, uint32_t ele, uint8_t _period);
    std::unordered_set<uint32_t> detect(uint32_t thr);
    void clean();
    uint32_t query(uint32_t key);
    void post_process();
};

#endif //RGS_H
