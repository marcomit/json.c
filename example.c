#include "json.h"
#include "zvec.h"
#include <stddef.h>
#include <stdio.h>

typedef struct User {
    char *name;
    int age;
} User;

Json *encodeUser (User *user) {
    JsonMapEntry *entries = NULL;

    vecpush(entries, ((JsonMapEntry){
        .key = "name", .value = JsonString(user->name)
    }));

    vecpush(entries, ((JsonMapEntry){
        .key = "age", .value = JsonNumber(user->age)
    }));

    return JsonMap(entries);
}

int main() {
    User user = {.name = "Marco", .age = 20};

    User users[] = {user, user, user, user};

    Json **list = NULL;

    vecpush(list, encodeUser(users + 0));
    vecpush(list, encodeUser(users + 1));
    vecpush(list, encodeUser(users + 2));

    Json *json = JsonList(list);

    printf("%s\n", JsonEncode(json));

    return 0;
}
