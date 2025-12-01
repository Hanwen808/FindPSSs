#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unordered_map>
#include <algorithm>
#include <curand.h>
#include <curand_kernel.h>
#include <unordered_set>
using namespace std;

#define mix(a,b,c) \
{ \
a -= b; a -= c; a ^= (c>>13); \
b -= c; b -= a; b ^= (a<<8); \
c -= a; c -= b; c ^= (b>>13); \
a -= b; a -= c; a ^= (c>>12);  \
b -= c; b -= a; b ^= (a<<16); \
c -= a; c -= b; c ^= (b>>5); \
a -= b; a -= c; a ^= (c>>3);  \
b -= c; b -= a; b ^= (a<<10); \
c -= a; c -= b; c ^= (b>>15); \
}

#define MIN(a, b) ((a)<(b)?(a):(b))

inline cudaError_t checkCuda(cudaError_t result) {
#if defined(DEBUG) || defined(_DEBUG)
    if (result != cudaSuccess) {
        fprintf(stderr, "CUDA Runtime Error: %s\n", cudaGetErrorString(result));
        assert(result == cudaSuccess);
    }
#endif
    return result;
}

__host__ __device__
uint32_t hash1(uint32_t key) {
    return (key * 2654435761u) >> 15;
}

__host__ __device__ uint32_t hash2(uint32_t key) {
    //register ub4 a,b,c,len;
    uint32_t a,b,c;
    uint32_t len = 4;
    char* str = (char*)&key;
    //  uint32_t initval = 0;
    /* Set up the internal state */
    //len = length;
    a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
    c = 8311;//prime32[1000];         /* the previous hash value */

    /*---------------------------------------- handle most of the key */
    while (len >= 12)
    {
        a += (str[0] +((uint32_t)str[1]<<8) +((uint32_t)str[2]<<16) +((uint32_t)str[3]<<24));
        b += (str[4] +((uint32_t)str[5]<<8) +((uint32_t)str[6]<<16) +((uint32_t)str[7]<<24));
        c += (str[8] +((uint32_t)str[9]<<8) +((uint32_t)str[10]<<16)+((uint32_t)str[11]<<24));
        mix(a,b,c);
        str += 12; len -= 12;
    }

    /*------------------------------------- handle the last 11 bytes */
    c += len;
    switch(len)              /* all the case statements fall through */
    {
        case 11: c+=((uint32_t)str[10]<<24);
        // fall through
        case 10: c+=((uint32_t)str[9]<<16);
        // fall through
        case 9 : c+=((uint32_t)str[8]<<8);
        /* the first byte of c is reserved for the length */
        // fall through
        case 8 : b+=((uint32_t)str[7]<<24);
        // fall through
        case 7 : b+=((uint32_t)str[6]<<16);
        // fall through
        case 6 : b+=((uint32_t)str[5]<<8);
        // fall through
        case 5 : b+=str[4];
        // fall through
        case 4 : a+=((uint32_t)str[3]<<24);
        // fall through
        case 3 : a+=((uint32_t)str[2]<<16);
        // fall through
        case 2 : a+=((uint32_t)str[1]<<8);
        // fall through
        case 1 : a+=str[0];
        /* case 0: nothing left to add */
    }
    mix(a,b,c);
    /*-------------------------------------------- report the result */
    return c;
}

const int REG_LEN = 419430;
const int CELL_NUM = 8738;
const int BLOCK_SIZE = 32;

__device__ uint8_t kernel_R[REG_LEN];
__device__ float kernel_prob;
__device__ uint32_t kernel_cur_seq;
__device__ uint32_t kernel_IDs[CELL_NUM * 3];
__device__ uint8_t kernel_Weights[CELL_NUM * 3];
__device__ uint8_t kernel_Times[CELL_NUM * 3];
__device__ float kernel_Cards[CELL_NUM * 3];
__device__ uint32_t mutex[CELL_NUM];

uint32_t IDs[CELL_NUM * 3];
uint8_t Weights[CELL_NUM * 3];
uint8_t Times[CELL_NUM * 3];
float Cards[CELL_NUM * 3];

curandGenerator_t gen;

