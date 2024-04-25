#include "platform.h"
#include "cpp_tiles_wrapper.h"

#if ON_TILE(2)
#include <xs1.h>
#include <xcore/channel.h>
#include <stdio.h>


int robot_task_wrapperA(){

    cpp_mainA();
    return 0;
    
}

#endif