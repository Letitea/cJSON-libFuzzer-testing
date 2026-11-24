#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "cJSON.h"

static void check_all_predicates(const cJSON *item) {
    (void)cJSON_IsInvalid(item);
    (void)cJSON_IsFalse(item);
    (void)cJSON_IsTrue(item);
    (void)cJSON_IsBool(item);
    (void)cJSON_IsNull(item);
    (void)cJSON_IsNumber(item);
    (void)cJSON_IsString(item);
    (void)cJSON_IsArray(item);
    (void)cJSON_IsObject(item);
    (void)cJSON_IsRaw(item);
    (void)cJSON_GetStringValue(item);
    (void)cJSON_GetNumberValue(item);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 4096) return 0;

    char *str = (char *)malloc(size + 1);
    if (!str) return 0;
    memcpy(str, data, size);
    str[size] = '\0';

    /* 1. Exercise all cJSON_Create* scalar & container APIs */
    cJSON *nodes[11];
    nodes[0] = cJSON_CreateNull();
    nodes[1] = cJSON_CreateTrue();
    nodes[2] = cJSON_CreateFalse();
    nodes[3] = cJSON_CreateBool(data[0] & 1);
    nodes[4] = cJSON_CreateBool(!(data[0] & 1));
    nodes[5] = cJSON_CreateNumber((double)INT_MAX * 2.0);
    nodes[6] = cJSON_CreateNumber((double)INT_MIN * 2.0);
    nodes[7] = cJSON_CreateString(str);
    nodes[8] = cJSON_CreateRaw(str);
    nodes[9] = cJSON_CreateArray();
    nodes[10] = cJSON_CreateObject();

    for (int i = 0; i < 11; ++i) {
        if (nodes[i]) {
            check_all_predicates(nodes[i]);
            cJSON_SetBoolValue(nodes[i], (data[0] >> 1) & 1);
            cJSON_Delete(nodes[i]);
        }
    }

    /* 2. Exercise cJSON_CreateInt/Float/Double/StringArray */
    int count = (int)(data[0] % 8);
    int ints[8];
    float floats[8];
    double doubles[8];
    const char *strs[8];
    for (int i = 0; i < 8; ++i) {
        uint8_t b = data[i % size];
        ints[i] = (int)b - 128;
        floats[i] = (float)ints[i] * 0.5f;
        doubles[i] = (double)ints[i] * 1.25;
        strs[i] = (i % 2 == 0) ? str : "static_str";
    }

    cJSON *ia = cJSON_CreateIntArray(ints, count);
    if (ia) {
        check_all_predicates(ia);
        cJSON *elem = NULL;
        cJSON_ArrayForEach(elem, ia) {
            check_all_predicates(elem);
        }
        cJSON_Delete(ia);
    }

    cJSON *fa = cJSON_CreateFloatArray(floats, count);
    if (fa) cJSON_Delete(fa);

    cJSON *da = cJSON_CreateDoubleArray(doubles, count);
    if (da) cJSON_Delete(da);

    cJSON *sa = cJSON_CreateStringArray(strs, count);
    if (sa) cJSON_Delete(sa);

    /* 3. Exercise all 9 cJSON_Add*ToObject helpers (valid object + NULL object) */
    (void)cJSON_AddNullToObject(NULL, "k");
    (void)cJSON_AddTrueToObject(NULL, "k");
    (void)cJSON_AddFalseToObject(NULL, "k");
    (void)cJSON_AddBoolToObject(NULL, "k", 1);
    (void)cJSON_AddNumberToObject(NULL, "k", 1.0);
    (void)cJSON_AddStringToObject(NULL, "k", "v");
    (void)cJSON_AddRawToObject(NULL, "k", "1");
    (void)cJSON_AddObjectToObject(NULL, "k");
    (void)cJSON_AddArrayToObject(NULL, "k");

    cJSON *obj = cJSON_CreateObject();
    if (obj) {
        cJSON *n_null = cJSON_AddNullToObject(obj, "null_k");
        cJSON *n_true = cJSON_AddTrueToObject(obj, "true_k");
        cJSON *n_false = cJSON_AddFalseToObject(obj, "false_k");
        cJSON *n_bool = cJSON_AddBoolToObject(obj, "bool_k", data[0] & 1);
        cJSON *n_num = cJSON_AddNumberToObject(obj, "num_k", (double)data[0]);
        cJSON *n_str = cJSON_AddStringToObject(obj, "str_k", str);
        cJSON *n_raw = cJSON_AddRawToObject(obj, "raw_k", "{\"r\":1}");
        cJSON *n_obj = cJSON_AddObjectToObject(obj, "obj_k");
        cJSON *n_arr = cJSON_AddArrayToObject(obj, "arr_k");

        if (n_num) {
            cJSON_SetIntValue(n_num, (int)data[0]);
            cJSON_SetNumberValue(n_num, (double)data[0] + 0.5);
        }
        (void)n_null; (void)n_true; (void)n_false; (void)n_bool;
        (void)n_str; (void)n_raw; (void)n_obj; (void)n_arr;

        cJSON *child = NULL;
        cJSON_ArrayForEach(child, obj) {
            check_all_predicates(child);
        }

        char *printed = cJSON_Print(obj);
        if (printed) cJSON_free(printed);

        cJSON_Delete(obj);
    }

    /* Check invalid node predicate */
    cJSON invalid;
    memset(&invalid, 0, sizeof(invalid));
    check_all_predicates(&invalid);
    check_all_predicates(NULL);

    free(str);
    return 0;
}
