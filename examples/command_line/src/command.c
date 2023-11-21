#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//
#include "hardware/clocks.h"
#include "hardware/rtc.h"
#include "pico/stdlib.h"
//
#include "FreeRTOS.h"

#include <errno.h>
//
#include "FreeRTOS_strerror.h"
#include "FreeRTOS_time.h"
#include "ff_sddisk.h"
#include "ff_stdio.h"
#include "ff_utils.h"
#include "hw_config.h"
#include "my_debug.h"
#include "sd_card.h"
#include "tests.h"
#include "util.h"
//
#include "command.h"

static char *saveptr;  // For strtok_r

bool die_now;

#pragma GCC diagnostic ignored "-Wunused-function"
#ifdef NDEBUG
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static void missing_argument_msg() {
    printf("Missing argument\n");
}
static void extra_argument_msg(const char *s) {
    printf("Unexpected argument: %s\n", s);
}
static bool extra_args() {
    const char *extra = strtok_r(NULL, " ", &saveptr);
    if (extra) {
        extra_argument_msg(extra);
        return true;
    }
    return false;
}

static void run_setrtc() {
    const char *dateStr = strtok_r(NULL, " ", &saveptr);
    if (!dateStr) {
        missing_argument_msg();
        return;
    }
    int date = atoi(dateStr);

    const char *monthStr = strtok_r(NULL, " ", &saveptr);
    if (!monthStr) {
        missing_argument_msg();
        return;
    }
    int month = atoi(monthStr);

    const char *yearStr = strtok_r(NULL, " ", &saveptr);
    if (!yearStr) {
        missing_argument_msg();
        return;
    }
    int year = atoi(yearStr) + 2000;

    const char *hourStr = strtok_r(NULL, " ", &saveptr);
    if (!hourStr) {
        missing_argument_msg();
        return;
    }
    int hour = atoi(hourStr);

    const char *minStr = strtok_r(NULL, " ", &saveptr);
    if (!minStr) {
        missing_argument_msg();
        return;
    };
    int min = atoi(minStr);

    const char *secStr = strtok_r(NULL, " ", &saveptr);
    if (!secStr) {
        missing_argument_msg();
        return;
    }
    int sec = atoi(secStr);

    if (extra_args()) return;

    datetime_t t = {.year = year,
                    .month = month,
                    .day = date,
                    .dotw = 0,  // 0 is Sunday, so 5 is Friday
                    .hour = hour,
                    .min = min,
                    .sec = sec};
    // rtc_set_datetime(&t);
    setrtc(&t);
}
static void run_date() {
    if (extra_args()) return;

    char buf[128] = {0};
    time_t epoch_secs = FreeRTOS_time(NULL);
    struct tm *ptm = localtime(&epoch_secs);
    size_t n = strftime(buf, sizeof(buf), "%c", ptm);
    assert(n);
    printf("%s\n", buf);
    strftime(buf, sizeof(buf), "%j",
             ptm);  // The day of the year as a decimal number (range
                    // 001 to 366).
    printf("Day of year: %s\n", buf);
}
static void run_info() {
    const char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1 && 1 == sd_get_num())
        arg1 = sd_get_by_num(0)->device_name;
    else if (!arg1) {
        printf("Missing argument: Specify device name\n");
        return;
    }
    if (extra_args()) return;

    sd_card_t *sd_card_p = sd_get_by_name(arg1);
    if (!sd_card_p) {
        printf("Unknown device name: \"%s\"\n", arg1);
        return;
    }
    // Card IDendtification register. 128 buts wide.
    cidDmp(sd_card_p, printf);
    // Card-Specific Data register. 128 bits wide.
    csdDmp(sd_card_p, printf);
    
    
    // SD Status
    // TODO: ACMD13

    if (!sd_card_p->ff_disk.xStatus.bIsMounted) {
        printf("Drive \"%s\" is not mounted\n", arg1);
        return;
    }
    printf("\n");
    FF_SDDiskShowPartition(&sd_card_p->ff_disk);

    // Report Partition Starting Offset
    uint64_t offs = sd_card_p->ff_disk.pxIOManager->xPartition.ulBeginLBA;
    printf("Partition Starting Offset: %llu sectors (%llu bytes)\n",
            offs, offs * sd_card_p->ff_disk.pxIOManager->xPartition.usBlkSize);

    // Report cluster size ("allocation unit")
    uint64_t spc = sd_card_p->ff_disk.pxIOManager->xPartition.ulSectorsPerCluster;
    printf("Cluster size (\"allocation unit\"): %llu sectors (%llu bytes)\n",
            spc, spc * sd_card_p->ff_disk.pxIOManager->xPartition.usBlkSize);
}
static void run_lliot() {
    char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    sd_card_t *sd_card_p = sd_get_by_name(arg1);
    if (!sd_card_p) {
        printf("Unknown device name: \"%s\"\n", arg1);
        return;
    }
    low_level_io_tests(arg1);
}
static void run_format() {
    const char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    bool rc = format(arg1);
    if (!rc)
        EMSG_PRINTF("Format failed!\n");
}
static void run_mount() {
    size_t argc = 0;
    const char *args[5] = {0};
    const char *argp;
    do {
        argp = strtok_r(NULL, " ", &saveptr);
        if (argp) {
            if (argc >= count_of(args)) {
                extra_argument_msg(argp);
                return;
            }
            args[argc++] = argp;
        }
    } while (argp);
    if (argc < 1) {
        missing_argument_msg();
        return;
    }
    for (size_t i = 0; i < argc; ++i) {
        bool rc = mount(args[i]);
        if (!rc) EMSG_PRINTF("Mount failed!\n");
    }
}
static void run_unmount() {
    const char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    unmount(arg1);
    sd_card_t *sd_card_p = sd_get_by_name(arg1);
    assert(sd_card_p);
    sd_card_p->m_Status |= STA_NOINIT;  // in case medium is removed
}
static void run_getfree() {
    const char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    sd_card_t *sd_card_p = sd_get_by_name(arg1);
    if (!sd_card_p) {
        EMSG_PRINTF("Unknown device name %s\n", arg1);
        return;
    }
    if (!sd_card_p->ff_disk.xStatus.bIsMounted) {
        printf("Drive \"%s\" is not mounted\n", arg1);
        return;
    }
    uint64_t freeMB;
    unsigned freePct;
    getFree(&sd_card_p->ff_disk, &freeMB, &freePct);
    printf("%s free space: %llu MB, %u %%\n", arg1, freeMB, freePct);
}
static void run_cd() {
    char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    int32_t lResult = ff_chdir(arg1);
    if (-1 == lResult)
        EMSG_PRINTF("ff_chdir(\"%s\") failed: %s (%d)\n", arg1,
                    FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
}
static void run_mkdir() {
    char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    // int ff_mkdir( const char *pcDirectory );
    int lResult = ff_mkdir(arg1);
    if (-1 == lResult)
        EMSG_PRINTF("ff_mkdir(\"%s\") failed: %s (%d)\n", arg1,
                    FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
}
static void run_ls() {
    const char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) arg1 = "";
    if (extra_args()) return;

    ls(arg1);
}
static void run_pwd() {
    if (extra_args()) return;

    char buf[512];
    char *ret = ff_getcwd(buf, sizeof buf);
    if (!ret) {
        printf("ff_getcwd failed\n");
    } else {
        printf("%s", ret);
    }
}
static void run_cat() {
    char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    FF_FILE *f = ff_fopen(arg1, "r");
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
static void run_cp() {
    char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    char *arg2 = strtok_r(NULL, " ", &saveptr);
    if (!arg2) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    FF_FILE *sf = ff_fopen(arg1, "r");
    if (!sf) {
        EMSG_PRINTF("ff_fopen: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        return;
    }
    FF_FILE *df = ff_fopen(arg2, "w");
    if (!df) {
        EMSG_PRINTF("ff_fopen: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        return;
    }
    uint8_t buf[1024];
    size_t br;
    do {
        br = ff_fread(buf, 1, sizeof buf, sf);
        size_t bw = ff_fwrite(buf, 1, br, df);
        if (br != bw) {
            EMSG_PRINTF("ff_fwrite: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
            break;
        }
    } while (br > 0);
    ff_fclose(sf);
    ff_fclose(df);
}
static void run_mv() {
    char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    char *arg2 = strtok_r(NULL, " ", &saveptr);
    if (!arg2) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    int ec = ff_rename(arg1, arg2, false);
    if (-1 == ec) {
        EMSG_PRINTF("ff_fwrite: %s (%d)\n", FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
    }
}
static void run_big_file_test() {    
    const char *pcPathName = strtok_r(NULL, " ", &saveptr);
    if (!pcPathName) {
        missing_argument_msg();
        return;
    }
    const char *pcSize = strtok_r(NULL, " ", &saveptr);
    if (!pcSize) {
        missing_argument_msg();
        return;
    }
    size_t size = strtoul(pcSize, 0, 0);
    const char *pcSeed = strtok_r(NULL, " ", &saveptr);
    if (!pcSeed) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    uint32_t seed = atoi(pcSeed);
    big_file_test(pcPathName, size, seed);
}
static void run_mtbft() {
    // breaking with Unix tradition of arg[0] being command name
    size_t argc = 0;
    const char *args[7] = {0};
    const char *argp;
    do {
        argp = strtok_r(NULL, " ", &saveptr);
        if (argp) {
            if (argc >= count_of(args)) {
                extra_argument_msg(argp);
                return;
            }
            args[argc++] = argp;
        }
    } while (argp);
    // "mtbft <size in MiB> <pathname 0> [pathname 1...]"
    if (argc < 2) {
        missing_argument_msg();
        return;
    }
    size_t size = strtoul(args[0], 0, 0);
    for (size_t i = 1; i < argc; ++i) {
        if ('/' != args[i][0]) {
            EMSG_PRINTF("<pathname> \"%s\" must be absolute\n", args[0]);
            return;
        }
    }
    mtbft(argc - 1,  size, &args[1]);
}
static void run_rm() {
    char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    char *arg2 = strtok_r(NULL, " ", &saveptr);
    if (extra_args()) return;

    if (arg2) {
        if (0 == strcmp("-r", arg1)) {
            int rc = ff_deltree(arg2);
            if (-1 == rc)
                EMSG_PRINTF("ff_deltree(\"%s\") failed.\n", arg2);
        } else if (0 == strcmp("-d", arg1)) {
            int rc = ff_rmdir(arg2);
            if (-1 == rc)
                EMSG_PRINTF("ff_rmdir(\"%s\") failed: %s (%d)\n", arg1,
                            FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
        } else {
            EMSG_PRINTF("Unknown option: %s\n", arg1);
        }
    } else {
        int rc = ff_remove(arg1);
        if (-1 == rc)
            EMSG_PRINTF("ff_remove(\"%s\") failed: %s (%d)\n", arg1,
                        FreeRTOS_strerror(stdioGET_ERRNO()), stdioGET_ERRNO());
    }
}
static void run_bench() {
    if (extra_args()) return;

    bench();
}
static void run_cvef() {
    if (extra_args()) return;

    sd_card_t *sd_card_p = get_current_sd_card_p();
    if (!sd_card_p) return;
    vCreateAndVerifyExampleFiles(sd_card_p->mount_point);
}
static void run_swcwdt() {
    if (extra_args()) return;

    sd_card_t *sd_card_p = get_current_sd_card_p();
    if (!sd_card_p) return;

    vStdioWithCWDTest(sd_card_p->mount_point);
}
static void loop_swcwdt_task(void *arg) {
    sd_card_t *sd_card_p = arg;
    while (!die_now) {
        vCreateAndVerifyExampleFiles(sd_card_p->mount_point);
        vStdioWithCWDTest(sd_card_p->mount_point);
    }
    vTaskDelete(NULL);
}
static void run_loop_swcwdt() {
    if (extra_args()) return;

    sd_card_t *sd_card_p = get_current_sd_card_p();
    if (!sd_card_p) return;

    xTaskCreate(loop_swcwdt_task, "loop_swcwdt", 768,
                sd_card_p, uxTaskPriorityGet(xTaskGetCurrentTaskHandle()) - 1, NULL);
}
void runMultiTaskStdioWithCWDTest() {
    if (extra_args()) return;

    sd_card_t *sd_card_p = get_current_sd_card_p();
    if (!sd_card_p) return;

    vCreateAndVerifyExampleFiles(sd_card_p->mount_point);
    vMultiTaskStdioWithCWDTest(sd_card_p->mount_point, 744);
}
static void run_start_logger() {
    if (extra_args()) return;

    data_log_demo();
}
static void run_die() {
    if (extra_args()) return;

    die_now = true;
}
static void run_undie() {
    if (extra_args()) return;

    die_now = false;
}
static void run_task_stats() {
    if (extra_args()) return;

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
static void run_heap_stats() {
    if (extra_args()) return;

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
static void run_run_time_stats() {
    if (extra_args()) return;

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
static void run_measure_freqs(void) {
    if (extra_args()) return;

    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    printf("pll_sys  = %dkHz\n", f_pll_sys);
    printf("pll_usb  = %dkHz\n", f_pll_usb);
    printf("rosc     = %dkHz\n", f_rosc);
    printf("clk_sys  = %dkHz\treported  = %lukHz\n", f_clk_sys, clock_get_hz(clk_sys) / KHZ);
    printf("clk_peri = %dkHz\treported  = %lukHz\n", f_clk_peri, clock_get_hz(clk_peri) / KHZ);
    printf("clk_usb  = %dkHz\treported  = %lukHz\n", f_clk_usb, clock_get_hz(clk_usb) / KHZ);
    printf("clk_adc  = %dkHz\treported  = %lukHz\n", f_clk_adc, clock_get_hz(clk_adc) / KHZ);
    printf("clk_rtc  = %dkHz\treported  = %lukHz\n", f_clk_rtc, clock_get_hz(clk_rtc) / KHZ);

    // Can't measure clk_ref / xosc as it is the ref
}
static void run_set_sys_clock_48mhz() {
    if (extra_args()) return;

    set_sys_clock_48mhz();
    setup_default_uart();
}
static void run_set_sys_clock_khz() {
    char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    int khz = atoi(arg1);

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
    assert(ok);

    setup_default_uart();
}

static void clr() {
    char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    int gp = atoi(arg1);

    gpio_init(gp);
    gpio_set_dir(gp, GPIO_OUT);
    gpio_put(gp, 0);
}
static void set() {
    char *arg1 = strtok_r(NULL, " ", &saveptr);
    if (!arg1) {
        missing_argument_msg();
        return;
    }
    if (extra_args()) return;

    int gp = atoi(arg1);

    gpio_init(gp);
    gpio_set_dir(gp, GPIO_OUT);
    gpio_put(gp, 1);
}

static void run_help();

typedef void (*p_fn_t)();
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
    // {"getfree", run_getfree,
    //  "getfree <device name>:\n"
    //  " Print the free space on drive"},
    {"cd", run_cd,
     "cd <path>:\n"
     " Changes the current directory of the device name.\n"
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
     "  -d Remove an empty directory\n"
     "  -r Recursively remove a directory and its contents"},
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
    {"simple", simple, "simple:\n Run simple FS tests"},
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
    {"bft", run_big_file_test, "Alias for big_file_test"},
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
    {"help", run_help,
     "help:\n"
     " Shows this command help."}
};
static void run_help() {
    if (extra_args()) return;

    for (size_t i = 0; i < count_of(cmds); ++i) {
        printf("%s\n\n", cmds[i].help);
    }
}

void process_stdio(int cRxedChar) {
    static char cmd[256];
    static size_t ix;

    if (!isprint(cRxedChar) && !isspace(cRxedChar) && '\r' != cRxedChar &&
        '\b' != cRxedChar && cRxedChar != (char)127)
        return;
    printf("%c", cRxedChar);  // echo
    stdio_flush();
    if (cRxedChar == '\r') {
        /* Just to space the output from the input. */
        printf("%c", '\n');
        stdio_flush();

        if (!strnlen(cmd, sizeof cmd)) {  // Empty input
            printf("> ");
            stdio_flush();
            return;
        }
        /* Process the input string received prior to the newline. */
        char *cmdn = strtok_r(cmd, " ", &saveptr);
        if (cmdn) {
            size_t i;
            for (i = 0; i < count_of(cmds); ++i) {
                if (0 == strcmp(cmds[i].command, cmdn)) {
                    (*cmds[i].function)();
                    break;
                }
            }
            if (count_of(cmds) == i) printf("Command \"%s\" not found\n", cmdn);
        }
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

