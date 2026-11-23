#include "shop.h"
#include "game_logic.h"
#include "logger.h"
#include <string.h>

static Weapon* find_weapon_on_ship(Spaceship *ship, int weapon_id) {
    for (int i = 0; i < ship->weapon_count; i++) {
        if (ship->weapons[i].id == weapon_id) {
            return &ship->weapons[i];
        }
    }
    return NULL;
}

int buy_item(int fd, int item_code) {
    Spaceship *ship = NULL;
    Match *m = find_match_by_fd(fd, &ship);
    if (!m || !ship || ship->is_dead) {
        return -1;
    }

    switch (item_code) {
        case ITEM_AMMO_30MM: {
            if (ship->coin < 100) return -2;
            Weapon *w = find_weapon_on_ship(ship, 1);
            if (!w) return -3;
            ship->coin -= 100;
            w->ammo += 50;
            LOG_INFO("[Match #%d] FD %d mua 50 dan Phao 30mm (-100c, con %dc)", m->match_id, fd, ship->coin);
            return 0;
        }
        case ITEM_LASER_CANNON: {
            if (ship->coin < 1000) return -2;
            Weapon *existing = find_weapon_on_ship(ship, 2);
            if (existing) {
                ship->coin -= 1000;
                existing->ammo += 10;
                LOG_INFO("[Match #%d] FD %d nap them Phao Laze (-1000c)", m->match_id, fd);
                return 0;
            }
            if (ship->weapon_count >= MAX_WEAPONS) return -4;
            ship->coin -= 1000;
            Weapon *w = &ship->weapons[ship->weapon_count++];
            w->id = 2;
            strncpy(w->name, "Phao Laze", sizeof(w->name) - 1);
            w->dam = 80;
            w->ammo = 10;
            LOG_INFO("[Match #%d] FD %d trang bi Phao Laze (-1000c, con %dc)", m->match_id, fd, ship->coin);
            return 0;
        }
        case ITEM_LASER_BATTERY: {
            if (ship->coin < 100) return -2;
            Weapon *w = find_weapon_on_ship(ship, 2);
            if (!w) return -3;
            ship->coin -= 100;
            w->ammo += 10;
            LOG_INFO("[Match #%d] FD %d mua Nguon Laze (-100c, con %dc)", m->match_id, fd, ship->coin);
            return 0;
        }
        case ITEM_MISSILE: {
            if (ship->coin < 2000) return -2;
            Weapon *existing = find_weapon_on_ship(ship, 3);
            if (existing) {
                ship->coin -= 2000;
                existing->ammo += 2;
                LOG_INFO("[Match #%d] FD %d mua them 2 Ten lua (-2000c)", m->match_id, fd);
                return 0;
            }
            if (ship->weapon_count >= MAX_WEAPONS) return -4;
            ship->coin -= 2000;
            Weapon *w = &ship->weapons[ship->weapon_count++];
            w->id = 3;
            strncpy(w->name, "Ten lua", sizeof(w->name) - 1);
            w->dam = 250;
            w->ammo = 2;
            LOG_INFO("[Match #%d] FD %d trang bi Ten lua (-2000c, con %dc)", m->match_id, fd, ship->coin);
            return 0;
        }
        case ITEM_BASIC_ARMOR: {
            if (ship->coin < 1000) return -2;
            if (ship->armor_count >= MAX_ARMORS) return -5;
            ship->coin -= 1000;
            Armor *a = &ship->armors[ship->armor_count++];
            a->id = 1;
            strncpy(a->name, "Giap co ban", sizeof(a->name) - 1);
            a->armor_points = 300;
            ship->armor += 300;
            LOG_INFO("[Match #%d] FD %d lap Giap co ban (+300 Armor, con %dc)", m->match_id, fd, ship->coin);
            return 0;
        }
        case ITEM_HEAVY_ARMOR: {
            if (ship->coin < 2000) return -2;
            if (ship->armor_count >= MAX_ARMORS) return -5;
            ship->coin -= 2000;
            Armor *a = &ship->armors[ship->armor_count++];
            a->id = 2;
            strncpy(a->name, "Giap tang cuong", sizeof(a->name) - 1);
            a->armor_points = 700;
            ship->armor += 700;
            LOG_INFO("[Match #%d] FD %d lap Giap tang cuong (+700 Armor, con %dc)", m->match_id, fd, ship->coin);
            return 0;
        }
        default:
            return -6;
    }
}

int repair_ship(int fd, int hp_amount) {
    if (hp_amount <= 0) return -1;

    Spaceship *ship = NULL;
    Match *m = find_match_by_fd(fd, &ship);
    if (!m || !ship || ship->is_dead) {
        return -1;
    }

    int missing_hp = ship->max_hp - ship->hp;
    if (missing_hp <= 0) return 0;

    int actual_repair = hp_amount;
    if (actual_repair > missing_hp) actual_repair = missing_hp;
    if (actual_repair > ship->coin) actual_repair = ship->coin;

    if (actual_repair <= 0) return -2;

    ship->coin -= actual_repair;
    ship->hp += actual_repair;

    LOG_INFO("[Match #%d] FD %d sua chua +%d HP (HP hien tai: %d/%d, con %dc)",
             m->match_id, fd, actual_repair, ship->hp, ship->max_hp, ship->coin);
    return actual_repair;
}
