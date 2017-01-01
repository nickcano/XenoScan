#include "LuaPrimitive.h"

#include <Windows.h>

std::map<lua_State*, void*> __luaPrimitiveObjectMap;

LuaPrimitive::LuaPrimitive()
{
	this->L = luaL_newstate();
	luaL_openlibs(this->L);
	__luaPrimitiveObjectMap.insert(std::make_pair(this->L, static_cast<void*>(this)));
}
LuaPrimitive::~LuaPrimitive()
{
	auto ret = __luaPrimitiveObjectMap.find(this->L);
	if (ret != __luaPrimitiveObjectMap.end())
		__luaPrimitiveObjectMap.erase(ret);
	lua_close(this->L);
}

bool LuaPrimitive::doFile(const std::wstring& file)
{
	do
	{
		wchar_t transformString[MAX_PATH];
		char shortName[MAX_PATH * 2];

		if (GetShortPathNameW(file.c_str(), NULL, 0) == 0) break;
		if (GetShortPathNameW(file.c_str(), transformString, MAX_PATH) == 0) break;
		for (int i = 0; i < MAX_PATH; i++)
			shortName[i] = ((char*)&transformString[0])[i*2];
			
		if (!luaL_loadfile(L, shortName))
		{
			if (lua_pcall(L, 0, 0,0 ))
				break;
		}
		else
			break;

		return true;
	} while (0);

	this->collectErrors(true);
	return false;
}

bool LuaPrimitive::doString(const std::string& block)
{
	if (luaL_loadstring(L, block.c_str()))
	{
		this->collectErrors(false);
		return false;
	}
	else
	{
		if (lua_pcall(L,0,0,0))
		{
			this->collectErrors(false);
			return false;
		}
	}
	return true;
}

bool LuaPrimitive::getGlobal(const std::string &name, LuaVariant &var) const
{
	lua_getglobal(L, name.c_str());
	var = LuaVariant::parse(this->L, -1, false, false);
	return !var.isNil();
}

void LuaPrimitive::pushLocal(const LuaVariant &var)
{
	if (!var.isNil())
		var.push(this->L);
}

void LuaPrimitive::pushGlobal(const std::string &name, const LuaVariant &var)
{
	if (!var.isNil())
	{
		var.push(this->L);
		lua_setglobal(this->L, name.c_str());
	}
}

bool LuaPrimitive::executeFunction(std::string name, std::vector<LuaVariant> arguments, int32_t returns)
{
	lua_getglobal(this->L, name.c_str());
	for (auto iarg = arguments.begin(); iarg != arguments.end(); iarg++)
		iarg->push(this->L);

	if (lua_pcall(this->L, arguments.size(), returns, 0))
	{
		this->collectErrors(false);
		return false;
	}
	return true;
}

int LuaPrimitive::luaRet()
{
	return 0;
}
int LuaPrimitive::luaRet(const LuaVariant &value)
{
	value.push(this->L);
	return 1;
}
int LuaPrimitive::luaRet(const LuaVariant &value, const std::string &message)
{
	value.push(this->L);
	LuaVariant(message).push(this->L);
	return 2;
}

void LuaPrimitive::collectErrors(bool fatal)
{
	auto err = LuaVariant::parse(this->L, -1, false, false);
	lua_pop(this->L, 1);

	std::string errText;
	if (err.getAsString(errText))
		this->displayError(errText, fatal);
	else
		this->displayError("Unknown Lua error!", fatal);
}

void LuaPrimitive::throwError(const char* error, ...)
{
	char buffer[4096] = {'\0'};
	va_list args;
	va_start(args, error);
	vsprintf_s(buffer, error, args);
	va_end(args);
	luaL_error(this->L, buffer);
}

std::vector<LuaVariant> LuaPrimitive::getArguments(bool parseNumbersAsDouble)
{
	std::vector<LuaVariant> ret;
	auto top = lua_gettop(this->L);
	for (int i = 1; i <= top; i++)
		ret.push_back(LuaVariant::parse(this->L, i, parseNumbersAsDouble, false));
	return ret;
}