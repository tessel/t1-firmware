/*
 * runtime.h
 *
 *  Created on: Jun 17, 2013
 *      Author: tim
 */

#ifndef RUNTIME_H_
#define RUNTIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

LUALIB_API int luaopen_hw (lua_State *L);

#ifdef __cplusplus
}
#endif

#endif /* RUNTIME_H_ */
