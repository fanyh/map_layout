#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include <lua.h>
#include <luaxlib.h>

#define MX 30    //now sport 1m grid
#define MY MX

#define CELL_LEN 10

typedef struct point_s {
    uint8_t state:2;
    uint8_t side:4;      //sidexside
    uint8_t watcher:1;
} point_t;

typedef struct map_s{
    point_t point[MX][MY];
} map_t;

/* think to change x y
typedef struct range_s {
    uint16_t x;
    uint16_t y;
    uint16_t side;
} range_t;
*/
static inline map_t*
__map_instance(lua_State *L){
    map_t *u = lua_touserdata(L, 1);
    if( u == NULL ){
        luaL_error(L, "must be map_t object");
    }

    return u;
}

//static uint8_t
//__is_edge(uint16_t x, uint16_t y, uint8_t side){
//}
//

static uint8_t
__is_filled(map_t *t, uint16_t x, uint16_t y, uint8_t side){
    uint16_t i,j;
    for(i = x; i < x + side; ++i){
        for(j = y; j < j + side; ++j){
            if(t->point[i][j].state != 0){
               return 1; 
            }
        }
    }

    return 0;
}

static void
__fill(map_t *t, uint16_t x, uint16_t y, uint8_t side){
    uint16_t i,j;
    for(i = x; i < i + side; ++i){
        for(j = y; j < j + side; ++j){
            t->point[i][j].state = 1;
        }
    }
}

static void
__clear(map_t *t, uint16_t x, uint16_t y, uint8_t side){
    uint16_t i,j;
    for(i = x; i < i + side; ++i){
        for(j = y; j < j + side; ++j){
            point_t p;
            t->point[i][j].state = p;
        }
    }
}

static uint8_t
__can_add(map_t *t, uint16_t x, uint16_t y, uint8_t side){
    if(x > MX || y >MY ){
        return 0;
    }

    if (x + side > MX || y + side > MY){
        return 0;
    }

    return __is_filled(t, x, y, side);
}

static uint32_t
__get_lid(uint16_t x, uint16_t y){
    return x << 16 | y;
}

static uint8_t
__get_coordinate(uint32_t lid, uint16_t *x, uint16_t *y){
    uint16_t ly = lid & 0xffff;
    uint16_t lx = lid >> 16 & 0xffff;
    if (lx > MX || ly > MY){
        return 0
    }

    *x = lx;
    *y = ly;

    return 1;
}

static uint8_t
_calc_length(lua_State *L){
    uint32_t lid  = luaL_checkint(L, 1);
    uint32_t tlid = luaL_checkint(L, 2);
    uint16_t x,y,tx,ty;
    if( !__get_coordinate(lid, &x, &y) || !__get_coordinate(tlid, &tx, &ty)){
        lua_pushnil(L);
        return 1;
    }

    uint16_t l = abs(tx - x);
    uint16_t w = abs(ty - y);
    uint16_t s = sqrt(l*l + w*w);
    lua_pushinteger(L, s);
    return 1;
}

static uint8_t
_add(lua_State *L){
    map_t *t = __map_instance(L);
    uint16_t x = luaL_checkint(L, 2);
    uint16_t y = luaL_checkint(L, 3);

    uint8_t side = luaL_checkint(L, 4);
    uint8_t watcher = lua_toboolean(L, 5);

    if( !__can_add(x, y, side)){
        lua_pushnil(L);
        return 1;
    }

    point_t p;
    p.state = 2;
    p.side = side;
    p.watcher = watcher;
    __fill(t, x, y, side);
    t->point[x][y] = p;
    
    lua_pushinteger(L, __get_lid(x, y));
    return 1;
}

static uint8_t
_move(lua_State *L){
    map_t *t = __map_instance(L);
    uint32_t lid = luaL_checkint(L, 2);
    uint16_t x = luaL_checkint(L, 3);
    uint16_t y = luaL_checkint(L, 4);

    uint16_t lx,ly;
    if( !__get_coordinate(lid, &lx, &ly) || t->point[lx][ly].state != 2){
        lua_pushnil(L)
        return 1;
    }

    point_t p = t->point[lx][ly];
    if( !__can_add(x, y, p.side)){
        lua_pushnil(L);
        return 1;
    }

    __clear(lx, ly, p.side);
    __fill(t, x, y, p.side);
    t->point[x][y] = p;
    lua_pushinteger(L, __get_lid(x, y));
    return 1;
}

static uint8_t
_remove(lua_State *L){
    map_t *t = __map_instance(L);
    uint32_t lid = luaL_checkint(L, 2);
    uint16_t x,y;
    if(!__get_coordinate(lid, &x, &y) || t->point[x][y].state != 2){
        lua_pushnil(L);
        return 1;
    }

    __clear(x, y, t->point[x][y].side);
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

static uint8_t
_try_fill_in_range(lua_State *L){
    map_t *t = __map_instance(L);
    uint16_t rx = luaL_checkint(L, 2);
    uint16_t ry = luaL_checkint(L, 3);
    uint16_t rside = luaL_checkint(L, 4);
    uint16_t dx = luaL_checkint(L, 5);
    uint16_t dy = luaL_checkint(L, 6);
    uint16_t side = luaL_checkint(L, 7);
    uint16_t i,j, x, y;
    uint16_t x_end = x + rside;
    for(i = 0; i < rside; ++i){
        for(j = 0; j < side; ++j){
            x = ((dx + i) / rside) + rx;
            y = ((dy + j) / rside) + ry;
            if(__can_add(x, y, side)){
                point p;
                p.state = 2;
                p.side = side;         
                __fill(t, x, y, side);
                t->point[x][y] = p;
                lua_pushinteger(L, __get_lid(x, y));
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

static uint8_t
_get_lid_by_range(lua_State *L){
    map_t *t = __map_instance(L);
    uint16_t x = luaL_checkint(L, 2);
    uint16_t y = luaL_checkint(L, 3);
    uint16_t side = luaL_checkint(L, 4);

    if (x > MX || y > MY){
        lua_pushnil(L);
        return 1;
    }

    uint16_t x_end = x + side;
    uint16_t y_end = y + side;
    if(x_end > MX || y_end > MY){
    
    }
}
