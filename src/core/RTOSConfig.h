/**
 * @file RTOSConfig.h
 * @brief FreeRTOS configuration parameters
 */

 #ifndef RTOS_CONFIG_H
 #define RTOS_CONFIG_H
 
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 
 // FreeRTOS configuration overrides
 // These settings override the default FreeRTOS configuration for the ESP32
 
 /**
  * @brief Enable SMP (Symmetric Multi-Processing)
  * 
  * This enables the use of both cores on the ESP32.
  */
 #define configUSE_TASK_NOTIFICATIONS      1
 #define configUSE_PREEMPTION              1
 #define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
 #define configUSE_TICKLESS_IDLE           0
 #define configCPU_CLOCK_HZ                (240000000)
 #define configTICK_RATE_HZ                (1000)
 #define configMAX_PRIORITIES              (25)
 #define configMINIMAL_STACK_SIZE          (2048)
 #define configMAX_TASK_NAME_LEN           (16)
 #define configUSE_16_BIT_TICKS            0
 #define configIDLE_SHOULD_YIELD           1
 #define configUSE_MUTEXES                 1
 #define configUSE_RECURSIVE_MUTEXES       1
 #define configUSE_COUNTING_SEMAPHORES     1
 #define configQUEUE_REGISTRY_SIZE         16
 #define configUSE_QUEUE_SETS              1
 #define configUSE_TIME_SLICING            1
 #define configUSE_NEWLIB_REENTRANT        1
 #define configENABLE_BACKWARD_COMPATIBILITY 0
 #define configSTACK_DEPTH_TYPE            uint32_t
 #define configMESSAGE_BUFFER_LENGTH_TYPE  size_t
 
 // Memory allocation settings
 #define configSUPPORT_STATIC_ALLOCATION       1
 #define configSUPPORT_DYNAMIC_ALLOCATION      1
 #define configTOTAL_HEAP_SIZE                 (128*1024)  // 128KB
 #define configAPPLICATION_ALLOCATED_HEAP      0
 #define configUSE_MALLOC_FAILED_HOOK          1
 
 // Task and stack protection
 #define configCHECK_FOR_STACK_OVERFLOW        2
 #define configUSE_TRACE_FACILITY              1
 #define configUSE_STATS_FORMATTING_FUNCTIONS  1
 
 // Timer settings
 #define configUSE_TIMERS                      1
 #define configTIMER_TASK_PRIORITY             (configMAX_PRIORITIES - 1)
 #define configTIMER_QUEUE_LENGTH              10
 #define configTIMER_TASK_STACK_DEPTH          (configMINIMAL_STACK_SIZE * 2)
 
 // Run-time analysis settings
 #define configGENERATE_RUN_TIME_STATS         0
 
 // System settings
 #define configUSE_IDLE_HOOK                   0
 #define configUSE_TICK_HOOK                   0
 #define configUSE_DAEMON_TASK_STARTUP_HOOK    0
 #define configUSE_CO_ROUTINES                 0
 #define configMAX_CO_ROUTINE_PRIORITIES       2
 #define configPRIO_BITS                       5
 #define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         0x1
 #define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    0x5
 
 // Used by ESP32 port
 #define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
 #define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
 
 // SMP settings for dual-core ESP32
 #define configNUM_CORES                         2
 #define configUSE_CORE_AFFINITY                 1
 #define configRUN_MULTIPLE_PRIORITIES           1
 #define configUSE_TASK_PREEMPTION_DISABLE       0
 #define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
 
 // Hook function for debugging
 extern void vApplicationMallocFailedHook(void);
 extern void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
 
 #endif // RTOS_CONFIG_H