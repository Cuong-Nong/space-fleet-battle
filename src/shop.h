#ifndef SHOP_H
#define SHOP_H

typedef enum {
    ITEM_AMMO_30MM      = 1,
    ITEM_LASER_CANNON   = 2,
    ITEM_LASER_BATTERY  = 3,
    ITEM_MISSILE        = 4,
    ITEM_BASIC_ARMOR    = 5,
    ITEM_HEAVY_ARMOR    = 6
} ShopItemCode;

int buy_item(int fd, int item_code);
int repair_ship(int fd, int hp_amount);

#endif
