// Tencent is pleased to support the open source community by making UnLua available.
// 
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the MIT License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#include "UnLuaEx.h"
#include "LuaCore.h"
#include "Containers/LuaMap.h"

static int32 TMap_New(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 3)
        return luaL_error(L, "invalid parameters");

    TSharedPtr<UnLua::ITypeInterface> KeyInterface(CreateTypeInterface(L, 2));
    if (!KeyInterface)
        return luaL_error(L, "invalid key type");

    TSharedPtr<UnLua::ITypeInterface> ValueInterface(CreateTypeInterface(L, 3));
    if (!ValueInterface)
        return luaL_error(L, "invalid value type");

    auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetContainerRegistry();
    Registry->NewMap(L, KeyInterface, ValueInterface, FLuaMap::OwnedBySelf);

    return 1;
}

static int TMap_Enumerable(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
        return luaL_error(L, "invalid parameters");

    FLuaMap::FLuaMapEnumerator** Enumerator = (FLuaMap::FLuaMapEnumerator**)(lua_touserdata(L, 1));
    if (!Enumerator || !*Enumerator)
        return luaL_error(L, "invalid enumerator");

    const auto Map = (*Enumerator)->LuaMap;
    if (!Map)
        return luaL_error(L, "invalid TMap");

    while ((*Enumerator)->Index < Map->GetMaxIndex())
    {
        if (!Map->IsValidIndex((*Enumerator)->Index))
        {
            ++(*Enumerator)->Index;
        }
        else
        {
            Map->KeyInterface->Read(L, Map->GetData((*Enumerator)->Index), false);
            Map->ValueInterface->Read(L, Map->GetData((*Enumerator)->Index) + Map->MapLayout.ValueOffset - Map->ValueInterface->GetOffset(), false);
            ++(*Enumerator)->Index;
            return 2;
        }
    }

    return 0;
}

static int32 TMap_Pairs(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
        return luaL_error(L, "invalid parameters");

    FLuaMap* Map = (FLuaMap*)(GetCppInstanceFast(L, 1));
    if (!Map)
        return 0;

    lua_pushcfunction(L, TMap_Enumerable);
    FLuaMap::FLuaMapEnumerator** Enumerator = (FLuaMap::FLuaMapEnumerator**)lua_newuserdata(L, sizeof(FLuaMap::FLuaMapEnumerator*));
    *Enumerator = new FLuaMap::FLuaMapEnumerator(Map, 0);

    lua_newtable(L);
    lua_pushcfunction(L, FLuaMap::FLuaMapEnumerator::gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    lua_pushnil(L);

    return 3;
}

/**
 * @see FLuaMap::Num(...)
 */
static int32 TMap_Length(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
        return luaL_error(L, "invalid parameters");

    FLuaMap* Map = (FLuaMap*)(GetCppInstanceFast(L, 1));
    if (!Map)
        return luaL_error(L, "invalid TMap");

    lua_pushinteger(L, Map->Num());
    return 1;
}

/**
 * @see FLuaMap::Add(...)
 */
static int32 TMap_Add(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 3)
        return luaL_error(L, "invalid parameters");

    FLuaMap* Map = (FLuaMap*)(GetCppInstanceFast(L, 1));
    if (!Map)
        return luaL_error(L, "invalid TMap");

    void* ValueCache = (uint8*)Map->ElementCache + Map->MapLayout.ValueOffset;
    Map->KeyInterface->Initialize(Map->ElementCache);
    Map->ValueInterface->Initialize(ValueCache);
    Map->KeyInterface->Write(L, Map->ElementCache, 2);
    Map->ValueInterface->Write(L, Map->ValueInterface->GetOffset() > 0 ? Map->ElementCache : ValueCache, 3);
    Map->Add(Map->ElementCache, ValueCache);
    Map->KeyInterface->Destruct(Map->ElementCache);
    Map->ValueInterface->Destruct(ValueCache);
    return 0;
}

/**
 * @see FLuaMap::Remove(...)
 */
static int32 TMap_Remove(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
        return luaL_error(L, "invalid parameters");

    FLuaMap* Map = (FLuaMap*)(GetCppInstanceFast(L, 1));
    if (!Map)
        return luaL_error(L, "invalid TMap");

    Map->KeyInterface->Initialize(Map->ElementCache);
    Map->KeyInterface->Write(L, Map->ElementCache, 2);
    bool bSuccess = Map->Remove(Map->ElementCache);
    Map->KeyInterface->Destruct(Map->ElementCache);
    lua_pushboolean(L, bSuccess);
    return 1;
}

/**
 * @see FLuaMap::Find(...). Create a copy for the value
 */
static int32 TMap_Find(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
        return luaL_error(L, "invalid parameters");

    FLuaMap* Map = (FLuaMap*)(GetCppInstanceFast(L, 1));
    if (!Map)
        return luaL_error(L, "invalid TMap");

    void* ValueCache = (uint8*)Map->ElementCache + Map->MapLayout.ValueOffset;
    Map->KeyInterface->Initialize(Map->ElementCache);
    Map->ValueInterface->Initialize(ValueCache);
    Map->KeyInterface->Write(L, Map->ElementCache, 2);
    bool bSuccess = Map->Find(Map->ElementCache, ValueCache);
    if (bSuccess)
    {
        Map->ValueInterface->Read(L, Map->ValueInterface->GetOffset() > 0 ? Map->ElementCache : ValueCache, true);
    }
    else
    {
        lua_pushnil(L);
    }
    Map->KeyInterface->Destruct(Map->ElementCache);
    Map->ValueInterface->Destruct(ValueCache);
    return 1;
}

