#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int low_level_io_tests(const char *diskName);
// void ls(const char *dir);
void simple();
void bench();
void big_file_test(const char *const pathname, size_t size,
                   uint32_t seed);
void mtbft(const size_t size, const size_t parallelism,
           char const *pathname[]);
void vCreateAndVerifyExampleFiles(const char *pcMountPath);
void vStdioWithCWDTest(const char *pcMountPath);
void vMultiTaskStdioWithCWDTest(const char *const pcMountPath, uint16_t usStackSizeWords);
void data_log_demo();

#ifdef __cplusplus
}
#endif
