// Copyright 2022-2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/* System headers */
#include <platform.h>
#include <xs1.h>
#include <xcore/channel.h>

/* FreeRTOS headers */
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "queue.h"
#include "event_groups.h"
#include "rtos_intertile.h"

/* Library headers */
#include "rtos_printf.h"
#include "rtos_mic_array.h"

/* App headers */
#include "app_conf.h"
#include "platform/platform_conf.h"
#include "platform/platform_init.h"
#include "platform/driver_instances.h"
#include "audio_pipeline.h"
#include "intent_engine/intent_engine.h"
#include "fs_support.h"
#include "gpio_ctrl/gpi_ctrl.h"
#include "gpio_ctrl/leds.h"
#include "intent_handler/intent_handler.h"

#if appconfI2S_ENABLED
#include "src.h"
#endif

#if appconfRECOVER_MCLK_I2S_APP_PLL
/* Config headers for sw_pll */
#include "sw_pll.h"
#endif

#ifndef MEM_ANALYSIS_ENABLED
#define MEM_ANALYSIS_ENABLED 0
#endif


#if appconfI2S_ENABLED && (appconfI2S_MODE == appconfI2S_MODE_SLAVE)
void i2s_slave_intertile()
{
    int32_t tmp[appconfAUDIO_PIPELINE_FRAME_ADVANCE][appconfAUDIO_PIPELINE_CHANNELS];
    while(1) {
        memset(tmp, 0x00, sizeof(tmp));

        size_t bytes_received = 0;
        bytes_received = rtos_intertile_rx_len(
                intertile_ctx,
                appconfI2S_OUTPUT_SLAVE_PORT,
                portMAX_DELAY);

        xassert(bytes_received == sizeof(tmp));

        rtos_intertile_rx_data(
                intertile_ctx,
                tmp,
                bytes_received);

        rtos_i2s_tx(i2s_ctx,
                    (int32_t*) tmp,
                    appconfAUDIO_PIPELINE_FRAME_ADVANCE,
                    portMAX_DELAY);
    }
}
#endif

#if appconfRECOVER_MCLK_I2S_APP_PLL
void sw_pll_control(void *args)
{

    while(1)
    {
        sw_pll_ctx_t* i2s_callback_args = (sw_pll_ctx_t*) args;
        port_clear_buffer(i2s_callback_args->p_bclk_count);
        port_in(i2s_callback_args->p_bclk_count);
        uint16_t mclk_pt = port_get_trigger_time(i2s_callback_args->p_mclk_count);
        uint16_t bclk_pt = port_get_trigger_time(i2s_callback_args->p_bclk_count);

        sw_pll_lut_do_control(i2s_callback_args->sw_pll, mclk_pt, bclk_pt);
    }
}
#endif

void audio_pipeline_input(void *input_app_data,
                        int32_t **input_audio_frames,
                        size_t ch_count,
                        size_t frame_count)
{
    (void) input_app_data;
    int32_t **mic_ptr = (int32_t **)(input_audio_frames + (2 * frame_count));

    static int flushed;
    while (!flushed) {
        size_t received;
        received = rtos_mic_array_rx(mic_array_ctx,
                                     mic_ptr,
                                     frame_count,
                                     0);
        if (received == 0) {
            rtos_mic_array_rx(mic_array_ctx,
                              mic_ptr,
                              frame_count,
                              portMAX_DELAY);
            flushed = 1;
        }
    }

    /*
     * NOTE: ALWAYS receive the next frame from the PDM mics,
     * even if USB is the current mic source. The controls the
     * timing since usb_audio_recv() does not block and will
     * receive all zeros if no frame is available yet.
     */
    rtos_mic_array_rx(mic_array_ctx,
                      mic_ptr,
                      frame_count,
                      portMAX_DELAY);

}

int audio_pipeline_output(void *output_app_data,
                        int32_t **output_audio_frames,
                        size_t ch_count,
                        size_t frame_count)
{
    (void) output_app_data;

#if appconfINTENT_ENABLED

    int32_t ww_samples[appconfAUDIO_PIPELINE_FRAME_ADVANCE];
    for (int j=0; j<appconfAUDIO_PIPELINE_FRAME_ADVANCE; j++) {
        /* ASR output is first */
        ww_samples[j] = (uint32_t) *(output_audio_frames+j);
    }

    intent_engine_sample_push(ww_samples,
                              frame_count);
#endif

    return AUDIO_PIPELINE_FREE_FRAME;
}

