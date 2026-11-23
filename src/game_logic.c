#include "game_logic.h"
#include "logger.h"
#include <string.h>
#include <stdio.h>

static Match matches[MAX_MATCHES];
static int next_match_id = 1;

void init_spaceship(Spaceship *ship, int owner_fd, int team_id) {
    if (!ship) return;
    memset(ship, 0, sizeof(Spaceship));

    ship->owner_fd = owner_fd;
    ship->team_id = team_id;
    ship->hp = 1000;
    ship->max_hp = 1000;
    ship->armor = 0;
    ship->coin = 0;
    ship->is_dead = false;

    ship->weapons[0].id = 1;
    strncpy(ship->weapons[0].name, "Phao 30mm", sizeof(ship->weapons[0].name) - 1);
    ship->weapons[0].dam = 10;
    ship->weapons[0].ammo = 50;
    ship->weapon_count = 1;
    ship->armor_count = 0;
}

Match* create_match(int team1_id, const int team1_fds[], int count1,
                    int team2_id, const int team2_fds[], int count2) {
    if (count1 <= 0 || count1 > TEAM_SIZE || count2 <= 0 || count2 > TEAM_SIZE) {
        LOG_ERROR("So luong thanh vien khong hop le khi tao tran (%d vs %d)", count1, count2);
        return NULL;
    }

    for (int i = 0; i < MAX_MATCHES; i++) {
        if (!matches[i].is_active) {
            memset(&matches[i], 0, sizeof(Match));
            matches[i].match_id = next_match_id++;
            matches[i].team1_id = team1_id;
            matches[i].team2_id = team2_id;
            matches[i].is_active = true;

            int idx = 0;
            for (int j = 0; j < count1; j++) {
                init_spaceship(&matches[i].ships[idx++], team1_fds[j], team1_id);
            }
            for (int j = 0; j < count2; j++) {
                init_spaceship(&matches[i].ships[idx++], team2_fds[j], team2_id);
            }
            matches[i].ship_count = idx;

            LOG_INFO("Tao Match #%d thanh cong: Team %d (%d tau) vs Team %d (%d tau)",
                     matches[i].match_id, team1_id, count1, team2_id, count2);
            return &matches[i];
        }
    }

    LOG_WARN("Da dat gioi han MAX_MATCHES (%d), khong the tao them phong dau", MAX_MATCHES);
    return NULL;
}

Match* find_match_by_fd(int fd, Spaceship **out_ship) {
    for (int i = 0; i < MAX_MATCHES; i++) {
        if (!matches[i].is_active) continue;
        for (int j = 0; j < matches[i].ship_count; j++) {
            if (matches[i].ships[j].owner_fd == fd) {
                if (out_ship) {
                    *out_ship = &matches[i].ships[j];
                }
                return &matches[i];
            }
        }
    }
    if (out_ship) *out_ship = NULL;
    return NULL;
}

void destroy_match(int match_id) {
    for (int i = 0; i < MAX_MATCHES; i++) {
        if (matches[i].is_active && matches[i].match_id == match_id) {
            matches[i].is_active = false;
            LOG_INFO("Da don dep va dong Match #%d", match_id);
            return;
        }
    }
}
