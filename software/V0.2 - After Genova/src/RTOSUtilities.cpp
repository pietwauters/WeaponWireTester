#include "RTOSUtilities.h"

// Core FreeRTOS includes
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <freertos/timers.h>
#include <freertos/event_groups.h>

// ESP32 specific includes
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

// Arduino compatibility
#include <Arduino.h>

// Standard C++ includes (if you're using any STL containers or algorithms)
#include <vector>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>

// If you have task monitoring/debugging functions
#include "esp_freertos_hooks.h"

// If you're doing memory analysis
#include "esp_heap_trace.h"

void printTasks() {
    // Get task list buffer
    char *taskListBuffer = (char *)pvPortMalloc(2048);  // Adjust size as needed
    if(!taskListBuffer) {
        ESP_LOGE("TASKS", "Failed to allocate task list buffer");
        return;
    }

    // Get raw task list
    vTaskList(taskListBuffer);
    
    // Parse and enhance with core info
    printf("\nTask Name      | Core | State | Pri | Stack Free |\n");
    printf("---------------------------------------------------\n");
    
    char *line = strtok(taskListBuffer, "\n");
    while(line != NULL) {
        char taskName[32];
        char state;
        unsigned int priority;
        unsigned int stackFree;
        unsigned int taskNumber;

        // Parse vTaskList format: "TaskName S R 1 1234 5"
        sscanf(line, "%31s %c %*c %u %u %u", 
              taskName, &state, &priority, &stackFree, &taskNumber);

        // Get task handle from task number (ESP32-specific)
        TaskHandle_t handle = xTaskGetHandle(taskName);
        
        // Get core affinity
        BaseType_t core = -1;
        if(handle) {
            BaseType_t affinity = xTaskGetAffinity(handle);
            if(affinity == 1) core = 0;
            else if(affinity == 2) core = 1;
        }

        // Print enhanced line
        printf("%-14s | %-3s | %-5c | %-3u | %-10u |\n",
              taskName,
              (core >= 0) ? (core == 0 ? "0" : "1") : "?",
              state,
              priority,
              stackFree);

        line = strtok(NULL, "\n");
    }
    
    vPortFree(taskListBuffer);
}


