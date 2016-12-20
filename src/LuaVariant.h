#pragma once

#include <lua.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <map>
#include <vector>
#include <string>
#include <functional>

enum LUA_VARIANT_TYPE : uint32_t
{
	LUA_VARIANT_DOUBLE,
	LUA_VARIANT_INT,
	LUA_VARIANT_POINTER,
	LUA_VARIANT_STRING,
	LUA_VARIANT_BOOL,
	LUA_VARIANT_FUNCTION,
	LUA_VARIANT_ITABLE,
	LUA_VARIANT_KTABLE,
	LUA_VARIANT_NIL
};

#define CONST_TO_STRING_SWITCH(c) case c: { return #c; }

class LuaVariant
{
public:
	typedef double LuaVariantDouble;
	typedef LUA_INTEGER LuaVariantInt;
	typedef void* LuaVariantPointer;
	typedef std::string LuaVariantString;
	typedef bool LuaVariantBool;
	typedef lua_CFunction LuaVariantFunction;
	typedef std::vector<LuaVariant> LuaVariantITable;
	typedef std::map<std::string, LuaVariant> LuaVariantKTable;

	static const char* typeToString(uint32_t type)
	{
		switch (type)
		{
			CONST_TO_STRING_SWITCH(LUA_VARIANT_DOUBLE);
			CONST_TO_STRING_SWITCH(LUA_VARIANT_INT);
			CONST_TO_STRING_SWITCH(LUA_VARIANT_POINTER);
			CONST_TO_STRING_SWITCH(LUA_VARIANT_STRING);
			CONST_TO_STRING_SWITCH(LUA_VARIANT_BOOL);
			CONST_TO_STRING_SWITCH(LUA_VARIANT_FUNCTION);
			CONST_TO_STRING_SWITCH(LUA_VARIANT_ITABLE);
			CONST_TO_STRING_SWITCH(LUA_VARIANT_KTABLE);
			CONST_TO_STRING_SWITCH(LUA_VARIANT_NIL);
			default:
				return "unknown type";
		}
	}

	static LuaVariant parse(lua_State *L, int32_t index, bool parseNumbersAsDouble = false, bool pop = true)
	{
		LuaVariant v;
		auto type = lua_type(L, index);
		if (type == LUA_TUSERDATA || type == LUA_TLIGHTUSERDATA)
		{
			LuaVariantPointer value = (LuaVariantPointer)lua_touserdata(L, index);
			v = LuaVariant(value);
		}
		else if (type == LUA_TNUMBER)
		{
			if (parseNumbersAsDouble)
			{
				LuaVariantDouble value = lua_tonumber(L, index);
				v = LuaVariant(value);
			}
			else
			{
				LuaVariantInt value = lua_tointeger(L, index);
				v = LuaVariant(value);
			}
		}
		else if (type == LUA_TSTRING)
		{
			LuaVariantString value = lua_tostring(L, index);
			v = LuaVariant(value);
		}
		else if (type == LUA_TBOOLEAN)
		{
			LuaVariantBool value = (lua_toboolean(L, index) == 1);
			v = LuaVariant(value);
		}
		else if (type == LUA_TTABLE)
		{
			static const int32_t KEY_INDEX = -2;
			static const int32_t VALUE_INDEX = -1;
			typedef std::function<void(lua_State*)> WalkCallback;


			// The table parsing loop is conducted by lua_next(), which:
			//    - reads a key from the stack
			//    - read the value of the next key from the table (table index specified by second arg)
			//    - pushes the key (KEY_INDEX, -2) and value (VALUE_INDEX, -1) to the stack
			//    - returns 0 and pops all extraneous stuff on completion
			// Starting with a key of nil will allow us to process the entire table.
			// Leaving the last key on the stack each iteration will go to the next item
			auto walk = [](lua_State *L, int32_t index, WalkCallback callback) -> void
			{
				lua_pushnil(L);
				while (lua_next(L, index))
				{
					callback(L);
					lua_pop(L, 1);
				}
			};

			// we're going to walk the table one time to see if we can use an LuaVariantITable,
			// or if it we need a LuaVariantKTable.
			bool requiresStringKeys = false;
			walk(L, index, [&requiresStringKeys](lua_State *L) -> void
			{
				if (lua_type(L, KEY_INDEX) == LUA_TSTRING)
					requiresStringKeys = true;
			});

			// the below are variations, since the table might have only numeric indices (iTable)
			// or it might also have string indices (kTable)
			LuaVariantKTable kTableValue;
			LuaVariantITable iTableValue;
			auto parseKTableItem = [parseNumbersAsDouble, &kTableValue](lua_State *L) -> void
			{
				// TODO: probably type-check the key here and have some error handling mechanism
				std::string key = lua_tostring(L, KEY_INDEX);
				int32_t valueIndex = (lua_type(L, VALUE_INDEX) == LUA_TTABLE) ? lua_gettop(L) : VALUE_INDEX;
				kTableValue[key] = LuaVariant::parse(L, valueIndex, parseNumbersAsDouble, false);
			};
			auto parseITableItem = [parseNumbersAsDouble, &iTableValue](lua_State *L) -> void
			{
				// TODO: probably type-check the key here and have some error handling mechanism
				auto key = lua_tonumber(L, KEY_INDEX);
				int32_t valueIndex = (lua_type(L, VALUE_INDEX) == LUA_TTABLE) ? lua_gettop(L) : VALUE_INDEX;

				// fill with nil values up to key
				while (key >= iTableValue.size())
					iTableValue.push_back(LuaVariant());

				iTableValue[static_cast<size_t>(key)] = LuaVariant::parse(L, valueIndex, parseNumbersAsDouble, false);
			};

			if (requiresStringKeys)
			{
				walk(L, index, parseKTableItem);
				v = LuaVariant(kTableValue);
			}
			else
			{
				walk(L, index, parseITableItem);
				v = LuaVariant(iTableValue);
			}
		}

		if (pop) // TODO: verify this works as expected .. don't think so
			lua_pop(L, index);
		return v;
	}

