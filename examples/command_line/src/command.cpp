#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//
#include "hardware/structs/clocks.h"
#include "hardware/clocks.h"
#include "pico/platform.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/types.h"
//
#include "FreeRTOS.h"
#include "FreeRTOS_strerror.h"
#include "FreeRTOS_time.h"
#include "ff_sddisk.h"
#include "ff_stdio.h"
#include "ff_utils.h"
//
#include "crash.h"
#include "hw_config.h"
#include "my_debug.h"
#include "sd_card.h"
#include "tests.h"
//
#include "command.h"

static char *saveptr;  // For strtok_r

volatile bool die_now;

#pragma GCC diagnostic ignored "-Wunused-function"
#ifdef NDEBUG
#  pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static void missing_argument_msg() {
    printf("Missing argument\n");
}
static void extra_argument_msg(const char *s) {
    printf("Unexpected argument: %s\n", s);
}
static bool expect_argc(const size_t argc, const char *argv[], const size_t expected) {
    if (argc < expected) {
        missing_argument_msg();
        return false;
    }
    if (argc > expected) {
        extra_argument_msg(argv[expected]);
        return false;
    }
    return true;
}
static void run_date(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    char buf[128] = {0};
    time_t epoch_secs = FreeRTOS_time(NULL);
    if (epoch_secs < 1) {
        printf("RTC not running\n");
        return;
    }
    struct tm *ptm = localtime(&epoch_secs);
    configASSERT(ptm);
    size_t n = strftime(buf, sizeof(buf), "%c", ptm);
    configASSERT(n);
    printf("%s\n", buf);
    strftime(buf, sizeof(buf), "%j",
             ptm);  // The day of the year as a decimal number (range
                    // 001 to 366).
    printf("Day of year: %s\n", buf);
}

