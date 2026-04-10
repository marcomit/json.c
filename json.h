#ifndef JSON_H
#define JSON_H

#define JSON_TERMINAL_MASK 1 << 8
#define JSON_COMPOSED_MASK 1 << 9

typedef struct Json Json;

enum {
    JSON_NULL = JSON_TERMINAL_MASK,
    JSON_BOOL,
    JSON_STRING,
    JSON_NUMBER,

    JSON_LIST = JSON_COMPOSED_MASK,
    JSON_MAP
};

typedef struct JsonMapEntry {
    char    *key;
    Json    *value;
} JsonMapEntry;

struct Json {
    int type;

    union {
        double num;
        int boolean;
        char *string;
        Json **list;
        JsonMapEntry *map;
    };
};

char *JsonEncode(Json *);
Json *JsonDecode(char *);

#endif // JSON_H
