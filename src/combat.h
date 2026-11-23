#ifndef COMBAT_H
#define COMBAT_H

#include <stdbool.h>

typedef struct {
    int attacker_fd;
    int target_fd;
    int damage_dealt;
    int target_remaining_hp;
    int target_remaining_armor;
    bool target_destroyed;
    int winning_team_id;
} AttackResult;

int execute_attack(int attacker_fd, int target_fd, int weapon_idx, AttackResult *out_res);
int check_game_over(int match_id);

#endif