void vApplicationMallocFailedHook(void)
{
    rtos_printf("Malloc Failed on tile %d!\n", THIS_XCORE_TILE);
    xassert(0);
    for(;;);
}

#if MEM_ANALYSIS_ENABLED
static void mem_analysis(void)
{
    for (;;) {
        rtos_printf("Tile[%d]:\n\tMinimum heap free: %d\n\tCurrent heap free: %d\n", THIS_XCORE_TILE, xPortGetMinimumEverFreeHeapSize(), xPortGetFreeHeapSize());
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
#endif

void startup_task(void *arg)
{
    rtos_printf("Startup task running from tile %d on core %d\n", THIS_XCORE_TILE, portGET_CORE_ID());

    platform_start();

#if ON_TILE(I2S_TILE_NO) && appconfI2S_ENABLED && (appconfI2S_MODE == appconfI2S_MODE_SLAVE)

    xTaskCreate((TaskFunction_t) i2s_slave_intertile,
                "i2s_slave_intertile",
                RTOS_THREAD_STACK_SIZE(i2s_slave_intertile),
                NULL,
                appconfAUDIO_PIPELINE_TASK_PRIORITY,
                NULL);
#if appconfRECOVER_MCLK_I2S_APP_PLL
    xTaskCreate((TaskFunction_t) sw_pll_control,
                "sw_pll_control",
                RTOS_THREAD_STACK_SIZE(sw_pll_control),
                sw_pll_ctx,
                appconfAUDIO_PIPELINE_TASK_PRIORITY,
                NULL);
#endif
#endif

#if ON_TILE(0)
    led_task_create(appconfLED_TASK_PRIORITY, NULL);
#endif

#if ON_TILE(1)
    gpio_gpi_init(gpio_ctx_t0);
#endif

#if ON_TILE(FS_TILE_NO)
    rtos_fatfs_init(qspi_flash_ctx);
    // Setup flash low-level mode
    //   NOTE: must call rtos_qspi_flash_fast_read_shutdown_ll to use non low-level mode calls
    rtos_qspi_flash_fast_read_setup_ll(qspi_flash_ctx);
#endif

#if appconfINTENT_ENABLED && ON_TILE(ASR_TILE_NO) // This block now executes on Tile 0
    QueueHandle_t q_intent = xQueueCreate(appconfINTENT_QUEUE_LEN, sizeof(int32_t));
    intent_handler_create(appconfINTENT_MODEL_RUNNER_TASK_PRIORITY, q_intent);

    // Use the standard create function
    intent_engine_create(appconfINTENT_MODEL_RUNNER_TASK_PRIORITY, q_intent);
#endif

#if ON_TILE(AUDIO_PIPELINE_OUTPUT_TILE_NO)
#if appconfINTENT_ENABLED
    // Wait until the intent engine is initialized before starting the
    // audio pipeline.
    intent_engine_ready_sync();
#endif
    audio_pipeline_init(NULL, NULL);
#endif

#if MEM_ANALYSIS_ENABLED
    mem_analysis();
#else
    vTaskSuspend(NULL);
    while(1){;} /* Trap */
#endif
}

void vApplicationMinimalIdleHook(void)
{
    rtos_printf("idle hook on tile %d core %d\n", THIS_XCORE_TILE, rtos_core_id_get());
    asm volatile("waiteu");
}

void tile_common_init(chanend_t c_rpc)
{
    platform_init(c_rpc);
    chanend_free(c_rpc);

    xTaskCreate((TaskFunction_t) startup_task,
                "startup_task",
                RTOS_THREAD_STACK_SIZE(startup_task),
                NULL,
                appconfSTARTUP_TASK_PRIORITY,
                NULL);

    rtos_printf("start scheduler on tile %d\n", THIS_XCORE_TILE);
    vTaskStartScheduler();
}


#if ON_TILE(0)
void main_tile0(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{

    rtos_printf("main_tile%d: c0=%p c1=%p c2=%p\n", THIS_XCORE_TILE, c0, c1, c2);


    (void) c0;
    (void) c2;
    (void) c3;

    tile_common_init(c1);
}
#endif

#if ON_TILE(1)
void main_tile1(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    rtos_printf("main_tile%d: c0=%p c1=%p c2=%p\n", THIS_XCORE_TILE, c0, c1, c2);

    (void) c1;
    (void) c2;
    (void) c3;

    tile_common_init(c0);
}
#endif
