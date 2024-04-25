#include "platform.h"
#include "cpp_tiles_wrapper.h"

#if ON_TILE(3)
#include <xs1.h>
#include <xcore/channel.h>
#include <stdio.h>


int robot_task_wrapperB(){

    cpp_mainB();
    return 0;

}

#endif