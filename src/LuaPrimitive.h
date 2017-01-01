#pragma once

#include <lua.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <map>
#include <vector>
#include <string>

#include "LuaVariant.h"

#define LUA_PRIMITIVE_CHECK_ERROR_ARG_COUNT "Too many arguments! expected %d, got %d"
#define LUA_PRIMITIVE_CHECK_ERROR_ARG_TYPE "Argument %d expected type %s, got %s"
#define LUA_PRIMITIVE_CHECK_ERROR_CANT_GET_POINTER "Failed to get current script pointer!"

extern std::map<lua_State*, void*> __luaPrimitiveObjectMap;
class LuaPrimitive
{
public:
	LuaPrimitive();
	virtual ~LuaPrimitive();

	bool doFile(const std::wstring& file);
	bool doString(const std::string& block);

	bool getGlobal(const std::string &name, LuaVariant &var) const;

	template<class CONTAINER_TYPE>
	static CONTAINER_TYPE* getInstance(lua_State* L)
	{
		auto ret = __luaPrimitiveObjectMap.find(L);
		if (ret == __luaPrimitiveObjectMap.end())
		{
			luaL_error(L, "Failed to get LuaPrimitive pointer!");
			return nullptr;
		}
		return static_cast<CONTAINER_TYPE*>(ret->second);
	}

protected:
	lua_State *L;

	void pushLocal(const LuaVariant &var);
	void pushGlobal(const std::string &name, const LuaVariant &var);
	bool executeFunction(std::string name, std::vector<LuaVariant> arguments, int32_t returns);

	int luaRet();
	int luaRet(const LuaVariant &value);
	int luaRet(const LuaVariant &value, const std::string &message);

	void collectErrors(bool fatal = false);

	std::vector<LuaVariant> getArguments(bool parseNumbersAsDouble = false);

#define LUA_PRIMITIVE_CHECK_ARG(number) \
	do { \
		auto type = args[number-1].getType(); \
		auto coerce = (T ## number == LUA_VARIANT_POINTER && type == LUA_VARIANT_INT); \
		if (type != T ## number && !coerce) \
			throwError(LUA_PRIMITIVE_CHECK_ERROR_ARG_TYPE, number, LuaVariant::typeToString(T ## number), LuaVariant::typeToString(args[number-1].getType())); \
		if (coerce) args[number-1].coerceToPointer(); \
	} while (0);


	template<uint32_t T1>
	std::vector<LuaVariant> getArguments(bool parseNumbersAsDouble = false)
	{
		auto&& args = this->getArguments(parseNumbersAsDouble);
		if (args.size() != 1) this->throwError(LUA_PRIMITIVE_CHECK_ERROR_ARG_COUNT, 1, args.size());
		LUA_PRIMITIVE_CHECK_ARG(1);
		return args;
	}

	template<uint32_t T1, uint32_t T2>
	std::vector<LuaVariant> getArguments(bool parseNumbersAsDouble = false)
	{
		auto&& args = this->getArguments(parseNumbersAsDouble);
		if (args.size() != 2) this->throwError(LUA_PRIMITIVE_CHECK_ERROR_ARG_COUNT, 2, args.size());
		LUA_PRIMITIVE_CHECK_ARG(1);
		LUA_PRIMITIVE_CHECK_ARG(2);
		return args;
	}

	template<uint32_t T1, uint32_t T2, uint32_t T3>
	std::vector<LuaVariant> getArguments(bool parseNumbersAsDouble = false)
	{
		auto&& args = this->getArguments(parseNumbersAsDouble);
		if (args.size() != 3) this->throwError(LUA_PRIMITIVE_CHECK_ERROR_ARG_COUNT, 3, args.size());
		LUA_PRIMITIVE_CHECK_ARG(1);
		LUA_PRIMITIVE_CHECK_ARG(2);
		LUA_PRIMITIVE_CHECK_ARG(3);
		return args;
	}

	template<uint32_t T1, uint32_t T2, uint32_t T3, uint32_t T4>
	std::vector<LuaVariant> getArguments(bool parseNumbersAsDouble = false)
	{
		auto&& args = this->getArguments(parseNumbersAsDouble);
		if (args.size() != 4) this->throwError(LUA_PRIMITIVE_CHECK_ERROR_ARG_COUNT, 4, args.size());
		LUA_PRIMITIVE_CHECK_ARG(1);
		LUA_PRIMITIVE_CHECK_ARG(2);
		LUA_PRIMITIVE_CHECK_ARG(3);
		LUA_PRIMITIVE_CHECK_ARG(4);
		return args;
	}

	template<uint32_t T1, uint32_t T2, uint32_t T3, uint32_t T4, uint32_t T5>
	std::vector<LuaVariant> getArguments(bool parseNumbersAsDouble = false)
	{
		auto&& args = this->getArguments(parseNumbersAsDouble);
		if (args.size() != 5) this->throwError(LUA_PRIMITIVE_CHECK_ERROR_ARG_COUNT, 5, args.size());
		LUA_PRIMITIVE_CHECK_ARG(1);
		LUA_PRIMITIVE_CHECK_ARG(2);
		LUA_PRIMITIVE_CHECK_ARG(3);
		LUA_PRIMITIVE_CHECK_ARG(4);
		LUA_PRIMITIVE_CHECK_ARG(5);
		return args;
	}

	template<uint32_t T1, uint32_t T2, uint32_t T3, uint32_t T4, uint32_t T5, uint32_t T6>
	std::vector<LuaVariant> getArguments(bool parseNumbersAsDouble = false)
	{
		auto&& args = this->getArguments(parseNumbersAsDouble);
		if (args.size() != 6) this->throwError(LUA_PRIMITIVE_CHECK_ERROR_ARG_COUNT, 6, args.size());
		LUA_PRIMITIVE_CHECK_ARG(1);
		LUA_PRIMITIVE_CHECK_ARG(2);
		LUA_PRIMITIVE_CHECK_ARG(3);
		LUA_PRIMITIVE_CHECK_ARG(4);
		LUA_PRIMITIVE_CHECK_ARG(5);
		LUA_PRIMITIVE_CHECK_ARG(6);
		return args;
	}
#undef LUA_PRIMITIVE_CHECK_ARG

protected:
	virtual void displayError(std::string error, bool fatal) = 0;
	void throwError(const char* error, ...);
};
