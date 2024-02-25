#include "AIUtility.h"
#include <ncnn/net.h>
bool AIUtility::hasGPU(){
    return ncnn::get_gpu_count() > 0;
}