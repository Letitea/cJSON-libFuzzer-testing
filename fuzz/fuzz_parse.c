#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (data == NULL || size > 1024 * 1024) return 0;

    (void)cJSON_Version();
    void *tmp = cJSON_malloc(16);
    if (tmp != NULL) cJSON_free(tmp);

    char *input = (char *)malloc(size + 1);
    if (input == NULL) return 0;
    memcpy(input, data, size);
    input[size] = '\0';

    cJSON *root = cJSON_Parse(input);
    if (root != NULL) {
        cJSON_Delete(root);
    } else {
        (void)cJSON_GetErrorPtr();
    }

    cJSON *with_len = cJSON_ParseWithLength((const char *)data, size);
    if (with_len != NULL) cJSON_Delete(with_len);

    const char *parse_end = NULL;
    cJSON_bool require_null = (size > 0) ? (data[0] & 1) : 0;

    cJSON *with_opts = cJSON_ParseWithOpts(input, &parse_end, require_null);
    if (with_opts != NULL) {
        cJSON_Delete(with_opts);
    } else {
        (void)cJSON_GetErrorPtr();
    }

    cJSON *with_len_opts = cJSON_ParseWithLengthOpts((const char *)data, size, &parse_end, !require_null);
    if (with_len_opts != NULL) cJSON_Delete(with_len_opts);

    cJSON *with_len_opts_no_end = cJSON_ParseWithLengthOpts((const char *)data, size, NULL, 1);
    if (with_len_opts_no_end != NULL) cJSON_Delete(with_len_opts_no_end);

    free(input);
    return 0;
}
