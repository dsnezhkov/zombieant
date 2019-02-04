#include <stdio.h>
#include <stdlib.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

int main(int argc, char *argv[]) {

  printf("Argc: %d\n", argc);
  if (argc != 2){
    printf("Usage: %s <script.lua>\n", argv[0] );
    exit(1);
  }

  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  luaL_dofile(L, argv[1]);

  return 0;
}
