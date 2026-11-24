#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "cJSON.h"

static void verify_list(const cJSON *container) {
    if (!container || !container->child) return;
    const cJSON *child = container->child;
    const cJSON *tail = child;
    while (tail->next != NULL) {
        if (tail->next->prev != tail) abort();
        tail = tail->next;
    }
    if (child->prev != tail) abort();
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 4 || size > 8192) return 0;

    /* Use first 4 bytes as control knobs, rest as JSON or string payload */
    uint8_t k0 = data[0];
    uint8_t k1 = data[1];
    uint8_t k2 = data[2];
    uint8_t k3 = data[3];

    size_t payload_len = size - 4;
    char *payload = (char *)malloc(payload_len + 1);
    if (!payload) return 0;
    memcpy(payload, data + 4, payload_len);
    payload[payload_len] = '\0';

    /* --- 1. Array mutation operations --- */
    cJSON *arr = cJSON_Parse(payload);
    if (!arr || !cJSON_IsArray(arr)) {
        if (arr) cJSON_Delete(arr);
        arr = cJSON_CreateArray();
    }
    if (arr) {
        for (int i = 0; i < 4; ++i) {
            cJSON *item = cJSON_CreateNumber((double)((k0 + i) % 100));
            if (item && !cJSON_AddItemToArray(arr, item)) cJSON_Delete(item);
        }

        int sz = cJSON_GetArraySize(arr);
        int idx_ins = (sz > 0) ? (int)(k1 % (unsigned)(sz + 2)) : 0;

        /* Insert at head (0), middle/end, and out-of-range */
        cJSON *ins_head = cJSON_CreateString("head");
        if (ins_head && !cJSON_InsertItemInArray(arr, 0, ins_head)) cJSON_Delete(ins_head);
        verify_list(arr);

        cJSON *ins_mid = cJSON_CreateString(payload_len < 64 ? payload : "mid");
        if (ins_mid && !cJSON_InsertItemInArray(arr, idx_ins, ins_mid)) cJSON_Delete(ins_mid);
        verify_list(arr);

        cJSON *ins_end = cJSON_CreateBool(k2 & 1);
        if (ins_end && !cJSON_InsertItemInArray(arr, 9999, ins_end)) cJSON_Delete(ins_end);
        verify_list(arr);

        /* Replace in array (head, middle, tail, self-replace) */
        sz = cJSON_GetArraySize(arr);
        if (sz > 0) {
            cJSON *first = cJSON_GetArrayItem(arr, 0);
            if (first) (void)cJSON_ReplaceItemViaPointer(arr, first, first); /* self-replace */

            cJSON *rep0 = cJSON_CreateNumber((double)k2);
            if (rep0 && !cJSON_ReplaceItemInArray(arr, 0, rep0)) cJSON_Delete(rep0);
            verify_list(arr);

            cJSON *rep_last = cJSON_CreateNull();
            if (rep_last && !cJSON_ReplaceItemInArray(arr, sz - 1, rep_last)) cJSON_Delete(rep_last);
            verify_list(arr);

            if (sz > 2) {
                cJSON *rep_mid = cJSON_CreateTrue();
                if (rep_mid && !cJSON_ReplaceItemInArray(arr, sz / 2, rep_mid)) cJSON_Delete(rep_mid);
                verify_list(arr);
            }
        }

        /* Detach & Delete from array (middle, head, tail) */
        sz = cJSON_GetArraySize(arr);
        if (sz > 1) {
            cJSON *det = cJSON_DetachItemFromArray(arr, (int)(k3 % (unsigned)sz));
            if (det) cJSON_Delete(det);
            verify_list(arr);
        }
        if (cJSON_GetArraySize(arr) > 0) {
            cJSON_DeleteItemFromArray(arr, 0);
            verify_list(arr);
        }

        /* Single-element replace to hit parent->child->prev == parent->child */
        cJSON *single = cJSON_CreateArray();
        if (single) {
            cJSON *only = cJSON_CreateNumber(1);
            if (only && !cJSON_AddItemToArray(single, only)) cJSON_Delete(only);
            cJSON *rep_only = cJSON_CreateNumber(2);
            if (rep_only && !cJSON_ReplaceItemInArray(single, 0, rep_only)) cJSON_Delete(rep_only);
            verify_list(single);
            cJSON_Delete(single);
        }

        cJSON_Delete(arr);
    }

    /* --- 2. Object query, replace, detach, delete (case-insensitive & case-sensitive) --- */
    cJSON *obj = cJSON_Parse(payload);
    if (!obj || !cJSON_IsObject(obj)) {
        if (obj) cJSON_Delete(obj);
        obj = cJSON_CreateObject();
    }
    if (obj) {
        (void)cJSON_AddNumberToObject(obj, "KeyOne", 10);
        (void)cJSON_AddStringToObject(obj, "KeyTwo", "hello_world");
        (void)cJSON_AddTrueToObject(obj, "KeyThree");

        (void)cJSON_HasObjectItem(obj, "keyone");
        (void)cJSON_HasObjectItem(obj, "missing_key");
        (void)cJSON_GetObjectItem(obj, "KEYONE");
        (void)cJSON_GetObjectItemCaseSensitive(obj, "KeyOne");
        (void)cJSON_GetObjectItemCaseSensitive(obj, "keyone");

        /* Replace item (including a replacement that already has a string key) */
        cJSON *rep1 = cJSON_CreateNumber(20);
        if (rep1 && !cJSON_ReplaceItemInObject(obj, "keyone", rep1)) cJSON_Delete(rep1);
        verify_list(obj);

        cJSON *rep2 = cJSON_CreateString("replaced_cs");
        if (rep2 && !cJSON_ReplaceItemInObjectCaseSensitive(obj, "KeyTwo", rep2)) cJSON_Delete(rep2);
        verify_list(obj);

        /* Replacement whose string is already allocated (from detach) */
        cJSON *det_cs = cJSON_DetachItemFromObjectCaseSensitive(obj, "KeyThree");
        if (det_cs) {
            if (!cJSON_ReplaceItemInObjectCaseSensitive(obj, "KeyTwo", det_cs)) {
                cJSON_Delete(det_cs);
            }
        }
        verify_list(obj);

        /* Replace nonexistent key (must free replacement ourselves) */
        cJSON *rep_miss = cJSON_CreateNull();
        if (rep_miss && !cJSON_ReplaceItemInObjectCaseSensitive(obj, "no_such_key", rep_miss)) {
            cJSON_Delete(rep_miss);
        }

        cJSON_DeleteItemFromObject(obj, "KEYONE");
        cJSON_DeleteItemFromObjectCaseSensitive(obj, "KeyTwo");
        cJSON_DeleteItemFromObject(obj, "missing_key");
        verify_list(obj);

        cJSON_Delete(obj);
    }

    /* --- 3. SetNumberHelper & SetValuestring --- */
    cJSON *num = cJSON_CreateNumber(0);
    if (num) {
        (void)cJSON_SetNumberHelper(num, (double)INT_MAX + 100.0);
        (void)cJSON_SetNumberHelper(num, (double)INT_MIN - 100.0);
        (void)cJSON_SetNumberHelper(num, (double)((int8_t)k0) + 0.25);
        /* SetValuestring on non-string must return NULL */
        (void)cJSON_SetValuestring(num, "fail");
        cJSON_Delete(num);
    }

    cJSON *str_item = cJSON_CreateString("initial_string_value");
    if (str_item) {
        /* Shorter string (in-place copy) */
        (void)cJSON_SetValuestring(str_item, "short");
        /* Overlapping string (must safely reject and return NULL) */
        if (str_item->valuestring && str_item->valuestring[0] != '\0') {
            (void)cJSON_SetValuestring(str_item, str_item->valuestring + 1);
        }
        /* Longer string (realloc/new allocation) */
        (void)cJSON_SetValuestring(str_item, "a_significantly_longer_string_to_force_reallocation");
        /* Payload string */
        if (payload_len <= 256) {
            (void)cJSON_SetValuestring(str_item, payload);
        }
        /* NULL valuestring */
        (void)cJSON_SetValuestring(str_item, NULL);
        cJSON_Delete(str_item);
    }

    cJSON *str_ref = cJSON_CreateStringReference("const_ref");
    if (str_ref) {
        (void)cJSON_SetValuestring(str_ref, "nope");
        cJSON_Delete(str_ref);
    }

    free(payload);
    return 0;
}
