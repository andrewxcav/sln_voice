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
    port_t p_miso  = XS1_PORT_1P;
    port_t p_ss = XS1_PORT_1A; //Active Low CHIP SELECT
    port_t p_sclk = XS1_PORT_1C; //SPI CLK
    port_t p_mosi = XS1_PORT_1D;
    xclock_t cb = XS1_CLKBLK_1;
    port_t led = XS1_PORT_4F;

    int cpp_mainA(chanend_t cVoice, chanend_t cB){
        uint32_t test = 0xFEEDF00D;
        chan_out_word(cB, test);
        std::cout << "Hello A" << std::endl; 

    }
#endif

#if ON_TILE(3)
    int cpp_mainB(chanend_t cA){
        uint32_t test;
        test = chan_in_word(cA);
        std::cout << "Hello B" << std::endl; 
    }
#endif