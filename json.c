#include "zvec.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void _JsonEncode(Json *, char **);
static Json *_JsonDecode(char **);

Json *Obj(int type) {
    Json *self = malloc(sizeof(Json));
    *self = (Json){ 0 };
    self->type = type;
    return self;
}

static Json *String(char *str) {
    Json *self = Obj(JSON_STRING);
    self->string = strdup(str);
    return self;
}

static Json *Number(double num) {
    Json *self = Obj(JSON_NUMBER);
    self->num = num;
    return self;
}

static Json *Bool(int val) {
    Json *self = Obj(JSON_BOOL);
    self->boolean = val > 0;
    return self;
}

static Json *List(Json **array) {
    Json *self = Obj(JSON_LIST);
    self->list = NULL;

    while (*array) {
        vecpush(self->list, *array);
        array++;
    }
    return self;
}

static JsonMapEntry *Entry(char *key, Json *value) {
    JsonMapEntry *self = malloc(sizeof(JsonMapEntry));
    self->key = strdup(key);
    self->value = value;
    return self;
}

static Json *Null() { return Obj(JSON_NULL); }

static Json *Map(JsonMapEntry *entries) {
    Json *self = Obj(JSON_MAP);
    self->map = NULL;

    while (entries->key != NULL) {
        vecpush(self->map, *entries);
        entries++;
    }
    return self;
}

static void JsonEncodeList(Json *list, char **buf) {
    vecpush(*buf, '[');
    size_t len = veclen(list->list);
    for (size_t i = 0; i < len; i++) {
        _JsonEncode(list->list[i], buf);

        if (i + 1 < len) vecpush(*buf, ',');
    }
    vecpush(*buf, ']');
}

static void JsonEncodeMap(Json *map, char **buf) {
    if (!map) return;
    vecpush(*buf, '{');
    size_t len = veclen(map->map);
    for (size_t i = 0; i < len; i++) {
        vecpush(*buf, '"');
        for (size_t j = 0; j < strlen(map->map[i].key); j++)
            vecpush(*buf, map->map[i].key[j]);
        vecpush(*buf, '"');
        vecpush(*buf, ':');

        _JsonEncode(map->map[i].value, buf);
        if (i + 1 < len) vecpush(*buf, ',');
    }
    vecpush(*buf, '}');
}

static void JsonEncodeNumber(Json *root, char **buf) {
    char num[100] = { 0 };

    sprintf(num, "%g", root->num);
    vecunion(*buf, num, strlen(num));
}

static void _JsonEncode(Json *root, char **buf) {
    if (!root || !buf || !*buf) return;
    switch (root->type) {
    case JSON_BOOL:
        if(root->boolean) vecunion(*buf, "true", 4);
        else vecunion(*buf, "false", 5);
        break;
    case JSON_NULL: vecunion(*buf, "null", 4); break;
    case JSON_STRING:
        vecpush(*buf, '"');
        vecunion(*buf, root->string, strlen(root->string));
        vecpush(*buf, '"');
        break;
    case JSON_NUMBER:   JsonEncodeNumber(root, buf);    break;
    case JSON_LIST:     JsonEncodeList(root, buf);      break;
    case JSON_MAP:      JsonEncodeMap(root, buf);       break;
    default:            *buf = NULL;                    break;
    }
}

char *JsonEncode(Json *root) {
    char *buf = NULL;
    _JsonEncode(root, &buf);
    if (!buf) return NULL;
    vecpush(buf, '\0');
    return buf;
}

static void SkipWhitespace(char **src) {
    while (**src && isspace(**src)) (*src)++;
}

char *DecodeString(char **src) {
    if (**src != '"') return NULL;
    (*src)++;
    char *buf = NULL;

    while (**src && **src != '"') {
        if (**src == '\\') {
            (*src)++;
            switch (**src) {
            case '"':  vecpush(buf, '"');  break;
            case '\\': vecpush(buf, '\\'); break;
            case '/':  vecpush(buf, '/');  break;
            case 'n':  vecpush(buf, '\n'); break;
            case 't':  vecpush(buf, '\t'); break;
            case 'r':  vecpush(buf, '\r'); break;
            case 'b':  vecpush(buf, '\b'); break;
            case 'f':  vecpush(buf, '\f'); break;
            default:   vecpush(buf, **src); break;
            }
        } else {
            vecpush(buf, **src);
        }
        (*src)++;
    }

    if (**src != '"') return NULL;
    (*src)++;

    vecpush(buf, '\0');
    return buf;
}

static Json *JsonDecodeString(char **src) {
    char *key = DecodeString(src);
    if (!key) return NULL;
    Json *str = Obj(JSON_STRING);
    str->string = key;
    return str;
}

static Json *JsonDecodeNum(char **src) {
    char *end;
    double val = strtod(*src, &end);
    if (end == *src) return NULL;
    *src = end;
    return Number(val);
}

static Json *JsonDecodeList(char **src) {
    (*src)++; // skip '['
    SkipWhitespace(src);

    Json *list = Obj(JSON_LIST);

    if (**src == ']') {
        (*src)++;
        return list;
    }

    while (1) {
        SkipWhitespace(src);
        Json *val = _JsonDecode(src);
        if (!val) return NULL;
        vecpush(list->list, val);

        SkipWhitespace(src);
        if (**src == ']') break;
        if (**src != ',') return NULL;
        (*src)++;
    }

    (*src)++;
    return list;
}

static Json *JsonDecodeMap(char **src) {
    (*src)++; // skip '{'
    SkipWhitespace(src);

    Json *map = Obj(JSON_MAP);

    if (**src == '}') {
        (*src)++;
        return map;
    }

    while (1) {
        SkipWhitespace(src);
        char *key = DecodeString(src);
        if (!key) return NULL;

        SkipWhitespace(src);
        if (**src != ':') return NULL;
        (*src)++;

        SkipWhitespace(src);
        Json *value = _JsonDecode(src);
        if (!value) return NULL;

        JsonMapEntry entry = (JsonMapEntry){ key, value };
        vecpush(map->map, entry);

        SkipWhitespace(src);
        if (**src != ',') break;
        (*src)++;
    }

    if (**src != '}') return NULL;
    (*src)++;
    return map;
}

static Json *_JsonDecode(char **src) {
    SkipWhitespace(src);
    switch (**src) {
    case '[': return JsonDecodeList(src);
    case '{': return JsonDecodeMap(src);
    case '"': return JsonDecodeString(src);
    default:
        if (isdigit(**src) || **src == '-') {
            return JsonDecodeNum(src);
        } else if (strncmp(*src, "null", 4) == 0) {
            *src += 4;
            return Null();
        } else if (strncmp(*src, "true", 4) == 0) {
            *src += 4;
            return Bool(1);
        } else if (strncmp(*src, "false", 5) == 0) {
            *src += 5;
            return Bool(0);
        }
        return NULL;
    }
}

Json *JsonDecode(char *src) {
    char *source = src;
    return _JsonDecode(&source);
}
