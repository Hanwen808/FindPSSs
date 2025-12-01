#ifndef KTSKETCH_H
#define KTSKETCH_H
#include "Sketch.h"
#include <algorithm>
#include <iostream>

typedef struct A {
    uint32_t ID;
    float Card;
    float Error;
    A() {
        ID = 0;
        Card = 0.0;
        Error = 0.0;
    }
    bool operator<(const A &y) const {
        return Card < y.Card;
    }

} Cell2;

class Bucket2 {
public:
    // bool comCells(const Cell2* x,const Cell2* y) {
    //     return x->Card < y->Card;
    // }
    uint32_t c;
    Cell2 *cells;
    Bucket2(uint32_t _c) {
        c = _c;
        cells = new Cell2[c];
    }

    void update(uint32_t key, float inc) {
        int empty_loc = -1;
        int min_loc = -1;
        float min_value = INT32_MAX * 1.0;
        for (int i = 0; i < c; ++i) {
            if (cells[i].ID == key) {
                cells[i].Card += inc;
                std::sort(cells, cells + c);
                return;
            } else {
                if (cells[i].ID == 0) {
                    empty_loc = i;
                } else {
                    if (min_loc == -1) {
                        min_loc = i;
                        min_value = cells[i].Card;
                    } else {
                        if (cells[i].Card < min_value) {
                            min_loc = i;
                            min_value = cells[i].Card;
                        }
                    }
                }
            }
        }
        if (empty_loc != -1) {
            cells[empty_loc].ID = key;
            cells[empty_loc].Card = inc;
            cells[empty_loc].Error = 0.0;
        } else {
            double p = 1.0 * inc / (1.0 * inc + cells[min_loc].Card);
            if (1.0 * (rand() % 10000) <= 10000.0 * p) {
                cells[min_loc].ID = key;
                cells[min_loc].Error = cells[min_loc].Card;
                cells[min_loc].Card += inc;
            } else {
                cells[min_loc].Card += inc;
            }
        }
        std::sort(cells, cells + c);
    }

    void clean() {
        delete cells;
        cells = new Cell2[c];
    }
};

typedef struct B {
    uint32_t ID;
    float Card;
    uint32_t n1, n2;
    int weight; // only for test
    B() {
        ID = 0;
        Card = 0.0;
        n1 = 0;
        n2 = 0;
        weight = 0;
    }
    bool operator<(const B &y) const {
        return weight < y.weight;
    }
} Cell3;

class Bucket3 {
public:
    uint32_t c;
    Cell3 *cells;

    // bool comCellsInPart3(const Cell3* x,const Cell3* y) {
    //     return x->weight < y->weight;
    // }

    Bucket3(uint32_t _c) {
        this->c = _c;
        cells = new Cell3[c];
    }

    int update(uint32_t key, float inc) {
        int is_insert = -1;
        for (int i = 0; i < c; ++i) {
            if (cells[i].ID == key) {
                cells[i].Card += inc;
                is_insert = 1;
                break;
            }
        }
        return is_insert;
    }

    void clean() {
        delete cells;
        cells = new Cell3[c];
    }

    void msort() {
        std::sort(cells, cells + c);
    }

    void updateFromStage2(uint32_t key) {
        int empty_loc = -1;
        int min_loc = -1, min_weight = -1;
        for (int i = 0; i < c; i++) {
            if (cells[i].ID == 0) {
                empty_loc = i;
            } else {
                if (min_loc == -1) {
                    min_loc = i;
                    min_weight = cells[i].weight;
                } else {
                    if (cells[i].weight < min_weight) {
                        min_loc = i;
                        min_weight = cells[i].weight;
                    }
                }
            }
        }
        if (empty_loc != -1) {
            cells[empty_loc].ID = key;
            cells[empty_loc].Card = 0.0;
            cells[empty_loc].n1 = 1;
            cells[empty_loc].n2 = 0;
            cells[empty_loc].weight = 1;
        } else {
            cells[min_loc].ID = key;
            cells[min_loc].Card = 0.0;
            cells[min_loc].n1 = 1;
            cells[min_loc].n2 = 0;
            cells[min_loc].weight = 1;
        }
        std::sort(cells, cells + c);
    }
};

class KTSketch : public Sketch {
public:
    // Part 1
    uint32_t M1, d1;
    uint8_t *BloomFilter;
    uint32_t *filter_seeds;

    // Part 2
    uint32_t M2, N2, c2, Z2;
    uint8_t *Bitmap2;
    Bucket2 **H2;
    uint32_t bitmap_hash_seed, summary_hash_seed;

    // Part 3
    uint32_t M3, N3, c3, Z3;
    uint8_t *Bitmap3;
    Bucket3 **H3;

    // other metadata
    float lam;
    uint32_t threshold_t;

    KTSketch(uint32_t M1, uint32_t d1, uint32_t M2, uint32_t N2, uint32_t cellnum2, uint32_t M3, uint32_t N3, uint32_t cellnum3, uint32_t threshold_t, float lam);
    ~KTSketch() {
        delete BloomFilter;
        delete filter_seeds;
        delete Bitmap2;
        delete Bitmap3;
        delete[] H2;
        delete[] H3;
    }

    void insert2(uint32_t key, uint32_t ele);

    void insert3(uint32_t key, uint32_t ele);

    void insert(uint32_t key, uint32_t ele, uint8_t _period);
    void clean();
    std::unordered_set<uint32_t> detect(uint32_t thr);
    uint32_t query(uint32_t key);
    void post_process();
};

#endif //KTSKETCH_H
