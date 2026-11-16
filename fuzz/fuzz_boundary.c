#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "cJSON.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 1024) return 0; 
    cJSON_Parse(NULL);
    cJSON_Print(NULL);
    cJSON_Delete(NULL);
    cJSON *empty = cJSON_Parse("");
    if (empty) cJSON_Delete(empty);
    cJSON *arr = cJSON_CreateArray();
    if (arr) {
        cJSON_GetArrayItem(arr, -1);
        cJSON_GetArrayItem(arr, 999999);
        cJSON_Delete(arr);
    }
    return 0;
}