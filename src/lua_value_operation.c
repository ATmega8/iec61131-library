//
// Created by life on 20-7-7.
//
#include <string.h>

#include "lua_value_operation.h"
#include "glueVars.h"

struct ValueTable {
    const char* direction; //I: input, Q output
    const char* size;      //X bit, B byte, W word, D double word

    uint16_t address;
    uint8_t  bit;
};

static int c_create_new_value(lua_State* L);
static int c_set_value(lua_State* L);
static int c_get_value(lua_State* L);

static const struct luaL_Reg operation_value_f[] = {
        {"new", c_create_new_value},
        {NULL, NULL}
};

static const struct luaL_Reg operation_value_m[] = {
        {"set", c_set_value},
        {"get", c_get_value},
        {NULL, NULL}
};

static int get_value(struct ValueTable* vt)
{
    if((strcmp(vt->direction, "Q") == 0) && (strcmp(vt->size, "X") == 0)) {
        return get_output_bool_value(vt->address, vt->bit);
    } else if((strcmp(vt->direction, "I") == 0) && (strcmp(vt->size, "X") == 0)) {
        return get_input_bool_value(vt->address, vt->bit);
    } else if((strcmp(vt->direction, "M") == 0) && (strcmp(vt->size, "X") == 0)) {
        /*TODO: get memory bit */
        return 0;
    } else {
        /* invalid operation */
        return 0;
    }
}

static void set_value(struct ValueTable* vt, int value)
{
    if((strcmp(vt->direction, "Q") == 0) && (strcmp(vt->size, "X") == 0)) {
        if(value <= 0) {
            set_output_bool_value(vt->address, vt->bit, 0);
        } else {
            set_output_bool_value(vt->address, vt->bit, 1);
        }
        return;
    } else if((strcmp(vt->direction, "M") == 0) && (strcmp(vt->size, "X") == 0)) {
        /*TODO: set memory bit */
        return;
    } else {
        /* invalid operation */
        return;
    }
}

static int c_set_value(lua_State* L)
{
    struct ValueTable* vt = luaL_checkudata(L, 1, "IEC_IO.Value");
    int value = luaL_checkinteger(L, 2);

    printf("set %s_%s%d.%d: %d\n", vt->direction, vt->size, vt->address, vt->bit, value);
    set_value(vt, value);
    return 0;
}

static int c_get_value(lua_State* L)
{
    struct ValueTable* vt = (struct ValueTable*)luaL_checkudata(L, 1, "IEC_IO.Value");

    int res = get_value(vt);
    lua_pushinteger(L, res);

    printf("get %s_%s%d.%d: %d\n", vt->direction, vt->size, vt->address, vt->bit, res);

    return 1;
}

static int c_create_new_value(lua_State* L)
{
    const char* direction = luaL_checkstring(L, 1);
    luaL_argcheck(L, direction != NULL, 1, "invalid direction");

    const char* size      = luaL_checkstring(L, 2);
    luaL_argcheck(L, size != NULL, 2, "invalid size");

    uint16_t address   = luaL_checkinteger(L, 3);
    luaL_argcheck(L, address >= 0 && address < 1024, 3, "address out of range");

    uint8_t bit       = luaL_checkinteger(L, 4);
    luaL_argcheck(L, bit >= 0 && bit < 8, 4, "bit out of range");

    struct ValueTable* vt = lua_newuserdata(L, sizeof(struct ValueTable));
    vt->direction = direction;
    vt->size      = size;
    vt->address   = address;
    vt->size      = size;

    luaL_getmetatable(L, "IEC_IO.Value");
    lua_setmetatable(L, -2);

    return 1;
}

int luaopen_operation_value(lua_State* L)
{
    luaL_newmetatable(L, "IEC_IO.Value");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, operation_value_m, 0);
    luaL_newlib(L, operation_value_f);
    return 1;
}
