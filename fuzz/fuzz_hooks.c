#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

static size_t allowed_mallocs = 0; 
static void* failing_malloc(size_t sz) {
    if (allowed_mallocs == 0) return NULL;
    allowed_mallocs--;
    return malloc(sz);
}
static void my_free(void *ptr) { free(ptr); }

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2 || size > 1024) return 0;
    allowed_mallocs = data[0]; // 使用第一個 byte 控制失敗次數
    
    cJSON_Hooks hooks;
    hooks.malloc_fn = failing_malloc;
    hooks.free_fn = my_free;
    cJSON_InitHooks(&hooks);

    char *input = (char *)malloc(size);
    if (!input) { cJSON_InitHooks(NULL); return 0; }
    memcpy(input, data + 1, size - 1);
    input[size - 1] = '\0';

    cJSON *root = cJSON_Parse(input);
    if (root) {
        char *printed = cJSON_Print(root);
        if (printed) cJSON_free(printed);
        cJSON_Delete(root);
    }
    free(input);
    cJSON_InitHooks(NULL); 
    return 0;
}