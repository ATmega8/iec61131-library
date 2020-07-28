//
// Created by life on 20-7-7.
//

#include <time.h>
#include <stdio.h>
#include <stdint.h>

#include "glueVars.h"
#include "lua_value_operation.h"
#include "lua_bit_array.h"
#include "lua_timer.h"

extern uint64_t common_ticktime__;

static const luaL_Reg loadedlibs[] = {
        {"iec_io", luaopen_operation_value},
        {"iec_timer", luaopen_timer},
        {NULL, NULL}
};

void sleep_until(struct timespec *ts, uint64_t delay)
{
    ts->tv_nsec += delay;
    if(ts->tv_nsec >= 1000*1000*1000)
    {
        ts->tv_nsec -= 1000*1000*1000;
        ts->tv_sec++;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ts,  NULL);
}

void load_lua_library(lua_State* L)
{
    luaL_openlibs(L);

    const luaL_Reg *lib;
    for (lib = loadedlibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }
}

int run_lua_file(const char* file_name, lua_State* L)
{
    int error = luaL_loadfile(L, file_name) || lua_pcall(L, 0, 0, 0);

    if (error) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);  /* pop error message from the stack */
    }

    return error;
}

int main(void)
{
    /* initial */
    //gets the starting point for the clock
    printf("Getting current time\n");
    struct timespec timer_start;
    clock_gettime(CLOCK_MONOTONIC, &timer_start);

    //make sure the buffer pointers are correct and
    //attached to the user variables
    glueVars();

    /* run program */
    char* lua_argv[] = { (char *)"lua", NULL };

    lua_State *L = luaL_newstate();  /* create state */

    if (L == NULL) {
        printf("cannot create state: not enough memory");
    }

    load_lua_library(L);

    run_lua_file("../lua/init.lua", L);

    while(1) {
        //1.read input image
        //2.lock mutex
        //3.update input image table with data from slave devices

        //4.execute plc program logic
        run_lua_file("../lua/run.lua", L);
        //5.unlock mutex

        //6.write output image

        //7.update time
        update_time();
        sleep_until(&timer_start, common_ticktime__);
    }

    lua_close(L);
    return 0;
}

