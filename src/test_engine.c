#include <stdio.h>
#include <assert.h>
#include "game_logic.h"
#include "shop.h"

int main(void) {
    printf("============================================================\n");
    printf("   KIEM THU GAME ENGINE - GIAI DOAN 1 (MATCH & SHOP)\n");
    printf("============================================================\n");

    int team1[] = {10, 11};
    int team2[] = {20, 21};

    Match *m = create_match(1, team1, 2, 2, team2, 2);
    assert(m != NULL);
    assert(m->ship_count == 4);
    printf("[PASS] Ca 1: Khoi tao phong dau 2v2 thanh cong (Match #%d)\n", m->match_id);

    Spaceship *s10 = NULL;
    assert(find_match_by_fd(10, &s10) == m);
    assert(s10->hp == 1000 && s10->armor == 0 && s10->weapon_count == 1);
    assert(s10->weapons[0].dam == 10 && s10->weapons[0].ammo == 50);
    printf("[PASS] Ca 2: Thong so tau mac dinh (HP=1000, Armor=0, Phao 30mm)\n");

    assert(buy_item(10, ITEM_LASER_CANNON) == -2);
    printf("[PASS] Ca 3: Chan mua vu khi khi khong du Coin\n");

    s10->coin = 5000;
    assert(buy_item(10, ITEM_LASER_CANNON) == 0);
    assert(buy_item(10, ITEM_MISSILE) == 0);
    assert(buy_item(10, ITEM_BASIC_ARMOR) == 0);
    assert(buy_item(10, ITEM_BASIC_ARMOR) == 0);
    assert(s10->coin == 0 && s10->armor == 600 && s10->weapon_count == 3);

    s10->coin = 5000;
    assert(buy_item(10, ITEM_HEAVY_ARMOR) == -5);
    printf("[PASS] Ca 4: Mua vu khi, cong don giap va chan tran slot giap (MAX=2)\n");

    s10->hp = 750;
    int healed = repair_ship(10, 400);
    assert(healed == 250 && s10->hp == 1000 && s10->coin == 4750);
    printf("[PASS] Ca 5: Sua chua tau dung ty le 1c = 1HP va khong vuot max_hp\n");

    destroy_match(m->match_id);
    assert(find_match_by_fd(10, NULL) == NULL);
    printf("[PASS] Ca 6: Don dep phong dau sau khi ket thuc\n");

    printf("============================================================\n");
    printf("Ket qua: 6/6 ca kiem thu Game Engine THANH CONG (PASS).\n");
    printf("============================================================\n");
    return 0;
}
