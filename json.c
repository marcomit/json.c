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

Json *Obj(int type) {
    Json *self = malloc(sizeof(Json));
    *self = (Json){ 0 };
    self->type = type;
    return self;
}

Json *String(char *str) {
    Json *self = Obj(JSON_STRING);
    self->string = strdup(str);
    return self;
}

Json *Number(double num) {
    Json *self = Obj(JSON_NUMBER);
    self->num = num;
    return self;
}

Json *Bool(int val) {
    Json *self = Obj(JSON_BOOL);
    self->boolean = val > 0;
    return self;
}

Json *List(Json **array) {
    Json *self = Obj(JSON_LIST);
    self->list = NULL;

    while (*array) {
        vecpush(self->list, *array);
        array++;
    }
    return self;
}

JsonMapEntry *Entry(char *key, Json *value) {
    JsonMapEntry *self = malloc(sizeof(JsonMapEntry));
    self->key = strdup(key);
    self->value = value;
    return self;
}

Json *Null() { return Obj(JSON_NULL); }

Json *Map(JsonMapEntry *entries) {
    Json *self = Obj(JSON_MAP);
    self->map = NULL;

    while (entries->key != NULL) {
        vecpush(self->map, *entries);
        entries++;
    }
    return self;
}

char *JsonEncode(Json *);
void _JsonEncode(Json *, char **);

void JsonEncodeList(Json *list, char **buf) {
    vecpush(*buf, '[');
    size_t len = veclen(list->list);
    for (size_t i = 0; i < len; i++) {
        _JsonEncode(list->list[i], buf);

        if (i + 1 < len) vecpush(*buf, ',');
    }
    vecpush(*buf, ']');
}

void JsonEncodeMap(Json *map, char **buf) {
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

void JsonEncodeNumber(Json *root, char **buf) {
    char num[100] = { 0 };

    sprintf(num, "%g", root->num);
    vecunion(*buf, num, strlen(num));
}

char *JsonEncode(Json *root) {
    char *buf = NULL;
    _JsonEncode(root, &buf);
    vecpush(buf, '\0');
    return buf;
}


Json *_JsonDecode(char **);

Json *JsonDecodeList(char **src) {
    if (**src == '[') (*src)++;

    Json *list = Obj(JSON_LIST);
    while (**src != ']') {
        Json *val = _JsonDecode(src);
        if (!val) return NULL;

        if (**src == ']') break; 
        if (**src != ',') return NULL;
        (*src)++;

        vecpush(list->list, val);
    }

    (*src)++;
    return list;
}

Json *JsonDecodeMap(char **src) {
    return NULL;
}

Json *JsonDecodeString(char **src) {
    return NULL;
}

Json *JsonDecodeNum(char **src) {
    return NULL;
}

Json *_JsonDecode(char **src) {
    switch (**src) {
    case '[': return JsonDecodeList(src);
    case '{': return JsonDecodeMap(src);
    case '"': return JsonDecodeString(src);
    default:
        if (isdigit(**src)) {
            return JsonDecodeNum(src);
        } else if (strncmp(*src, "null", 4) == 0) {
            src += 4;
            return Null();
        } else if (strncmp(*src, "true", 4) == 0) {
            src += 4;
            return Bool(1);
        } else if (strncmp(*src, "false", 5) == 0) {
            src += 5;
            return Bool(0);
        }
        return NULL;
    }
}

Json *JsonDecode(char *src) {
    char *source = src;
    return _JsonDecode(&source);
}

void _JsonEncode(Json *root, char **buf) {
    if (!root) return;
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
    default:            printf("Not handled\n");        break;
    }
}