void readFile(char* filename, uint32_t *len, uint32_t** keys, uint32_t** eles) {
    FILE * f = fopen(filename, "rb");
    fseek(f, 0L, SEEK_END);
    uint32_t filesize = ftell(f);
    rewind(f);
    //printf("%d\n", filesize);
    if (filesize % 26) {
        printf("The file size is wrong!\n");
        fclose(f);
        return ;
    }
    *len = filesize / 26;
    //printf("The file has %d packets.\n", *len);
    checkCuda( cudaMallocHost((void**)keys, sizeof(uint32_t) * (*len)) );
    checkCuda( cudaMallocHost((void**)eles, sizeof(uint32_t) * (*len)) );
    for (int i = 0; i < *len; ++i) {
        char tmp_key[13];
        char tmp_ele[13];
        fread(tmp_key, 13, sizeof(char), f);
        fread(tmp_ele, 13, sizeof(char), f);
        *(*keys + i) = *(uint32_t*) tmp_key;
        *(*eles + i) = *(uint32_t*) tmp_ele;
    }
    fclose(f);
}

__global__ void kernel_init(uint8_t _period) {
    if (_period == 0) {
        kernel_prob = 1.0;
        kernel_cur_seq = 0;
        for (int i = 0; i < REG_LEN; ++i) {
            kernel_R[i] = 0;
        }
        for (int i = 0; i < CELL_NUM * 3; i++) {
            kernel_IDs[i] = 0;
            kernel_Weights[i] = 0;
            kernel_Times[i] = 0;
            kernel_Cards[i] = 0.0;
        }
        for (int i = 0; i < CELL_NUM; i++) {
            mutex[i] = 0;
        }
    } else {
       kernel_prob = 1.0;
        for (int i = 0; i < REG_LEN; ++i) {
            kernel_R[i] = 0;
        }
        for (int i = 0; i < CELL_NUM; i++) {
            mutex[i] = 0;
        }
    }
}

__device__ inline void kernel_update(uint32_t key, uint32_t ele, uint8_t _period) {
    atomicAdd(&kernel_cur_seq, 1);
    uint32_t leftmost = 0;
    uint32_t R_index = (hash1(key) ^ hash1(ele)) % REG_LEN;
    uint32_t hash_key_bits = hash2(key) ^ hash2(ele);
    while (hash_key_bits) {
        hash_key_bits >>= 1;
        leftmost ++;
    }
    leftmost = 32 - leftmost + 1;
    if (kernel_R[R_index] < leftmost) {
        float inc = 1.0 / kernel_prob;
        kernel_prob += -(1.0 / REG_LEN) * (1.0 / (1<<kernel_R[R_index])) + (1.0 / REG_LEN) * (1.0 / (1<<leftmost));
        kernel_R[R_index] = leftmost;
        //atomicSub(&kernel_cnt_zero, 1);
        uint32_t index = hash2(key) % CELL_NUM;
        int lock_requests = 10;
        int next = 1;
        while (next && lock_requests --) {
            int status = atomicCAS(mutex + index, 0, 1);
            if (status == 0) {
                int empty_index = -1;
                int min_index = -1;
                float min_value = (1<<30)-1;
                for (int j = 0; j < 3; j++) {
                    if (kernel_IDs[j * CELL_NUM + index] == 0) {
                        empty_index = j * CELL_NUM + index;
                    } else {
                        if (kernel_IDs[j * CELL_NUM + index] == key) {
                            if (kernel_Times[j * CELL_NUM + index] == _period) {
                                kernel_Cards[j * CELL_NUM + index] += inc;
                            } else {
                                if (kernel_Cards[j * CELL_NUM + index] >= 20) {
                                    kernel_Weights[j * CELL_NUM + index] += 1;
                                }
                                kernel_Cards[j * CELL_NUM + index] = inc;
                                kernel_Times[j * CELL_NUM + index] = _period;
                            }
                            status = atomicCAS(mutex + index, 1, 0);
                            return;
                        } else {
                            if (min_index == -1) {
                                min_index = j * CELL_NUM + index;
                                min_value = kernel_Cards[j * CELL_NUM + index];
                            } else {
                                if (kernel_Cards[j * CELL_NUM + index] < min_value) {
                                    min_index = j * CELL_NUM + index;
                                    min_value = kernel_Cards[j * CELL_NUM + index];
                                }
                            }
                        }
                    }
                }
                if (empty_index != -1) {
                    kernel_IDs[empty_index] = key;
                    kernel_Cards[empty_index] = inc;
                    kernel_Times[empty_index] = _period;
                    kernel_Weights[empty_index] = 0;
                } else {
                    double Pr = 0.0;
                    int delta_t = static_cast<int>(_period - kernel_Times[min_index]);
                    if (delta_t == 0 && kernel_Weights[min_index] != 0)
                        return;
                    float reinforce_inc = 1.0 * inc * (1<<delta_t);
                    int delta_w = static_cast<int>(kernel_Weights[min_index]);
                    float reinforce_x = 1.0 * kernel_Cards[min_index] * (1<<delta_w);
                    Pr = reinforce_inc / (reinforce_inc + reinforce_x);
                    float cuda_rand = 1.0 * (hash1(kernel_cur_seq) % 10000);
                    if (cuda_rand <= Pr * 10000.0) {
                        kernel_IDs[min_index] = key;
                        kernel_Cards[min_index] = inc;
                        kernel_Times[min_index] = _period;
                        kernel_Weights[min_index] = 0;
                    }
                }
            }
            //status = atomicCAS(mutex + index, 1, 0);
            atomicExch(mutex + index, 0);
            next = 0;
        }
    }
}

