#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string.h>
#include <unordered_map>
#include <vector>
#include <chrono>
#include "Headers/Sketch.h"
#include "Headers/PSS_BM.h"
#include "Headers/PSS_HLL.h"
#include "Headers/KTSketch.h"
#include "Headers/HLLsampler.h"
#include "Headers/SpreadSketch.h"
#include "Headers/FreeRS.h"
#define KEY_SIZE 16

using namespace std;

uint32_t convertIPv4ToUint32(char* ipAddress) {
    uint32_t result = 0;
    int octet = 0;
    char ipCopy[KEY_SIZE];
    strncpy(ipCopy, ipAddress, sizeof(ipCopy) - 1);
    ipCopy[sizeof(ipCopy) - 1] = '\0';
    char* token = strtok(ipCopy, ".");
    while (token != nullptr) {
        octet = std::stoi(token);
        result = (result << 8) + octet;
        token = strtok(nullptr, ".");
    }
    return result;
}

// Updating Sketch and testing the processing throughput.
double processPackets(Sketch* skt, vector<pair<pair<uint32_t, uint32_t>, uint8_t>>& dataset) {
    uint32_t pre_period, cur_period;
    double throughput_sum = 0.0;
    clock_t start = clock(); // The start time of processing packet stream
    uint32_t count = 0;
    uint32_t cnt_thr = 0;
    for (int i = 0; i < dataset.size(); i++) {
        count ++;
        if (i == 0) {
            pre_period = dataset[i].second;
            cur_period = dataset[i].second;
        } else {
            cur_period = dataset[i].second;
            if (cur_period != pre_period) {
                clock_t current = clock(); // The end time of processing packet stream
                cout << count << " lines: have used " << ((double)current - start) / CLOCKS_PER_SEC << " seconds" << endl;
                double throughput = (count / 1000000.0) / (((double)current - start) / CLOCKS_PER_SEC);
                std::cout << throughput << std::endl;
                throughput_sum += throughput;
                cnt_thr ++;
                skt->clean();
                start = clock();
                pre_period = cur_period;
                count = 0;
            }
        }
        skt->insert(dataset[i].first.first, dataset[i].first.second, dataset[i].second);
    }
    //skt->clean();
    return throughput_sum / (1.0 * cnt_thr);
}

void getDataSet(string dataDir, vector<pair<pair<uint32_t, uint32_t>,uint8_t>>& dataset,
                std::unordered_map<uint8_t, std::unordered_map<uint32_t, std::unordered_set<uint32_t>>>& realFlowInfo, const char* dataFileName, uint8_t period_num)
{
    uint32_t src_ip_int, dst_ip_int;
    string line, source, destination;
    clock_t start = clock();
    fstream fin(dataFileName);
    cout << dataFileName << endl;
    while (fin.is_open() && fin.peek() != EOF) {
        getline(fin, line);
        stringstream ss(line.c_str());
        ss >> source >> destination;
        src_ip_int = convertIPv4ToUint32((char*) source.c_str());
        dst_ip_int = convertIPv4ToUint32((char*) destination.c_str());
        dataset.push_back(make_pair(make_pair(src_ip_int, dst_ip_int), period_num));
        realFlowInfo[period_num][src_ip_int].insert(dst_ip_int);
        if (dataset.size() % 5000000 == 0) {
            clock_t current = clock();
            cout << "have added " << dataset.size() << " packets, have used " << ((double)current - start) / CLOCKS_PER_SEC << " seconds." << endl;
        }
    }
    if (!fin.is_open()) {
        cout << "dataset file " << dataFileName << "closed unexpectedlly"<<endl;
        exit(-1);
    } else
        fin.close();
    clock_t current = clock();
    cout << "have added " << dataset.size() << " packets, have used " << ((double)current - start) / CLOCKS_PER_SEC << " seconds" << endl;
}

