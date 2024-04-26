#include "platform.h"
#include "cpp_tiles_wrapper.h"

#include "app_conf.h"
#include "platform/platform_conf.h"
#include "platform/platform_init.h"
#include "platform/driver_instances.h"

#include <xs1.h>
#include <xcore/channel.h>
#include <stdio.h>

#if ON_TILE(2)



    int robot_task_wrapperA(){

        cpp_mainA(interpackage_ctx, intertile23_ctx);
        return 0;
        
    }

#endif


#if ON_TILE(3)



    int robot_task_wrapperB(){

        cpp_mainB(intertile23_ctx);
        return 0;

    }

#endif