__global__ void kernel_insert(uint32_t* keys, uint32_t* eles, uint32_t len, uint8_t _period) {
    const uint32_t id = threadIdx.x + blockIdx.x * blockDim.x;
    if (id >= len) return;
    uint32_t key = keys[id];
    uint32_t ele = eles[id];
    kernel_update(key, ele, _period);
}

double expr_result[10];
int expr_pos = 0;

void update(uint32_t *keys, uint32_t* eles, uint32_t len, uint8_t _period) {
    uint32_t * kernel_keys;
    uint32_t * kernel_eles;
    checkCuda( cudaMalloc((void**)&kernel_keys, sizeof(uint32_t) * len) );
    checkCuda( cudaMalloc((void**)&kernel_eles, sizeof(uint32_t) * len) );
    cudaEvent_t startEvent, stopEvent;
    checkCuda( cudaEventCreate(&startEvent) );
    checkCuda( cudaEventCreate(&stopEvent) );
    int batch_num = (len + BATCH_SIZE - 1) / BATCH_SIZE;
    cudaStream_t stream[batch_num];
    for (int i = 0; i < batch_num; ++i)
        checkCuda( cudaStreamCreate(&stream[i]) );

    checkCuda( cudaEventRecord(startEvent, 0) );
    for (uint32_t i = 0, left = len; i < len; i += BATCH_SIZE, left -= BATCH_SIZE) {
        uint32_t size = MIN(left, BATCH_SIZE);
        //printf("batch size=%d\n", size);
        checkCuda( cudaMemcpyAsync(kernel_keys + i, keys + i, sizeof(uint32_t) * size, cudaMemcpyHostToDevice, stream[i % BATCH_SIZE]) );
        checkCuda( cudaMemcpyAsync(kernel_eles + i, eles + i, sizeof(uint32_t) * size, cudaMemcpyHostToDevice, stream[i % BATCH_SIZE]) );
        kernel_insert<<<(size + BLOCK_SIZE - 1) / BLOCK_SIZE, BLOCK_SIZE, 0, stream[i % BATCH_SIZE]>>>(kernel_keys + i, kernel_eles + i, size, _period);
    }

    checkCuda( cudaEventRecord(stopEvent, 0) );
    checkCuda( cudaEventSynchronize(stopEvent) );
    float ms;
    checkCuda( cudaEventElapsedTime(&ms, startEvent, stopEvent) );

    expr_result[expr_pos++] = (len / (ms / 1000)) / 1000000;
    cudaFree(kernel_keys);
    cudaFree(kernel_eles);
}

void true_result_current(uint32_t* keys, uint32_t* eles, uint32_t* len, uint8_t _period, unordered_map<uint8_t, unordered_map<uint32_t, unordered_set<uint32_t>>>& timeFlowMap) {
    for (int i = 0; i < *len; ++i) {
        timeFlowMap[_period][keys[i]].insert(eles[i]);
    } 
}