	LuaVariant() : type(LUA_VARIANT_NIL) {}
	LuaVariant(const LuaVariantDouble &valueDouble)     : valueDouble(valueDouble),                                 type(LUA_VARIANT_DOUBLE) { };
	LuaVariant(const float &valueDouble)                : valueDouble(valueDouble),                                 type(LUA_VARIANT_DOUBLE) { };
	LuaVariant(const uint8_t &valueInt)                 : valueInt(static_cast<LuaVariantInt>(valueInt)),           type(LUA_VARIANT_INT) { };
	LuaVariant(const int8_t &valueInt)                  : valueInt(static_cast<LuaVariantInt>(valueInt)),           type(LUA_VARIANT_INT) { };
	LuaVariant(const uint16_t &valueInt)                : valueInt(static_cast<LuaVariantInt>(valueInt)),           type(LUA_VARIANT_INT) { };
	LuaVariant(const int16_t &valueInt)                 : valueInt(static_cast<LuaVariantInt>(valueInt)),           type(LUA_VARIANT_INT) { };
	LuaVariant(const uint32_t &valueInt)                : valueInt(static_cast<LuaVariantInt>(valueInt)),           type(LUA_VARIANT_INT) { };
	LuaVariant(const int32_t &valueInt)                 : valueInt(static_cast<LuaVariantInt>(valueInt)),           type(LUA_VARIANT_INT) { };
	LuaVariant(const uint64_t &valueInt)                : valueInt(static_cast<LuaVariantInt>(valueInt)),           type(LUA_VARIANT_INT) { };
	LuaVariant(const int64_t &valueInt)                 : valueInt(static_cast<LuaVariantInt>(valueInt)),           type(LUA_VARIANT_INT) { };
	LuaVariant(const LuaVariantPointer &valuePointer)   : valuePointer(valuePointer),                               type(LUA_VARIANT_POINTER) { };
	LuaVariant(const LuaVariantString &valueString)     : valueString(valueString),                                 type(LUA_VARIANT_STRING) { };
	LuaVariant(const LuaVariantBool &valueBool)         : valueBool(valueBool),                                     type(LUA_VARIANT_BOOL) { };
	LuaVariant(const LuaVariantFunction &valueFunction) : valueFunction(valueFunction),                             type(LUA_VARIANT_FUNCTION) { };
	LuaVariant(const LuaVariantITable &valueITable)     : valueITable(valueITable),                                 type(LUA_VARIANT_ITABLE) { };
	LuaVariant(const LuaVariantKTable &valueKTable)     : valueKTable(valueKTable),                                 type(LUA_VARIANT_KTABLE) { };
	LuaVariant(const std::wstring &valueWString)        : valueString(valueWString.begin(), valueWString.end()),    type(LUA_VARIANT_STRING) { };
	~LuaVariant() {}

	void coerceToPointer()
	{
		if (this->type == LUA_VARIANT_INT)
		{
			this->type = LUA_VARIANT_POINTER;
			this->valuePointer = (LuaVariantPointer)this->valueInt;
		}
	}

