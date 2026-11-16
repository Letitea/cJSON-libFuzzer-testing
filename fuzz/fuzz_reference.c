#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "cJSON.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 4096) return 0;
    char *input = (char *)malloc(size + 1);
    if (!input) return 0;
    for (size_t i = 0; i < size; ++i) input[i] = (char)data[i];
    input[size] = '\0';
    cJSON *root = cJSON_Parse(input);
    if (root) {
        cJSON *ref_array = cJSON_CreateArray();
        if (ref_array) {
            cJSON_AddItemReferenceToArray(ref_array, root);
            cJSON_Delete(ref_array);
        }
        cJSON_Delete(root);
    }
    free(input);
    return 0;
}