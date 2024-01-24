#include "lwip/apps/httpd.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"
//
#include "FreeRTOS.h"
#include "task.h"
//
#include "sd_filesystem.h"

#if ANALYZE    
static void run_task_stats(const char *fn) {    
    printf(
        "\n--- %s ---\n"
		"Task            State   Priority Stack    # CoreAffinityMask\n"
        "************************************************************",
		fn);
    /* NOTE - for simplicity, this example assumes the
     write buffer length is adequate, so does not check for buffer overflows. */
    char buf[1024] = {0};
    buf[sizeof buf - 1] = 0xA5;  // Crude overflow guard
    /* Generate a table of task stats. */
    vTaskList(buf);
    configASSERT(0xA5 == buf[sizeof buf - 1]);
    printf("\n%s\n", buf);
}
static void run_run_time_stats() {
    /* A buffer into which the execution times will be
     * written, in ASCII form.  This buffer is assumed to be large enough to
     * contain the generated report.  Approximately 40 bytes per task should
     * be sufficient.
     */
    printf("%s",
           "Task            Abs Time      % Time\n"
           "****************************************\n");
    /* Generate a table of task stats. */
    char buf[1024] = {0};
    vTaskGetRunTimeStats(buf);
    printf("%s\n", buf);
}
#endif

void MainTask(__unused void *params) {
    // Initialize filesytem and mount SD card
    sd_init_mount();    

    cyw43_arch_init();

    cyw43_arch_enable_sta_mode();

    // Connect to the WiFI network - loop until connected
    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0){
        printf("Attempting to connect to %s...\n", WIFI_SSID);
    }
    // Print a success message once connected
    printf("Connected! \n");
    
    // Initialise web server
    cyw43_arch_lwip_begin();
    httpd_init();
    cyw43_arch_lwip_end();
    printf("Http server initialised\n");

    printf("\nListening at %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));

#if ANALYZE    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(20000));
        run_task_stats(__func__);
        run_run_time_stats();
    }
#else
    vTaskDelete(NULL);
#endif    

}

int main() {
    stdio_init_all();
    xTaskCreate(MainTask, "TestMainThread", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();
}