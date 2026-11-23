#ifndef CHEST_H
#define CHEST_H

#include <stdbool.h>

typedef enum {
    CHEST_BRONZE = 100,
    CHEST_SILVER = 500,
    CHEST_GOLD   = 2000
} ChestType;

typedef struct {
    int chest_id;
    int match_id;
    ChestType type;
    char question[256];
    char answer[64];
    bool is_opened;
} Chest;

int load_questions(const char *filepath);
Chest* spawn_random_chest(int match_id);
int handle_chest_answer(int fd, int chest_id, const char *user_answer, int *reward_coin);

#endif
