#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "cJSON.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 1024) return 0;

    const char *parse_end = NULL;
    cJSON_Parse(NULL);
    cJSON_ParseWithLength(NULL, 0);
    cJSON_ParseWithLength("{}", 0);
    cJSON_ParseWithOpts(NULL, &parse_end, 1);
    cJSON_ParseWithLengthOpts(NULL, 0, &parse_end, 1);
    cJSON_ParseWithLengthOpts("{}", 0, &parse_end, 1);
    cJSON_Print(NULL);
    cJSON_PrintUnformatted(NULL);
    cJSON_PrintBuffered(NULL, -1, 0);
    cJSON_PrintBuffered(NULL, 16, 1);
    cJSON_PrintPreallocated(NULL, NULL, -1, 0);
    cJSON_Delete(NULL);
    cJSON_Minify(NULL);

    cJSON *empty = cJSON_Parse("");
    if (empty) cJSON_Delete(empty);

    /* NULL and out-of-bounds checks across query/mutate/create/type APIs */
    (void)cJSON_GetArraySize(NULL);
    (void)cJSON_GetArrayItem(NULL, 0);
    (void)cJSON_GetObjectItem(NULL, NULL);
    (void)cJSON_GetObjectItemCaseSensitive(NULL, NULL);
    (void)cJSON_HasObjectItem(NULL, NULL);
    (void)cJSON_GetStringValue(NULL);
    (void)cJSON_GetNumberValue(NULL);
    (void)cJSON_SetNumberHelper(NULL, 1.0);
    (void)cJSON_SetValuestring(NULL, "abc");
    (void)cJSON_Duplicate(NULL, 1);
    (void)cJSON_Compare(NULL, NULL, 1);

    (void)cJSON_AddItemToArray(NULL, NULL);
    (void)cJSON_AddItemToObject(NULL, NULL, NULL);
    (void)cJSON_AddItemToObjectCS(NULL, NULL, NULL);
    (void)cJSON_AddItemReferenceToArray(NULL, NULL);
    (void)cJSON_AddItemReferenceToObject(NULL, NULL, NULL);
    (void)cJSON_DetachItemViaPointer(NULL, NULL);
    (void)cJSON_DetachItemFromArray(NULL, -1);
    (void)cJSON_DetachItemFromObject(NULL, NULL);
    (void)cJSON_DetachItemFromObjectCaseSensitive(NULL, NULL);
    cJSON_DeleteItemFromArray(NULL, -1);
    cJSON_DeleteItemFromObject(NULL, NULL);
    cJSON_DeleteItemFromObjectCaseSensitive(NULL, NULL);
    (void)cJSON_InsertItemInArray(NULL, -1, NULL);
    (void)cJSON_ReplaceItemViaPointer(NULL, NULL, NULL);
    (void)cJSON_ReplaceItemInArray(NULL, -1, NULL);
    (void)cJSON_ReplaceItemInObject(NULL, NULL, NULL);
    (void)cJSON_ReplaceItemInObjectCaseSensitive(NULL, NULL, NULL);

    (void)cJSON_CreateIntArray(NULL, 1);
    (void)cJSON_CreateFloatArray(NULL, 1);
    (void)cJSON_CreateDoubleArray(NULL, 1);
    (void)cJSON_CreateStringArray(NULL, 1);
    int dummy_i = 1;
    float dummy_f = 1.0f;
    double dummy_d = 1.0;
    const char *dummy_s = "a";
    (void)cJSON_CreateIntArray(&dummy_i, -1);
    (void)cJSON_CreateFloatArray(&dummy_f, -1);
    (void)cJSON_CreateDoubleArray(&dummy_d, -1);
    (void)cJSON_CreateStringArray(&dummy_s, -1);
    (void)cJSON_CreateString(NULL);
    (void)cJSON_CreateRaw(NULL);

    (void)cJSON_IsInvalid(NULL);
    (void)cJSON_IsFalse(NULL);
    (void)cJSON_IsTrue(NULL);
    (void)cJSON_IsBool(NULL);
    (void)cJSON_IsNull(NULL);
    (void)cJSON_IsNumber(NULL);
    (void)cJSON_IsString(NULL);
    (void)cJSON_IsArray(NULL);
    (void)cJSON_IsObject(NULL);
    (void)cJSON_IsRaw(NULL);

    cJSON *arr = cJSON_CreateArray();
    if (arr) {
        (void)cJSON_GetArrayItem(arr, -1);
        (void)cJSON_GetArrayItem(arr, 999999);
        (void)cJSON_AddItemToArray(arr, arr); /* self-add rejected */
        (void)cJSON_AddItemToObject(arr, "k", arr); /* self-add rejected */
        (void)cJSON_DetachItemFromArray(arr, -1);
        (void)cJSON_DetachItemFromArray(arr, 10);
        cJSON_DeleteItemFromArray(arr, -1);
        cJSON *extra = cJSON_CreateNull();
        if (extra && !cJSON_InsertItemInArray(arr, -1, extra)) {
            cJSON_Delete(extra);
        }
        cJSON *rep = cJSON_CreateNull();
        if (rep && !cJSON_ReplaceItemInArray(arr, -1, rep)) {
            cJSON_Delete(rep);
        }
        (void)cJSON_PrintPreallocated(arr, NULL, 10, 0);
        char tiny[1];
        (void)cJSON_PrintPreallocated(arr, tiny, -1, 0);
        cJSON_Delete(arr);
    }

    cJSON invalid;
    memset(&invalid, 0, sizeof(invalid));
    (void)cJSON_IsInvalid(&invalid);
    (void)cJSON_SetValuestring(&invalid, "x");
    (void)cJSON_Compare(&invalid, &invalid, 1);

    /* Corrupted string node (NULL valuestring) */
    cJSON bad_str;
    memset(&bad_str, 0, sizeof(bad_str));
    bad_str.type = cJSON_String;
    bad_str.valuestring = NULL;
    (void)cJSON_SetValuestring(&bad_str, "x");
    char *printed = cJSON_Print(&bad_str);
    if (printed) cJSON_free(printed);

    /* Hit case_insensitive_strcmp NULL and identical-pointer branches */
    cJSON *arr_for_cmp = cJSON_CreateArray();
    if (arr_for_cmp) {
        cJSON *elem = cJSON_CreateNumber(1);
        if (elem && !cJSON_AddItemToArray(arr_for_cmp, elem)) cJSON_Delete(elem);
        (void)cJSON_GetObjectItem(arr_for_cmp, "k"); /* current->string is NULL */
        cJSON_Delete(arr_for_cmp);
    }
    cJSON *obj_for_cmp = cJSON_CreateObject();
    if (obj_for_cmp) {
        (void)cJSON_AddNumberToObject(obj_for_cmp, "same_ptr", 1);
        if (obj_for_cmp->child && obj_for_cmp->child->string) {
            (void)cJSON_GetObjectItem(obj_for_cmp, obj_for_cmp->child->string); /* string1 == string2 */
        }
        cJSON_Delete(obj_for_cmp);
    }

    /* Hit CJSON_CIRCULAR_LIMIT in cJSON_Duplicate_rec via self-referencing array */
    cJSON *circ = cJSON_CreateArray();
    if (circ) {
        (void)cJSON_AddItemReferenceToArray(circ, circ);
        cJSON *dup_circ = cJSON_Duplicate(circ, 1);
        if (dup_circ) cJSON_Delete(dup_circ);
        cJSON_Delete(circ);
    }

    (void)data;
    return 0;
}