void saveResults(string outputFilePath, Sketch* skt, unordered_map<uint8_t, unordered_map<uint32_t, std::unordered_set<uint32_t>>>& realFlowInfo, uint32_t T, uint32_t K, string name, double throughput) {
    ofstream fout;
    std::unordered_set<uint32_t> pred_kt;
    clock_t start = clock();
    skt->clean();
    pred_kt = skt->detect(K);
    clock_t current = clock();
    double detection_time = ((double)current - start) / CLOCKS_PER_SEC;
    
    std::unordered_set<uint32_t> real_kt;
    std::unordered_map<uint32_t, uint32_t> flowMap;
    for (auto iter = realFlowInfo.begin(); iter != realFlowInfo.end(); iter ++) {
        for (auto iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2 ++) {
            if (iter2->second.size() >= T) {
                flowMap[iter2->first] += 1;
            }
        }
    }
    double AAE = 0.0, ARE = 0.0;
    uint32_t cnt = 0;
    for (auto iter = flowMap.begin();  iter != flowMap.end(); ++iter) {
        if (iter->second >= K) {
            real_kt.insert(iter->first);
            uint32_t r = iter->second;
            uint32_t e = skt->query(iter->first);
            AAE += std::abs(1.0 * r - 1.0 * e);
            ARE += std::abs(1.0 * r - 1.0 * e) / (1.0 * r);
            cnt ++;
        }
    }
    AAE = AAE / (1.0 * cnt);
    ARE = ARE / (1.0 * cnt);
    fout.open(outputFilePath, ios::out);
    std::unordered_set<uint32_t> TP_set;
    std::unordered_set<uint32_t> FP_set;
    std::unordered_set<uint32_t> FN_set;
    for (auto iter = real_kt.begin(); iter != real_kt.end(); iter ++) {
        if (pred_kt.find(*iter) != pred_kt.end())
            TP_set.insert(*iter);
        else
            FN_set.insert(*iter);
    }
    for (auto iter = pred_kt.begin(); iter != pred_kt.end(); iter ++) {
        if (real_kt.find(*iter) == real_kt.end())
            FP_set.insert(*iter);
    }
    double precision = (1.0 * TP_set.size()) / (1.0 * TP_set.size() + 1.0 * FP_set.size());
    double recall = ((1.0 * TP_set.size()) / (1.0 * TP_set.size() + 1.0 * FN_set.size()));
    double F1_score = (2 * precision * recall) / (precision + recall);

    fout << "Precision: " << precision << ", recall: " << recall << ", F1 score: " << F1_score << ", Throughput:" << throughput << ", Detection Time(s):" << detection_time << ", AAE:" << AAE << ", ARE:" << ARE << "\n";
    
    if (!fout.is_open())
        cout << outputFilePath << " closed unexpectedlly";
    else
        fout.close();
    cout << "";
}

