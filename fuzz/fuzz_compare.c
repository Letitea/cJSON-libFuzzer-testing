#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2 || size > 8192) return 0;

    /* Split input into two JSON strings to compare */
    size_t split = (size_t)data[0] % (size - 1);
    const uint8_t *p1 = data + 1;
    size_t len1 = split;
    const uint8_t *p2 = data + 1 + split;
    size_t len2 = (size - 1) - split;

    cJSON *a = cJSON_ParseWithLength((const char *)p1, len1);
    cJSON *b = cJSON_ParseWithLength((const char *)p2, len2);

    (void)cJSON_Compare(a, b, 0);
    (void)cJSON_Compare(a, b, 1);

    if (a) {
        (void)cJSON_Compare(a, a, 1);
        (void)cJSON_Compare(a, NULL, 1);
        (void)cJSON_Compare(NULL, a, 0);

        cJSON *dup = cJSON_Duplicate(a, 1);
        if (dup) {
            (void)cJSON_Compare(a, dup, 0);
            (void)cJSON_Compare(a, dup, 1);

            /* Mutate dup slightly to exercise mismatch paths inside arrays/objects/numbers/strings */
            if (cJSON_IsObject(dup)) {
                (void)cJSON_AddNumberToObject(dup, "EXTRA_KEY", 123);
                (void)cJSON_Compare(a, dup, 0);
                (void)cJSON_Compare(a, dup, 1);
                (void)cJSON_Compare(dup, a, 1);
            } else if (cJSON_IsArray(dup)) {
                cJSON *extra = cJSON_CreateNull();
                if (extra && !cJSON_AddItemToArray(dup, extra)) cJSON_Delete(extra);
                (void)cJSON_Compare(a, dup, 1);
                (void)cJSON_Compare(dup, a, 1);
            }
            cJSON_Delete(dup);
        }
    }

    /* Compare cJSON_Raw (equal, unequal, NULL valuestring) & case-sensitive object keys */
    cJSON *raw1 = cJSON_CreateRaw("{\"x\":1}");
    cJSON *raw2 = cJSON_CreateRaw("{\"x\":1}");
    cJSON *raw3 = cJSON_CreateRaw("{\"x\":2}");
    cJSON raw_null;
    memset(&raw_null, 0, sizeof(raw_null));
    raw_null.type = cJSON_Raw;
    raw_null.valuestring = NULL;

    (void)cJSON_Compare(raw1, raw2, 1);
    (void)cJSON_Compare(raw1, raw3, 1);
    (void)cJSON_Compare(raw1, &raw_null, 1);
    (void)cJSON_Compare(&raw_null, raw1, 1);
    if (raw1) cJSON_Delete(raw1);
    if (raw2) cJSON_Delete(raw2);
    if (raw3) cJSON_Delete(raw3);

    /* Compare strings with NULL valuestring */
    cJSON str_null;
    memset(&str_null, 0, sizeof(str_null));
    str_null.type = cJSON_String;
    str_null.valuestring = NULL;
    cJSON *s1 = cJSON_CreateString("abc");
    (void)cJSON_Compare(s1, &str_null, 1);
    (void)cJSON_Compare(&str_null, s1, 1);
    if (s1) cJSON_Delete(s1);

    /* Compare objects with case-differing keys & same keys with different values */
    cJSON *o1 = cJSON_CreateObject();
    cJSON *o2 = cJSON_CreateObject();
    cJSON *o3 = cJSON_CreateObject();
    if (o1 && o2 && o3) {
        (void)cJSON_AddNumberToObject(o1, "AbC", 1);
        (void)cJSON_AddNumberToObject(o2, "abc", 1);
        (void)cJSON_AddNumberToObject(o3, "AbC", 2);
        (void)cJSON_Compare(o1, o2, 0); /* equal case-insensitive */
        (void)cJSON_Compare(o1, o2, 1); /* unequal case-sensitive */
        (void)cJSON_Compare(o1, o3, 1); /* same key, different value */
        (void)cJSON_AddNumberToObject(o2, "extra", 9);
        (void)cJSON_Compare(o1, o2, 0); /* different key count */
    }
    if (o1) cJSON_Delete(o1);
    if (o2) cJSON_Delete(o2);
    if (o3) cJSON_Delete(o3);

    /* Hit second object loop mismatch (cJSON_Compare L3184): a={"A":1}, b={"a":1,"A":2} */
    cJSON *m1 = cJSON_CreateObject();
    cJSON *m2 = cJSON_CreateObject();
    if (m1 && m2) {
        (void)cJSON_AddNumberToObject(m1, "A", 1);
        (void)cJSON_AddNumberToObject(m2, "a", 1);
        (void)cJSON_AddNumberToObject(m2, "A", 2);
        (void)cJSON_Compare(m1, m2, 0);
    }
    if (m1) cJSON_Delete(m1);
    if (m2) cJSON_Delete(m2);

    /* Compare invalid & mismatched types & unknown type default branch (two distinct pointers!) */
    cJSON unk1, unk2;
    memset(&unk1, 0, sizeof(unk1));
    memset(&unk2, 0, sizeof(unk2));
    unk1.type = 255;
    unk2.type = 255;
    (void)cJSON_Compare(&unk1, &unk2, 1);

    if (a) cJSON_Delete(a);
    if (b) cJSON_Delete(b);
    return 0;
}
