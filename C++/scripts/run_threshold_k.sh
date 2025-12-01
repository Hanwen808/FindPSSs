#!/bin/bash

# Step 0: 设置变量
TARGET_NAME="KKK"
MAIN_FILE="threshold_k.cpp"

# Step 1: 创建并进入构建目录
mkdir -p build
cd build

# Step 2: 生成 Makefile 并编译项目
cmake .. -DTARGET_NAME=${TARGET_NAME} -DMAIN_FILE=${MAIN_FILE}
make -j$(nproc)

# Step 3: 定义要测试的算法列表
algs=("PSSBM" "PSSHLL" "FreeRS" "SpreadSketch" "HLLsampler" "KTSketch")
echo "开始并行运行 ${TARGET_NAME} 各算法……"

mems=(128)
spreads=(50)
Ks=(2 4 6 8 10)
Totals=(11)
# Step 4: 并行执行每个算法
for alg in "${algs[@]}"; do
    echo ">>> 启动算法：$alg"
    for mem in "${mems[@]}"; do
        for spread in "${spreads[@]}"; do
            for K in "${Ks[@]}"; do
                for total in "${Totals[@]}"; do
                    ./${TARGET_NAME} "$alg" "$mem" "$spread" "$K" "$total" "$alg" > "../logs/kkk_${alg}_${mem}_${spread}_${K}.log" 2>&1 &
                done
            done
        done
    done
done

# 等待所有后台任务完成
wait

echo "所有算法执行完毕。"