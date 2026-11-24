#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

static size_t allowed_mallocs = 0;

static void *failing_malloc(size_t sz) {
    if (allowed_mallocs == 0) return NULL;
    allowed_mallocs--;
    return malloc(sz);
}

static void my_free(void *ptr) {
    free(ptr);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2 || size > 1024) return 0;

    /* Cover cJSON_InitHooks branches: NULL fields, partial hooks, full hooks */
    cJSON_Hooks default_hooks = { NULL, NULL };
    cJSON_InitHooks(&default_hooks);
    cJSON_Hooks only_malloc = { failing_malloc, NULL };
    cJSON_InitHooks(&only_malloc);
    cJSON_Hooks only_free = { NULL, my_free };
    cJSON_InitHooks(&only_free);

    allowed_mallocs = data[0] % 64;
    uint8_t mode = data[1] % 9;

    cJSON_Hooks hooks;
    hooks.malloc_fn = failing_malloc;
    hooks.free_fn = my_free;

    char *input = (char *)malloc(size);
    if (!input) {
        cJSON_InitHooks(NULL);
        return 0;
    }
    memcpy(input, data + 1, size - 1);
    input[size - 1] = '\0';

    if (mode == 6) {
        /* Build rich tree with normal allocator, then test Duplicate & Print OOM */
        cJSON_InitHooks(NULL);
        cJSON *rich = cJSON_Parse("{\"key\":\"value_string\",\"arr\":[1,2,3,\"hello\",null,true,false],\"nested\":{\"k\":\"v\"}}");
        if (rich) {
            allowed_mallocs = data[0] % 24;
            cJSON_InitHooks(&hooks);
            cJSON *dup = cJSON_Duplicate(rich, 1);
            if (dup) cJSON_Delete(dup);

            allowed_mallocs = data[0] % 16;
            char *p1 = cJSON_Print(rich);
            if (p1) cJSON_free(p1);
            char *p2 = cJSON_PrintBuffered(rich, 4, 1);
            if (p2) cJSON_free(p2);

            cJSON_InitHooks(NULL);
            cJSON_Delete(rich);
        }
        free(input);
        return 0;
    } else if (mode == 7) {
        /* Deterministic rich JSON parse under OOM (hits parse_string/array/object/New_Item OOM) */
        allowed_mallocs = data[0] % 16;
        cJSON_InitHooks(&hooks);
        cJSON *root = cJSON_Parse("{\"key\":\"hello\\u0041world\",\"arr\":[1,2,3],\"obj\":{\"a\":\"b\"}}");
        if (root) cJSON_Delete(root);
        cJSON_InitHooks(NULL);
        free(input);
        return 0;
    } else if (mode == 8) {
        /* Pre-built container, then OOM inside create_reference and replace_item_in_object */
        cJSON_InitHooks(NULL);
        cJSON *obj = cJSON_CreateObject();
        cJSON *arr = cJSON_CreateArray();
        cJSON *item = cJSON_CreateNumber(7);
        cJSON *rep = cJSON_CreateNumber(8);
        if (obj && arr && item && rep) {
            (void)cJSON_AddNumberToObject(obj, "k", 1);
            allowed_mallocs = 0;
            cJSON_InitHooks(&hooks);
            (void)cJSON_AddItemReferenceToArray(arr, item);
            if (!cJSON_ReplaceItemInObject(obj, "k", rep)) {
                cJSON_InitHooks(NULL);
                cJSON_Delete(rep);
                rep = NULL;
            }
            cJSON_InitHooks(NULL);
        }
        if (rep) cJSON_Delete(rep);
        if (item) cJSON_Delete(item);
        if (arr) cJSON_Delete(arr);
        if (obj) cJSON_Delete(obj);
        free(input);
        return 0;
    }

    cJSON_InitHooks(&hooks);

    if (mode == 0) {
        /* Parse + Print + Buffered + Duplicate under OOM */
        cJSON *root = cJSON_Parse(input);
        if (root) {
            char *p1 = cJSON_Print(root);
            if (p1) cJSON_free(p1);

            char *p2 = cJSON_PrintUnformatted(root);
            if (p2) cJSON_free(p2);

            char *p3 = cJSON_PrintBuffered(root, 4, 1);
            if (p3) cJSON_free(p3);

            cJSON *dup = cJSON_Duplicate(root, 1);
            if (dup) cJSON_Delete(dup);

            cJSON_Delete(root);
        }
    } else if (mode == 1) {
        /* All basic Create* APIs under OOM */
        cJSON *items[12];
        items[0] = cJSON_CreateNull();
        items[1] = cJSON_CreateTrue();
        items[2] = cJSON_CreateFalse();
        items[3] = cJSON_CreateBool(data[0] & 1);
        items[4] = cJSON_CreateNumber((double)data[0]);
        items[5] = cJSON_CreateString(input);
        items[6] = cJSON_CreateRaw(input);
        items[7] = cJSON_CreateArray();
        items[8] = cJSON_CreateObject();
        items[9] = cJSON_CreateStringReference(input);
        items[10] = cJSON_CreateObjectReference(items[8]);
        items[11] = cJSON_CreateArrayReference(items[7]);
        for (int i = 11; i >= 0; --i) {
            if (items[i]) cJSON_Delete(items[i]);
        }
    } else if (mode == 2) {
        /* Create*Array APIs under OOM */
        int ints[4] = {1, 2, 3, 4};
        float floats[4] = {1.5f, 2.5f, 3.5f, 4.5f};
        double doubles[4] = {1.25, 2.25, 3.25, 4.25};
        const char *strs[4] = {"alpha", "beta", "gamma", "delta"};

        cJSON *a1 = cJSON_CreateIntArray(ints, 4);
        if (a1) cJSON_Delete(a1);
        cJSON *a2 = cJSON_CreateFloatArray(floats, 4);
        if (a2) cJSON_Delete(a2);
        cJSON *a3 = cJSON_CreateDoubleArray(doubles, 4);
        if (a3) cJSON_Delete(a3);
        cJSON *a4 = cJSON_CreateStringArray(strs, 4);
        if (a4) cJSON_Delete(a4);
    } else if (mode == 3) {
        /* Add*ToObject helpers under OOM */
        cJSON *obj = cJSON_CreateObject();
        if (obj) {
            (void)cJSON_AddNullToObject(obj, "n");
            (void)cJSON_AddTrueToObject(obj, "t");
            (void)cJSON_AddFalseToObject(obj, "f");
            (void)cJSON_AddBoolToObject(obj, "b", 1);
            (void)cJSON_AddNumberToObject(obj, "num", 3.14);
            (void)cJSON_AddStringToObject(obj, "s", input);
            (void)cJSON_AddRawToObject(obj, "r", "{\"k\":1}");
            (void)cJSON_AddObjectToObject(obj, "o");
            (void)cJSON_AddArrayToObject(obj, "a");
            cJSON_Delete(obj);
        }
    } else if (mode == 4) {
        /* Add / Reference / Replace / SetValuestring under OOM */
        cJSON *obj = cJSON_CreateObject();
        cJSON *arr = cJSON_CreateArray();
        cJSON *str = cJSON_CreateString("ab");
        if (str) {
            (void)cJSON_SetValuestring(str, "a_much_longer_string_that_triggers_allocation");
        }
        if (obj && str) {
            if (!cJSON_AddItemToObject(obj, "k1", str)) {
                cJSON_Delete(str);
                str = NULL;
            }
        } else if (str) {
            cJSON_Delete(str);
            str = NULL;
        }
        if (obj) {
            cJSON *rep = cJSON_CreateNumber(99);
            if (rep && !cJSON_ReplaceItemInObject(obj, "k1", rep)) {
                cJSON_Delete(rep);
            }
            cJSON *n = cJSON_CreateNull();
            if (n) {
                (void)cJSON_AddItemReferenceToObject(obj, "ref", n); //memory leak if OOM
                if (arr) (void)cJSON_AddItemReferenceToArray(arr, n);
                cJSON_Delete(n);
            }
        }
        if (arr) cJSON_Delete(arr);
        if (obj) cJSON_Delete(obj);
    } else {
        /* Print with raw + string escaping + array/object under OOM */
        cJSON *obj = cJSON_CreateObject();
        if (obj) {
            (void)cJSON_AddStringToObject(obj, "esc", "a\"b\\c\n\t\r\b\f\x01");
            (void)cJSON_AddRawToObject(obj, "raw", "[1,2,3]");
            (void)cJSON_AddNumberToObject(obj, "num", 12345.6789);
            cJSON *sub = cJSON_AddArrayToObject(obj, "arr");
            if (sub) {
                cJSON *x = cJSON_CreateTrue();
                if (x && !cJSON_AddItemToArray(sub, x)) cJSON_Delete(x);
                cJSON *y = cJSON_CreateFalse();
                if (y && !cJSON_AddItemToArray(sub, y)) cJSON_Delete(y);
                cJSON *z = cJSON_CreateNull();
                if (z && !cJSON_AddItemToArray(sub, z)) cJSON_Delete(z);
            }
            char *printed = cJSON_Print(obj);
            if (printed) cJSON_free(printed);
            char *unfmt = cJSON_PrintUnformatted(obj);
            if (unfmt) cJSON_free(unfmt);
            cJSON_Delete(obj);
        }
    }

    free(input);
    cJSON_InitHooks(NULL);
    return 0;
}