unordered_set<uint32_t> get_true_results(unordered_map<uint8_t, unordered_map<uint32_t, unordered_set<uint32_t>>> timeFlowMap) {
    unordered_set<uint32_t> res;
    unordered_map<uint32_t, uint32_t> flowOccurs;
    for (auto iter = timeFlowMap.begin(); iter != timeFlowMap.end(); iter ++) {
        for (auto iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2 ++) {
            if (iter2->second.size() >= 20) {
                flowOccurs[iter2->first] ++;
            }
        }
    }
    for (auto iter = flowOccurs.begin(); iter != flowOccurs.end(); iter ++) {
        if (iter->second >= 3) {
            res.insert(iter->first);
        }
    }
    return res;
}

unordered_set<uint32_t> query() {
    unordered_set<uint32_t> est;
    checkCuda( cudaMemcpyFromSymbol(IDs, kernel_IDs, sizeof(uint32_t) * CELL_NUM * 3) );
    checkCuda( cudaMemcpyFromSymbol(Weights, kernel_Weights, sizeof(uint8_t) * CELL_NUM * 3) );
    checkCuda( cudaMemcpyFromSymbol(Cards, kernel_Cards, sizeof(float) * CELL_NUM * 3) );
    checkCuda( cudaMemcpyFromSymbol(Times, kernel_Times, sizeof(uint8_t) * CELL_NUM * 3) );
    for (int i = 0; i < CELL_NUM * 3; ++i) {
        if (Weights[i] >= 3 || (Weights[i] >= 2 && Cards[i] >= 20)) {
            est.insert(IDs[i]);
        }
    }
    return est;
}

void experiment_filename(char* filename, uint8_t _period, unordered_map<uint8_t, unordered_map<uint32_t, unordered_set<uint32_t>>>& flowTimeMap) {
    uint32_t len;
    uint32_t* keys;
    uint32_t* eles;
    readFile(filename, &len, &keys, &eles);
    true_result_current(keys, eles, &len, _period, flowTimeMap);
    //printf("%d\n", flowTimeMap[_period].size());
    kernel_init<<<1, 1>>>(_period);
    //printf("The file has %d packets.\n", len);
    //printf("The first packet is %d %d.\n", keys[0], eles[0]);
    update(keys, eles, len, _period);
    cudaFreeHost(keys);
    cudaFreeHost(eles);
}

int main() {
    char filename[100];
    printf("%d ", BATCH_SIZE);
    unordered_map<uint8_t, unordered_map<uint32_t, unordered_set<uint32_t>>> flowTimeMap;
    for (int i = 0; i < 10; ++i) {
        sprintf(filename, "./data/%d.dat", i);
        experiment_filename(filename, i + 1, flowTimeMap);
    }
    unordered_set<uint32_t> res = get_true_results(flowTimeMap);
    //printf("There are %d persistent super-spreaders.\n", res.size());
    unordered_set<uint32_t> est = query();
    //printf("There are %d predicted pss flows.\n", est.size());
    unordered_set<uint32_t> TP_set;
    unordered_set<uint32_t> FP_set;
    unordered_set<uint32_t> FN_set;
    for (auto iter = res.begin(); iter != res.end(); iter ++) {
        if (est.find(*iter) != est.end())
            TP_set.insert(*iter);
        else
            FN_set.insert(*iter);
    }
    for (auto iter = est.begin(); iter != est.end(); iter ++) {
        if (res.find(*iter) == res.end())
            FP_set.insert(*iter);
    }
    double precision = (1.0 * TP_set.size()) / (1.0 * TP_set.size() + 1.0 * FP_set.size());
    double recall = ((1.0 * TP_set.size()) / (1.0 * TP_set.size() + 1.0 * FN_set.size()));
    double F1_score = (2 * precision * recall) / (precision + recall);
    printf("P=%lf, R=%lf, F1=%lf\n", precision, recall, F1_score);
    sort(expr_result, expr_result + 10);
    printf("%lf %lf %lf\n",
        (expr_result[0] + expr_result[1]) / 2,
        (expr_result[4] + expr_result[5]) / 2,
        (expr_result[8] + expr_result[9]) / 2
    );
    return 0;
}