#include "json.h"
#include "zvec.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void _JsonEncode(Json *, char **);
static Json *_JsonDecode(char **);

Json *Obj(int type) {
    Json *self = malloc(sizeof(Json));
    *self = (Json){ 0 };
    self->type = type;
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
    Json *res = Obj(JSON_NUMBER);
    res->num = val;
    return res;
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
            return Obj(JSON_NULL);
        } else if (strncmp(*src, "true", 4) == 0) {
            *src += 4;
            Json *obj = Obj(JSON_BOOL);
            obj->boolean = 1;
            return obj;
        } else if (strncmp(*src, "false", 5) == 0) {
            *src += 5;
            Json *obj = Obj(JSON_BOOL);
            obj->boolean = 0;
            return obj;
        }
        return NULL;
    }
}

Json *JsonDecode(char *src) {
    char *source = src;
    return _JsonDecode(&source);
}