/**
 * @see FLuaMap::Find(...). Return a reference for the value
 */
static int32 TMap_FindRef(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 2)
        return luaL_error(L, "invalid parameters");

    FLuaMap* Map = (FLuaMap*)(GetCppInstanceFast(L, 1));
    if (!Map)
        return luaL_error(L, "invalid TMap");

    Map->KeyInterface->Initialize(Map->ElementCache);
    Map->KeyInterface->Write(L, Map->ElementCache, 2);
    void* Value = Map->Find(Map->ElementCache);
    if (Value)
    {
        const void* Key = (uint8*)Value - Map->ValueInterface->GetOffset();
        Map->ValueInterface->Read(L, Key, false);
    }
    else
    {
        lua_pushnil(L);
    }
    Map->KeyInterface->Destruct(Map->ElementCache);
    return 1;
}

/**
 * @see FLuaMap::Clear(...)
 */
static int32 TMap_Clear(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
        return luaL_error(L, "invalid parameters");

    FLuaMap* Map = (FLuaMap*)(GetCppInstanceFast(L, 1));
    if (!Map)
        return luaL_error(L, "invalid TMap");

    Map->Clear();
    return 0;
}

/**
 * @see FLuaMap::Keys(...)
 */
static int32 TMap_Keys(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
        return luaL_error(L, "invalid parameters");

    FLuaMap* Map = (FLuaMap*)(GetCppInstanceFast(L, 1));
    if (!Map)
        return luaL_error(L, "invalid TMap");

    void* Userdata = NewUserdataWithPadding(L, sizeof(FLuaArray), "TArray");
    FLuaArray* Array = Map->Keys(Userdata);
    return 1;
}

/**
 * @see FLuaMap::Values(...)
 */
static int32 TMap_Values(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
        return luaL_error(L, "invalid parameters");

    FLuaMap* Map = (FLuaMap*)(GetCppInstanceFast(L, 1));
    if (!Map)
        return luaL_error(L, "invalid TMap");

    void* Userdata = NewUserdataWithPadding(L, sizeof(FLuaArray), "TArray");
    FLuaArray* Array = Map->Values(Userdata);
    return 1;
}

/**
 * GC function
 */
static int32 TMap_Delete(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
        return luaL_error(L, "invalid parameters");

    FLuaMap* Map = (FLuaMap*)(GetCppInstanceFast(L, 1));
    if (!Map)
        return luaL_error(L, "invalid TMap");

    auto Registry = UnLua::FLuaEnv::FindEnvChecked(L).GetContainerRegistry();
    Registry->Remove(Map);

    Map->~FLuaMap();
    return 0;
}

/**
 * Convert the map to a Lua table
 */
static int32 TMap_ToTable(lua_State* L)
{
    int32 NumParams = lua_gettop(L);
    if (NumParams != 1)
        return luaL_error(L, "invalid parameters");

    FLuaMap* Map = (FLuaMap*)(GetCppInstanceFast(L, 1));
    if (!Map)
        return luaL_error(L, "invalid TMap");

    if (!Map->KeyInterface->IsValid())
        return luaL_error(L, TCHAR_TO_UTF8(*FString::Printf(TEXT("invalid TMap key type:%s"), *Map->ValueInterface->GetName())));

    if (!Map->ValueInterface->IsValid())
        return luaL_error(L, TCHAR_TO_UTF8(*FString::Printf(TEXT("invalid TMap value type:%s"), *Map->ValueInterface->GetName())));

    void* MemData = FMemory::Malloc(sizeof(FLuaArray), alignof(FLuaArray));
    FLuaArray* Keys = Map->Keys(MemData);
    Keys->Inner->Initialize(Keys->ElementCache);
    lua_newtable(L);
    for (int32 i = 0; i < Keys->Num(); ++i)
    {
        Keys->Get(i, Keys->ElementCache);
        Keys->Inner->Read(L, Keys->ElementCache, true);

        void* ValueCache = (uint8*)Map->ElementCache + Map->MapLayout.ValueOffset;
        Map->ValueInterface->Initialize(ValueCache);
        bool bSuccess = Map->Find(Keys->ElementCache, ValueCache);
        if (bSuccess)
        {
            Map->ValueInterface->Read(L, Map->ValueInterface->GetOffset() > 0 ? Map->ElementCache : ValueCache, true);
        }
        else
        {
            lua_pushnil(L);
        }

        lua_rawset(L, -3);

        Map->ValueInterface->Destruct(ValueCache);
    }
    Keys->Inner->Destruct(Keys->ElementCache);
    FMemory::Free(MemData);
    return 1;
}

static const luaL_Reg TMapLib[] =
{
    {"Length", TMap_Length},
    {"Num", TMap_Length},
    {"Add", TMap_Add},
    {"Remove", TMap_Remove},
    {"Find", TMap_Find},
    {"FindRef", TMap_FindRef},
    {"Clear", TMap_Clear},
    {"Keys", TMap_Keys},
    {"Values", TMap_Values},
    {"ToTable", TMap_ToTable},
    {"__gc", TMap_Delete},
    {"__call", TMap_New},
    {"__pairs", TMap_Pairs},
    {nullptr, nullptr}
};

EXPORT_UNTYPED_CLASS(TMap, false, TMapLib)

IMPLEMENT_EXPORTED_CLASS(TMap)