static void run_setrtc(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 6)) return;

    int8_t date = atoi(argv[0]);
    int8_t month = atoi(argv[1]);
    int16_t year = atoi(argv[2]) + 2000;
    int8_t hour = atoi(argv[3]);
    int8_t min = atoi(argv[4]);
    int8_t sec = atoi(argv[5]);


    struct tm t = {
        // tm_sec	int	seconds after the minute	0-61*
        .tm_sec = sec,
        // tm_min	int	minutes after the hour	0-59
        .tm_min = min,
        // tm_hour	int	hours since midnight	0-23
        .tm_hour = hour,
        // tm_mday	int	day of the month	1-31
        .tm_mday = date,
        // tm_mon	int	months since January	0-11
        .tm_mon = month - 1,
        // tm_year	int	years since 1900
        .tm_year = year - 1900,
        // tm_wday	int	days since Sunday	0-6
        .tm_wday = 0,
        // tm_yday	int	days since January 1	0-365
        .tm_yday = 0,
        // tm_isdst	int	Daylight Saving Time flag
        .tm_isdst = 0
    };
    /* The values of the members tm_wday and tm_yday of timeptr are ignored, and the values of
       the other members are interpreted even if out of their valid ranges */
    time_t epoch_secs = mktime(&t);
    if (-1 == epoch_secs) {
        printf("The passed in datetime was invalid\n");
        return;
    }
    struct timespec ts = {.tv_sec = epoch_secs, .tv_nsec = 0};
    setrtc(&ts);
}
static void run_info(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;

    sd_card_t *sd_card_p = sd_get_by_name(argv[0]);
    if (!sd_card_p) {
        printf("Unknown device name: \"%s\"\n", argv[0]);
        return;
    }
    int ds = sd_card_p->init(sd_card_p);
    if (STA_NODISK & ds || STA_NOINIT & ds) {
        printf("SD card initialization failed\n");
        return;
    }
    // Card IDendtification register. 128 buts wide.
    cidDmp(sd_card_p, printf);
    // Card-Specific Data register. 128 bits wide.
    csdDmp(sd_card_p, printf);
    
    // SD Status
    size_t au_size_bytes;
    bool ok = sd_allocation_unit(sd_card_p, &au_size_bytes);
    if (ok)
        printf("\nSD card Allocation Unit (AU_SIZE) or \"segment\": %zu bytes (%zu sectors)\n", 
            au_size_bytes, au_size_bytes / sd_block_size);
    
    if (!sd_card_p->state.ff_disk.xStatus.bIsMounted) {
        printf("Drive \"%s\" is not mounted\n", argv[0]);
        return;
    }
    printf("\n");
    FF_SDDiskShowPartition(&sd_card_p->state.ff_disk);

    // Report Partition Starting Offset
    uint64_t offs = sd_card_p->state.ff_disk.pxIOManager->xPartition.ulBeginLBA;
    printf("\nPartition Starting Offset: %llu sectors (%llu bytes)\n",
            offs, offs * sd_card_p->state.ff_disk.pxIOManager->xPartition.usBlkSize);

    // Report cluster size ("allocation unit")
    uint64_t spc = sd_card_p->state.ff_disk.pxIOManager->xPartition.ulSectorsPerCluster;
    printf("FAT Cluster size (\"allocation unit\"): %llu sectors (%llu bytes)\n",
            spc, spc * sd_card_p->state.ff_disk.pxIOManager->xPartition.usBlkSize);
}
static void run_format(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;

    bool rc = format(argv[0]);
    if (!rc)
        EMSG_PRINTF("Format failed!\n");
}
static void run_mount(const size_t argc, const char *argv[]) {
    if (argc < 1) {
        missing_argument_msg();
        return;
    }
    for (size_t i = 0; i < argc; ++i) {
        bool rc = mount(argv[i]);
        if (!rc) EMSG_PRINTF("Mount failed!\n");
    }
}
static void run_unmount(const size_t argc, const char *argv[]) {
    if (argc < 1) {
        missing_argument_msg();
        return;
    }
    for (size_t i = 0; i < argc; ++i) {
        unmount(argv[i]);
        sd_card_t *sd_card_p = sd_get_by_name(argv[i]);
        if (!sd_card_p) {
            EMSG_PRINTF("Unknown device name: %s\n", argv[1]);
            return;
        }
        sd_card_p->state.m_Status |= STA_NOINIT;  // in case medium is removed
    }
}
static void run_cd(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;
    
    int32_t lResult = ff_chdir(argv[0]);
    if (-1 == lResult)
        EMSG_PRINTF("ff_chdir(\"%s\") failed: %s (%d)\n", argv[0],
                    FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
}
static void run_mkdir(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;
    
    // int ff_mkdir( const char *pcDirectory );
    int lResult = ff_mkdir(argv[0]);
    if (-1 == lResult)
        EMSG_PRINTF("ff_mkdir(\"%s\") failed: %s (%d)\n", argv[0],
                    FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
}
static void run_ls(const size_t argc, const char *argv[]) {
    if (argc > 1) {
        extra_argument_msg(argv[1]);
        return;
    }
    if (argc)
        ls(argv[0]);
    else
        ls("");
}
static void run_pwd(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
    char buf[512];
    char *ret = ff_getcwd(buf, sizeof buf);
    if (!ret) {
        printf("ff_getcwd failed\n");
    } else {
        printf("%s", ret);
    }
}
static void run_cat(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;
    
    FF_FILE *f = ff_fopen(argv[0], "r");
    if (!f) {
        EMSG_PRINTF("ff_fopen: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        return;
    }
    char buf[256];
    size_t len;
    do {
        len = ff_fread(buf, 1, sizeof buf, f);
        printf("%.*s", len, buf);
    } while (len > 0);
    int rc = ff_fclose(f);
    if (-1 == rc) {
        EMSG_PRINTF("ff_fclose: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
    }
}
static void run_cp(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 2)) return;
    
    FF_FILE *sf = ff_fopen(argv[0], "r");
    if (!sf) {
        EMSG_PRINTF("ff_fopen: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        return;
    }
    FF_FILE *df = ff_fopen(argv[1], "w");
    if (!df) {
        EMSG_PRINTF("ff_fopen: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        ff_fclose(sf);
        return;
    }

    /* File copy buffer */
    size_t buf_sz = ff_filelength(sf);
    if (!buf_sz) {
        EMSG_PRINTF("ff_filelength: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        ff_fclose(df);
        ff_fclose(sf);
        return;
    }
    if (buf_sz > 32768)
        buf_sz = 32768;
    uint8_t *buf = (uint8_t *)pvPortMalloc(buf_sz);
    if (!buf) {
        printf("pvPortMalloc(%zu) failed\n", buf_sz);
        ff_fclose(df);
        ff_fclose(sf);
        return;
    }

    size_t br;
    do {
        br = ff_fread(buf, 1, buf_sz, sf);
        size_t bw = ff_fwrite(buf, 1, br, df);
        if (br != bw) {
            EMSG_PRINTF("ff_fwrite: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
            break;
        }
    } while (br > 0);

    vPortFree(buf);
    ff_fclose(df);
    ff_fclose(sf);
}
static void run_mv(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 2)) return;
    
    int ec = ff_rename(argv[0], argv[1], false);
    if (-1 == ec) {
        EMSG_PRINTF("ff_fwrite: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
    }
}
static void run_lliot(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;

    sd_card_t *sd_card_p = sd_get_by_name(argv[0]);
    if (!sd_card_p) {
        EMSG_PRINTF("Unknown device name: \"%s\"\n", argv[0]);
        return;
    }
    low_level_io_tests(argv[0]);
}
static void run_big_file_test(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 3)) return;

    const char *pcPathName = argv[0];
    size_t size = strtoul(argv[1], 0, 0);
    uint32_t seed = atoi(argv[2]);
    big_file_test(pcPathName, size, seed);
}
static void run_mtbft(const size_t argc, const char *argv[]) {
    if (argc < 2) {
        missing_argument_msg();
        return;
    }
    size_t size = strtoul(argv[0], 0, 0);
    for (size_t i = 1; i < argc; ++i) {
        if ('/' != argv[i][0]) {
            EMSG_PRINTF("<pathname> \"%s\" must be absolute\n", argv[0]);
            return;
        }
    }
    mtbft(argc - 1, size, &argv[1]);
}
static void run_rm(const size_t argc, const char *argv[]) {
    if (argc < 1) {
        missing_argument_msg();
        return;
    }
    if (argc > 2) {
        extra_argument_msg(argv[2]);
        return;
    }
    if (2 == argc) {
        if (0 == strcmp("-r", argv[0])) {
            if (0 == strcmp("*", argv[1])) {
                FF_FindData_t xFindStruct;
                if (ff_findfirst("", &xFindStruct) == 0) {
                    do {
                        if (0 == strcmp(".", xFindStruct.pcFileName)) continue;
                        if (0 == strcmp("..", xFindStruct.pcFileName)) continue;
                        int rc = ff_deltree(xFindStruct.pcFileName);
                        if (-1 == rc)
                            EMSG_PRINTF("ff_deltree(\"%s\") failed.\n", xFindStruct.pcFileName);
                    } while (ff_findnext(&xFindStruct) == 0);
                }
            } else {
                int rc = ff_deltree(argv[1]);
                if (-1 == rc) EMSG_PRINTF("ff_deltree(\"%s\") failed.\n", argv[1]);
            }
        } else if (0 == strcmp("-d", argv[0])) {
            int rc = ff_rmdir(argv[1]);
            if (-1 == rc)
                EMSG_PRINTF("ff_rmdir(\"%s\") failed: %s (%d)\n", argv[1],
                            FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        } else {
            EMSG_PRINTF("Unknown option: %s\n", argv[0]);
        }
    } else {
        int rc = ff_remove(argv[0]);
        if (-1 == rc)
            EMSG_PRINTF("ff_remove(\"%s\") failed: %s (%d)\n", argv[0],
                        FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
    }
}
static void run_simple(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    simple();
}
static void run_bench(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
    bench();
}
static void run_cvef(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
    sd_card_t *sd_card_p = get_current_sd_card_p();
    if (!sd_card_p) return;
    vCreateAndVerifyExampleFiles(sd_card_p->mount_point);
}
static void run_swcwdt(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    sd_card_t *sd_card_p = get_current_sd_card_p();
    if (!sd_card_p) return;

    vStdioWithCWDTest(sd_card_p->mount_point);
}
static void loop_swcwdt_task(void *arg) {
    sd_card_t *sd_card_p = (sd_card_t *)arg;
    while (!die_now) {
        vCreateAndVerifyExampleFiles(sd_card_p->mount_point);
        vStdioWithCWDTest(sd_card_p->mount_point);
    }
    vTaskDelete(NULL);
}
static void run_loop_swcwdt(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;

    sd_card_t *sd_card_p = get_current_sd_card_p();
    if (!sd_card_p) return;

    xTaskCreate(loop_swcwdt_task, "loop_swcwdt", 768,
                sd_card_p, uxTaskPriorityGet(xTaskGetCurrentTaskHandle()) - 1, NULL);
}
void runMultiTaskStdioWithCWDTest(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
    sd_card_t *sd_card_p = get_current_sd_card_p();
    if (!sd_card_p) return;

    vCreateAndVerifyExampleFiles(sd_card_p->mount_point);
    vMultiTaskStdioWithCWDTest(sd_card_p->mount_point, 1024);
}
static void run_start_logger(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
    data_log_demo();
}
static void run_die(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
    die_now = true;
}
static void run_undie(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
    die_now = false;
}
static void run_task_stats(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
    printf(
        "Task          State  Priority  Stack        "
        "#\n************************************************\n");
    /* NOTE - for simplicity, this example assumes the
     write buffer length is adequate, so does not check for buffer overflows. */
    char buf[1024] = {0};
    buf[sizeof buf - 1] = 0xA5;  // Crude overflow guard
    /* Generate a table of task stats. */
    vTaskList(buf);
    configASSERT(0xA5 == buf[sizeof buf - 1]);
    printf("%s\n", buf);
}
static void run_heap_stats(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
#if 1  // HEAP4
    printf(
        "Configured total heap size:\t%d\n"
        "Free bytes in the heap now:\t%u\n"
        "Minimum number of unallocated bytes that have ever existed in the heap:\t%u\n",
        configTOTAL_HEAP_SIZE, xPortGetFreeHeapSize(),
        xPortGetMinimumEverFreeHeapSize());
#else
    printf("Free bytes in the heap now:\t%u\n", xPortGetFreeHeapSize());
#endif
    // malloc_stats();
}
static void run_run_time_stats(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
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

/* Derived from pico-examples/clocks/hello_48MHz/hello_48MHz.c */
static void run_measure_freqs(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
#if HAS_RP2040_RTC
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);
#endif    

    printf("pll_sys  = %dkHz\n", f_pll_sys);
    printf("pll_usb  = %dkHz\n", f_pll_usb);
    printf("rosc     = %dkHz\n", f_rosc);
    printf("clk_sys  = %dkHz\treported  = %lukHz\n", f_clk_sys, clock_get_hz(clk_sys) / KHZ);
    printf("clk_peri = %dkHz\treported  = %lukHz\n", f_clk_peri, clock_get_hz(clk_peri) / KHZ);
    printf("clk_usb  = %dkHz\treported  = %lukHz\n", f_clk_usb, clock_get_hz(clk_usb) / KHZ);
    printf("clk_adc  = %dkHz\treported  = %lukHz\n", f_clk_adc, clock_get_hz(clk_adc) / KHZ);
#if HAS_RP2040_RTC
    printf("clk_rtc  = %dkHz\treported  = %lukHz\n", f_clk_rtc, clock_get_hz(clk_rtc) / KHZ);
#endif    

    // Can't measure clk_ref / xosc as it is the ref
}
static void run_set_sys_clock_48mhz(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
    set_sys_clock_48mhz();
    setup_default_uart();
}
static void run_set_sys_clock_khz(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;
    
    int khz = atoi(argv[0]);

    bool configured = set_sys_clock_khz(khz, false);
    if (!configured) {
        printf("Not possible. Clock not configured.\n");
        return;
    }
    /*
    By default, when reconfiguring the system clock PLL settings after runtime initialization,
    the peripheral clock is switched to the 48MHz USB clock to ensure continuity of peripheral operation.
    There seems to be a problem with running the SPI 2.4 times faster than the system clock,
    even at the same SPI baud rate.
    Anyway, for now, reconfiguring the peripheral clock to the system clock at its new frequency works OK.
    */
    bool ok = clock_configure(clk_peri,
                              0,
                              CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                              clock_get_hz(clk_sys),
                              clock_get_hz(clk_sys));
    configASSERT(ok);

    setup_default_uart();
}
static void set(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;
    
    int gp = atoi(argv[0]);

    gpio_init(gp);
    gpio_set_dir(gp, GPIO_OUT);
    gpio_put(gp, 1);
}
static void clr(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 1)) return;

    int gp = atoi(argv[0]);

    gpio_init(gp);
    gpio_set_dir(gp, GPIO_OUT);
    gpio_put(gp, 0);
}
static void run_test(const size_t argc, const char *argv[]) {    
    if (!expect_argc(argc, argv, 0)) return;

    // void trigger_hard_fault() {
    void (*bad_instruction)() = (void (*)())0xE0000000;
    bad_instruction();
}

static void run_help(const size_t argc, const char *argv[]);

typedef void (*p_fn_t)(const size_t argc, const char *argv[]);
typedef struct {
    char const *const command;
    p_fn_t const function;
    char const *const help;
} cmd_def_t;

static cmd_def_t cmds[] = {
    {"setrtc", run_setrtc,
     "setrtc <DD> <MM> <YY> <hh> <mm> <ss>:\n"
     " Set Real Time Clock\n"
     " Parameters: new date (DD MM YY) new time in 24-hour format "
     "(hh mm ss)\n"
     "\te.g.:setrtc 16 3 21 0 4 0"},
    {"date", run_date, "date:\n Print current date and time"},
    {"format", run_format,
     "format <device name>:\n"
     " Creates an FAT/exFAT volume on the device name.\n"
     "\te.g.: format sd0"},
    {"mount", run_mount,
     "mount <device name> [device_name...]:\n"
     " Makes the specified device available at its mount point in the directory tree.\n"
     "\te.g.: mount sd0"},
    {"unmount", run_unmount,
     "unmount <device name>:\n"
     " Unregister the work area of the volume"},
    {"info", run_info,
     "info <device name>:\n"
     " Print information about an SD card"},
    {"cd", run_cd,
     "cd <path>:\n"
     " Changes the current directory.\n"
     " <path> Specifies the directory to be set as current directory.\n"
     "\te.g.: cd /dir1"},
    {"mkdir", run_mkdir,
     "mkdir <path>:\n"
     " Make a new directory.\n"
     " <path> Specifies the name of the directory to be created.\n"
     "\te.g.: mkdir /dir1"},
    {"rm", run_rm,
     "rm [options] <pathname>:\n"
     " Removes (deletes) a file or directory\n"
     " <pathname> Specifies the path to the file or directory to be removed\n"
     " Options:\n"
     " -d Remove an empty directory\n"
     " -r Recursively remove a directory and its contents"},
    {"cp", run_cp,
     "cp <source file> <dest file>:\n"
     " Copies <source file> to <dest file>"},
    {"mv", run_mv,
     "mv <source file> <dest file>:\n"
     " Moves (renames) <source file> to <dest file>"},
    {"pwd", run_pwd,
     "pwd:\n"
     " Print Working Directory"},
    {"ls", run_ls, "ls [pathname]:\n List directory"},
    // {"dir", run_ls, "dir:\n List directory"},
    {"cat", run_cat, "cat <filename>:\n Type file contents"},
    {"simple", run_simple, "simple:\n Run simple FS tests"},
    {"lliot", run_lliot,
     "lliot <device name>\n !DESTRUCTIVE! Low Level I/O Driver Test\n"
     "The SD card will need to be reformatted after this test.\n"
     "\te.g.: lliot sd0"},
    {"bench", run_bench, "bench <device name>:\n A simple binary write/read benchmark"},
    {"big_file_test", run_big_file_test,
     "big_file_test <pathname> <size in MiB> <seed>:\n"
     " Writes random data to file <pathname>.\n"
     " Specify <size in MiB> in units of mebibytes (2^20, or 1024*1024 bytes)\n"
     "\te.g.: big_file_test /sd0/bf 1 1\n"
     "\tor: big_file_test /sd1/big3G-3 3072 3"},
    {"bft", run_big_file_test, "bft:\n Alias for big_file_test"},
    {"mtbft", run_mtbft, 
     "mtbft <size in MiB> <pathname 0> [pathname 1...]\n"
     "Multi Task Big File Test\n"
     " pathname: Absolute path to a file (must begin with '/' and end with file name)"},
    {"cvef", run_cvef,
     "cvef:\n Create and Verify Example Files\n"
     "Expects card to be already formatted and mounted"},
    {"swcwdt", run_swcwdt,
     "swcwdt:\n Stdio With CWD Test\n"
     "Expects card to be already formatted and mounted.\n"
     "Note: run cvef first!"},
    {"loop_swcwdt", run_loop_swcwdt,
     "loop_swcwdt:\n Run Create Disk and Example Files and Stdio With CWD "
     "Test in a loop.\n"
     "Expects card to be already formatted and mounted.\n"
     "Note: Stop with \"die\"."},
    {"mtswcwdt", runMultiTaskStdioWithCWDTest,
     "mtswcwdt:\n MultiTask Stdio With CWD Test\n"
     "\te.g.: mtswcwdt"},
    {"start_logger", run_start_logger,
     "start_logger:\n"
     " Start Data Log Demo"},
    {"die", run_die,
     "die:\n Kill background tasks"},
    {"undie", run_undie,
     "undie:\n Allow background tasks to live again"},
    {"task-stats", run_task_stats, "task-stats:\n Show task statistics"},
    {"heap-stats", run_heap_stats, "heap-stats:\n Show heap statistics"},
    {"run-time-stats", run_run_time_stats,
     "run-time-stats:\n Displays a table showing how much processing time "
     "each FreeRTOS task has used"},
    // // Clocks testing:
    // {"set_sys_clock_48mhz", run_set_sys_clock_48mhz,
    //  "set_sys_clock_48mhz:\n"
    //  " Set the system clock to 48MHz"},
    // {"set_sys_clock_khz", run_set_sys_clock_khz,
    //  "set_sys_clock_khz <khz>:\n"
    //  " Set the system clock system clock frequency in khz."},
    // {"measure_freqs", run_measure_freqs,
    //  "measure_freqs:\n"
    //  " Count the RP2040 clock frequencies and report."},
    // {"clr", clr, "clr <gpio #>: clear a GPIO"},
    // {"set", set, "set <gpio #>: set a GPIO"},
    // {"test", run_test, "test:\n"
    //  " Development test"},
    {"help", run_help,
     "help:\n"
     " Shows this command help."}
};
static void run_help(const size_t argc, const char *argv[]) {
    if (!expect_argc(argc, argv, 0)) return;
    
    for (size_t i = 0; i < count_of(cmds); ++i) {
        printf("%s\n\n", cmds[i].help);
    }
}

static void process_cmd(char *cmd) {
    configASSERT(cmd);
    configASSERT(cmd[0]);
    char *cmdn = strtok_r(cmd, " ", &saveptr);
    if (cmdn) {

        /* Breaking with Unix tradition of argv[0] being command name,
        argv[0] is first argument after command name */

        size_t argc = 0;
        const char *argv[10] = {0}; // Arbitrary limit of 10 arguments
        const char *arg_p;
        do {
            arg_p = strtok_r(NULL, " ", &saveptr);
            if (arg_p) {
                if (argc >= count_of(argv)) {
                    extra_argument_msg(arg_p);
                    return;
                }
                argv[argc++] = arg_p;
            }
        } while (arg_p);

        size_t i;
        for (i = 0; i < count_of(cmds); ++i) {
            if (0 == strcmp(cmds[i].command, cmdn)) {                
                // run the command
                (*cmds[i].function)(argc, argv);
                break;
            }
        }
        if (count_of(cmds) == i) printf("Command \"%s\" not found\n", cmdn);
    }
}

/**
 * @brief Process a character received from the console
 *
 * @param cRxedChar A character received from the console
 */
void process_stdio(int cRxedChar) {
    static char cmd[256];
    static size_t ix = 0;

    if (!(0 < cRxedChar && cRxedChar <= 0x7F)) {
        return;  // Not dealing with multibyte characters or NULLs
    }

    switch (cRxedChar) {
        case 3:  // Ctrl-C
            SYSTEM_RESET();
            break;
        case 27:  // Esc
            __breakpoint();
    }
    if (!isprint(cRxedChar) && !isspace(cRxedChar) && '\r' != cRxedChar &&
        '\b' != cRxedChar && cRxedChar != 127) {
        return;
    }
    printf("%c", cRxedChar);  // echo
    stdio_flush();
    if (cRxedChar == '\r') {
        /* Just to space the output from the input. */
        printf("%c", '\n');
        stdio_flush();

        if (!cmd[0]) {  // Empty input
            printf("> ");
            stdio_flush();
            return;
        }

        /* Process the input string received prior to the newline. */
        process_cmd(cmd);

        /* Reset everything for next cmd */
        ix = 0;
        memset(cmd, 0, sizeof cmd);
        printf("\n> ");
        stdio_flush();
    } else {  // Not newline
        if (cRxedChar == '\b' || cRxedChar == (char)127) {
            /* Backspace was pressed.  Erase the last character
             in the string - if any. */
            if (ix > 0) {
                ix--;
                cmd[ix] = '\0';
            }
        } else {
            /* A character was entered.  Add it to the string
             entered so far.  When a \n is entered the complete
             string will be passed to the command interpreter. */
            if (ix < sizeof cmd - 1) {
                cmd[ix] = cRxedChar;
                ix++;
            }
        }
    }
}
