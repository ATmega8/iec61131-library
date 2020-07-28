//
// Created by life on 20-7-8.
//

#include <string.h>

#include "lua_timer.h"
#include "iec_types_all.h"

extern TIME __CURRENT_TIME;

typedef struct iec_timer_t {
    uint8_t out;
    uint8_t prev_in;

    int state; /* 0-reset, 1-counting, 2-set */
    uint64_t current_time;
    uint64_t start_time;
} iec_timer_t;

static uint64_t timespec_to_tick(TIME* ts)
{
    return (ts->tv_sec + ((ts->tv_nsec)/1000000));
}

static int new_timer_ton(lua_State* L)
{
    iec_timer_t* timer = lua_newuserdata(L, sizeof(iec_timer_t));
    memset(timer, 0, sizeof(iec_timer_t));

    luaL_getmetatable(L, "IEC_Timer.TON");
    lua_setmetatable(L, -2);
    return 1;
}

static int new_timer_tof(lua_State* L)
{
    iec_timer_t* timer = lua_newuserdata(L, sizeof(iec_timer_t));
    memset(timer, 0, sizeof(iec_timer_t));

    luaL_getmetatable(L, "IEC_Timer.TOF");
    lua_setmetatable(L, -2);
    return 1;
}

static int new_timer_tp(lua_State* L)
{
   iec_timer_t* timer = lua_newuserdata(L, sizeof(iec_timer_t));
   memset(timer, 0, sizeof(iec_timer_t));

   luaL_getmetatable(L, "IEC_Timer.TP");
   lua_setmetatable(L, -2);
    return 1;
}

static int run_timer_ton(lua_State* L)
{
    /* get parameters */
    iec_timer_t* timer = (iec_timer_t*)luaL_checkudata(L, 1, "IEC_Timer.TON");

    int pt = luaL_checkinteger(L, 2);
    luaL_argcheck(L, pt > 0, 2, "invalid time");

    luaL_checkany(L, 3);
    int in = lua_toboolean(L, 3);

    /* run program */
    timer->current_time = timespec_to_tick(&__CURRENT_TIME);

    if((timer->state == 0) && (timer->prev_in == 0) && in) {
        timer->state = 1;
        timer->out = 0;
        timer->start_time = timer->current_time;
    } else {
        if(in == 0) {
            timer->out = 0;
            timer->state = 0;
        } else if(timer->state == 1) {
            if((pt + timer->start_time) <= timer->current_time) {
                timer->state = 2;
                timer->out = 1;
            }
        }
    }

    timer->prev_in = in;
    lua_pushboolean(L, timer->out);
    return 1;
}

static int run_timer_tof(lua_State* L)
{
    /* get parameters */
    iec_timer_t* timer = (iec_timer_t*)luaL_checkudata(L, 1, "IEC_Timer.TOF");

    int pt = luaL_checkinteger(L, 2);
    luaL_argcheck(L, pt > 0, 2, "invalid time");

    luaL_checkany(L, 3);
    int in = lua_toboolean(L, 3);

    /* run program */
    timer->current_time = timespec_to_tick(&__CURRENT_TIME);

    /* found falling edge on in */
    if((timer->state == 0) && (timer->prev_in == 1) && (in == 0)) {
        timer->state = 1;
        timer->start_time = timer->current_time;
    } else {
        if(in == 1) {
            timer->state = 0;
        } else if(timer->state == 1) {
            if((pt + timer->start_time) <= timer->current_time) {
                timer->state = 2;
            }
        }
    }

    timer->prev_in = in;
    timer->out = (in || (timer->state == 1));
    lua_pushboolean(L, timer->out);
    return 1;
}

static int run_timer_tp(lua_State* L)
{
    /* get parameters */
    iec_timer_t* timer = (iec_timer_t*)luaL_checkudata(L, 1, "IEC_Timer.TP");

    int pt = luaL_checkinteger(L, 2);
    luaL_argcheck(L, pt > 0, 2, "invalid time");

    luaL_checkany(L, 3);
    int in = lua_toboolean(L, 3);

    /* run program */
    timer->current_time = timespec_to_tick(&__CURRENT_TIME);

    /* found rising edge on in */
    if((timer->state == 0) && (timer->prev_in == 0) && (in == 1)) {
        timer->state = 1;
        timer->out = 1;
        timer->start_time = timer->current_time;
    } else if(timer->state == 1){
        if((pt + timer->start_time) <= timer->current_time) {
            timer->state = 2;
            timer->out = 0;
        }
    }

    if((timer->state == 2) && (in == 0)) {
        timer->state = 0;
    }

    timer->prev_in = in;
    lua_pushboolean(L, timer->out);
    return 1;
}

static const struct luaL_Reg timer_f[] = {
        {"new_ton", new_timer_ton},
        {"new_tof", new_timer_tof},
        {"new_tp",  new_timer_tp},
        {NULL, NULL}
};

static const struct luaL_Reg timer_ton_m[] = {
        {"run", run_timer_ton},
        {NULL, NULL}
};

static const struct luaL_Reg timer_tof_m[] = {
        {"run", run_timer_tof},
        {NULL, NULL}
};

static const struct luaL_Reg timer_tp_m[] = {
        {"run", run_timer_tp},
        {NULL, NULL}
};

int luaopen_timer(lua_State* L) {
    luaL_newmetatable(L, "IEC_Timer.TON");
    lua_pushvalue(L, -1); /*duplicate the meta table */
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, timer_ton_m, 0);

    luaL_newmetatable(L, "IEC_Timer.TOF");
    lua_pushvalue(L, -1); /*duplicate the meta table */
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, timer_tof_m, 0);

    luaL_newmetatable(L, "IEC_Timer.TP");
    lua_pushvalue(L, -1); /*duplicate the meta table */
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, timer_tp_m, 0);

    luaL_newlib(L, timer_f);
    return 1;
}
