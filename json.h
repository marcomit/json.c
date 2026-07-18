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


char *JsonEncode(Json *);
Json *JsonDecode(char *);

Json *JsonList  (Json **);
Json *JsonBool  (int);
Json *JsonNumber(double);
Json *JsonString(char *);
Json *JsonMap   (JsonMapEntry *);
Json *JsonNull  (void);
#endif // JSON_H