	void push(lua_State *L) const
	{
		switch (this->type)
		{
			case LUA_VARIANT_DOUBLE:
			{
				lua_pushnumber(L, this->valueDouble);
				break;
			}
			case LUA_VARIANT_INT:
			{
				lua_pushinteger(L, this->valueInt);
				break;
			}
			case LUA_VARIANT_POINTER:
			{
				lua_pushlightuserdata(L, this->valuePointer);
				break;
			}
			case LUA_VARIANT_STRING:
			{
				lua_pushstring(L, this->valueString.c_str());
				break;
			}
			case LUA_VARIANT_BOOL:
			{
				lua_pushboolean(L, this->valueBool);
				break;
			}
			case LUA_VARIANT_FUNCTION:
			{
				lua_pushcfunction(L, this->valueFunction);
				break;
			}
			case LUA_VARIANT_ITABLE:
			{
				lua_newtable(L);
				auto table = lua_gettop(L);

				for (size_t i = 0; i < this->valueITable.size(); i++)
				{
					lua_pushinteger(L, i); // key
					this->valueITable[i].push(L); // value
					lua_settable(L, table); // add to table
				}
				break;
			}
			case LUA_VARIANT_KTABLE:
			{
				lua_newtable(L);
				auto table = lua_gettop(L);

				for (auto obj = this->valueKTable.begin(); obj != this->valueKTable.end(); obj++)
				{
					lua_pushstring(L, obj->first.c_str()); // key
					obj->second.push(L); // value
					lua_settable(L, table); // add to table
				}
				break;
			}
		}
	}
	bool isNil() const { return this->type == LUA_VARIANT_NIL; }

	bool getAsDouble(LuaVariantDouble &value) const
	{
		if (this->type != LUA_VARIANT_DOUBLE) return false;
		value = this->valueDouble;
		return true;
	}
	bool getAsInt(uint8_t &value) const
	{
		if (this->type != LUA_VARIANT_INT) return false;
		value = this->valueInt;
		return true;
	}
	bool getAsInt(int8_t &value) const
	{
		if (this->type != LUA_VARIANT_INT) return false;
		value = this->valueInt;
		return true;
	}
	bool getAsInt(uint16_t &value) const
	{
		if (this->type != LUA_VARIANT_INT) return false;
		value = this->valueInt;
		return true;
	}
	bool getAsInt(int16_t &value) const
	{
		if (this->type != LUA_VARIANT_INT) return false;
		value = this->valueInt;
		return true;
	}
	bool getAsInt(uint32_t &value) const
	{
		if (this->type != LUA_VARIANT_INT) return false;
		value = this->valueInt;
		return true;
	}
	bool getAsInt(int32_t &value) const
	{
		if (this->type != LUA_VARIANT_INT) return false;
		value = this->valueInt;
		return true;
	}
	bool getAsInt(uint64_t &value) const
	{
		if (this->type != LUA_VARIANT_INT) return false;
		value = this->valueInt;
		return true;
	}
	bool getAsInt(int64_t &value) const
	{
		if (this->type != LUA_VARIANT_INT) return false;
		value = this->valueInt;
		return true;
	}
	bool getAsPointer(LuaVariantPointer &value) const
	{
		if (this->type != LUA_VARIANT_POINTER) return false;
		value = this->valuePointer;
		return true;
	}
	bool getAsString(LuaVariantString &value) const
	{
		if (this->type != LUA_VARIANT_STRING) return false;
		value = this->valueString;
		return true;
	}
	bool getAsBool(LuaVariantBool &value) const
	{
		if (this->type != LUA_VARIANT_BOOL) return false;
		value = this->valueBool;
		return true;
	}
	bool getAsFunction(LuaVariantFunction &value) const
	{
		if (this->type != LUA_VARIANT_FUNCTION) return false;
		value = this->valueFunction;
		return true;
	}
	bool getAsITable(LuaVariantITable &value) const
	{
		if (this->type != LUA_VARIANT_ITABLE) return false;
		value = this->valueITable;
		return true;
	}
	bool getAsKTable(LuaVariantKTable &value) const
	{
		if (this->type != LUA_VARIANT_KTABLE) return false;
		value = this->valueKTable;
		return true;
	}

	uint32_t getType() const { return this->type; }
private:
	uint32_t type;
	LuaVariantDouble valueDouble;
	LuaVariantInt valueInt;
	LuaVariantPointer valuePointer;
	LuaVariantString valueString;
	LuaVariantBool valueBool;
	LuaVariantFunction valueFunction;
	LuaVariantITable valueITable;
	LuaVariantKTable valueKTable;

};

#undef CONST_TO_STRING_SWITCH