void run(uint32_t mem, uint32_t detectT, uint32_t detectK, uint8_t total_periods, string name) {
    //prepare the dataset
    cout << "prepare the dataset" << endl;
    string dataDir = R"(../../MAWI25/)";
    vector<pair<pair<uint32_t, uint32_t>, uint8_t>> dataset;
    std::unordered_map<uint8_t, std::unordered_map<uint32_t, std::unordered_set<uint32_t>>> realFlowInfo;
    for (int i = 0; i < total_periods; ++i) {
        std::ostringstream filename_stream;
        filename_stream << dataDir << std::setfill('0') << i << ".txt";
        std::string filename_str = filename_stream.str();
        const char* filename = filename_str.c_str();
        getDataSet(dataDir, dataset, realFlowInfo, filename, i);
    }
    cout << endl;
    
    if (strcmp(name.c_str(), "PSSBM") == 0) {
        float ratio = 0.5;
        uint32_t mem1 = static_cast<uint32_t>(mem * 1024 * ratio);
        uint32_t mem2 = static_cast<uint32_t>(mem * 1024 * (1.0 - ratio));
        uint32_t m = static_cast<uint32_t>(mem1 * 8);
        uint32_t d = 3;
        uint32_t w = static_cast<uint32_t>(mem2 / d / 10);
        PSSBM *skt = new PSSBM(m, d, w, detectT);
        cout << endl;
        cout << "Start processing..." << endl;
        double throughput = processPackets(skt, dataset);
        //save the result in files
        cout << endl;
        cout << "save the result in txt ..." << endl;
        string outputFilePath = "../CardExp/PSS_BM_" + std::to_string(mem) + "_" + std::to_string(detectT) + "_" + std::to_string(detectK) + ".txt";
        saveResults(outputFilePath, skt, realFlowInfo, detectT, detectK, name, throughput);
    }
    
    if (strcmp(name.c_str(), "PSSHLL") == 0) {
        float ratio = 0.5;
        uint32_t mem1 = static_cast<uint32_t>(mem * 1024 * ratio / 5);
        uint32_t mem2 = static_cast<uint32_t>(mem * 1024 * (1.0 - ratio));
        uint32_t m = static_cast<uint32_t>(mem1 * 8);
        uint32_t d = 3;
        uint32_t w = static_cast<uint32_t>(mem2 / d / 10);
        PSSHLL *skt = new PSSHLL(m, d, w, detectT);
        cout << endl;
        cout << "Start processing..." << endl;
        double throughput = processPackets(skt, dataset);
        //save the result in files
        cout << endl;
        cout << "save the result in txt ..." << endl;
        string outputFilePath = "../CardExp/PSS_HLL_" + std::to_string(mem) + "_" + std::to_string(detectT) + "_" + std::to_string(detectK) + ".txt";
        saveResults(outputFilePath, skt, realFlowInfo, detectT, detectK, name, throughput);
    }
    
    if (strcmp(name.c_str(), "KTSketch") == 0) {
        int stage1_mem = 1;
        int lenA = stage1_mem * 1024 * 8;
        int kA = 2;
        int stage2_mem = static_cast<int>(((mem - stage1_mem) * (0.667) * 1024));
        int stage3_mem = static_cast<int>(((mem - stage1_mem) * (0.333) * 1024));
        int stage2_mem_B = static_cast<int>(stage2_mem * (0.75));
        int bitmapSizeB = stage2_mem_B * 8;
        int bktSizeB = 8;
        int stage2_mem_H = static_cast<int>(stage2_mem * (0.25));
        int bktNumB = stage2_mem_H / bktSizeB / 12;
        int stage3_mem_B = static_cast<int>(stage3_mem * (0.75));
        int stage3_mem_H = static_cast<int>(stage3_mem * (0.25));
        int lenC = stage3_mem_B * 8;
        int bktSizeC = 8;
        int bktNumC = stage3_mem_H / bktSizeC / 16;
        KTSketch *ktSketch = new KTSketch(lenA, kA, bitmapSizeB, bktNumB, bktSizeB, lenC, bktNumC, bktSizeC, detectT, 0.9);
        cout << endl;
        cout << "Start processing..." << endl;
        double throughput = processPackets(ktSketch, dataset);
        //save the result in files
        cout << endl;
        cout << "save the result in txt ..." << endl;
        string outputFilePath = "../CardExp/KTSketch_" + std::to_string(mem) + "_" + std::to_string(detectT) + "_" + std::to_string(detectK) + ".txt";
        saveResults(outputFilePath, ktSketch, realFlowInfo, detectT, detectK, name, throughput);
    }
    
    if (strcmp(name.c_str(), "HLLsampler") == 0) {
        uint32_t d = 4;
        uint32_t n = 128;
        uint32_t w = static_cast<uint32_t>(mem * 1024 / d / 88);
        HLLsampler *skt = new HLLsampler(d, w, n, detectT);
        cout << endl;
        cout << "Start processing..." << endl;
        double throughput = processPackets(skt, dataset);
        //save the result in files
        cout << endl;
        cout << "save the result in txt ..." << endl;
        string outputFilePath = "../CardExp/HLLsampler_" + std::to_string(mem) + "_" + std::to_string(detectT) + "_" + std::to_string(detectK) + ".txt";
        saveResults(outputFilePath, skt, realFlowInfo, detectT, detectK, name, throughput);
    }
    
    if (strcmp(name.c_str(), "SpreadSketch") == 0) {
        uint32_t d = 3;
        uint32_t k = 10;
        uint32_t m = 128;
        uint32_t T = 10;
        uint32_t w = static_cast<uint32_t>(mem * 1024 / d / 61);
        SpreadSketch *skt = new SpreadSketch(d, w, m, k, T, detectT);
        cout << endl;
        cout << "Start processing..." << endl;
        double throughput = processPackets(skt, dataset);
        //save the result in files
        cout << endl;
        cout << "save the result in txt ..." << endl;
        string outputFilePath = "../CardExp/SpreadSketch_" + std::to_string(mem) + "_" + std::to_string(detectT) + "_" + std::to_string(detectK) + ".txt";
        saveResults(outputFilePath, skt, realFlowInfo, detectT, detectK, name, throughput);
    }
    
    if (strcmp(name.c_str(), "FreeRS") == 0) {
        float ratio = 0.2;
        uint32_t mem_R = static_cast<uint32_t>(ratio * mem * 1024 * 8);
        uint32_t mem_SS = static_cast<uint32_t>((1-ratio) * mem * 1024 * 8);
        uint32_t m = mem_R / 5;
        uint32_t sslen = mem_SS / 288;
        FreeRS *skt = new FreeRS(sslen, m, detectT);
        cout << endl;
        cout << "Start processing..." << endl;
        double throughput = processPackets(skt, dataset);
        //save the result in files
        cout << endl;
        cout << "save the result in txt ..." << endl;
        string outputFilePath = "../CardExp/FreeRS_" + std::to_string(mem) + "_" + std::to_string(detectT) + "_" + std::to_string(detectK) + ".txt";
        saveResults(outputFilePath, skt, realFlowInfo, detectT, detectK, name, throughput);
    }
}

int main(int argc, char *argv[]) {
    //run(256, 50, 10, 12);
    if (strcmp(argv[1], "PSSBM") == 0) {
       run(std::stoi(argv[2]), std::stoi(argv[3]), std::stoi(argv[4]), std::stoi(argv[5]), "PSSBM"); 
    } else if (strcmp(argv[1], "PSSHLL") == 0) {
        run(std::stoi(argv[2]), std::stoi(argv[3]), std::stoi(argv[4]), std::stoi(argv[5]), "PSSHLL"); 
    } else if (strcmp(argv[1], "KTSketch") == 0) {
        run(std::stoi(argv[2]), std::stoi(argv[3]), std::stoi(argv[4]), std::stoi(argv[5]), "KTSketch"); 
    } else if (strcmp(argv[1], "HLLsampler") == 0) {
        run(std::stoi(argv[2]), std::stoi(argv[3]), std::stoi(argv[4]), std::stoi(argv[5]), "HLLsampler"); 
    } else if (strcmp(argv[1], "SpreadSketch") == 0) {
        run(std::stoi(argv[2]), std::stoi(argv[3]), std::stoi(argv[4]), std::stoi(argv[5]), "SpreadSketch"); 
    } else if (strcmp(argv[1], "FreeRS") == 0) {
        run(std::stoi(argv[2]), std::stoi(argv[3]), std::stoi(argv[4]), std::stoi(argv[5]), "FreeRS");
    }
    return 0;
}
