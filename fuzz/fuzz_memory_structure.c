#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 1024 * 1024) return 0;
    char *input = (char *)malloc(size + 1);
    if (!input) return 0;
    memcpy(input, data, size);
    input[size] = '\0';

    cJSON *root = cJSON_Parse(input);
    free(input);

    if (root) {
        cJSON *shallow = cJSON_Duplicate(root, 0);
        if (shallow) cJSON_Delete(shallow);

        cJSON *duplicate = cJSON_Duplicate(root, 1);
        if (duplicate) {
            if (duplicate->type == cJSON_Object) {
                cJSON *num = cJSON_CreateNumber(1);
                if (num && !cJSON_AddItemToObject(duplicate, "fuzz_key_1", num)) {
                    cJSON_Delete(num);
                }

                cJSON *child = duplicate->child;
                if (child != NULL) {
                    cJSON *tail = child;
                    while (tail->next != NULL) tail = tail->next;
                    if (child->prev != tail) abort(); /* 驗證鏈結串列一致性 */
                }

                cJSON *detached = cJSON_DetachItemFromObject(duplicate, "fuzz_key_1");
                if (detached) cJSON_Delete(detached);
            }
            cJSON_Delete(duplicate);
        }
        cJSON_Delete(root);
    }
    return 0;
}
