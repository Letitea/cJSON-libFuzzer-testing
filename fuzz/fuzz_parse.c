#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "cJSON.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (data == NULL || size > 1024 * 1024) return 0;
    char *input = (char *)malloc(size + 1);
    if (input == NULL) return 0;
    for (size_t i = 0; i < size; ++i) input[i] = (char)data[i];
    input[size] = '\0';

    cJSON *root = cJSON_Parse(input);
    if (root != NULL) cJSON_Delete(root);

    free(input);
    return 0;
}