//changeset:   288:e440976b9f9f
/*
    SLB - Simple Lua Binder
    Copyright (C) 2007-2011 Jose L. Hidalgo Valiño (PpluX)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
	
	Jose L. Hidalgo (www.pplux.com)
	pplux@pplux.com
*/
#include "SLB.hpp"
#include <cstdlib>
namespace
{
  static SLB::MallocFn s_mallocFn = NULL;
  static SLB::FreeFn s_freeFn = NULL;
}
namespace SLB
{
  SLB_EXPORT void SetMemoryManagement(MallocFn mallocFn, FreeFn freeFn)
  {
    s_mallocFn = mallocFn;
    s_freeFn = freeFn;
  }
  SLB_EXPORT void* Malloc(size_t sz)
  {
    SLB_DEBUG_CALL;
    void *result = 0;
    if (s_mallocFn)
    {
      result = (s_mallocFn)(sz);
      SLB_DEBUG(100, "Allocating memory (allocator=%p) : %lu bytes -> %p", (void*)s_mallocFn, sz, result);
    }
    else
    {
      result = malloc(sz);
      SLB_DEBUG(100, "Allocating memory (allocator='default') : %lu bytes -> %p", sz, result);
    }
    return result;
  }
  SLB_EXPORT void Free(void* ptr)
  {
    SLB_DEBUG_CALL;
    if (s_freeFn)
    {
      SLB_DEBUG(100, "Deallocating memory (deallocator=%p) : ptr %p", (void*) s_freeFn, ptr);
      (s_freeFn)(ptr);
    }
    else
    {
      SLB_DEBUG(100, "Deallocating memory (deallocator='default') : ptr %p", ptr);
      free(ptr);
    }
  }
}
namespace SLB {
  ClassInfo::ClassInfo(Manager *m, const TypeInfoWrapper &ti) :
    Namespace(true),
    _manager(m),
    __TIW(ti),
    _name(""),
    _instanceFactory(0),
    _isHybrid(false)
  {
    SLB_DEBUG_CALL;
    _name = __TIW.name();
  }
  ClassInfo::~ClassInfo()
  {
    SLB_DEBUG_CALL;
    _baseClasses.clear();
    _properties.clear();
    Free_T(&_instanceFactory);
  }
  void ClassInfo::setName(const String& name)
  {
    SLB_DEBUG_CALL;
    _manager->rename(this, name);
    _name = name;
  }
  void ClassInfo::setInstanceFactory( InstanceFactory *factory)
  {
    SLB_DEBUG_CALL;
    Free_T(&_instanceFactory);
    _instanceFactory = factory;
  }
  void ClassInfo::setConstructor( FuncCall *constructor )
  {
    SLB_DEBUG_CALL;
    _constructor = constructor;
  }
  void ClassInfo::setClass__index( FuncCall *func )
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(2, "ClassInfo(%p) '%s' set Class __index -> %p", this, _name.c_str(), func);
    _meta__index[0] = func;
  }
  void ClassInfo::setClass__newindex( FuncCall *func )
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(2, "ClassInfo(%p) '%s' set Class __newindex -> %p", this, _name.c_str(), func);
    _meta__newindex[0] = func;
  }
  void ClassInfo::setObject__index( FuncCall *func )
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(2, "ClassInfo(%p) '%s' set Object __index -> %p", this, _name.c_str(), func);
    _meta__index[1] = func;
  }
  void ClassInfo::setObject__newindex( FuncCall *func )
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(2, "ClassInfo(%p) '%s' set Object __newindex -> %p", this, _name.c_str(), func);
    _meta__newindex[1] = func;
  }
  void ClassInfo::set__eq( FuncCall *func )
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(2, "ClassInfo(%p) '%s' set __eq -> %p", this, _name.c_str(), func);
    _meta__eq = func;
  }
  void ClassInfo::pushImplementation(lua_State *L)
  {
    Table::pushImplementation(L);
    lua_getmetatable(L, -1);
    lua_pushstring(L,"__back_reference");
    lua_pushvalue(L,-3);
    lua_rawset(L,-3);
    lua_pushstring(L, "__class_ptr");
    lua_pushlightuserdata(L, (void*)this);
    lua_rawset(L, -3);
    lua_pop(L,1);
  }
  void ClassInfo::pushInstance(lua_State *L, InstanceBase *instance)
  {
    SLB_DEBUG_CALL;
    int top = lua_gettop(L);
    if (instance == 0)
    {
      SLB_DEBUG(7, "Push(%s) Invalid!!", _name.c_str());
      lua_pushnil(L);
      return;
    }
    InstanceBase **obj = reinterpret_cast<InstanceBase**>
      (lua_newuserdata(L, sizeof(InstanceBase*)));
    *obj = instance;
    SLB_DEBUG(7, "Push(%s) InstanceHandler(%p) -> ptr(%p) const_ptr(%p)",
        _name.c_str(), instance, instance->get_ptr(), instance->get_const_ptr());
    push(L);
    lua_getmetatable(L,-1);
    lua_replace(L,-2);
    lua_pushvalue(L,-1);
    lua_setmetatable(L, top+1);
    lua_settop(L, top+1);
  }
  void *ClassInfo::get_ptr(lua_State *L, int pos) const
  {
    SLB_DEBUG_CALL;
    pos = lua_absindex(L,pos);
    void *obj = 0;
    InstanceBase *i = getInstance(L, pos);
    if (i)
    {
      ClassInfo *i_ci = i->getClass();
      assert("Invalid ClassInfo" && (i_ci != 0));
      obj = _manager->convert(
          i_ci->getTypeid(),
          getTypeid(),
          i->get_ptr()
        );
    }
    SLB_DEBUG(7, "Class(%s) get_ptr at %d -> %p", _name.c_str(), pos, obj);
    return obj;
  }
  const void* ClassInfo::get_const_ptr(lua_State *L, int pos) const
  {
    SLB_DEBUG_CALL;
    pos = lua_absindex(L,pos);
    const void *obj = 0;
    InstanceBase *i = getInstance(L, pos);
    if (i)
    {
      ClassInfo *i_ci = i->getClass();
      assert("Invalid ClassInfo" && (i_ci != 0));
      obj = _manager->convert(
          i_ci->getTypeid(),
          getTypeid(),
          i->get_const_ptr() );
    }
    SLB_DEBUG(7, "Class(%s) get_const_ptr -> %p", _name.c_str(), obj);
    return obj;
  }
  InstanceBase* ClassInfo::getInstance(lua_State *L, int pos) const
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(10, "L=%p; Pos = %i (abs = %i)",L, pos, lua_absindex(L,pos) );
    pos = lua_absindex(L,pos);
    InstanceBase *instance = 0;
    int top = lua_gettop(L);
    if (lua_getmetatable(L, pos))
    {
      lua_getfield(L, -1, "__class_ptr");
      if (!lua_isnil(L,-1))
      {
        void *obj = lua_touserdata(L, pos);
        if (obj == 0)
        {
          luaL_error(L,
            "Expected object of type %s at #%d",
            _name.c_str(), pos);
        }
        instance = *reinterpret_cast<InstanceBase**>(obj);
      }
    }
    lua_settop(L, top);
    SLB_DEBUG(10, "Instance(%p) -> %p (%s)",
      instance,
      instance? instance->get_const_ptr() : 0,
      instance? (instance->get_const_ptr() ? "const" : "non_const") : "nil");
    return instance;
  }
  BaseProperty* ClassInfo::getProperty(const String &key)
  {
    BaseProperty::Map::iterator prop = _properties.find(key);
    BaseProperty::Map::const_iterator end = _properties.end();
    for(BaseClassMap::iterator i = _baseClasses.begin(); (prop == end) && i != _baseClasses.end(); ++i)
    {
      prop = i->second->_properties.find(key);
      end = i->second->_properties.end();
    }
    if (prop != end) return prop->second.get();
    return 0;
  }
  void ClassInfo::push_ref(lua_State *L, void *ref )
  {
    SLB_DEBUG_CALL;
    if (_instanceFactory)
    {
      if (ref)
      {
        pushInstance(L, _instanceFactory->create_ref(_manager, ref) );
        SLB_DEBUG(7, "Class(%s) push_ref -> %p", _name.c_str(), ref);
      }
      else
      {
        luaL_error(L, "Can not push a NULL reference of class %s", _name.c_str());
      }
    }
    else
    {
      luaL_error(L, "Unknown class %s (push_reference)", _name.c_str());
    }
  }
  void ClassInfo::push_ptr(lua_State *L, void *ptr)
  {
    SLB_DEBUG_CALL;
    if (_instanceFactory)
    {
      pushInstance(L, _instanceFactory->create_ptr(_manager, ptr) );
    }
    else
    {
      luaL_error(L, "Can not push a ptr of class %s", _name.c_str());
    }
  }
  void ClassInfo::push_const_ptr(lua_State *L, const void *const_ptr)
  {
    SLB_DEBUG_CALL;
    if (_instanceFactory)
    {
      pushInstance(L, _instanceFactory->create_const_ptr(_manager, const_ptr) );
      SLB_DEBUG(7, "Class(%s) push const_ptr -> %p", _name.c_str(), const_ptr);
    }
    else
    {
      luaL_error(L, "Can not push a const_ptr of class %s", _name.c_str());
    }
  }
  void ClassInfo::push_copy(lua_State *L, const void *ptr)
  {
    SLB_DEBUG_CALL;
    if (_instanceFactory)
    {
      if (ptr)
      {
        pushInstance(L, _instanceFactory->create_copy(_manager, ptr) );
        SLB_DEBUG(7, "Class(%s) push copy -> %p", _name.c_str(), ptr);
      }
      else
      {
        luaL_error(L, "Can not push copy from NULL of class %s", _name.c_str());
      }
    }
    else
    {
      luaL_error(L, "Can not push a copy of class %s", _name.c_str());
    }
  }
  int ClassInfo::__index(lua_State *L)
  {
    SLB_DEBUG_CALL;
    int result = Table::__index(L);
    if ( result < 1 )
    {
      const int type = lua_istable(L,1)? 0 : 1;
      SLB_DEBUG(4, "Called ClassInfo(%p) '%s' __index %s", this, _name.c_str(), type? "OBJECT" : "CLASS");
      if (lua_isstring(L,2))
      {
        const char *key = lua_tostring(L,2);
        Object *obj = 0;
        for(BaseClassMap::iterator i = _baseClasses.begin(); obj == 0L && i != _baseClasses.end(); ++i)
        {
          Table *table = (i->second);
          obj = table->get(key);
        }
        if (obj)
        {
          obj->push(L);
          result = 1;
        }
        else
        {
          BaseProperty *prop = getProperty(key);
          if (prop)
          {
            prop->get(L,1);
            return 1;
          }
        }
      }
      if (result < 1)
      {
        if (_meta__index[type].valid())
        {
          _meta__index[type]->push(L);
          lua_insert(L,1);
          SLB_DEBUG_STACK(8,L, "Class(%s) __index {%s} metamethod -> %p",
            _name.c_str(), type? "OBJECT" : "CLASS", (void*)_meta__index[type].get() );
          assert("Error in number of stack elements" && lua_gettop(L) == 3);
          lua_call(L,lua_gettop(L)-1, LUA_MULTRET);
          result = lua_gettop(L);
        }
        else
        {
          SLB_DEBUG(4, "ClassInfo(%p) '%s' doesn't have __index[%s] implementation.",
            this, _name.c_str(), type? "OBJECT" : "CLASS");
        }
      }
    }
    return result;
  }
  int ClassInfo::__newindex(lua_State *L)
  {
    SLB_DEBUG_CALL;
    int type = lua_istable(L,1)? 0 : 1;
    SLB_DEBUG(4, "Called ClassInfo(%p) '%s' __newindex %s", this, _name.c_str(), type? "OBJECT" : "CLASS");
    if (type == 1 && lua_isstring(L,2))
    {
      const char *key = lua_tostring(L,2);
      BaseProperty *prop = getProperty(key);
      if (prop)
      {
        prop->set(L,1);
        return 1;
      }
    }
    if (_meta__newindex[type].valid())
    {
      _meta__newindex[type]->push(L);
      lua_insert(L,1);
      SLB_DEBUG_STACK(8,L, "Class(%s) __index {%s} metamethod -> %p",
        _name.c_str(), type? "OBJECT" :"CLASS", (void*)_meta__newindex[type].get() );
      assert("Error in number of stack elements" && lua_gettop(L) == 4);
      lua_call(L,lua_gettop(L)-1, LUA_MULTRET);
      return lua_gettop(L);
    }
    else
    {
      SLB_DEBUG(4, "ClassInfo(%p) '%s' doesn't have __newindex[%s] implementation.",
        this, _name.c_str(), type? "OBJECT" : "CLASS");
    }
    return Table::__newindex(L);
  }
  int ClassInfo::__garbageCollector(lua_State *L)
  {
    SLB_DEBUG_CALL;
    void *raw_ptr = lua_touserdata(L,1);
    if (raw_ptr == 0) luaL_error(L, "SLB check ERROR: __garbageCollector called on a no-instance value");
    InstanceBase* instance =
      *reinterpret_cast<InstanceBase**>(raw_ptr);
    const void* addr = instance->memoryRawPointer();
    instance->~InstanceBase();
    Free(const_cast<void*>(addr));
    return 0;
  }
  int ClassInfo::__call(lua_State *L)
  {
    SLB_DEBUG_CALL;
    if ( _constructor.valid() )
    {
      _constructor->push(L);
      lua_replace(L, 1);
      lua_call(L, lua_gettop(L)-1 , LUA_MULTRET);
      InstanceBase* instance_base = getInstance(L, -1);
      if (instance_base) {
        instance_base->setMustFreeMemFlag();
        if (_isHybrid)
        {
          int top = lua_gettop(L);
          HybridBase *hb = SLB::get<HybridBase*>(L,top);
          if (!hb) assert("Invalid push of hybrid object" && false);
          if (!hb->isAttached()) hb->attach(L);
          assert("Invalid lua stack..." && (top == lua_gettop(L)));
        }
      }
      return lua_gettop(L);
    }
    else
    {
      luaL_error(L,
        "ClassInfo '%s' is abstract ( or doesn't have a constructor ) ", _name.c_str());
    }
    return 0;
  }
  int ClassInfo::__tostring(lua_State *L)
  {
    SLB_DEBUG_CALL;
    int top = lua_gettop(L);
    lua_pushfstring(L, "Class(%s) [%s]", _name.c_str(), getInfo().c_str());
    ;
    for(BaseClassMap::iterator i = _baseClasses.begin(); i != _baseClasses.end(); ++i)
    {
        lua_pushfstring(L, "\n\tinherits from %s (%p)",i->second->getName().c_str(), (Object*) i->second);
    }
    for(Elements::iterator i = _elements.begin(); i != _elements.end(); ++i)
    {
      Object *obj = i->second.get();
      FuncCall *fc = slb_dynamic_cast<FuncCall>(obj);
      if (fc)
      {
        lua_pushfstring(L, "\n\tfunction (%s) [%s]",i->first.c_str(), obj->getInfo().c_str() );
        for (size_t i = 0; i < fc->getNumArguments(); ++i)
        {
          lua_pushfstring(L, "\n\t\t[%d] (%s) [%s]",i,
            fc->getArgType(i).name(),
            fc->getArgComment(i).c_str()
            );
        }
      }
      else
      {
        lua_pushfstring(L, "\n\t%s -> %p [%s] [%s]",i->first.c_str(), obj, obj->typeInfo().name(), obj->getInfo().c_str() );
      }
    }
    lua_concat(L, lua_gettop(L) - top);
    return 1;
  }
  bool ClassInfo::isSubClassOf( const ClassInfo *base )
  {
    SLB_DEBUG_CALL;
    if (base == this) return true;
    BaseClassMap::iterator i = _baseClasses.find( base->__TIW );
    if (i != _baseClasses.end()) return true;
    for(BaseClassMap::iterator i = _baseClasses.begin(); i != _baseClasses.end(); ++i)
    {
      if (i->second->isSubClassOf(base)) return true;
    }
    return false;
  }
  int ClassInfo::__eq(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(4, "Called ClassInfo(%p) '%s' __eq", this, _name.c_str() );
    if (_meta__eq.valid())
    {
      _meta__eq->push(L);
      lua_insert(L,1);
      lua_call(L, lua_gettop(L)-1 , LUA_MULTRET);
      return lua_gettop(L);
    }
    else return Table::__eq(L);
  }
}
int SLB_DEBUG_LEVEL_TAB = 0;
#include <sstream>
#include <cassert>
namespace SLB {
int ErrorHandler::call(lua_State *L, int nargs, int nresults)
{
  SLB_DEBUG_CALL;
  int base = lua_gettop(L) - nargs;
  lua_pushlightuserdata(L, this);
  lua_pushcclosure(L, _slb_stackHandler, 1);
  lua_insert(L, base);
  int result = lua_pcall(L, nargs, nresults, base);
  lua_remove(L, base);
  return result;
}
const char *ErrorHandler::SE_name()
{
  if (_lua_state) return _debug.name;
  return NULL;
}
const char *ErrorHandler::SE_nameWhat()
{
  if (_lua_state) return _debug.namewhat;
  return NULL;
}
const char *ErrorHandler::SE_what()
{
  if (_lua_state) return _debug.what;
  return NULL;
}
const char *ErrorHandler::SE_source()
{
  if (_lua_state) return _debug.source;
  return NULL;
}
const char *ErrorHandler::SE_shortSource()
{
  if (_lua_state) return _debug.short_src;
  return NULL;
}
int ErrorHandler::SE_currentLine()
{
  if (_lua_state) return _debug.currentline;
  return -1;
}
int ErrorHandler::SE_numberOfUpvalues()
{
  if (_lua_state) return _debug.nups;
  return -1;
}
int ErrorHandler::SE_lineDefined()
{
  if (_lua_state) return _debug.linedefined;
  return -1;
}
int ErrorHandler::SE_lastLineDefined()
{
  if (_lua_state) return _debug.lastlinedefined;
  return -1;
}
int ErrorHandler::_slb_stackHandler(lua_State *L)
{
  ErrorHandler *EH = reinterpret_cast<ErrorHandler*>(lua_touserdata(L,lua_upvalueindex(1)));
  if (EH) EH->process(L);
  return 1;
}
void ErrorHandler::process(lua_State *L)
{
  _lua_state = L;
  assert("Invalid state" && _lua_state != 0);
  const char *error = lua_tostring(_lua_state, -1);
  begin(error);
  for ( int level = 0; lua_getstack(_lua_state, level, &_debug ); level++)
  {
    if (lua_getinfo(L, "Slnu", &_debug) )
    {
      stackElement(level);
    }
    else
    {
      assert("[ERROR using Lua DEBUG INTERFACE]" && false);
    }
  }
  const char *msg = end();
  lua_pushstring(_lua_state, msg);
  _lua_state = 0;
}
void DefaultErrorHandler::begin(const char *error)
{
  _out.clear();
  _out.str("");
  _out << "SLB Exception: "
    << std::endl << "-------------------------------------------------------"
    << std::endl;
  _out << "Lua Error:" << std::endl << "\t"
    << error << std::endl
    << "Traceback:" << std::endl;
}
const char* DefaultErrorHandler::end()
{
  _final = _out.str();
  return _final.c_str();
}
void DefaultErrorHandler::stackElement(int level)
{
  _out << "\t [ " << level << " (" << SE_what() << ") ] ";
  int currentline = SE_currentLine();
  if (currentline > 0 )
  {
    _out << SE_shortSource() << ":" << currentline;
  }
  const char *name = SE_name();
  if (name)
  {
    _out << " @ " << name;
     if (SE_nameWhat()) _out << "(" << SE_nameWhat() << ")";
  }
  _out << std::endl;
}
}
namespace SLB {
  FuncCall::FuncCall() : _Treturn()
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(10, "Create FuncCall (%p)",this);
  }
  FuncCall::~FuncCall()
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(10, "Delete FuncCall (%p)",this);
  }
  void FuncCall::pushImplementation(lua_State *L)
  {
    SLB_DEBUG_CALL;
    lua_pushlightuserdata(L, (FuncCall*) this);
    lua_pushcclosure(L,FuncCall::_call, 1);
  }
  int FuncCall::_call(lua_State *L)
  {
    SLB_DEBUG_CALL;
    FuncCall *fc = (FuncCall*) lua_touserdata(L,lua_upvalueindex(1));
    assert("Invalid FuncCall" && fc);
#if SLB_USE_EXCEPTIONS != 0
    try
    {
      return fc->call(L);
    }
    catch ( std::exception &e )
    {
      luaL_error(L, e.what());
      return 0;
    }
#else
    return fc->call(L);
#endif
  }
  void FuncCall::setArgComment(size_t p, const String& c)
  {
    SLB_DEBUG_CALL;
    if (p < _Targs.size())
    {
      _Targs[p].second = c;
    }
    else
    {
    }
  }
  class LuaCFunction : public FuncCall
  {
  public:
    LuaCFunction(lua_CFunction f) : _func(f) { SLB_DEBUG_CALL; }
  protected:
    virtual ~LuaCFunction() { SLB_DEBUG_CALL; }
    void pushImplementation(lua_State *L) {SLB_DEBUG_CALL; lua_pushcfunction(L,_func); }
    virtual int call(lua_State *L)
    {
      SLB_DEBUG_CALL;
      luaL_error(L, "Code should never be reached %s:%d",__FILE__,__LINE__);
      return 0;
    }
    lua_CFunction _func;
  };
  FuncCall* FuncCall::create(lua_CFunction f)
  {
    SLB_DEBUG_CALL;
    return new (Malloc(sizeof(LuaCFunction))) LuaCFunction(f);
  }
}
#include <sstream>
#include <iostream>
namespace SLB {
   InvalidMethod::InvalidMethod(const HybridBase *obj, const char *c)
  {
    SLB_DEBUG_CALL;
    const ClassInfo *CI = obj->getClassInfo();
    std::ostringstream out;
    out << "Invalid Method '" << CI->getName() << "::" <<
      c << "' NOT FOUND!" << std::endl;
    _what = out.str();
  }
  struct InternalHybridSubclass : public Table
  {
    InternalHybridSubclass(ClassInfo *ci) : _CI(ci)
    {
      SLB_DEBUG_CALL;
      assert("Invalid ClassInfo" && _CI.valid());
    }
    int __newindex(lua_State *L)
    {
      SLB_DEBUG_CALL;
      SLB_DEBUG_CLEAN_STACK(L,-2);
      SLB_DEBUG_STACK(6,L, "Call InternalHybridSubclass(%p)::__nexindex", this);
      luaL_checkstring(L,2);
      if (lua_type(L,3) != LUA_TFUNCTION)
      {
        luaL_error(L, "Only functions can be added to hybrid classes"
          " (invalid %s of type %s)", lua_tostring(L,2), lua_typename(L, 3));
      }
      SLB_DEBUG(4, "Added method to an hybrid-subclass:%s", lua_tostring(L,2));
      lua_pushcclosure(L, HybridBase::call_lua_method, 1);
      setCache(L);
      return 0;
    }
    int __call(lua_State *L)
    {
      SLB_DEBUG_CALL;
      SLB_DEBUG_STACK(6,L, "Call InternalHybridSubclass(%p)::__call", this);
      ref_ptr<FuncCall> fc = _CI->getConstructor();
      assert("Invalid Constructor!" && fc.valid());
      fc->push(L);
      lua_replace(L,1);
      lua_call(L, lua_gettop(L) -1 , LUA_MULTRET);
      {
        SLB_DEBUG_CLEAN_STACK(L,0);
        HybridBase *obj = SLB::get<HybridBase*>(L,1);
        if (obj == 0) luaL_error(L, "Output(1) of constructor should be an HybridBase instance");
        obj->_subclassMethods = this;
      }
      return lua_gettop(L);
    }
  private:
    ref_ptr<ClassInfo> _CI;
  };
  HybridBase::HybridBase() : _lua_state(0),
    _data(0)
  {
    SLB_DEBUG_CALL;
  }
  HybridBase::~HybridBase()
  {
    SLB_DEBUG_CALL;
    unAttach();
  }
  void HybridBase::attach(lua_State *L)
  {
    SLB_DEBUG_CALL;
    if (_lua_state != 0 && _lua_state != L) {
      SLB_THROW(std::runtime_error("Trying to reattach an Hybrid instance"));
      SLB_CRITICAL_ERROR("Trying to reattach an Hybrid instance");
    }
    if (L)
    {
      SLB_DEBUG_CLEAN_STACK(L,0);
      _lua_state = L;
      lua_newtable(_lua_state);
      _data = luaL_ref(_lua_state, LUA_REGISTRYINDEX);
    }
  }
  void HybridBase::unAttach()
  {
    SLB_DEBUG_CALL;
    clearMethodMap();
    _subclassMethods = 0;
    if (_lua_state && _data )
    {
      luaL_unref(_lua_state, LUA_REGISTRYINDEX, _data);
      _data = 0;
      _lua_state = 0;
    }
  }
  void HybridBase::clearMethodMap()
  {
    SLB_DEBUG_CALL;
    for(MethodMap::iterator i = _methods.begin(); i != _methods.end(); i++ )
    {
      Free_T(&i->second);
    }
    _methods.clear();
  }
  bool HybridBase::getMethod(const char *name) const
  {
    SLB_DEBUG_CALL;
    if (_lua_state == 0) {
      SLB_THROW(std::runtime_error("Hybrid instance not attached"));
      SLB_CRITICAL_ERROR("Hybrid instance not attached")
    }
    SLB_DEBUG_STACK(5,_lua_state, "HybridBase(%p)::getMethod '%s' (_lua_state = %p)", this, name, _lua_state);
    int top = lua_gettop(_lua_state);
    if (_subclassMethods.valid())
    {
      lua_pushstring(_lua_state,name);
      _subclassMethods->getCache(_lua_state);
      if (!lua_isnil(_lua_state,-1))
      {
        assert("Invalid Stack" && (lua_gettop(_lua_state) == top+1));
        return true;
      }
      lua_pop(_lua_state,1);
      assert("Invalid Stack" && (lua_gettop(_lua_state) == top));
    }
    ClassInfo *ci = getClassInfo();
    ci->push(_lua_state);
    lua_getmetatable(_lua_state,-1);
    lua_getfield(_lua_state,-1, "__hybrid");
    if (!lua_isnil(_lua_state,-1))
    {
      lua_pushstring(_lua_state,name);
      lua_rawget(_lua_state,-2);
      if (!lua_isnil(_lua_state,-1))
      {
        lua_replace(_lua_state,top+1);
        lua_settop(_lua_state,top+1);
        SLB_DEBUG(6, "HybridBase(%p-%s)::getMethod '%s' (_lua_state = %p) -> FOUND",
            this, ci->getName().c_str(),name, _lua_state);
        assert("Invalid Stack" && (lua_gettop(_lua_state) == top+1));
        return true;
      }
      else SLB_DEBUG(6, "HybridBase(%p-%s)::getMethod '%s' (_lua_state = %p) -> *NOT* FOUND",
        this,ci->getName().c_str(), name, _lua_state);
    }
    else SLB_DEBUG(4, "HybridBase(%p-%s) do not have any hybrid methods", this, ci->getName().c_str());
    lua_settop(_lua_state,top);
    return false;
  }
  void HybridBase::setMethod(lua_State *L, ClassInfo *ci)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_CLEAN_STACK(L,-2);
    int top = lua_gettop(L);
    assert( "Invalid key for method" && lua_type(L,top-1) == LUA_TSTRING);
    assert( "Invalid type of method" && lua_type(L,top) == LUA_TFUNCTION);
    ci->push(L);
    lua_getmetatable(L,-1);
    lua_getfield(L,-1, "__hybrid");
    if (lua_isnil(L,-1))
    {
      lua_pop(L,1);
      lua_newtable(L);
      lua_pushstring(L, "__hybrid");
      lua_pushvalue(L,-2);
      lua_rawset(L, top+2);
    }
    lua_insert(L,top-2);
    lua_settop(L, top+1);
    lua_rawset(L,top-2);
    lua_settop(L, top-2);
  }
  void HybridBase::registerAsHybrid(ClassInfo *ci)
  {
    SLB_DEBUG_CALL;
    ci->setClass__newindex( FuncCall::create(class__newindex) );
    ci->setClass__index( FuncCall::create(class__index) );
    ci->setObject__index( FuncCall::create(object__index) );
    ci->setObject__newindex( FuncCall::create(object__newindex) );
    ci->setHybrid();
  }
  const HybridBase* get_hybrid(lua_State *L, int pos)
  {
    SLB_DEBUG_CALL;
    const HybridBase *obj = get<const HybridBase*>(L,pos);
    if (!obj)
    {
      if (lua_type(L,pos) == LUA_TUSERDATA)
      {
        void *dir = lua_touserdata(L,pos);
        ClassInfo *ci = Manager::getInstance(L)->getClass(L,pos);
        if (ci == 0)
        {
          luaL_error(L, "Invalid Hybrid object (index=%d) "
          "'%s' %p", pos, ci->getName().c_str(), dir);
        }
        else
        {
          luaL_error(L, "Invalid Hybrid object (index=%d) "
          "userdata (NOT REGISTERED WITH SLB) %p", pos, dir);
        }
      }
      else
      {
        luaL_error(L, "Invalid Hybrid object (index=%d) found %s", pos, luaL_typename(L,pos));
      }
    }
    return obj;
  }
  int HybridBase::call_lua_method(lua_State *L)
  {
    SLB_DEBUG_CALL;
    const HybridBase *hb = get_hybrid( L, 1 );
    if (hb->_lua_state == 0) luaL_error(L, "Instance(%p) not attached to any lua_State...", hb);
    if (hb->_lua_state != L) luaL_error(L, "This instance(%p) is attached to another lua_State(%p)", hb, hb->_lua_state);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_insert(L,1);
    SLB_DEBUG_STACK(10, L, "Hybrid(%p)::call_lua_method ...", hb);
    lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
    return lua_gettop(L);
  }
  int HybridBase::class__newindex(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_CLEAN_STACK(L,-2);
    ClassInfo *ci = Manager::getInstance(L)->getClass(L,1);
    if (ci == 0) luaL_error(L, "Invalid Class at #1");
    const int key = 2;
    const int value = 3;
    if (lua_isstring(L,key) && lua_isfunction(L,value))
    {
      lua_pushcclosure(L, HybridBase::call_lua_method, 1);
      setMethod(L, ci);
    }
    else
    {
      luaL_error(L,
        "hybrid instances can only have new methods (functions) "
        "indexed by strings ( called with: class[ (%s) ] = (%s) )",
        lua_typename(L, lua_type(L,key)), lua_typename(L, lua_type(L,value))
        );
    }
    return 0;
  }
  int HybridBase::object__index(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_CLEAN_STACK(L,+1);
    SLB_DEBUG(4, "HybridBase::object__index");
    HybridBase* obj = get<HybridBase*>(L,1);
    if (obj == 0) luaL_error(L, "Invalid instance at #1");
    if (!obj->_lua_state) luaL_error(L, "Hybrid instance not attached or invalid method");
    if (obj->_lua_state != L) luaL_error(L, "Can not use that object outside its lua_state(%p)", obj->_lua_state);
    const char *key = lua_tostring(L,2);
    lua_rawgeti(L, LUA_REGISTRYINDEX, obj->_data);
    lua_getfield(L, -1, key);
    if (lua_isnil(L,-1))
    {
      lua_pop(L,2);
    }
    else
    {
      lua_remove(L,-2);
      return 1;
    }
    if(obj->getMethod(key)) return 1;
    lua_pushnil(L);
    return 1;
  }
  int HybridBase::object__newindex(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_CLEAN_STACK(L,-2);
    SLB_DEBUG(4, "HybridBase::object__newindex");
    HybridBase* obj = get<HybridBase*>(L,1);
    if (obj == 0) luaL_error(L, "Invalid instance at #1");
    if (!obj->_lua_state) luaL_error(L, "Hybrid instance not attached or invalid method");
    if (obj->_lua_state != L) luaL_error(L, "Can not use that object outside its lua_state(%p)", obj->_lua_state);
    lua_rawgeti(L, LUA_REGISTRYINDEX, obj->_data);
    lua_replace(L,1);
    lua_settable(L,1);
    return 0;
  }
  int HybridBase::class__index(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_CLEAN_STACK(L,+1);
    SLB_DEBUG_STACK(6, L, "Call class__index");
    ClassInfo *ci = Manager::getInstance(L)->getClass(L,1);
    if (ci == 0) luaL_error(L, "Expected a valid class.");
    luaL_checkstring(L,2);
    if (!ci->hasConstructor())
    {
      luaL_error(L, "Hybrid Class(%s) doesn't have constructor."
        " You can not subclass(%s) from it", ci->getName().c_str(),
        lua_tostring(L,2));
    }
    ref_ptr<InternalHybridSubclass> subc = new (Malloc(sizeof(InternalHybridSubclass))) InternalHybridSubclass(ci) ;
    subc->push(L);
    lua_pushvalue(L,2);
    lua_pushvalue(L,-2);
    ci->setCache(L);
    return 1;
  }
}
namespace SLB {
  InstanceBase::InstanceBase(Type t,ClassInfo *ci) : _flags(t), _class(ci)
  {
    SLB_DEBUG_CALL;
  }
  InstanceBase::~InstanceBase() {SLB_DEBUG_CALL;}
  InstanceFactory::~InstanceFactory() {SLB_DEBUG_CALL;}
}
namespace SLB {
  Iterator::Iterator(IteratorBase *b) : _iterator(b)
  {
    SLB_DEBUG_CALL;
  }
  void Iterator::pushImplementation(lua_State *L)
  {
    SLB_DEBUG_CALL;
    lua_pushlightuserdata(L, (void*) _iterator );
    lua_pushcclosure( L, Iterator::iterator_call, 1 );
  }
  Iterator::~Iterator()
  {
    SLB_DEBUG_CALL;
    Free_T(&_iterator);
  }
  int Iterator::iterator_call(lua_State *L)
  {
    SLB_DEBUG_CALL;
    IteratorBase *ib = reinterpret_cast<IteratorBase*>( lua_touserdata(L, lua_upvalueindex(1) ) );
    return ib->push(L);
  }
}
#include <sstream>
namespace SLB {
  LuaCallBase::LuaCallBase(lua_State *L, int index) : _lua_state(L) { SLB_DEBUG_CALL; getFunc(index); }
  LuaCallBase::LuaCallBase(lua_State *L, const char *func) : _lua_state(L)
  {
    SLB_DEBUG_CALL;
    lua_getglobal(L,func);
    getFunc(-1);
    lua_pop(L,1);
  }
  LuaCallBase::~LuaCallBase()
  {
    SLB_DEBUG_CALL;
    luaL_unref(_lua_state, LUA_REGISTRYINDEX, _ref);
  }
  void LuaCallBase::getFunc(int index)
  {
    SLB_DEBUG_CALL;
    lua_pushvalue(_lua_state,index);
    if (lua_type(_lua_state, -1) != LUA_TFUNCTION)
    {
      SLB_THROW(std::runtime_error( "No Lua function was found at the index you provided." ));
      SLB_CRITICAL_ERROR("No Lua function was found at the index you provided.");
    }
    _ref = luaL_ref(_lua_state, LUA_REGISTRYINDEX);
  }
  int LuaCallBase::errorHandler(lua_State *L)
  {
    SLB_DEBUG_CALL;
    std::ostringstream out;
    lua_Debug debug;
    out << "SLB Exception: "
      << std::endl << "-------------------------------------------------------"
      << std::endl;
    out << "Lua Error:" << std::endl << "\t"
      << lua_tostring(L, -1) << std::endl
      << "Traceback:" << std::endl;
    for ( int level = 0; lua_getstack(L, level, &debug ); level++)
    {
      if (lua_getinfo(L, "Sln", &debug) )
      {
        out << "\t [ " << level << " (" << debug.what << ") ] ";
        if (debug.currentline > 0 )
        {
          out << debug.short_src << ":" << debug.currentline;
        }
        if (debug.name)
        {
          out << " @ " << debug.name;
           if (debug.namewhat) out << "(" << debug.namewhat << ")";
        }
        out << std::endl;
      }
      else
      {
        out << "[ERROR using Lua DEBUG INTERFACE]" << std::endl;
      }
    }
    lua_pushstring(L, out.str().c_str()) ;
    return 1;
  }
  void LuaCallBase::execute(int numArgs, int numOutput, int )
  {
    SLB_DEBUG_CALL;
    DefaultErrorHandler handler;
    if(handler.call(_lua_state, numArgs, numOutput))
    {
      const char* msg = lua_tostring(_lua_state, -1);
      SLB_THROW(std::runtime_error( msg ? msg : "Unknown Error" ));
      SLB_CRITICAL_ERROR(msg ? msg : "Unknown Error" );
    }
  }
}
#include <iostream>
namespace SLB {
  Mutex managerMutex;
  int SLB_type(lua_State *L)
  {
    SLB_DEBUG_CALL;
    const ClassInfo *ci = Manager::getInstance(L)->getClass(L,-1);
    if (ci)
    {
      lua_pushstring(L, ci->getName().c_str());
      return 1;
    }
    return 0;
  }
  int SLB_rawptr(lua_State *L)
  {
    SLB_DEBUG_CALL;
    int top = lua_gettop(L);
    if (lua_getmetatable(L,1))
    {
      lua_getfield(L, -1, "__class_ptr");
      if (!lua_isnil(L,-1))
      {
        ClassInfo* ci = reinterpret_cast<ClassInfo*>( lua_touserdata(L,-1) );
        const void* raw = ci->get_const_ptr(L, 1);
        lua_settop(L, top);
        lua_pushinteger(L, (lua_Integer) raw);
        return 1;
      }
    }
    lua_settop(L, top);
    return 0;
  }
  int SLB_copy(lua_State *L)
  {
    SLB_DEBUG_CALL;
    int top = lua_gettop(L);
    if (lua_getmetatable(L,1))
    {
      lua_getfield(L, -1, "__class_ptr");
      if (!lua_isnil(L,-1))
      {
        ClassInfo* ci = reinterpret_cast<ClassInfo*>( lua_touserdata(L,-1) );
        lua_settop(L, top);
        ci->push_copy(L, ci->get_const_ptr(L,1));
        return 1;
      }
    }
    lua_settop(L, top);
    return 0;
  }
  int SLB_using_index(lua_State *L)
  {
    SLB_DEBUG_CALL;
    lua_pushnil(L);
    while( lua_next(L, lua_upvalueindex(1)) )
    {
      lua_pushvalue(L,2);
      lua_gettable(L, -2);
      if (!lua_isnil(L,-1))
      {
        return 1;
      }
      lua_pop(L,2);
    }
    return 0;
  }
  int SLB_using(lua_State *L)
  {
    SLB_DEBUG_CALL;
    int top = lua_gettop(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, LUA_REGISTRYINDEX, "SLB_using");
    if ( lua_isnil(L,-1) )
    {
      lua_pop(L,1);
      lua_pushglobaltable(L);
      if(!lua_getmetatable(L, -1))
      {
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_setmetatable(L, -3);
      }
      else
      {
        luaL_error(L, "Can not use SLB.using,"
          " _G already has a metatable");
      }
      lua_newtable(L);
      lua_pushvalue(L,-1);
      lua_setfield(L, LUA_REGISTRYINDEX, "SLB_using");
      lua_pushvalue(L,-1);
      lua_pushcclosure(L, SLB_using_index, 1);
      lua_setfield(L, -3, "__index");
    }
    lua_pushvalue(L,1);
    luaL_ref(L, -2);
    lua_settop(L,top);
    return 0;
  }
  int SLB_isA(lua_State *L)
  {
    SLB_DEBUG_CALL;
    int top = lua_gettop(L);
    if (top != 2)
      luaL_error(L, "Invalid number of arguments (instance, class)");
    ClassInfo* ci_a = 0;
    if (lua_getmetatable(L,1))
    {
      lua_getfield(L, -1, "__class_ptr");
      if (!lua_isnil(L,-1))
      {
        ci_a = reinterpret_cast<ClassInfo*>( lua_touserdata(L,-1) );
      }
    }
    ClassInfo* ci_b = 0;
    if (lua_getmetatable(L,2))
    {
      lua_getfield(L, -1, "__class_ptr");
      if (!lua_isnil(L,-1))
      {
        ci_b = reinterpret_cast<ClassInfo*>( lua_touserdata(L,-1) );
      }
    }
    lua_settop(L, top);
    if ( ci_a && ci_b )
    {
      lua_pushboolean(L, ci_a->isSubClassOf(ci_b) );
    }
    else
    {
      lua_pushboolean(L, false);
    }
    return 1;
  }
  int SLB_allTypes(lua_State *L)
  {
    SLB_DEBUG_CALL;
    Manager::ClassMap &map = Manager::getInstance(L)->getClasses();
    lua_newtable(L);
    for(Manager::ClassMap::iterator i = map.begin(); i != map.end(); ++i)
    {
      lua_pushstring(L, i->second->getName().c_str());
      i->second->push(L);
      lua_rawset(L,-3);
    }
    return 1;
  }
  static const luaL_Reg SLB_funcs[] = {
    {"type", SLB_type},
    {"copy", SLB_copy},
    {"using", SLB_using},
    {"rawptr", SLB_rawptr},
    {"isA", SLB_isA},
    {"allTypes", SLB_allTypes},
    {NULL, NULL}
  };
  Manager* Manager::_default = 0;
  Manager::Manager()
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(0, "Manager initialization");
    _global = new (Malloc(sizeof(Namespace))) Namespace;
  }
  Manager::~Manager()
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(0, "Manager destruction");
  }
  void Manager::registerSLB(lua_State *L)
  {
    SLB_DEBUG_CALL;
    int top = lua_gettop(L);
    lua_newtable(L);
    luaL_setfuncs(L, SLB_funcs, 0);
    lua_pushvalue(L,-1);
    lua_setglobal(L,"SLB");
    lua_newtable(L);
    lua_pushstring(L,"__index");
    _global->push(L);
    lua_rawset(L,-3);
    lua_setmetatable(L,-2);
    lua_settop(L,top);
    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, "SLB::Manager");
  }
  const ClassInfo *Manager::getClass(const TypeInfoWrapper &ti) const
  {
    SLB_DEBUG_CALL;
    ActiveWaitCriticalSection lock(&managerMutex);
    assert("Invalid type_info" && (&ti) );
    ClassMap::const_iterator i = _classes.find(ti);
    if ( i != _classes.end() ) return i->second.get();
    return 0;
  }
  const ClassInfo *Manager::getClass(const String &name) const
  {
    SLB_DEBUG_CALL;
    NameMap::const_iterator i = _names.find(name);
    if ( i != _names.end() )
      return getClass( i->second );
    return 0;
  }
  ClassInfo *Manager::getClass(lua_State *L, int pos) const
  {
    SLB_DEBUG_CALL;
    pos = lua_absindex(L,pos);
    int top = lua_gettop(L);
    ClassInfo* ci = 0L;
    if (lua_getmetatable(L,pos))
    {
      lua_getfield(L, -1, "__class_ptr");
      if (!lua_isnil(L,-1))
      {
        ci = reinterpret_cast<ClassInfo*>( lua_touserdata(L,-1) );
      }
    }
    lua_settop(L, top);
    return ci;
  }
  ClassInfo *Manager::getClass(const String &name)
  {
    SLB_DEBUG_CALL;
    NameMap::iterator i = _names.find(name);
    if ( i != _names.end() ) return getClass( i->second );
    return 0;
  }
  ClassInfo *Manager::getClass(const TypeInfoWrapper &ti)
  {
    SLB_DEBUG_CALL;
    ActiveWaitCriticalSection lock(&managerMutex);
    ClassInfo *result = 0;
    ClassMap::iterator i = _classes.find(ti);
    if ( i != _classes.end() ) result = i->second.get();
    SLB_DEBUG(6, "ClassInfo = %p", (void*) result);
    return result;
  }
  bool Manager::copy(lua_State *from, int pos, lua_State *to)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_CLEAN_STACK(from,0);
    if (from == to)
    {
      lua_pushvalue(from, pos);
      return true;
    }
    switch(lua_type(from, pos))
    {
      case LUA_TNIL:
        {
          SLB_DEBUG(20, "copy from %p(%d)->%p [nil]", from,pos,to);
          lua_pushnil(to);
          return true;
        }
      case LUA_TNUMBER:
        {
          SLB_DEBUG(20, "copy from %p(%d)->%p [number]", from,pos,to);
          lua_Number n = lua_tonumber(from,pos);
          lua_pushnumber(to, n);
          return true;
        }
      case LUA_TBOOLEAN:
        {
          SLB_DEBUG(20, "copy from %p(%d)->%p [boolean]", from,pos,to);
          int b = lua_toboolean(from,pos);
          lua_pushboolean(to,b);
          return true;
        }
      case LUA_TSTRING:
        {
          SLB_DEBUG(20, "copy from %p(%d)->%p [string]", from,pos,to);
          const char *s = lua_tostring(from,pos);
          lua_pushstring(to,s);
          return true;
        }
      case LUA_TTABLE:
        {
          SLB_DEBUG(0, "*WARNING* copy of tables Not yet Implemented!!!");
          return false;
        }
      case LUA_TUSERDATA:
        {
          SLB_DEBUG(20, "copy from %p(%d)->%p [Object]", from,pos,to);
          ClassInfo *ci = getClass(from, pos);
          if (ci != 0L)
          {
            const void* ptr = ci->get_const_ptr(from, pos);
            SLB_DEBUG(25, "making a copy of the object %p", ptr);
            ci->push_copy(to,ptr);
            return true;
          }
          else
          {
            SLB_DEBUG(25, "Could not recognize the object");
            return false;
          }
        }
    }
    SLB_DEBUG(10,
      "Invalid copy from %p(%d)->%p %s",
      from,pos,to, lua_typename(from, lua_type(from,pos)));
    return false;
  }
  ClassInfo *Manager::getOrCreateClass(const TypeInfoWrapper &ti)
  {
    SLB_DEBUG_CALL;
    assert("Invalid type_info" && (&ti) );
    ClassInfo *c = 0;
    {
      CriticalSection lock(&managerMutex);
      ClassMap::iterator i = _classes.find(ti);
      if ( i != _classes.end() )
      {
        c = i->second.get();
      }
    }
    if (c == 0)
    {
      CriticalSection lock(&managerMutex);
      c = new (Malloc(sizeof(ClassInfo))) ClassInfo(this,ti);
      _classes[ c->getTypeid() ] = c;
    }
    return c;
  }
  void Manager::set(const String &name, Object *obj)
  {
    SLB_DEBUG_CALL;
    ActiveWaitCriticalSection lock(&managerMutex);
    _global->set(name, obj);
  }
  void Manager::rename(ClassInfo *ci, const String &new_name)
  {
    SLB_DEBUG_CALL;
    CriticalSection lock(&managerMutex);
    const String old_name = ci->getName();
    NameMap::iterator i = _names.find(old_name);
    if ( i != _names.end() )
    {
      _global->erase(old_name);
      _names.erase(i);
    }
    _global->set(new_name, ci);
    _names[ new_name ] = ci->getTypeid();
  }
  Manager *Manager::getInstance(lua_State *L)
  {
    Manager *m = 0L;
    lua_getfield(L,LUA_REGISTRYINDEX, "SLB::Manager");
    if (lua_islightuserdata(L,-1))
    {
      m = reinterpret_cast<Manager*>(lua_touserdata(L,-1));
    }
    lua_pop(L,1);
    return m;
  }
  Manager *Manager::defaultManager()
  {
    if (_default == 0)
    {
      _default = new (Malloc(sizeof(Manager))) Manager();
    }
    return _default;
  }
  void Manager::destroyDefaultManager()
  {
    if (_default)
    {
      Free_T(&_default);
    }
  }
  void* Manager::recursiveConvert(const TypeInfoWrapper& C1, const TypeInfoWrapper &C2, const TypeInfoWrapper& prev, void *obj)
  {
    for (ConversionsMap::iterator it=_conversions.begin(); it!=_conversions.end(); ++it)
    {
      if (it->first.first == C1)
      {
        if (it->first.second == C2)
        {
          return it->second( obj );
        }
        else if (!(it->first.second == prev))
        {
          void *foundObj = recursiveConvert(it->first.second, C2, C1, it->second(obj));
          if (foundObj)
          {
            return foundObj;
          }
        }
      }
    }
    return 0;
  }
}
#include<assert.h>
namespace SLB {
  const char *objectsTable_name = "SLB_Objects";
  const char *refTable_name = "SLB_References";
  Object::Object() : _refCounter(0)
  {
    SLB_DEBUG_CALL;
  }
  Object::~Object()
  {
    SLB_DEBUG_CALL;
  }
  void Object::initialize(lua_State *L) const
  {
    SLB_DEBUG_CALL;
    int top = lua_gettop(L);
    lua_newtable(L);
    lua_newtable(L);
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L,-2);
    lua_setfield(L, LUA_REGISTRYINDEX, objectsTable_name);
    lua_newtable(L);
    lua_newtable(L);
    lua_pushstring(L, "k");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L,-2);
    lua_setfield(L, LUA_REGISTRYINDEX, refTable_name);
    lua_settop(L, top);
  }
  void Object::push(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(3, "(L %p) Object::push(%p) [%s]", L, this, typeid(*this).name());
    int top = lua_gettop(L);
    lua_getfield(L, LUA_REGISTRYINDEX, objectsTable_name);
    if (lua_isnil(L, -1))
    {
      lua_pop(L, 1);
      initialize(L);
      lua_getfield(L, LUA_REGISTRYINDEX, objectsTable_name);
    }
    lua_getfield(L, LUA_REGISTRYINDEX, refTable_name);
    lua_pushlightuserdata(L, (void*) this );
    lua_rawget(L, top + 1);
    if (lua_isnil(L,-1))
    {
      lua_pop(L, 1);
      int objpos = lua_gettop(L) + 1;
      pushImplementation(L);
      SLB_DEBUG(5, "\t-new object");
      if (lua_gettop(L) != objpos)
      {
        luaL_error(L, "Error on Object::push the current stack "
          "has %d elments and should have only one.",
          lua_gettop(L) - objpos - 1);
        lua_settop(L, top);
        return;
      }
      lua_pushlightuserdata(L, (void*) this);
      lua_pushvalue(L, objpos);
      lua_rawset(L, top+1);
      lua_pushvalue(L, objpos);
      Object **objptr = reinterpret_cast<Object**>(
        lua_newuserdata(L, sizeof(Object*)));
      *objptr = this;
      ref();
      if(luaL_newmetatable(L, "SLB_ObjectPtr_GC"))
      {
        lua_pushcfunction(L, GC_callback);
        lua_setfield(L, -2, "__gc");
      }
      lua_setmetatable(L, -2);
      lua_rawset(L, top+2);
      assert( lua_gettop(L) == objpos);
    }
    else
    {
      SLB_DEBUG(6, "\t-object exists");
    }
    lua_replace(L, top+1);
    lua_settop(L, top+1);
  }
  int Object::GC_callback(lua_State *L)
  {
    SLB_DEBUG_CALL;
    Object **ptr_obj = reinterpret_cast<Object**>(lua_touserdata(L, 1));
    Object *obj = *ptr_obj;
    SLB_DEBUG(2, "(L %p) GC object %p (refcount %d - 1) [%s]", L, obj, obj->referenceCount(), typeid(*obj).name());
    obj->onGarbageCollection(L);
    obj->unref();
    *ptr_obj = 0L;
    return 0;
  }
}
#include <iostream>
namespace SLB {
BaseProperty::BaseProperty()
{
}
BaseProperty::~BaseProperty()
{
}
void BaseProperty::pushImplementation(lua_State *L)
{
  luaL_error(L, "Properties can not be accessed directly");
}
void BaseProperty::set(lua_State *L, int )
{
  luaL_error(L, "Invalid property write");
}
void BaseProperty::get(lua_State *L, int )
{
  luaL_error(L, "Invalid property read");
}
}
#include<sstream>
namespace SLB {
#if SLB_DEBUG_LEVEL != 0
  void ScriptHook(lua_State *L, lua_Debug *ar)
  {
    lua_getinfo(L, "Sl",ar);
    __SLB_ADJUST__();
    SLB_DEBUG_FUNC("SLB-X[%p] %s:%d",L,ar->short_src,ar->currentline );
    SLB_DEBUG_FUNC("\n");\
  }
#endif
  static void DefaultPrintCallback(Script *s, const char *str, size_t length) {
    std::cout.write(str,length);
  }
  Script::Script(Manager *m) :
    _manager(m),
    _lua_state(0),
    _printCallback(DefaultPrintCallback),
    _allocator(&Script::allocator),
    _allocator_ud(0),
    _errorHandler(0),
    _loadDefaultLibs(true)
  {
    SLB_DEBUG_CALL;
    DefaultErrorHandler *err = 0;
    New_T(&err);
    _errorHandler = err;
  }
  Script::~Script()
  {
    SLB_DEBUG_CALL;
    Free_T(&_errorHandler);
    close();
  }
  void Script::setAllocator(lua_Alloc f, void *ud)
  {
    _allocator = f;
    _allocator_ud = ud;
  }
  lua_State* Script::getState()
  {
    SLB_DEBUG_CALL;
    if (!_lua_state)
    {
      SLB_DEBUG(10, "Open default libs = %s", _loadDefaultLibs ? " true": " false");
      _lua_state = lua_newstate(_allocator, _allocator_ud);
      assert("Can not create more lua_states" && (_lua_state != 0L));
      if (_loadDefaultLibs) luaL_openlibs(_lua_state);
      _manager->registerSLB(_lua_state);
      lua_pushlightuserdata(_lua_state, this);
      lua_pushcclosure(_lua_state,PrintHook,1);
      lua_setglobal(_lua_state, "print");
      #if SLB_DEBUG_LEVEL != 0
      lua_sethook(_lua_state, ScriptHook, LUA_MASKLINE, 0);
      #endif
      onNewState(_lua_state);
    }
    return _lua_state;
  }
  void Script::close()
  {
    SLB_DEBUG_CALL;
    if (_lua_state)
    {
      onCloseState(_lua_state);
      lua_close(_lua_state);
      _lua_state = 0;
    }
  }
  void Script::callGC()
  {
    SLB_DEBUG_CALL;
    if (_lua_state)
    {
      onGC(_lua_state);
      lua_gc(_lua_state, LUA_GCCOLLECT, 0);
    }
  }
  size_t Script::memUsage()
  {
    SLB_DEBUG_CALL;
    size_t result = 0;
    if (_lua_state)
    {
      int r = lua_gc(_lua_state, LUA_GCCOUNT, 0);
      result = r;
    }
    return result;
  }
  void Script::doFile(const char *filename) SLB_THROW((std::exception))
  {
    if (!safeDoFile(filename)) {
      SLB_THROW(std::runtime_error( getLastError() ));
      SLB_CRITICAL_ERROR( getLastError() )
    };
  }
  bool Script::safeDoFile(const char *filename)
  {
    SLB_DEBUG_CALL;
    lua_State *L = getState();
    int top = lua_gettop(L);
    SLB_DEBUG(10, "filename %s = ", filename);
    bool result = true;
    switch(luaL_loadfile(L,filename))
    {
      case LUA_ERRFILE:
      case LUA_ERRSYNTAX:
        _lastError = lua_tostring(L, -1);
        result = false;
        break;
      case LUA_ERRMEM:
        _lastError = "Error allocating memory";
        result = false;
        break;
    }
    if( _errorHandler->call(_lua_state, 0, 0))
    {
      _lastError = lua_tostring(L,-1);
      result = false;
    }
    lua_settop(L,top);
    return result;
  }
  void Script::doString(const char *o_code, const char *hint) SLB_THROW((std::exception))
  {
    if (!safeDoString(o_code, hint)) {
      SLB_THROW(std::runtime_error( getLastError() ));
      SLB_CRITICAL_ERROR( getLastError() )
    };
  }
  bool Script::safeDoString(const char *o_code, const char *hint)
  {
    SLB_DEBUG_CALL;
    lua_State *L = getState();
    int top = lua_gettop(L);
    SLB_DEBUG(10, "code = %10s, hint = %s", o_code, hint);
    std::stringstream code;
    code << "--" << hint << std::endl << o_code;
    bool result = true;
    if(luaL_loadstring(L,code.str().c_str()) || _errorHandler->call(_lua_state, 0, 0))
    {
      const char *s = lua_tostring(L,-1);
      _lastError = lua_tostring(L,-1);
    }
    lua_settop(L,top);
    return result;
  }
  void Script::setErrorHandler( ErrorHandler *e )
  {
    Free_T(&_errorHandler);
    _errorHandler = e;
  }
  void *Script::allocator(void * , void *ptr, size_t osize, size_t nsize)
  {
    if (nsize == 0)
    {
      SLB::Free(ptr);
      return 0;
    }
    else
    {
      void *newpos = SLB::Malloc(nsize);
      size_t count = osize;
      if (nsize < osize) count = nsize;
      if (ptr) memcpy(newpos, ptr, count);
      SLB::Free(ptr);
      return newpos;
    }
  }
  int Script::PrintHook(lua_State *L) {
    Script *script = reinterpret_cast<Script*>(lua_touserdata(L,lua_upvalueindex(1)));
    int n = lua_gettop(L);
    int i;
    lua_getglobal(L, "tostring");
    for (i=1; i<=n; i++) {
      const char *s;
      size_t l;
      lua_pushvalue(L, -1);
      lua_pushvalue(L, i);
      lua_call(L, 1, 1);
      s = lua_tolstring(L, -1, &l);
      if (s == NULL)
        return luaL_error(L,
           LUA_QL("tostring") " must return a string to " LUA_QL("print"));
      if (i>1) script->_printCallback(script,"\t", 1);
      script->_printCallback(script,s, l);
      lua_pop(L, 1);
    }
    script->_printCallback(script,"\n",1);
    return 0;
  }
}
namespace SLB {
  Table::Table(const String &sep, bool c) : _cacheable(true), _separator(sep) {SLB_DEBUG_CALL;}
  Table::~Table() {SLB_DEBUG_CALL;}
  Object* Table::rawGet(const String &name)
  {
    SLB_DEBUG_CALL;
    Elements::iterator i = _elements.find(name);
    if (i == _elements.end())
    {
      SLB_DEBUG(10, "Access Table(%p) [%s] (FAILED!)", this, name.c_str());
      return 0;
    }
    SLB_DEBUG(10, "Access Table(%p) [%s] (OK!)", this, name.c_str());
    return i->second.get();
  }
  inline void Table::rawSet(const String &name, Object *obj)
  {
    SLB_DEBUG_CALL;
    if (obj == 0)
    {
      SLB_DEBUG(6, "Table (%p) remove '%s'", this, name.c_str());
      _elements.erase(name);
    }
    else
    {
      SLB_DEBUG(6, "Table (%p) [%s] = %p", this, name.c_str(), obj);
      _elements[name] = obj;
    }
  }
  Object* Table::get(const String &name)
  {
    SLB_DEBUG_CALL;
    TableFind t = getTable(name, false);
    if (t.first != 0) return t.first->rawGet(t.second);
    return 0;
  }
  void Table::erase(const String &name)
  {
    SLB_DEBUG_CALL;
    set(name, 0);
  }
  void Table::setCache(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_CLEAN_STACK(L,-2);
    SLB_DEBUG_STACK(8, L, "Table(%p) :: setCache BEGIN ", this);
    int top = lua_gettop(L);
    if (top < 2 ) luaL_error(L, "Not enough elements to perform Table::setCache");
    push(L);
    if (luaL_getmetafield(L,-1, "__indexCache"))
    {
      lua_insert(L, top - 1);
      lua_settop(L, top + 1);
      lua_rawset(L,-3);
    }
    else
    {
      luaL_error(L, "Invalid setCache;  %s:%d", __FILE__, __LINE__ );
    }
    SLB_DEBUG_STACK(8, L, "Table(%p) :: setCache END original top = %d", this, top);
    lua_settop(L, top - 2);
  }
  void Table::getCache(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_CLEAN_STACK(L,0);
    SLB_DEBUG_STACK(8, L, "Table(%p) :: getCache BEGIN ", this);
    int top = lua_gettop(L);
    if (top < 1 ) luaL_error(L, "Not enough elements to perform Table::getCache");
    push(L);
    if (!luaL_getmetafield(L,-1, "__indexCache"))
    {
      luaL_error(L, "Invalid setCache;  %s:%d", __FILE__, __LINE__ );
    }
    lua_pushvalue(L, top);
    lua_rawget(L, -2);
    SLB_DEBUG_STACK(8, L, "Table(%p) :: getCache END (result is at top) top was = %d", this, top);
    lua_replace(L, top);
    lua_settop(L, top);
  }
  void Table::set(const String &name, Object *obj)
  {
    SLB_DEBUG_CALL;
    TableFind t = getTable(name, true);
    t.first->rawSet(t.second, obj);
  }
  Table::TableFind Table::getTable(const String &key, bool create)
  {
    SLB_DEBUG_CALL;
    if (_separator.empty()) return TableFind(this,key);
    String::size_type pos = key.find(_separator);
    if (pos != String::npos)
    {
      const String &base = key.substr(0, pos);
      const String &next = key.substr(pos+_separator.length());
      Table* subtable = slb_dynamic_cast<Table>(rawGet(base));
      if (subtable == 0)
      {
        if (create)
        {
          SLB_DEBUG(6, "Table (%p) create Subtable %s -> %s", this,
            base.c_str(), next.c_str());
          subtable = new (Malloc(sizeof(Table))) Table(_separator, _cacheable);
          rawSet(base,subtable);
        }
        else
        {
          return TableFind((Table*)0,key);
        }
      }
      return subtable->getTable(next, create);
    }
    return TableFind(this,key);
  }
  int Table::__index(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_STACK(10,L,"Table::__index (%p)",this);
    int result = -1;
    {
      lua_pushvalue(L,2);
      lua_rawget(L, cacheTableIndex());
      if (lua_isnil(L,-1)) lua_pop(L,1);
      else
      {
        SLB_DEBUG(10, "Access Table(%p) (In CACHE)", this);
        result = 1;
      }
    }
    if (result < 0)
    {
      if (lua_type(L, 2) == LUA_TSTRING)
      {
        Object *obj = get(lua_tostring(L,2));
        if (obj)
        {
          result = 1;
          obj->push(L);
          if (_cacheable)
          {
            SLB_DEBUG(10, "L(%p) table(%p) key %s (->to cache)", L, this, lua_tostring(L,2));
            lua_pushvalue(L,2);
            lua_pushvalue(L,-2);
            lua_rawset(L, cacheTableIndex() );
          }
        }
      }
    }
    return result;
  }
  int Table::__newindex(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_STACK(10,L,"Table::__newindex (%p)",this);
    luaL_error(L, "(%p)__newindex metamethod not implemented", (void*)this);
    return 0;
  }
  int Table::__call(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_STACK(10,L,"Table::__call (%p)",this);
    luaL_error(L, "(%p)__call metamethod not implemented", (void*)this);
    return 0;
  }
  int Table::__garbageCollector(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_STACK(10,L,"Table::__GC (%p)",this);
    luaL_error(L, "(%p) __gc metamethod not implemented", (void*)this);
    return 0;
  }
  int Table::__tostring(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_STACK(10,L,"Table::__tostring (%p)",this);
    int top = lua_gettop(L);
    lua_pushfstring(L, "Table(%p) [%s] with keys:", this, typeInfo().name());
    for(Elements::iterator i = _elements.begin(); i != _elements.end(); ++i)
    {
      lua_pushfstring(L, "\n\t%s -> %p [%s]",i->first.c_str(), i->second.get(), i->second->typeInfo().name());
    }
    lua_concat(L, lua_gettop(L) - top);
    return 1;
  }
  int Table::__eq(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_STACK(10,L,"Table::__eq (%p)",this);
    luaL_error(L, "__eq metamethod called but no implementation was given");
    return 0;
  }
  void Table::pushImplementation(lua_State *L)
  {
    SLB_DEBUG_CALL;
    lua_newtable(L);
    lua_newtable(L);
    lua_pushvalue(L,-1);
    lua_setmetatable(L,-3);
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, "__indexCache");
    pushMeta(L, &Table::__indexProxy);
    lua_setfield(L,-3, "__index");
    pushMeta(L, &Table::__newindex);
    lua_setfield(L, -3, "__newindex");
    pushMeta(L, &Table::__tostring);
    lua_setfield(L, -3, "__tostring");
    pushMeta(L, &Table::__call);
    lua_setfield(L, -3, "__call");
    pushMeta(L, &Table::__garbageCollector);
    lua_setfield(L, -3, "__gc");
    pushMeta(L, &Table::__eq);
    lua_setfield(L, -3, "__eq");
    lua_pop(L,2);
  }
  int Table::__indexProxy(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(9, "---> __index search");
    SLB_DEBUG_STACK(10,L,"Table::__indexProxy (%p)",this);
    int result = __index(L);
    if (result < 0)
    {
      SLB_DEBUG(9, "Nothing found....");
      lua_pushnil(L);
      result = 1;
    }
    SLB_DEBUG(9, "<--- __index result = %d", result);
    assert(result == 1 && "Result must be 1 when it gets here");
    return result;
  }
  int Table::__meta(lua_State *L)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG_STACK(10,L,"Table::__meta (static method)");
    void *table_raw = lua_touserdata(L, lua_upvalueindex(2));
    void *table_member_raw = lua_touserdata(L, lua_upvalueindex(3));
    Table *table = reinterpret_cast<Table*>(table_raw);
    TableMember member = *reinterpret_cast<TableMember*>(table_member_raw);
    return (table->*member)(L);
  }
  void Table::pushMeta(lua_State *L, Table::TableMember member) const
  {
    assert("Invalid pushMeta, expected a table at the top (the cache-table)" &&
      lua_type(L,-1) == LUA_TTABLE);
    lua_pushvalue(L,-1);
    lua_pushlightuserdata(L, (void*) this);
    void *p = lua_newuserdata(L, sizeof(TableMember));
    memcpy(p,&member,sizeof(TableMember));
    lua_pushcclosure(L, __meta, 3 );
  }
}
