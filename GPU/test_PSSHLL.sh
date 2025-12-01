for ((i=1000; i<=10000; i+=1000))  
do  
    nvcc PSSHLL.cu -DBATCH_SIZE=$i -std=c++11 -Wno-deprecated-gpu-targets
    ./a.out
done 