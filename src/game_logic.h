#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_WEAPONS 4
#define MAX_ARMORS 2
#define TEAM_SIZE 3
#define MAX_MATCHES 16

typedef struct {
    int id;
    char name[32];
    int dam;
    int ammo;
} Weapon;

typedef struct {
    int id;
    char name[32];
    int armor_points;
} Armor;

typedef struct {
    int owner_fd;
    int team_id;
    int hp;
    int max_hp;
    int armor;
    Weapon weapons[MAX_WEAPONS];
    int weapon_count;
    Armor armors[MAX_ARMORS];
    int armor_count;
    int coin;
    bool is_dead;
} Spaceship;

typedef struct {
    int match_id;
    int team1_id;
    int team2_id;
    Spaceship ships[TEAM_SIZE * 2];
    int ship_count;
    bool is_active;
} Match;

void init_spaceship(Spaceship *ship, int owner_fd, int team_id);
Match* create_match(int team1_id, const int team1_fds[], int count1,
                    int team2_id, const int team2_fds[], int count2);
Match* find_match_by_fd(int fd, Spaceship **out_ship);
void destroy_match(int match_id);

#endif
