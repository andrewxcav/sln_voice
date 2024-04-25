#include <stdio.h>
#include <iomanip> // For std::hex
#include <stdint.h>
#include <xs1.h>
#include <syscall.h>
#include <timer.h>
#include <stdio.h>
#include <platform.h>
#include <xclib.h>
#include <iostream>
#include <xcore/hwtimer.h>
#include <vector>
#include <xcore/parallel.h>
#include <xcore/channel.h>
#include <xcore/channel_streaming.h> // non blocking channel comms
#include <xcore/port.h>
#include <stdexcept>
#include <cmath>
#include <array>
#include <utility> // for std::pair
#include <algorithm>
#include <atomic>

#include <type_traits>  
#include <chrono>


#include "platform.h"
#include "cpp_tiles_wrapper.h"


#if ON_TILE(2)
    int cpp_mainA(){
        std::cout << "Hello A" << std::endl; 
    }
#endif

#if ON_TILE(3)
    int cpp_mainB(){
        std::cout << "Hello B" << std::endl; 
    }
#endif