#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (data == NULL || size > 1024 * 1024) return 0;
    char *input = (char *)malloc(size + 1);
    if (input == NULL) return 0;
    memcpy(input, data, size);
    input[size] = '\0';

    cJSON *root = cJSON_Parse(input);
    if (root != NULL) {
        char *formatted = cJSON_Print(root);
        if (formatted != NULL) cJSON_free(formatted);

        char *unformatted = cJSON_PrintUnformatted(root);
        if (unformatted != NULL) cJSON_free(unformatted);

        int prebuf = (size > 0) ? (int)(data[0]) - 8 : 32;
        char *buf_fmt = cJSON_PrintBuffered(root, prebuf, 1);
        if (buf_fmt != NULL) cJSON_free(buf_fmt);

        char *buf_unfmt = cJSON_PrintBuffered(root, 1, 0);
        if (buf_unfmt != NULL) cJSON_free(buf_unfmt);

        char small_buf[16];
        (void)cJSON_PrintPreallocated(root, small_buf, (int)sizeof(small_buf), 1);
        (void)cJSON_PrintPreallocated(root, small_buf, 0, 0);

        char large_buf[4096];
        (void)cJSON_PrintPreallocated(root, large_buf, (int)sizeof(large_buf), 0);
        (void)cJSON_PrintPreallocated(root, large_buf, (int)sizeof(large_buf), 1);

        cJSON_Delete(root);
    }

    /* Exercise cJSON_Raw printing path (and NULL valuestring / invalid node) */
    if (size <= 4096) {
        cJSON *raw = cJSON_CreateRaw(input);
        if (raw != NULL) {
            char *p = cJSON_Print(raw);
            if (p != NULL) cJSON_free(p);
            char raw_buf[64];
            (void)cJSON_PrintPreallocated(raw, raw_buf, (int)sizeof(raw_buf), 0);
            cJSON_Delete(raw);
        }
    }

    cJSON bad_raw;
    memset(&bad_raw, 0, sizeof(bad_raw));
    bad_raw.type = cJSON_Raw;
    bad_raw.valuestring = NULL;
    char *p_bad = cJSON_Print(&bad_raw);
    if (p_bad != NULL) cJSON_free(p_bad);

    cJSON invalid_item;
    memset(&invalid_item, 0, sizeof(invalid_item));
    invalid_item.type = cJSON_Invalid;
    char *p_inv = cJSON_Print(&invalid_item);
    if (p_inv != NULL) cJSON_free(p_inv);

    /* Tiny preallocated buffer failures for null & empty string */
    cJSON *null_node = cJSON_CreateNull();
    cJSON *empty_str = cJSON_CreateString("");
    if (null_node && empty_str) {
        char tiny2[2];
        (void)cJSON_PrintPreallocated(null_node, tiny2, 1, 0);
        (void)cJSON_PrintPreallocated(empty_str, tiny2, 1, 0);
    }
    if (null_node) cJSON_Delete(null_node);
    if (empty_str) cJSON_Delete(empty_str);

    /* Hit CJSON_NESTING_LIMIT in print_array and print_object */
    if (size > 0 && (data[0] & 0x0F) == 0) {
        cJSON *deep_arr = cJSON_CreateArray();
        cJSON *cur = deep_arr;
        for (int i = 0; cur && i <= CJSON_NESTING_LIMIT; ++i) {
            cJSON *next = cJSON_CreateArray();
            if (!next || !cJSON_AddItemToArray(cur, next)) {
                if (next) cJSON_Delete(next);
                break;
            }
            cur = next;
        }
        if (deep_arr) {
            char *dp = cJSON_PrintUnformatted(deep_arr);
            if (dp) cJSON_free(dp);
            cJSON_Delete(deep_arr);
        }

        cJSON *deep_obj = cJSON_CreateObject();
        cur = deep_obj;
        for (int i = 0; cur && i <= CJSON_NESTING_LIMIT; ++i) {
            cJSON *next = cJSON_CreateObject();
            if (!next || !cJSON_AddItemToObjectCS(cur, "a", next)) {
                if (next) cJSON_Delete(next);
                break;
            }
            cur = next;
        }
        if (deep_obj) {
            char *dp = cJSON_PrintUnformatted(deep_obj);
            if (dp) cJSON_free(dp);
            cJSON_Delete(deep_obj);
        }
    }

    free(input);
    return 0;
}
