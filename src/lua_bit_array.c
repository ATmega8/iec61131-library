//
// Created by life on 20-7-8.
//
#include <limits.h>

#include "lua_bit_array.h"

#define  BITS_PER_WORD (CHAR_BIT * sizeof(unsigned int))
#define  I_WORD(i)     ((unsigned int)(i) / BITS_PER_WORD)
#define  I_BIT(i)      (1 << ((unsigned int)(i) % BITS_PER_WORD))

typedef struct BitArray {
    int size;
    unsigned int values[1];
} BitArray;

static int newarray(lua_State* L)
{
    int i;
    size_t nbytes;
    BitArray *a;

    int n = (int)luaL_checkinteger(L, 1);
    luaL_argcheck(L, n >= 1, 1, "invalid size");
    nbytes = sizeof(BitArray) + I_WORD(n - 1)* sizeof(unsigned int);

    a = (BitArray*)lua_newuserdata(L, nbytes);

    a->size = n;
    for (unsigned long j = 0; j < I_WORD(n - 1); ++j) {
       a->values[j] = 0;
    }

    luaL_getmetatable(L, "BitArray.array");
    lua_setmetatable(L, -2);
    return 1;
}

static int setarray(lua_State* L)
{
    BitArray* a = (BitArray *)luaL_checkudata(L, 1, "BitArray.array");
    int index   = (int)luaL_checkinteger(L, 2) - 1;

    luaL_argcheck(L, a != NULL, 1, "'array' expected");
    luaL_argcheck(L, 0 <= index && index < a->size, 2, "index out of range");

    luaL_checkany(L, 3);

    if(lua_toboolean(L, 3))
        a->values[I_WORD(index)] |= I_BIT(index);
    else
        a->values[I_WORD(index)] &= ~I_BIT(index);

    return 0;
}

static int getarray(lua_State* L)
{
    BitArray* a = (BitArray *)luaL_checkudata(L, 1, "BitArray.array");
    int index   = (int)luaL_checkinteger(L, 2) - 1;

    luaL_argcheck(L, a != NULL, 1, "'array' expected");
    luaL_argcheck(L, 0 <= index && index < a->size, 2, "index out of range");

    lua_pushboolean(L, a->values[I_WORD(index)] & I_BIT(index));
    return 1;
}

static int getsize(lua_State* L)
{
    BitArray* a = (BitArray *)lua_touserdata(L, 1);
    luaL_argcheck(L, a != NULL, 1, "'array' expected");
    lua_pushinteger(L, a->size);
    return 1;
}

static const struct luaL_Reg arraylib [] = {
    {"new", newarray},
    {"set", setarray},
    {"get", getarray},
    {"size", getsize},
    {NULL, NULL}
};

int luaopen_bit_array(lua_State* L) {
    luaL_newmetatable(L, "BitArray.array");
    luaL_newlib(L, arraylib);
    return 1;
}
