#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 4096) return 0;
    char *input = (char *)malloc(size + 1);
    if (!input) return 0;
    memcpy(input, data, size);
    input[size] = '\0';

    cJSON *root = cJSON_Parse(input);
    if (root) {
        cJSON *ref_array = cJSON_CreateArray();
        if (ref_array) {
            cJSON_AddItemReferenceToArray(ref_array, root);
            cJSON_AddItemReferenceToArray(ref_array, NULL);
            char *s = cJSON_PrintUnformatted(ref_array);
            if (s) cJSON_free(s);
            cJSON_Delete(ref_array);
        }

        cJSON *ref_obj = cJSON_CreateObject();
        if (ref_obj) {
            cJSON_AddItemReferenceToObject(ref_obj, "ref_key", root);
            cJSON_AddItemReferenceToObject(ref_obj, NULL, root);
            cJSON_AddItemReferenceToObject(ref_obj, "null_item", NULL);

            cJSON *cs_item = cJSON_CreateNumber(42);
            if (cs_item) {
                /* First add with dynamic key, detach, then re-add with CS key to cover freeing old string */
                cJSON *tmp_obj = cJSON_CreateObject();
                if (tmp_obj) {
                    if (cJSON_AddItemToObject(tmp_obj, "dyn_key", cs_item)) {
                        cJSON *detached = cJSON_DetachItemFromObject(tmp_obj, "dyn_key");
                        if (detached && !cJSON_AddItemToObjectCS(ref_obj, "const_key", detached)) {
                            cJSON_Delete(detached);
                        }
                    } else {
                        cJSON_Delete(cs_item);
                    }
                    cJSON_Delete(tmp_obj);
                } else {
                    cJSON_Delete(cs_item);
                }
            }

            cJSON *str_ref = cJSON_CreateStringReference(input);
            if (str_ref) {
                if (!cJSON_AddItemToObjectCS(ref_obj, "str_ref_key", str_ref)) {
                    cJSON_Delete(str_ref);
                }
            }

            char *s = cJSON_PrintUnformatted(ref_obj);
            if (s) cJSON_free(s);
            cJSON_Delete(ref_obj);
        }

        cJSON *obj_ref = cJSON_CreateObjectReference(root);
        if (obj_ref) {
            char *s = cJSON_PrintUnformatted(obj_ref);
            if (s) cJSON_free(s);
            cJSON_Delete(obj_ref);
        }

        cJSON *arr_ref = cJSON_CreateArrayReference(root);
        if (arr_ref) {
            char *s = cJSON_PrintUnformatted(arr_ref);
            if (s) cJSON_free(s);
            cJSON_Delete(arr_ref);
        }

        cJSON_Delete(root);
    }

    cJSON *str_ref_only = cJSON_CreateStringReference(input);
    if (str_ref_only) {
        char *s = cJSON_Print(str_ref_only);
        if (s) cJSON_free(s);
        cJSON_Delete(str_ref_only);
    }

    free(input);
    return 0;
}
