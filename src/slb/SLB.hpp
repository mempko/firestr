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

#include <set>

#ifndef __SLB_CONFIG__
#define __SLB_CONFIG__

// Enable to compile thread safety support
#ifndef SLB_THREAD_SAFE
  #define SLB_THREAD_SAFE 1
#endif

// Enable to support exceptions, otherwise asserts will be used
#ifndef SLB_USE_EXCEPTIONS
  #define SLB_USE_EXCEPTIONS 1
#endif

// To compile SLB as a dynamic library
// You also need to define SLB_LIBRARY macro when compiling SLB's cpp's
#ifndef SLB_DYNAMIC_LIBRARY
  #define SLB_DYNAMIC_LIBRARY 0
#endif

// use SLB's internal lua (latest lua version available)
// or use lua from your own project.
#ifndef SLB_EXTERNAL_LUA
  #define SLB_EXTERNAL_LUA 0
#endif


#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)  || defined( __MWERKS__)
  #define SLB_WINDOWS
#endif


// -- EXCEPTIONS -----------------------------------------------------------------
#if SLB_USE_EXCEPTIONS
  #define SLB_CRITICAL_ERROR(...) /*nothing*/
  #define SLB_THROW(...) throw __VA_ARGS__
#else
  #include <stdlib.h>
  #define SLB_CRITICAL_ERROR(msg) \
    {fprintf(stderr, "SLB Critical Error (%s:%d) -> %s", __FILE__, __LINE__, msg); \
    exit(129);}
  #define SLB_THROW(...) /*nothing*/
#endif // SLB_USE_EXCEPTIONS
// ------------------------------------------------------------------------------


// -- DEBUG -----------------------------------------------------------------
#ifndef SLB_DEBUG_OUTPUT
  #define SLB_DEBUG_OUTPUT stderr
#endif

// you can redefine the function to be used to debug, should have a 
// printf-like interface.
#ifndef SLB_DEBUG_FUNC
  #include <cstdio>
  #include <cstring>
  #define SLB_DEBUG_FUNC(...) fprintf(SLB_DEBUG_OUTPUT, __VA_ARGS__);
#endif

#ifndef SLB_DEBUG_LEVEL
  #define SLB_DEBUG_LEVEL 0
#endif
// --------------------------------------------------------------------------

// Specific MSC pragma's
#if defined(_MSC_VER)
#pragma warning( disable: 4251 )
#pragma warning( disable: 4290 )
#pragma warning( disable: 4127 ) // constant expressions
#endif

#endif



#ifndef __SIMPLE_PREPROCESSOR__
#define __SIMPLE_PREPROCESSOR__

  #define SPP_CONCATENATE_MACRO(x, y)   x##y
  #define SPP_CONCATENATE_MACRO3(x,y,z) SPP_CONCATENATE_MACRO(x##y,z)
  #define SPP_LINEID1(x, y)             SPP_CONCATENATE_MACRO(x, y)
  #define SPP_LINEID(x)                 SPP_LINEID1(x, __LINE__)
  #define SPP_STRING(X)                 #X
  #define SPP_TOSTRING(X)               SPP_STRING(X)

  #define SPP_STATIC_BLOCK(...) \
      namespace { \
    static struct SPP_LINEID(DUMMY) { SPP_LINEID(DUMMY)() {      \
      __VA_ARGS__ \
    }                                    \
    } SPP_LINEID(__dummy__); }

  //enumerates "X" from 1 to num with "INTER" as separator
  // SPP_ENUM(3,class T, = void SPP_COMMA) --> class T1 = void, class T2 = void, class T3 = void
  #define SPP_ENUM(num, X, INTER)                  SPP_ENUM_ ## num (X, SPP_UNPAR(INTER) )

  // enumerates using COMMA as separator 
  #define SPP_ENUM_D(num,X)                        SPP_ENUM_ ## num (X, SPP_COMMA)

  // Repeats "X" which should be a macro(N) num times, REPEAT_Z starts from 0
  // #define DO(N) T##N;
  // SPP_REPEAT(3,DO) --> DO(1) DO(2) DO(3) --> T1; T2; T3;
  #define SPP_REPEAT(num, X)                       SPP_REPEAT_ ## num (SPP_UNPAR(X))
  #define SPP_REPEAT_Z(num, X)                     X(0) SPP_REPEAT_ ## num (SPP_UNPAR(X))
  
  #define SPP_REPEAT_BASE(num, X, base)            SPP_REPEAT_BASE_ ## num \
                                                              (SPP_UNPAR(X),SPP_UNPAR(base))
  #define SPP_REPEAT_BASE_Z(num, X, base)          X(0,base) SPP_REPEAT_BASE_ ## num \
                                                              (SPP_UNPAR(X), SPP_UNPAR(base))

  #define SPP_IF(num,X)                            SPP_IF_ ## num (SPP_UNPAR(X))

  #define SPP_MAIN_REPEAT(num, X)                  SPP_MAIN_REPEAT_ ## num (SPP_UNPAR(X))
  #define SPP_MAIN_REPEAT_Z(num, X)                X(0) SPP_MAIN_REPEAT_ ## num (SPP_UNPAR(X))
  #define SPP_COMMA                                SPP_UNPAR(,)
  #define SPP_SPACE 
  #define SPP_COMMA_IF(num)                        SPP_IF(num,SPP_COMMA)

  #define SPP_UNPAR(...) __VA_ARGS__

  #define SPP_ENUM_0(X, INTER)  
  #define SPP_ENUM_1(X, INTER)  X##1
  #define SPP_ENUM_2(X, INTER)  SPP_ENUM_1(X,  SPP_UNPAR(INTER)) INTER X##2
  #define SPP_ENUM_3(X, INTER)  SPP_ENUM_2(X,  SPP_UNPAR(INTER)) INTER X##3
  #define SPP_ENUM_4(X, INTER)  SPP_ENUM_3(X,  SPP_UNPAR(INTER)) INTER X##4
  #define SPP_ENUM_5(X, INTER)  SPP_ENUM_4(X,  SPP_UNPAR(INTER)) INTER X##5
  #define SPP_ENUM_6(X, INTER)  SPP_ENUM_5(X,  SPP_UNPAR(INTER)) INTER X##6
  #define SPP_ENUM_7(X, INTER)  SPP_ENUM_6(X,  SPP_UNPAR(INTER)) INTER X##7
  #define SPP_ENUM_8(X, INTER)  SPP_ENUM_7(X,  SPP_UNPAR(INTER)) INTER X##8
  #define SPP_ENUM_9(X, INTER)  SPP_ENUM_8(X,  SPP_UNPAR(INTER)) INTER X##9
  #define SPP_ENUM_10(X, INTER) SPP_ENUM_9(X,  SPP_UNPAR(INTER)) INTER X##10
  #define SPP_ENUM_11(X, INTER) SPP_ENUM_10(X, SPP_UNPAR(INTER)) INTER X##11
  #define SPP_ENUM_12(X, INTER) SPP_ENUM_11(X, SPP_UNPAR(INTER)) INTER X##12
  #define SPP_ENUM_13(X, INTER) SPP_ENUM_12(X, SPP_UNPAR(INTER)) INTER X##13
  #define SPP_ENUM_14(X, INTER) SPP_ENUM_13(X, SPP_UNPAR(INTER)) INTER X##14
  #define SPP_ENUM_15(X, INTER) SPP_ENUM_14(X, SPP_UNPAR(INTER)) INTER X##15
  #define SPP_ENUM_16(X, INTER) SPP_ENUM_15(X, SPP_UNPAR(INTER)) INTER X##16
  #define SPP_ENUM_17(X, INTER) SPP_ENUM_16(X, SPP_UNPAR(INTER)) INTER X##17
  #define SPP_ENUM_18(X, INTER) SPP_ENUM_17(X, SPP_UNPAR(INTER)) INTER X##18
  #define SPP_ENUM_19(X, INTER) SPP_ENUM_18(X, SPP_UNPAR(INTER)) INTER X##19
  #define SPP_ENUM_20(X, INTER) SPP_ENUM_19(X, SPP_UNPAR(INTER)) INTER X##20
  #define SPP_ENUM_21(X, INTER) SPP_ENUM_20(X, SPP_UNPAR(INTER)) INTER X##21
  #define SPP_ENUM_22(X, INTER) SPP_ENUM_21(X, SPP_UNPAR(INTER)) INTER X##22
  #define SPP_ENUM_23(X, INTER) SPP_ENUM_22(X, SPP_UNPAR(INTER)) INTER X##23
  #define SPP_ENUM_24(X, INTER) SPP_ENUM_23(X, SPP_UNPAR(INTER)) INTER X##24
  #define SPP_ENUM_25(X, INTER) SPP_ENUM_24(X, SPP_UNPAR(INTER)) INTER X##25
  #define SPP_ENUM_26(X, INTER) SPP_ENUM_25(X, SPP_UNPAR(INTER)) INTER X##26
  #define SPP_ENUM_27(X, INTER) SPP_ENUM_26(X, SPP_UNPAR(INTER)) INTER X##27
  #define SPP_ENUM_28(X, INTER) SPP_ENUM_27(X, SPP_UNPAR(INTER)) INTER X##28
  #define SPP_ENUM_29(X, INTER) SPP_ENUM_28(X, SPP_UNPAR(INTER)) INTER X##29
  #define SPP_ENUM_30(X, INTER) SPP_ENUM_29(X, SPP_UNPAR(INTER)) INTER X##30
  #define SPP_ENUM_31(X, INTER) SPP_ENUM_30(X, SPP_UNPAR(INTER)) INTER X##31
  #define SPP_ENUM_32(X, INTER) SPP_ENUM_31(X, SPP_UNPAR(INTER)) INTER X##32
  #define SPP_ENUM_MAX(X, INTER) SPP_ENUM_10(X, INTER) /* Change this up to 32 */

  #define SPP_REPEAT_0(X) 
  #define SPP_REPEAT_1(X) X(1)
  #define SPP_REPEAT_2(X) SPP_REPEAT_1(X) X(2) 
  #define SPP_REPEAT_3(X) SPP_REPEAT_2(X) X(3) 
  #define SPP_REPEAT_4(X) SPP_REPEAT_3(X) X(4) 
  #define SPP_REPEAT_5(X) SPP_REPEAT_4(X) X(5) 
  #define SPP_REPEAT_6(X) SPP_REPEAT_5(X) X(6) 
  #define SPP_REPEAT_7(X) SPP_REPEAT_6(X) X(7) 
  #define SPP_REPEAT_8(X) SPP_REPEAT_7(X) X(8) 
  #define SPP_REPEAT_9(X) SPP_REPEAT_8(X) X(9) 
  #define SPP_REPEAT_10(X) SPP_REPEAT_9(X) X(10) 
  #define SPP_REPEAT_11(X) SPP_REPEAT_10(X) X(11) 
  #define SPP_REPEAT_12(X) SPP_REPEAT_11(X) X(12) 
  #define SPP_REPEAT_13(X) SPP_REPEAT_12(X) X(13) 
  #define SPP_REPEAT_14(X) SPP_REPEAT_13(X) X(14) 
  #define SPP_REPEAT_15(X) SPP_REPEAT_14(X) X(15) 
  #define SPP_REPEAT_16(X) SPP_REPEAT_15(X) X(16) 
  #define SPP_REPEAT_17(X) SPP_REPEAT_16(X) X(17) 
  #define SPP_REPEAT_18(X) SPP_REPEAT_17(X) X(18) 
  #define SPP_REPEAT_19(X) SPP_REPEAT_18(X) X(19) 
  #define SPP_REPEAT_20(X) SPP_REPEAT_19(X) X(20) 
  #define SPP_REPEAT_21(X) SPP_REPEAT_20(X) X(21) 
  #define SPP_REPEAT_22(X) SPP_REPEAT_21(X) X(22) 
  #define SPP_REPEAT_23(X) SPP_REPEAT_22(X) X(23) 
  #define SPP_REPEAT_24(X) SPP_REPEAT_23(X) X(24) 
  #define SPP_REPEAT_25(X) SPP_REPEAT_24(X) X(25) 
  #define SPP_REPEAT_26(X) SPP_REPEAT_25(X) X(26) 
  #define SPP_REPEAT_27(X) SPP_REPEAT_26(X) X(27) 
  #define SPP_REPEAT_28(X) SPP_REPEAT_27(X) X(28) 
  #define SPP_REPEAT_29(X) SPP_REPEAT_28(X) X(29) 
  #define SPP_REPEAT_30(X) SPP_REPEAT_29(X) X(30) 
  #define SPP_REPEAT_31(X) SPP_REPEAT_30(X) X(31) 
  #define SPP_REPEAT_32(X) SPP_REPEAT_31(X) X(32) 
  #define SPP_REPEAT_MAX(X) SPP_REPEAT_10(X)  /* Change this up to 32 */

  #define SPP_REPEAT_BASE_0(X,base) 
  #define SPP_REPEAT_BASE_1(X,base) X(1, base)
  #define SPP_REPEAT_BASE_2(X,base) SPP_REPEAT_BASE_1(X,base) X(2, base) 
  #define SPP_REPEAT_BASE_3(X,base) SPP_REPEAT_BASE_2(X,base) X(3, base) 
  #define SPP_REPEAT_BASE_4(X,base) SPP_REPEAT_BASE_3(X,base) X(4, base) 
  #define SPP_REPEAT_BASE_5(X,base) SPP_REPEAT_BASE_4(X,base) X(5, base) 
  #define SPP_REPEAT_BASE_6(X,base) SPP_REPEAT_BASE_5(X,base) X(6, base) 
  #define SPP_REPEAT_BASE_7(X,base) SPP_REPEAT_BASE_6(X,base) X(7, base) 
  #define SPP_REPEAT_BASE_8(X,base) SPP_REPEAT_BASE_7(X,base) X(8, base) 
  #define SPP_REPEAT_BASE_9(X,base) SPP_REPEAT_BASE_8(X,base) X(9, base) 
  #define SPP_REPEAT_BASE_10(X,base) SPP_REPEAT_BASE_9(X,base) X(10, base) 
  #define SPP_REPEAT_BASE_11(X,base) SPP_REPEAT_BASE_10(X,base) X(11, base) 
  #define SPP_REPEAT_BASE_12(X,base) SPP_REPEAT_BASE_11(X,base) X(12, base) 
  #define SPP_REPEAT_BASE_13(X,base) SPP_REPEAT_BASE_12(X,base) X(13, base) 
  #define SPP_REPEAT_BASE_14(X,base) SPP_REPEAT_BASE_13(X,base) X(14, base) 
  #define SPP_REPEAT_BASE_15(X,base) SPP_REPEAT_BASE_14(X,base) X(15, base) 
  #define SPP_REPEAT_BASE_16(X,base) SPP_REPEAT_BASE_15(X,base) X(16, base) 
  #define SPP_REPEAT_BASE_17(X,base) SPP_REPEAT_BASE_16(X,base) X(17, base) 
  #define SPP_REPEAT_BASE_18(X,base) SPP_REPEAT_BASE_17(X,base) X(18, base) 
  #define SPP_REPEAT_BASE_19(X,base) SPP_REPEAT_BASE_18(X,base) X(19, base) 
  #define SPP_REPEAT_BASE_20(X,base) SPP_REPEAT_BASE_19(X,base) X(20, base) 
  #define SPP_REPEAT_BASE_21(X,base) SPP_REPEAT_BASE_20(X,base) X(21, base) 
  #define SPP_REPEAT_BASE_22(X,base) SPP_REPEAT_BASE_21(X,base) X(22, base) 
  #define SPP_REPEAT_BASE_23(X,base) SPP_REPEAT_BASE_22(X,base) X(23, base) 
  #define SPP_REPEAT_BASE_24(X,base) SPP_REPEAT_BASE_23(X,base) X(24, base) 
  #define SPP_REPEAT_BASE_25(X,base) SPP_REPEAT_BASE_24(X,base) X(25, base) 
  #define SPP_REPEAT_BASE_26(X,base) SPP_REPEAT_BASE_25(X,base) X(26, base) 
  #define SPP_REPEAT_BASE_27(X,base) SPP_REPEAT_BASE_26(X,base) X(27, base) 
  #define SPP_REPEAT_BASE_28(X,base) SPP_REPEAT_BASE_27(X,base) X(28, base) 
  #define SPP_REPEAT_BASE_29(X,base) SPP_REPEAT_BASE_28(X,base) X(29, base) 
  #define SPP_REPEAT_BASE_30(X,base) SPP_REPEAT_BASE_29(X,base) X(30, base) 
  #define SPP_REPEAT_BASE_31(X,base) SPP_REPEAT_BASE_30(X,base) X(31, base) 
  #define SPP_REPEAT_BASE_32(X,base) SPP_REPEAT_BASE_31(X,base) X(32, base) 
  #define SPP_REPEAT_BASE_MAX(X,base) SPP_REPEAT_BASE_10(X,base)  /* Change this up to 32 */

  #define SPP_MAIN_REPEAT_1(X) X(1)
  #define SPP_MAIN_REPEAT_2(X) SPP_MAIN_REPEAT_1(X) X(2) 
  #define SPP_MAIN_REPEAT_3(X) SPP_MAIN_REPEAT_2(X) X(3) 
  #define SPP_MAIN_REPEAT_4(X) SPP_MAIN_REPEAT_3(X) X(4) 
  #define SPP_MAIN_REPEAT_5(X) SPP_MAIN_REPEAT_4(X) X(5) 
  #define SPP_MAIN_REPEAT_6(X) SPP_MAIN_REPEAT_5(X) X(6) 
  #define SPP_MAIN_REPEAT_7(X) SPP_MAIN_REPEAT_6(X) X(7) 
  #define SPP_MAIN_REPEAT_8(X) SPP_MAIN_REPEAT_7(X) X(8) 
  #define SPP_MAIN_REPEAT_9(X) SPP_MAIN_REPEAT_8(X) X(9) 
  #define SPP_MAIN_REPEAT_10(X) SPP_MAIN_REPEAT_9(X) X(10) 
  #define SPP_MAIN_REPEAT_11(X) SPP_MAIN_REPEAT_10(X) X(11) 
  #define SPP_MAIN_REPEAT_12(X) SPP_MAIN_REPEAT_11(X) X(12) 
  #define SPP_MAIN_REPEAT_13(X) SPP_MAIN_REPEAT_12(X) X(13) 
  #define SPP_MAIN_REPEAT_14(X) SPP_MAIN_REPEAT_13(X) X(14) 
  #define SPP_MAIN_REPEAT_15(X) SPP_MAIN_REPEAT_14(X) X(15) 
  #define SPP_MAIN_REPEAT_16(X) SPP_MAIN_REPEAT_15(X) X(16) 
  #define SPP_MAIN_REPEAT_17(X) SPP_MAIN_REPEAT_16(X) X(17) 
  #define SPP_MAIN_REPEAT_18(X) SPP_MAIN_REPEAT_17(X) X(18) 
  #define SPP_MAIN_REPEAT_19(X) SPP_MAIN_REPEAT_18(X) X(19) 
  #define SPP_MAIN_REPEAT_20(X) SPP_MAIN_REPEAT_19(X) X(20) 
  #define SPP_MAIN_REPEAT_21(X) SPP_MAIN_REPEAT_20(X) X(21) 
  #define SPP_MAIN_REPEAT_22(X) SPP_MAIN_REPEAT_21(X) X(22) 
  #define SPP_MAIN_REPEAT_23(X) SPP_MAIN_REPEAT_22(X) X(23) 
  #define SPP_MAIN_REPEAT_24(X) SPP_MAIN_REPEAT_23(X) X(24) 
  #define SPP_MAIN_REPEAT_25(X) SPP_MAIN_REPEAT_24(X) X(25) 
  #define SPP_MAIN_REPEAT_26(X) SPP_MAIN_REPEAT_25(X) X(26) 
  #define SPP_MAIN_REPEAT_27(X) SPP_MAIN_REPEAT_26(X) X(27) 
  #define SPP_MAIN_REPEAT_28(X) SPP_MAIN_REPEAT_27(X) X(28) 
  #define SPP_MAIN_REPEAT_29(X) SPP_MAIN_REPEAT_28(X) X(29) 
  #define SPP_MAIN_REPEAT_30(X) SPP_MAIN_REPEAT_29(X) X(30) 
  #define SPP_MAIN_REPEAT_31(X) SPP_MAIN_REPEAT_30(X) X(31) 
  #define SPP_MAIN_REPEAT_32(X) SPP_MAIN_REPEAT_31(X) X(32) 
  #define SPP_MAIN_REPEAT_MAX(X) SPP_MAIN_REPEAT_10(X)  /* Change this up to 32 */

  #define SPP_IF_0(X) 
  #define SPP_IF_1(X) X
  #define SPP_IF_2(X) X
  #define SPP_IF_3(X) X
  #define SPP_IF_4(X) X
  #define SPP_IF_5(X) X
  #define SPP_IF_6(X) X
  #define SPP_IF_7(X) X
  #define SPP_IF_8(X) X
  #define SPP_IF_9(X) X
  #define SPP_IF_10(X) X
  #define SPP_IF_11(X) X
  #define SPP_IF_12(X) X
  #define SPP_IF_13(X) X
  #define SPP_IF_14(X) X
  #define SPP_IF_15(X) X
  #define SPP_IF_16(X) X
  #define SPP_IF_17(X) X
  #define SPP_IF_18(X) X
  #define SPP_IF_19(X) X
  #define SPP_IF_20(X) X
  #define SPP_IF_21(X) X
  #define SPP_IF_22(X) X
  #define SPP_IF_23(X) X
  #define SPP_IF_24(X) X
  #define SPP_IF_25(X) X
  #define SPP_IF_26(X) X
  #define SPP_IF_27(X) X
  #define SPP_IF_28(X) X
  #define SPP_IF_29(X) X
  #define SPP_IF_30(X) X
  #define SPP_IF_31(X) X
  #define SPP_IF_32(X) X
  #define SPP_IF_MAX(X) SPP_IF_10(X) /* Change this up to 32 */

#endif

//->#include "Config.hpp"

#ifndef __SLB_EXPORT__
#define __SLB_EXPORT__

#if defined(SLB_WINDOWS)
  #if SLB_DYNAMIC_LIBRARY
    #if defined(SLB_LIBRARY)
      #define SLB_EXPORT   __declspec(dllexport)
    #else
      #define SLB_EXPORT   __declspec(dllimport)
    #endif /* SLB_LIBRARY */
  #else
    #define SLB_EXPORT
  #endif
#else /* not windows */
  #  define SLB_EXPORT
#endif  

#endif


//->#include "Config.hpp"

#ifndef __SLB_DEBUG__
#define __SLB_DEBUG__

  
#if SLB_DEBUG_LEVEL != 0
  //->#include "SPP.hpp"
  //->#include "Export.hpp"

  extern SLB_EXPORT int SLB_DEBUG_LEVEL_TAB;

  inline void __SLB_ADJUST__(bool terminator = true)
  {
    if (SLB_DEBUG_LEVEL_TAB)
    {
      for(int i = 0; i < SLB_DEBUG_LEVEL_TAB-1; ++i) SLB_DEBUG_FUNC("| ");
      if (terminator) SLB_DEBUG_FUNC("|_");
    }
  }

  #define SLB_DEBUG(level,...) if (level <= SLB_DEBUG_LEVEL)\
    {\
      __dummy__SLB__debugcall.check_SLB_DEBUG_CALL(); /* to check a previous SLB_DEBUG_CALL */ \
      __SLB_ADJUST__();\
      SLB_DEBUG_FUNC("SLB-%-2d ", level);\
      SLB_DEBUG_FUNC(__VA_ARGS__);\
      SLB_DEBUG_FUNC(" [%s:%d]\n",__FILE__, __LINE__);\
    }

  #define SLB_DEBUG_STACK(level, L,  ... ) \
    {\
      SLB_DEBUG(level, " {stack} "  __VA_ARGS__ );\
      int top = lua_gettop(L); \
      for(int i = 1; i <= top; i++) \
      { \
        if (lua_type(L,i) == LUA_TNONE) \
        { \
          SLB_DEBUG(level, "\targ %d = (Invalid)", i);\
        } \
        else \
        { \
          lua_pushvalue(L,i);\
          SLB_DEBUG(level+1, "\targ %d = %s -> %s", i, \
            lua_typename(L,lua_type(L,-1)), lua_tostring(L,-1) );\
          lua_pop(L,1);\
        } \
      }\
    }

    #include<sstream>
    #include<stdexcept>
    //->#include "lua.hpp"
  
    struct __SLB__cleanstack
    {
      __SLB__cleanstack(struct lua_State *L, int delta, const char *where, int line)
        : L(L), delta(delta), where(where), line(line)
      {
        top = lua_gettop(L);
      }
      ~__SLB__cleanstack()
      {
        if (top+delta != lua_gettop(L))
        {
          std::ostringstream out;
          out << where << ":" << line << " -> ";
          out << "Invalid Stack Check. current = " << lua_gettop(L) << " expected = " << top + delta << std::endl;
          SLB_THROW(std::runtime_error(out.str()));
          SLB_CRITICAL_ERROR(out.str().c_str());
        }
      }

      struct lua_State *L;
      int top;
      int delta;
      const char *where;
      int line;
    };

  #define SLB_DEBUG_CLEAN_STACK(Lua_Stack_L, delta)  \
    __SLB__cleanstack __dummy__SLB__cleanstack__(Lua_Stack_L, delta, __FILE__, __LINE__);

    struct __SLB__debugcall
    {
      __SLB__debugcall(const char *f, int l, const char *n)
        : file(f), line(l), name(n)
      {
        __SLB_ADJUST__();
        SLB_DEBUG_FUNC("SLB >>> %s [%s:%d]\n", name, file, line);
        SLB_DEBUG_LEVEL_TAB++;
      }

      ~__SLB__debugcall()
      {
        __SLB_ADJUST__();
        SLB_DEBUG_FUNC("SLB <<< %s [%s:%d]\n", name, file, line);
        __SLB_ADJUST__(false);
        SLB_DEBUG_FUNC("\n");
        SLB_DEBUG_LEVEL_TAB--;
      }

      void check_SLB_DEBUG_CALL() const {}


      const char* file;
      int line;
      const char* name;

    };
  #define SLB_DEBUG_CALL \
    __SLB__debugcall __dummy__SLB__debugcall(__FILE__,__LINE__,__FUNCTION__);

#else
  #define SLB_DEBUG(level,...)
  #define SLB_DEBUG_STACK(... ) 
  #define SLB_DEBUG_CLEAN_STACK(...)
  #define SLB_DEBUG_CALL
#endif

#endif



#ifndef __SLB_LUA__
#define __SLB_LUA__

//->#include "Export.hpp"

extern "C" {
#if SLB_EXTERNAL_LUA
  #include <lua.h>
  #include <lauxlib.h>
  #include <lualib.h>
#else
  #if defined(POWERUP_WINDOWS)
    #if SLB_DYNAMIC_LIBRARY
      #define LUA_BUILD_AS_DLL
      #ifdef SLB_LIBRARY
        #define LUA_LIB
        #define LUA_CORE
      #endif
    #endif // SLB_DYNAMIC_LIBRARY
  #endif // on windows...

  // Local (static) lua (v 5.2.0)
  #include "lua/lua.h"
  #include "lua/lauxlib.h"
  #include "lua/lualib.h"

#endif
}

#endif

/*
   Based on Steven Lavavej's "Mallocator"
   http://blogs.msdn.com/b/vcblog/archive/2008/08/28/the-mallocator.aspx

   This is a stateless global allocator that redirects all memory 
   allocation to user supplied "malloc" and "free" functions.
   The default implementation actually uses the global malloc and free
   functions.  The user supplied functions must be registered before
   using any SLB component.
*/
#ifndef __SLB_ALLOCATOR__
#define __SLB_ALLOCATOR__

#include <stddef.h>  // Required for size_t and ptrdiff_t
#include <new>       // Required for placement new
#include <stdexcept> // Required for std::length_error

//->#include "Export.hpp"

// To define Maps using our allocator
#define SLB_Map(Key,T) std::map<Key, T, std::less<Key>, Allocator< std::pair<const Key, T> > >

namespace SLB
{
  typedef void* (*MallocFn)(size_t);
  typedef void (*FreeFn)(void*);

  SLB_EXPORT void SetMemoryManagement(MallocFn, FreeFn);

  SLB_EXPORT void* Malloc(size_t);
  SLB_EXPORT void Free(void*);

  //Generic stateless allocator that uses the SLB::Malloc and 
  // SLB::Free functions for memory management
  template <typename T>
  class Allocator
  {
  public:
    typedef T * pointer;
    typedef const T * const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    /*
    T * address(T& r) const 
    {
      return &r;
    }
    */

    const T * address(const T& s) const 
    {
      return &s;
    }


    size_t max_size() const 
    {
      // The following has been carefully written to be independent of
      // the definition of size_t and to avoid signed/unsigned warnings.
      return (static_cast<size_t>(0) - static_cast<size_t>(1)) / sizeof(T);
    }

    template <typename U> struct rebind 
    {
      typedef Allocator<U> other;
    };

    bool operator!=(const Allocator& other) const
    {
      return !(*this == other);
    }

    void construct(T * const p, const T& t) const 
    {
      void * const pv = static_cast<void *>(p);
      new (pv) T(t);
    }

    #if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4100) // unreferenced formal parameter
    #endif
    void destroy(T * const p) const
    {
      p->~T();
    }
    #if defined(_MSC_VER)
    #pragma warning(pop)
    #endif

    bool operator==(const Allocator& /*other*/) const 
    {
      return true;
    }

    Allocator() { }
    Allocator(const Allocator&) { }
    template <typename U>
    Allocator(const Allocator<U>&) { }
    ~Allocator() { }

    T * allocate(const size_t n) const
    {
      if (n == 0)
      {
        return NULL;
      }

      if (n > max_size())
      {
        SLB_THROW(std::length_error("Allocator<T>::allocate() - Integer overflow."));
        SLB_CRITICAL_ERROR("Allocator<T>::allocate() - Integer overflow.");
      }

      void * const pv = Malloc(n * sizeof(T));
      if (pv == NULL) 
      {
        SLB_THROW(std::bad_alloc());
        SLB_CRITICAL_ERROR("Out of memory");
      }

      return static_cast<T *>(pv);
    }


    void deallocate(T * const p, const size_t /* n */) const 
    {
      Free(p);
    }

    template <typename U> T * allocate(const size_t n, const U * /* const hint */) const 
    {
      return allocate(n);
    }

  private:
    Allocator& operator=(const Allocator&);
  };

  
  template <typename T>
  void Free_T(T** ptr)
  {
    if (ptr && *ptr)
    {
      /*#ifdef _MSC_VER
      #pragma warning(push)
      #pragma warning(disable: 4100) // unreferenced formal parameter
      #endif*/
      (*ptr)->~T();
      /*#ifdef _MSC_VER
      #pragma warning(pop)
      #endif*/
      SLB::Free(*ptr);

      // To clearly identify deleted pointers:
      *ptr = 0L;
    }
  }

  template<typename T>
  void New_T(T **ptr)
  {
    if (ptr) *ptr = new (SLB::Malloc(sizeof(T))) T();
  }

}
#endif

#ifndef __SLB_STRING__
#define __SLB_STRING__

//->#include "Allocator.hpp"
//->#include "Export.hpp"
#include <string>


namespace SLB
{
#if 1
  typedef std::basic_string< char, std::char_traits<char>, Allocator<char> > String;
#else
  class SLB_EXPORT String : public std::basic_string< char, std::char_traits<char>, Allocator<char> > 
  {
  public:
    typedef std::basic_string< char, std::char_traits<char>, Allocator<char> > Base;
    String() {}
    String(const String& s) : Base(s.c_str()) {}
    String(const char* s) : Base(s) {}
    String(const Base& b) : Base(b) {}
    String(const std::string& s) : Base(s.c_str()) {}

    String& operator=(const String& s) { if (&s != this) { Base::operator=(s.c_str()); } return *this; }
    String& operator=(const char* c) { Base::operator=(c); return *this; }

    bool operator==(const char *c)   const { return ( (*(Base*)this) == c); }
    bool operator==(const String& s) const { return ( (*(Base*)this) == s); }
    bool operator<(const char *c)    const { return ( (*(Base*)this) < c);  }
    bool operator<(const String &s)  const { return ( (*(Base*)this) < s);  }
  };
#endif

}

#endif



#ifndef __SLB_TYPEINFO_WRAPPER__
#define __SLB_TYPEINFO_WRAPPER__

#include <typeinfo>

namespace SLB {

  template<class T>
  class TypeID
  {
  public:
    static const char *Name();
    static unsigned long Hash();
  private:
    TypeID();
    static unsigned long hash_;
  };

  template<class T>
  unsigned long TypeID<T>::hash_ = 0;

  template<class T>
  const char *TypeID<T>::Name()
  {
    #if defined(_MSC_VER)
      return __FUNCTION__ ;
    #elif defined(__GNUC__) or defined(__SNC__)
      return __PRETTY_FUNCTION__;
    #else
      return __func__
    #endif
  }
  
  template<class T>
  inline unsigned long TypeID<T>::Hash()
  {
    if (hash_ == 0)
    {
      const char *name = Name();
      static const unsigned long hbits = 31;
      static const unsigned long hprime = (unsigned long) 16777619;
      static const unsigned long hmod = (unsigned long) 1 << hbits;
      while (*name != '\0')
      {
        hash_ = (hash_*hprime) % hmod;
        hash_ = hash_ ^ ((unsigned long) *name);
        name++;
      }
    }
    return hash_;
  }
  

  class TypeInfoWrapper
  {
  public:

    TypeInfoWrapper() :
      _ID(0), _name("Invalid")
    {
    }

    TypeInfoWrapper(unsigned long hash, const char *name) :
      _ID(hash), _name(name)
    {
    
    }

    unsigned long type() const { return _ID; }

    const char *name() const { return _name; }

    bool operator<(const TypeInfoWrapper &o) const
    {
      return _ID < o._ID;
    }

    bool operator==(const TypeInfoWrapper &o) const
    {
      return _ID == o._ID;
    }

    bool operator!=(const TypeInfoWrapper &o) const
    {
      return _ID != o._ID;
    }
    
  private:
    unsigned long _ID;
    const char *_name;
  };

#define _TIW(x) ::SLB::TypeInfoWrapper(::SLB::TypeID<x>::Hash(), ::SLB::TypeID<x>::Name())

} // end of SLB Namespace

#endif



#ifndef __SLB_OBJECT__
#define __SLB_OBJECT__

#include <assert.h>
//->#include "Export.hpp"
//->#include "Allocator.hpp"
//->#include "String.hpp"
//->#include "TypeInfoWrapper.hpp"

struct lua_State;

#define SLB_CLASS(T, Parent_T) \
  public:\
  SLB::TypeInfoWrapper typeInfo() const { return _TIW(T); } \
  const void * convertTo(const TypeInfoWrapper &tiw) const {\
    if (tiw == _TIW(T)) return this; \
    else return Parent_T::convertTo(tiw); \
  }\
  const void *memoryRawPointer() const { return this; } \

namespace SLB
{

  class SLB_EXPORT Object 
  {
  public:
    unsigned int referenceCount() const { return _refCounter; }
    void ref();
    void unref();

    void push(lua_State *L);
    void setInfo(const String&);
    const String& getInfo() const;

    virtual TypeInfoWrapper typeInfo() const = 0;
    virtual const void* convertTo(const TypeInfoWrapper &) const { return 0L; }
    virtual const void *memoryRawPointer() const = 0;

  protected:
    Object();
    virtual ~Object();

    virtual void pushImplementation(lua_State *) = 0;
    virtual void onGarbageCollection(lua_State *) {}

  private:
    void initialize(lua_State *) const;
    static int GC_callback(lua_State *);
    unsigned int _refCounter;
    String _info; // for metadata, documentation, ...

    
    Object( const Object &slbo);
    Object& operator=( const Object &slbo);
  };

  template<class T, class X>
  inline T* slb_dynamic_cast(X *obj) {
    T* result = 0L;
    if (obj)  { result = static_cast<T*>(const_cast<void*>(obj->convertTo(_TIW(T)))); }
#if SLB_USE_EXCEPTIONS != 0 // we asume exceptions means you can also use dynamic_cast
    // check the result is the same that dynamic_cast
    assert(result == dynamic_cast<T*>(obj) && "Invalid cast");
#endif
    return result;
  }

  template<class T, class X>
  inline const T* slb_dynamic_cast(const X *obj) {
    const T* result = 0L;
    if (obj)  { result = static_cast<const T*>(obj->convertTo(_TIW(T))); }
#if SLB_USE_EXCEPTIONS != 0 // we asume exceptions means you can also use dynamic_cast
    // check the result is the same that dynamic_cast
    assert(result == dynamic_cast<T*>(obj) && "Invalid cast");
#endif
    return result;
  }

  
  // ------------------------------------------------------------
  // ------------------------------------------------------------
  // ------------------------------------------------------------
    
  inline void Object::ref()
  {
    ++_refCounter;
  }

  inline void Object::unref()
  {
    assert(_refCounter > 0);
    --_refCounter; 
    if (_refCounter == 0) 
    {
      const void *ptr = memoryRawPointer();
      this->~Object();
      Free( const_cast<void*>(ptr) );
    }
  }

  inline void Object::setInfo(const String& s) {_info = s;}
  inline const String& Object::getInfo() const {return _info;}

} //end of SLB namespace

#endif



#ifndef __SLB_PUSH_GET__
#define __SLB_PUSH_GET__

//->#include "lua.hpp"
//->#include "Debug.hpp"

namespace SLB {

//----------------------------------------------------------------------------
//-- Push/Get/Check functions ------------------------------------------------
//----------------------------------------------------------------------------
  template<class T> void push(lua_State *L, T v);
  template<class T> T get(lua_State *L, int pos);
  template<class T> void setGlobal(lua_State *L, T v, const char *name);
  template<class T> T getGlobal(lua_State *L, const char *name);
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Private {
  template<class C>
  struct Type;
}

template<class T>
inline void push(lua_State *L, T v)
{
  SLB_DEBUG_CALL; 
  Private::Type<T>::push(L,v);
}

// get function based on Private::Type<class>
template<class T>
inline T get(lua_State *L, int pos)
{
  SLB_DEBUG_CALL; 
  return Private::Type<T>::get(L,pos);
}

template<class T> 
inline void setGlobal(lua_State *L, T v, const char *name)
{
  SLB_DEBUG_CALL; 
  SLB::push(L,v);
  lua_setglobal(L,name);
}

template<class T>
inline T getGlobal(lua_State *L, const char *name)
{
  SLB_DEBUG_CALL; 
  lua_getglobal(L,name);
  T value = SLB::get<T>(L, -1);
  lua_pop(L,1); // remove the value
  return value;
}

} // end of SLB namespace

#endif



#ifndef __SLB_VALUE__
#define __SLB_VALUE__

//->#include "Object.hpp"
//->#include "PushGet.hpp"
//->#include "Allocator.hpp"

struct lua_State;

namespace SLB {

  template<class T> class RefValue;
  template<class T> class AutoDeleteValue;
  template<class T> class CopyValue;

  namespace Value
  {
    // holds a reference to an object
    template<class T> inline Object* ref( T& obj )
    { return new (Malloc(sizeof(RefValue<T>))) RefValue<T>(obj); }

    // holds a reference to a const object
    template<class T> inline Object* ref( const T& obj )
    { return new (Malloc(sizeof(RefValue<T>))) RefValue<T>(obj); }
    
    // holds a reference to an object (ptr)
    template<class T> inline Object* ref( T *obj )
    { return new (Malloc(sizeof(RefValue<T>))) RefValue<T>(obj); }
    
    // holds a reference to a const object (const ptr)
    template<class T> inline Object* ref( const T *obj )
    { return new (Malloc(sizeof(RefValue<T>))) RefValue<T>(obj); }

    // creates a copy of the given object
    template<class T> inline Object* copy( const T &obj)
    { return new (Malloc(sizeof(CopyValue<T>))) CopyValue<T>(obj); }

    // holds a ptr to an object, the object will be automatically
    // deleted.
    template<class T> inline Object* autoDelete( T *obj )
    { return  new (Malloc(sizeof(AutoDeleteValue<T>))) AutoDeleteValue<T>(obj); }
  }

  template<class T>
  class RefValue: public Object {
  public:
    RefValue( T& obj );
    RefValue( const T& obj );
    RefValue( T *obj );
    RefValue( const T *obj );


  protected:
    void pushImplementation(lua_State *L);
    virtual ~RefValue() {}
  private:
    union {
      T* _obj;
      const T* _constObj;
    };
    bool _isConst;
    SLB_CLASS(RefValue<T>, Object)
  };

  template<class T>
  class CopyValue : public Object {
  public:
    CopyValue( const T& obj );
  protected:
    virtual ~CopyValue() {}
    void pushImplementation(lua_State *L);
  private:
    T _obj;
    SLB_CLASS(RefValue<T>, Object)
  };

  template<class T>
  class AutoDeleteValue : public Object {
  public:
    AutoDeleteValue( T *obj );
  protected:
    virtual ~AutoDeleteValue();
    void pushImplementation(lua_State *L);
  private:
    T *_obj;
    SLB_CLASS(AutoDeleteValue<T>, Object)
  };

  //--------------------------------------------------------------------
  // Inline implementations:
  //--------------------------------------------------------------------
  
  template<class T>
  inline RefValue<T>::RefValue( T& obj ) : 
    _obj(&obj), _isConst(false)
  {
  }

  template<class T>
  inline RefValue<T>::RefValue( const T& obj ) : 
    _constObj(&obj), _isConst(true)
  {
  }

  template<class T>
  inline RefValue<T>::RefValue( T *obj ) : 
    _obj(obj), _isConst(false)
  {
  }

  template<class T>
  inline RefValue<T>::RefValue( const T *obj ) : 
    _constObj(obj), _isConst(true)
  {
  }
  
  template<class T>
  inline void RefValue<T>::pushImplementation(lua_State *L)
  {
    if (_isConst) SLB::push(L, _constObj);  
    else SLB::push(L, _obj);  
  }

  template<class T>
  inline CopyValue<T>::CopyValue( const T& obj ) : _obj(obj)
  {
  }

  template<class T>
  inline void CopyValue<T>::pushImplementation(lua_State *L)
  {
    SLB::push(L,_obj);
  }

  template<class T>
  inline AutoDeleteValue<T>::AutoDeleteValue( T *obj ) : _obj(obj)
  {
  }
  
  template<class T>
  inline void AutoDeleteValue<T>::pushImplementation(lua_State *L)
  {
    // do not call GC on this object.
    SLB::Private::Type<T*>::push(L, _obj, false);
  }

  template<class T>
  inline AutoDeleteValue<T>::~AutoDeleteValue()
  {
    Free_T(&_obj);
  }
}


#endif



#ifndef __SLB_REF_PTR__
#define __SLB_REF_PTR__

namespace SLB {

  template<class T>
  class ref_ptr
  {
  public:
        typedef T element_type;

        ref_ptr() :_ptr(0L) {}
        ref_ptr(T* t):_ptr(t) { if (_ptr) _ptr->ref(); }
        ref_ptr(const ref_ptr& rp):_ptr(rp._ptr)  { if (_ptr) _ptr->ref(); }
        ~ref_ptr() { if (_ptr) _ptr->unref(); _ptr=0L; }

        inline ref_ptr& operator = (const ref_ptr& rp)
        {
            if (_ptr==rp._ptr) return *this;
            T* tmp_ptr = _ptr;
            _ptr = rp._ptr;
            if (_ptr) _ptr->ref();
            if (tmp_ptr) tmp_ptr->unref();
            return *this;
        }

        inline ref_ptr& operator = (T* ptr)
        {
            if (_ptr==ptr) return *this;
            T* tmp_ptr = _ptr;
            _ptr = ptr;
            if (_ptr) _ptr->ref();
            if (tmp_ptr) tmp_ptr->unref();
            return *this;
        }

        // comparison operators for ref_ptr.
        inline bool operator == (const ref_ptr& rp) const { return (_ptr==rp._ptr); }
        inline bool operator != (const ref_ptr& rp) const { return (_ptr!=rp._ptr); }
        inline bool operator < (const ref_ptr& rp) const { return (_ptr<rp._ptr); }
        inline bool operator > (const ref_ptr& rp) const { return (_ptr>rp._ptr); }

        // comparison operator for const T*.
        inline bool operator == (const T* ptr) const { return (_ptr==ptr); }
        inline bool operator != (const T* ptr) const { return (_ptr!=ptr); }
        inline bool operator < (const T* ptr) const { return (_ptr<ptr); }
        inline bool operator > (const T* ptr) const { return (_ptr>ptr); }

        inline T& operator*() { return *_ptr; }
        inline const T& operator*() const { return *_ptr; }
        inline T* operator->() { return _ptr; }
        inline const T* operator->() const { return _ptr; }
    inline bool operator!() const { return _ptr==0L; }
    inline bool valid() const { return _ptr!=0L; }
        inline T* get() { return _ptr; }
        inline const T* get() const { return _ptr; }
    inline T* release() { T* tmp=_ptr; if (_ptr) _ptr->unref_nodelete(); _ptr=0; return tmp;}

  private:
        T* _ptr;
  };

}


#endif



#ifndef __SLB_TABLE__
#define __SLB_TABLE__

//->#include "Export.hpp"
//->#include "Object.hpp"
//->#include "String.hpp"
//->#include "ref_ptr.hpp"
#include <typeinfo>
#include <map>
//->#include "lua.hpp"

namespace SLB {

  class SLB_EXPORT Table : public Object {
  public:
    typedef SLB_Map( String, ref_ptr<Object> ) Elements;
    typedef std::set<String> Keys;
    Table(const String &separator = "", bool cacheable = false);

    void erase(const String &);
    Object* get(const String &);
    Keys getKeys();
    void set(const String &, Object*);

    bool isCacheable() { return _cacheable; }

    // [ -2, 0, - ] pops two elements (key, value) from the top and pushes it into
    // the cache. If _cacheable == false this won't make much sense, but you can use
    // it anyway (you can recover the values with getCache).
    void setCache(lua_State *L);

    // [ -1, +1, - ] will pop a key, and push the value or nil if not found.
    void getCache(lua_State *L);

    const Elements& getElementsMap() const { return _elements; }

  protected:
    virtual ~Table();

    Object* rawGet(const String &);
    void rawSet(const String &, Object*);
    
    void pushImplementation(lua_State *);

    /** will try to find the object, if not present will return -1. If this
     * function is not overriden not finding an object will raise an error
     * It is highly recommended to call this method in subclasses of Table
     * first.*/
    virtual int __index(lua_State *L); 
    virtual int __newindex(lua_State *L);

    virtual int __call(lua_State *L);
    virtual int __garbageCollector(lua_State *L);
    virtual int __tostring(lua_State *L);
    virtual int __eq(lua_State *L);

    Elements _elements;

    /** this function returns the index where to find the cache table that
     * __index method uses if _cacheable is true. This method must NOT be called
     * outside metamethod's implementation. */
    static int cacheTableIndex() { return lua_upvalueindex(1); }

  private:
    typedef std::pair<Table*,const String> TableFind;
    typedef int (Table::*TableMember)(lua_State*);

    int __indexProxy(lua_State *L);
    static int __meta(lua_State*);
    void pushMeta(lua_State *L, TableMember) const;

    TableFind getTable(const String &key, bool create);

    bool _cacheable;
    String _separator;

    Table(const Table&);
    Table& operator=(const Table&);
    SLB_CLASS(Table, Object);
  };

  //--------------------------------------------------------------------
  // Inline implementations:
  //--------------------------------------------------------------------
    
}


#endif



#ifndef __SLB_CONVERSOR__
#define __SLB_CONVERSOR__

namespace SLB {
  
  struct Conversor
  {
    virtual void* operator()(void* obj) = 0;
    virtual ~Conversor() {}
  };

  template<class B, class D>
  struct BaseToDerived : public Conversor
  {
    void *operator()(void* obj_raw)
    {
      B *obj = reinterpret_cast<B*>(obj_raw);
      return dynamic_cast<D*>(obj);
    }
  };

  template<class D, class B>
  struct DerivedToBase : public Conversor
  {
    void *operator()(void* obj_raw)
    {
      D *obj = reinterpret_cast<D*>(obj_raw);
      B *base_obj = obj; // D -> B
      return base_obj;
    }
  };

}

#endif



#ifndef __SLB_ERROR__
#define __SLB_ERROR__

//->#include "Export.hpp"
//->#include "lua.hpp"
#include <sstream>

namespace SLB {

  /** This class handles any error raised while lua is working.
    You can subclass it and set it as default on SLB::Manager::setErrorHandler*/
  class SLB_EXPORT ErrorHandler
  {
  public:
    ErrorHandler() {}
    virtual ~ErrorHandler() {}

    // performs the lua_pcall using this errorHandler
    int call(lua_State *L, int nargs, int nresults);

    /// first function to be called with the main error
    virtual void begin(const char* /*error*/) {}

    /** Last function that will be called at the end, should
        return the string of the computed error. */
    virtual const char* end() = 0;

    /// Called on each stack element, here you can call SE_*
    /// functions. 
    virtual void stackElement(int /*level*/) {}

    int errorLine() { return _currentLine; }

  protected:

    /// StackElement function
    /// returns the possible name of the function (can be null)
    const char *SE_name();

    /// StackElement function
    /// returns the "global", "local", "method", "field", "upvalue", or ""
    const char *SE_nameWhat();

    /// StackElement function
    /// returns "Lua", "C", "main", "tail"
    const char *SE_what();

    /// StackElement function
    /// returns If the function was defined in a string, then source is that string.
    /// If the function was defined in a file, then source starts with a '@' followed by the file name
    const char *SE_source();

    /// StackElement function
    /// shorten version of source
    const char *SE_shortSource(); 

    /// StackElement function
    /// number of line, or -1 if there is no info available
    int SE_currentLine();
    int _currentLine;


    /// StackElement function
    /// number of upvalues of the function
    int SE_numberOfUpvalues();

    /// StackElement function
    /// the line number where the definition of the funciton starts
    int SE_lineDefined();

    /// StackElement function
    /// the last line where the definition of the function ends
    int SE_lastLineDefined();

    /// This function will be called by the handler to start the process of
    /// retrieving info from the lua failure. If you reinterpret this function
    /// Call to the parent's implementation.
    virtual void process(lua_State *L);

  private:
    static int _slb_stackHandler(lua_State *L);
    lua_Debug _debug;
    lua_State *_lua_state;
  };


  class SLB_EXPORT DefaultErrorHandler : public ErrorHandler
  {
  public:
    DefaultErrorHandler() {}
    virtual ~DefaultErrorHandler() {}
    virtual void begin(const char *error);
    virtual const char* end();
    virtual void stackElement(int level);
  private:
    std::ostringstream _out;
    std::string _final;
  };


#ifdef __SLB_TODO__
  class SLB_EXPORT ThreadSafeErrorHandler : public ErrorHandler
  {
  public:
    /** Will call EH but locking at the begin, and at the end. 
        The ErrorHandler passed will be freed be owned by this
      object */
    ThreadSafeErrorHandler( ErrorHandler *EH );
    virtual ~ThreadSafeErrorHandler();
    virtual void begin(const char *error);
    virtual const char* end();

  private:
    ErrorHandler *_EH;
  };
#endif

} /* SLB */


#endif /* EOF */



#ifndef __SLB_ITERATOR__
#define __SLB_ITERATOR__

//->#include "Export.hpp"
//->#include "Object.hpp"
//->#include "PushGet.hpp"

struct lua_State;

namespace SLB
{

  class SLB_EXPORT IteratorBase
  {
  public:
    virtual int push(lua_State *L) = 0;
    virtual ~IteratorBase() {}
  };

  class SLB_EXPORT Iterator : public Object
  {
  public: 
    Iterator(IteratorBase *b);

  protected:
    void pushImplementation(lua_State *L);
    virtual ~Iterator();
  private:
    static int iterator_call(lua_State *L);
    IteratorBase *_iterator;
    Iterator( const Iterator &slbo);
    Iterator& operator=( const Iterator &slbo);
    SLB_CLASS(Iterator, Object)
  };


  // Standard iterator
  template<class T, class T_iterator>
  struct StdIteratorTraits
  {
    typedef T Container;
    typedef T_iterator Iterator;
    typedef Iterator (Container::*GetIteratorMember)();
    //How unref iterators:
    static typename Iterator::value_type unref(Iterator& i) { return *i; }
  };

  template<class T, class T_iterator>
  struct StdConstIteratorTraits
  {
    typedef T Container;
    typedef T_iterator Iterator;
    typedef Iterator (Container::*GetIteratorMember)() const;

    //How unref iterators:
    static const typename Iterator::value_type unref(const Iterator& i) { return *i; }
  };

  template<typename Traits>
  class StdIterator : public IteratorBase
  {
  public:
    typedef typename Traits::GetIteratorMember MemberFuncs ;
    typedef typename Traits::Container Container;
    typedef typename Traits::Iterator  Iterator;

    StdIterator(MemberFuncs m_first, MemberFuncs m_end );
    int push(lua_State *L);
  protected:
    static int next(lua_State *L) ;
  private:
    MemberFuncs _begin, _end;

    StdIterator( const StdIterator &slbo);
    StdIterator& operator=( const StdIterator &slbo);
  };

  // ------------------------------------------------------------
  // ------------------------------------------------------------
  // ------------------------------------------------------------
  
  template<class T>
  inline StdIterator<T>::StdIterator(MemberFuncs m_first, MemberFuncs m_end)
    : _begin(m_first), _end(m_end)
  {
  }
  
  template<class T>
  inline int StdIterator<T>::push(lua_State *L)
  {
    SLB_DEBUG_CALL
    Container* container = SLB::get<Container*>(L,1);
    lua_pushcclosure(L, StdIterator<T>::next, 0);
    Iterator *d = reinterpret_cast<Iterator*>(lua_newuserdata(L, sizeof(Iterator)*2));
    Iterator empty;
    memcpy(d+0,&empty,sizeof(Iterator)); // copy the image of an empty iterator into the lua memory
    memcpy(d+1,&empty,sizeof(Iterator)); // ""
    //TODO: make sure iterators doesn't need to handle destruction...
    d[0] = (container->*_begin)();
    d[1] = (container->*_end)();
    return 2;
  }

  template<class T>
  inline int StdIterator<T>::next(lua_State *L)
  {
    SLB_DEBUG_CALL
    Iterator *d = reinterpret_cast<Iterator*>(lua_touserdata(L,1));
    
    if ( d[0] != d[1] )
    {
      SLB::push(L, T::unref(d[0]) );
      d[0]++; // inc iterator
    }
    else
    {
      lua_pushnil(L);
    }
    return 1;
  }

} //end of SLB namespace

#endif



#ifndef __SLB_LUACALL__
#define __SLB_LUACALL__

//->#include "lua.hpp"
//->#include "Export.hpp"
//->#include "SPP.hpp"
//->#include "Object.hpp"
//->#include "PushGet.hpp"
//->#include "Type.hpp"

#include <vector>
#include <typeinfo>
#include <iostream>
#include <stdexcept>

namespace SLB
{
    struct CallException : public std::runtime_error
    {
        int errorLine;
        std::string message;

        CallException(const std::string& m, int l) : 
            std::runtime_error(m.c_str()), 
            errorLine(l), message(m) {}
        ~CallException() throw () {}
    };

    class SLB_EXPORT LuaCallBase 
    { 
        public:
            // this allows to store a luaCall, mainly used by
            // Hybrid classes...
            virtual ~LuaCallBase();
        protected: 
            LuaCallBase(lua_State *L, int index);
            LuaCallBase(lua_State *L, const char *func);
            void execute(int numArgs, int numOutput, int top);

            lua_State *_lua_state;
            int _ref; 
        private:
            void getFunc(int index, const char* func);
            static int errorHandler(lua_State *L);
    }; 

    template<typename T>
        struct LuaCall;

#define SLB_ARG(N) T##N arg_##N, 
#define SLB_PUSH_ARGS(N) push<T##N>(_lua_state, arg_##N );

#define SLB_REPEAT(N) \
    \
    /* LuaCall: functions that return something  */ \
    template<class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    struct SLB_EXPORT LuaCall<R( SPP_ENUM_D(N,T) )> : public LuaCallBase\
    { \
        LuaCall(lua_State *L, int index) : LuaCallBase(L,index) {} \
        LuaCall(lua_State *L, const char *func) : LuaCallBase(L,func) {} \
        R operator()( SPP_REPEAT( N, SLB_ARG) char dummyARG = 0) /*TODO: REMOVE dummyARG */\
        { \
            int top = lua_gettop(_lua_state); \
            lua_rawgeti(_lua_state, LUA_REGISTRYINDEX,_ref); \
            SPP_REPEAT( N, SLB_PUSH_ARGS ); \
            execute(N, 1, top); \
            R result = get<R>(_lua_state, -1); \
            lua_settop(_lua_state,top); \
            return result; \
        } \
        bool operator==(const LuaCall& lc) { return (_lua_state == lc._lua_state && _ref == lc._ref); }\
    };
    SPP_MAIN_REPEAT_Z(MAX,SLB_REPEAT)
#undef SLB_REPEAT

#define SLB_REPEAT(N) \
        \
        /*LuaCall: functions that doesn't return anything */  \
        template<SPP_ENUM_D(N, class T)> \
        struct SLB_EXPORT LuaCall<void( SPP_ENUM_D(N,T) )> : public LuaCallBase\
    { \
        LuaCall(lua_State *L, int index) : LuaCallBase(L,index) {} \
        LuaCall(lua_State *L, const char *func) : LuaCallBase(L,func) {} \
        void operator()( SPP_REPEAT( N, SLB_ARG) char /*dummyARG*/ = 0) /*TODO: REMOVE dummyARG */\
        { \
            int top = lua_gettop(_lua_state); \
            lua_rawgeti(_lua_state, LUA_REGISTRYINDEX,_ref); \
            SPP_REPEAT( N, SLB_PUSH_ARGS ); \
            execute(N, 0, top); \
            lua_settop(_lua_state,top); \
        } \
        bool operator==(const LuaCall& lc) { return (_lua_state == lc._lua_state && _ref == lc._ref); }\
    }; \

        SPP_MAIN_REPEAT_Z(MAX,SLB_REPEAT)
#undef SLB_REPEAT
#undef SLB_ARG
#undef SLB_PUSH_ARGS

} //end of SLB namespace

  //--------------------------------------------------------------------
  // Inline implementations:
  //--------------------------------------------------------------------
  
#endif



#ifndef __SLB_MANAGER__
#define __SLB_MANAGER__

//->#include "Object.hpp"
//->#include "ref_ptr.hpp"
//->#include "Export.hpp"
//->#include "Debug.hpp"
//->#include "String.hpp"
//->#include "TypeInfoWrapper.hpp"
#include <map>
#include <cstdlib>

namespace SLB {

  class ClassInfo;
  class Namespace;

  // copy values and objects from one lua_State to another
  bool copy(lua_State *from, int pos, lua_State *to);

  class SLB_EXPORT Manager
  {
  public:
    typedef void* (*Conversor)(void *);
    typedef SLB_Map( TypeInfoWrapper, ref_ptr<ClassInfo> ) ClassMap;
    typedef SLB_Map( String, TypeInfoWrapper ) NameMap;
    typedef std::pair<TypeInfoWrapper, TypeInfoWrapper > TypeInfoWrapperPair;
    typedef SLB_Map( TypeInfoWrapperPair, Conversor ) ConversionsMap;

    Manager();
    ~Manager();

    /** extracts the Manager registered given a luaState.
      * Returns null if in the lua_State there was no registered Manager
      * (SLB::Manager::registerSLB).
      */
    static Manager *getInstance(lua_State *L);

    /** returns the defaultManager, just in case you don't need separated managers.
        To free the memory of the default manager call destroyDefaultManager(),
        the memory is NOT automatically freed at the end of the program.  */
    static Manager *defaultManager();

    /** removes the default manager, and resets its memory. It is safe to call defaultManager before
        destruction and a new instance will be created. */
    static void destroyDefaultManager();

    const ClassInfo *getClass(const TypeInfoWrapper&) const;
    const ClassInfo *getClass(const String&) const;
    /// Returns the classInfo of an object, or null if it is a basic lua-type
    ClassInfo *getClass(lua_State *L, int pos) const ;
    ClassInfo *getClass(const TypeInfoWrapper&);
    ClassInfo *getClass(const String&);
    ClassInfo *getOrCreateClass(const TypeInfoWrapper&);

    /** Copy from one lua_State to another:
      - for basic types it will make a basic copy
      - returns true if copy was made, otherwise returns false. 
      - doesn't touch the original element

      ** WARNING **
      copy of tables is not yet implemented...
    */
    bool copy(lua_State *from, int pos, lua_State *to);

    // set a global value ( will be registered automatically on every lua_State )
    void set(const String &, Object *obj);

    // This will add a SLB table to the current global state
    void registerSLB(lua_State *L);

    // convert from C1 -> C2
    void* convert( const TypeInfoWrapper &C1, const TypeInfoWrapper &C2, void *obj);
    const void* convert( const TypeInfoWrapper &C1, const TypeInfoWrapper &C2, const void *obj);

    Namespace* getGlobals() { return _global.get(); }

    /** Returns the classMap with all defined classes */
    const ClassMap& getClassMap() const { return _classes; }

  protected:

    void rename(ClassInfo *c, const String &new_name);
    template<class Derived, class Base>
    void addConversor();
    template<class Derived, class Base>
    void addStaticConversor();
    template<class A, class B>
    void addClassConversor( Conversor );

    /** Returns the classMap with all defined classes */
    ClassMap& getClasses() { return _classes; }

    void* recursiveConvert(const TypeInfoWrapper &C1, const TypeInfoWrapper &C2, const TypeInfoWrapper& prev, void *obj);

    friend int SLB_allTypes(lua_State *);

  private:
    Manager(const Manager&);
    Manager& operator=(const Manager&);

    ClassMap _classes;
    NameMap  _names;
    ref_ptr<Namespace> _global;
    ConversionsMap _conversions;

    static Manager *_default;
    friend class ClassInfo;
  };
  
  //--------------------------------------------------------------------
  // Inline implementations:
  //--------------------------------------------------------------------
  
  template<class D, class B>
  struct ClassConversor
  {
    static void* convertToBase(void *raw_d)
    {
      D* derived = reinterpret_cast<D*>(raw_d);
      B* base = derived;
      return (void*) base;
    }
    
    static void* convertToDerived(void *raw_b)
    {
      B* base = reinterpret_cast<B*>(raw_b);
      D* derived = dynamic_cast<D*>(base);
      return (void*) derived;
    }

    static void* staticConvertToDerived(void *raw_b)
    {
      B* base = reinterpret_cast<B*>(raw_b);
      D* derived = static_cast<D*>(base);
      return (void*) derived;
    }

    static B* defaultConvert( D* ptr )
    {
      return &static_cast<B>(*ptr);
    }
    
  };

  template<class D, class B>
  inline void Manager::addConversor()
  {
    _conversions[ ConversionsMap::key_type(_TIW(D), _TIW(B)) ] = &ClassConversor<D,B>::convertToBase;
    _conversions[ ConversionsMap::key_type(_TIW(B), _TIW(D)) ] = &ClassConversor<D,B>::convertToDerived;
  }

  template<class D, class B>
  inline void Manager::addStaticConversor()
  {
    _conversions[ ConversionsMap::key_type(_TIW(D), _TIW(B)) ] = &ClassConversor<D,B>::convertToBase;
    _conversions[ ConversionsMap::key_type(_TIW(B), _TIW(D)) ] = &ClassConversor<D,B>::staticConvertToDerived;
  }

  template<class A, class B>
  inline void Manager::addClassConversor( Conversor c )
  {
    _conversions[ ConversionsMap::key_type(_TIW(A), _TIW(B)) ] = c;
  }

  inline void* Manager::convert( const TypeInfoWrapper &C1, const TypeInfoWrapper &C2, void *obj)
  {
    SLB_DEBUG_CALL;
    SLB_DEBUG(10, "C1 = %s", C1.name());
    SLB_DEBUG(10, "C2 = %s", C2.name());
    SLB_DEBUG(10, "obj = %p", obj);
    if (C1 == C2)
    {
      SLB_DEBUG(11, "same class");
      return obj; 
    }

    ConversionsMap::iterator i = _conversions.find( ConversionsMap::key_type(C1,C2) );
    if (i != _conversions.end())
    {
      SLB_DEBUG(11, "directly convertible");
      return i->second( obj );
    }

    //The _conversions map only hold direct conversions added via .inherits<> or .static_inherits<>.
    // recursiveConvert can extract implied conversions from the _conversions table (but much less 
    // efficiently than a direct conversion).  For example if a direct conversion from Animal to Dog
    // exists and a conversion from Dog to Poodle exists, then recursiveConvert can convert an
    // Animal to a Poodle.
    void* result = recursiveConvert(C1, C2, TypeInfoWrapper(), obj);

    if (result)
    {
      SLB_DEBUG(11, "indirectly convertible");
    }
    else
    {
      SLB_DEBUG(11, "fail");
    }
    return result;
  }

  inline const void* Manager::convert( const TypeInfoWrapper &C1, const TypeInfoWrapper &C2, const void *obj)
  {
    return const_cast<void*>(convert(C1,C2, const_cast<void*>(obj)));
  }

  inline bool copy(lua_State *from, int pos, lua_State *to)
  {
    return Manager::getInstance(from)->copy(from,pos,to);
  }

}

#endif


//->#include "Export.hpp"

#ifndef __SLB_MUTEX__
#define __SLB_MUTEX__

#if SLB_THREAD_SAFE == 0
  namespace SLB { struct MutexData {}; }
#else // SLB_THREAD_SAFE
  // Win32 Mutex:
  #ifdef SLB_WINDOWS
    #include <windows.h>
    namespace SLB { typedef CRITICAL_SECTION MutexData; }
  #else // WIN32
  // Posix Mutex:
    #include <pthread.h>  
    namespace SLB { typedef pthread_mutex_t MutexData; }
  #endif
#endif //SLB_THREAD_SAFE

namespace SLB
{

  /// Multiplatform Mutex abstraction, needed to ensure thread safety of
  /// some algorithms. You can turn on Mutexes by compileing SLB with
  /// SLB_THREAD_SAFE

  struct Mutex
  {
    Mutex();
    ~Mutex();
    void lock();
    void unlock();
    bool trylock();
  private:
    MutexData _m;
  };

  struct CriticalSection
  {
    CriticalSection(Mutex *m) : _m(m)
    {
      if (_m) _m->lock();
    }
    ~CriticalSection()
    {
      if (_m) _m->unlock();
    }
  private:
    Mutex *_m;
  };

  struct ActiveWaitCriticalSection
  {
    ActiveWaitCriticalSection(Mutex *m) : _m(m)
    {
      if (_m) while(_m->trylock() == false) {/*try again*/};
    }
    ~ActiveWaitCriticalSection()
    {
      if (_m) _m->unlock();
    }
  private:
    Mutex *_m;
  };


#if SLB_THREAD_SAFE == 0
  inline Mutex::Mutex() {}
  inline Mutex::~Mutex() {}
  inline void Mutex::lock(){}
  inline void Mutex::unlock() {}
  inline bool Mutex::trylock() { return true; }
#else // SLB_THREAD_SAFE
#ifdef WIN32
  // Windows implementation...
  inline Mutex::Mutex()
  {
    InitializeCriticalSection(&_m);
  }
  inline Mutex::~Mutex()
  {
    DeleteCriticalSection(&_m);
  }
  inline void Mutex::lock()
  {
    EnterCriticalSection(&_m);
  }
  inline void Mutex::unlock()
  {
    LeaveCriticalSection(&_m);
  }
  inline bool Mutex::trylock()
  {
    return( TryEnterCriticalSection(&_m) != 0) ;
  }
#else
  // PTHREADS implementation...
  inline Mutex::Mutex()
  {
    pthread_mutex_init(&_m,0);
  }
  inline Mutex::~Mutex()
  {
    pthread_mutex_destroy(&_m);
  }
  inline void Mutex::lock()
  {
    pthread_mutex_lock(&_m);
  }
  inline void Mutex::unlock()
  {
    pthread_mutex_unlock(&_m);
  }
  inline bool Mutex::trylock()
  {
    return( pthread_mutex_trylock(&_m) == 0) ;
  }
#endif // WIN32


#endif // SLB_THREAD_SAFE


} // end of SLB's namespace

#endif



#ifndef __SLB_PROPERTY__
#define __SLB_PROPERTY__

//->#include "Export.hpp"
//->#include "Object.hpp"
//->#include "PushGet.hpp"
//->#include "ref_ptr.hpp"
#include <map>

#include <iostream>

struct lua_State;

namespace SLB {

  class SLB_EXPORT BaseProperty: public Object
  {
  public:
    typedef SLB_Map( String, ref_ptr<BaseProperty> ) Map;

    template<class T, class M>
    static BaseProperty* create(M T::*prop);

    /** gets the property from the object at index idx */
    virtual void set(lua_State *, int idx);
    /** sets the property of the object located at index idx, poping
        an element from the stack */
    virtual void get(lua_State *, int idx);

  
  protected:
    BaseProperty();
    virtual ~BaseProperty();

    virtual void pushImplementation(lua_State *);
    SLB_CLASS(BaseProperty, Object);
  };

  template<class T, class M>
  class Property: public virtual BaseProperty
  {
  public:
    typedef M T::*MemberPtr;
    Property( MemberPtr m) : _m(m) 
    {
      
    }
  protected:
    virtual ~Property() {}

    virtual void get(lua_State *L, int idx)
    {
      // get object at T
      const T *obj = SLB::get<const T*>(L,idx);
      if (obj == 0L) luaL_error(L, "Invalid object to get a property from");
      // push the property
      SLB::push(L, obj->*_m);
    }
    virtual void set(lua_State *L, int idx)
    {
      // get object at T
      T *obj = SLB::get<T*>(L,idx);
      if (obj == 0L) luaL_error(L, "Invalid object to set a property from");
      // set the property
      obj->*_m = SLB::get<M>(L,-1);

      lua_pop(L,1); // remove the last element
    }
  private:
    MemberPtr _m;
    typedef Property<T,M> T_This;
    SLB_CLASS(T_This, BaseProperty);
  };


  template<class T, class M>
  inline BaseProperty* BaseProperty::create(M T::*p)
  {
    typedef Property<T,M> Prop;
    Prop *nprop = new (SLB::Malloc(sizeof(Prop))) Prop(p);
    return nprop;
  }

}


#endif



#ifndef __SLB_FUNCCALL__
#define __SLB_FUNCCALL__

//->#include "Object.hpp"
//->#include "Export.hpp"
//->#include "Allocator.hpp"
//->#include "String.hpp"
//->#include "SPP.hpp"
//->#include "lua.hpp"
//->#include "TypeInfoWrapper.hpp"

#include <vector>
#include <typeinfo>

namespace SLB
{
  class SLB_EXPORT FuncCall : public Object
  {
  public:

    #define SLB_REPEAT(N) \
    \
      /* FunCall for class Methods */ \
      template<class C,class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      static FuncCall* create(R (C::*func)(SPP_ENUM_D(N,T)) ); \
    \
      /* FunCall for CONST class Methods */ \
      template<class C,class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      static FuncCall* create(R (C::*func)(SPP_ENUM_D(N,T)) const ); \
    \
      /* (explicit) FunCall for CONST class Methods */ \
      template<class C,class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      static FuncCall* createConst(R (C::*func)(SPP_ENUM_D(N,T)) const ); \
    \
      /* (explicit) FunCall for NON-CONST class Methods */ \
      template<class C,class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      static FuncCall* createNonConst(R (C::*func)(SPP_ENUM_D(N,T))); \
    \
      /* FunCall for C-functions  */ \
      template<class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      static FuncCall* create(R (func)(SPP_ENUM_D(N,T)) ); \
    \
      /* FunCall Class constructors  */ \
      template<class C SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      static FuncCall* defaultClassConstructor(); \

    SPP_MAIN_REPEAT_Z(MAX,SLB_REPEAT)
    #undef SLB_REPEAT

    /* special case of a proper lua Function */
    static FuncCall* create(lua_CFunction f);

    size_t getNumArguments() const { return _Targs.size(); }
    const TypeInfoWrapper& getArgType(size_t p) const { return _Targs[p].first; }
    const String& getArgComment(size_t p) const { return _Targs[p].second; }
    const TypeInfoWrapper& getReturnedType() const { return _Treturn; }
    void setArgComment(size_t p, const String& c);

  protected:
    FuncCall();
    virtual ~FuncCall();
  
    void pushImplementation(lua_State *L);
    virtual int call(lua_State *L) = 0;

    typedef std::pair<TypeInfoWrapper, SLB::String> TypeInfoStringPair;
    std::vector< TypeInfoStringPair, Allocator<TypeInfoStringPair> > _Targs;
    TypeInfoWrapper _Treturn;
  private:
    static int _call(lua_State *L);

  friend class Manager;  
  friend class ClassInfo;
  SLB_CLASS(FuncCall, Object);
  };

} //end of SLB namespace
  
//->#include "FuncCall_inline.hpp"
#endif



#ifndef __SLB_INSTANCE__
#define __SLB_INSTANCE__

//->#include "ref_ptr.hpp"
//->#include "Manager.hpp"
//->#include "Debug.hpp"
//->#include "Export.hpp"
//->#include "Allocator.hpp"

#define SLB_INSTANCE \
  virtual const void *memoryRawPointer() const { return this; }

namespace SLB {

  class SLB_EXPORT InstanceBase
  {
  public:
    enum Type
    {
      I_Invalid    = 0,
      I_UserFlag_0 = 1<<0,
      I_UserFlag_1 = 1<<1,
      I_UserFlag_2 = 1<<2,
      I_UserFlag_3 = 1<<3,
      I_UserFlag_4 = 1<<4,
      I_UserFlag_5 = 1<<5,
      I_UserFlag_6 = 1<<6,
      I_UserFlag_7 = 1<<7,
      //-----------------------
      I_UserMask   = 0xFF,
      //-----------------------
      I_MustFreeMem   = 1<<17,
      I_Reference     = 1<<18,
      I_Pointer       = 1<<19,
      I_Const_Pointer = 1<<20,
    };

    // functions to override:
    virtual void* get_ptr() = 0;
    virtual const void* get_const_ptr() = 0;
    virtual const void *memoryRawPointer() const = 0;

    // constructor:
    InstanceBase(Type , ClassInfo *c);

    ClassInfo *getClass() { return _class.get(); }

    size_t getFlags() const { return _flags; }
    size_t getUserFlags() const { return _flags & I_UserMask; }
    void addUserFlags(size_t flags) { _flags = _flags | (flags & I_UserMask); }
    void setUSerFlags(size_t flags) { _flags = (flags & I_UserMask); }

    bool mustFreeMem() const { return (_flags & I_MustFreeMem) != 0; }
    bool isConst() const     { return (_flags & I_Const_Pointer) != 0; }
    bool isPointer() const   { return (_flags & I_Pointer) || (_flags & I_Const_Pointer); }
    bool isReference() const { return (_flags & I_Reference) != 0; }
    virtual ~InstanceBase();

    // This is called if the memory comes from a constructor inside the script
    // You probably would never need to call this manually
    void setMustFreeMemFlag() { _flags = _flags | I_MustFreeMem; }

  protected:
    size_t _flags;
    ref_ptr<ClassInfo> _class;
  };

  namespace Instance {

    struct Default {
      template<class T>
      class Implementation : public virtual InstanceBase
      {
      public:
        // constructor from a pointer 
        Implementation(ClassInfo *ci, T* ptr) : InstanceBase( I_Pointer,ci ), _ptr(ptr)
        {
        }
        // constructor from const pointer
        Implementation(ClassInfo *ci, const T *ptr ) : InstanceBase( I_Const_Pointer, ci), _const_ptr(ptr)
        {
        }

        // constructor from reference
        Implementation(ClassInfo *ci, T &ref ) : InstanceBase( I_Reference, ci ), _ptr( &ref )
        {
        }

        // copy constructor,  
        Implementation(ClassInfo *ci, const T &ref) : InstanceBase( I_MustFreeMem, ci ), _ptr( 0L )
        {
          _ptr = new (Malloc(sizeof(T))) T(ref);
        }

        virtual ~Implementation() { if (mustFreeMem()) Free_T(&_ptr); }

        void* get_ptr() { return (isConst())? 0L : _ptr; }
        const void* get_const_ptr() { return _const_ptr; }
      protected:
        union {
          T *_ptr;
          const T *_const_ptr;
        };
        SLB_INSTANCE;
      };
    };


    struct NoCopy
    {
      template<class T>
      class Implementation : public virtual InstanceBase
      {
      public:
        Implementation(ClassInfo *ci, T* ptr) : InstanceBase( I_Pointer, ci ), _ptr(ptr)
        {
        }
        // constructor from const pointer
        Implementation(ClassInfo *ci, const T *ptr ) : InstanceBase( I_Const_Pointer, ci), _const_ptr(ptr)
        {
        }

        // constructor from reference
        Implementation(ClassInfo *ci, T &ref ) : InstanceBase( I_Reference, ci ), _ptr( &ref )
        {
        }

        // copy constructor,  
        Implementation(ClassInfo *ci, const T &ref) : InstanceBase( I_Invalid, ci ), _ptr( 0L )
        {
        }

        virtual ~Implementation() { if (mustFreeMem()) Free_T(&_ptr); }

        void* get_ptr() { return (isConst())? 0L : _ptr; }
        const void* get_const_ptr() { return _const_ptr; }
      protected:
        union {
          T *_ptr;
          const T *_const_ptr;
        };
        SLB_INSTANCE;
      };
    };

    struct NoCopyNoDestroy 
    {
      template<class T>
      class Implementation : public virtual InstanceBase
      {
      public:
        // constructor form a pointer 
        Implementation(ClassInfo *ci, T* ptr) : InstanceBase( I_Pointer, ci ), _ptr(ptr)
        {
        }
        // constructor from const pointer
        Implementation(ClassInfo *ci, const T *ptr ) : InstanceBase( I_Const_Pointer, ci), _const_ptr(ptr)
        {
        }

        // constructor from reference
        Implementation(ClassInfo *ci, T &ref ) : InstanceBase( I_Reference, ci ), _ptr( &ref )
        {
        }

        // copy constructor,  
        Implementation(ClassInfo *ci, const T &) : InstanceBase( I_Invalid, ci ), _ptr( 0L )
        {
        }

        virtual ~Implementation() {}

        void* get_ptr() { return (isConst())? 0L : _ptr; }
        const void* get_const_ptr() { return _const_ptr; }
      protected:
        union {
          T *_ptr;
          const T *_const_ptr;
        };
        SLB_INSTANCE;
      };
    };

    template<template <class> class T_SmartPtr>
    struct SmartPtr 
    {
      template<class T>
      class Implementation : public virtual InstanceBase
      {
      public:
        Implementation(ClassInfo *ci, T* ptr) : InstanceBase( I_Pointer, ci ), _sm_ptr(ptr)
        {
          _const_ptr = &(*_sm_ptr);
        }
        Implementation(ClassInfo *ci, const T *ptr ) : InstanceBase( I_Const_Pointer, ci), _const_ptr(ptr)
        {
        }
        // What should we do with references and smart pointers?
        Implementation(ClassInfo *ci, T &ref ) : InstanceBase( I_Reference, ci ), _sm_ptr( &ref )
        {
          _const_ptr = &(*_sm_ptr);
        }

        // copy constructor,  
        Implementation(ClassInfo *ci, const T &ref) : InstanceBase( I_MustFreeMem, ci ), _sm_ptr( 0L ), _const_ptr(0)
        {
          _sm_ptr = new (Malloc(sizeof(T))) T( ref );
          _const_ptr = &(*_sm_ptr);
        }

        virtual ~Implementation() {}

        void* get_ptr() { return &(*_sm_ptr); }
        const void* get_const_ptr() { return _const_ptr; }

      protected:
        T_SmartPtr<T> _sm_ptr;
        const T *_const_ptr;
        SLB_INSTANCE;
      };
    };

    template<template <class> class T_SmartPtr>
    struct SmartPtrNoCopy
    {
      template<class T>
      class Implementation : public virtual InstanceBase
      {
      public:
        Implementation(ClassInfo *ci, T* ptr) : InstanceBase( I_Pointer, ci ), _sm_ptr(ptr)
        {
          _const_ptr = &(*_sm_ptr);
        }
        Implementation(ClassInfo *ci, const T *ptr ) : InstanceBase( I_Const_Pointer,ci), _const_ptr(ptr)
        {
        }

        // What should we do with references and smart pointers?
        Implementation(ClassInfo *ci, T &ref ) : InstanceBase( I_Reference, ci ), _sm_ptr( &ref )
        {
          _const_ptr = &(*_sm_ptr);
        }

        // copy constructor,  
        Implementation(ClassInfo *ci, const T &ref) : InstanceBase( I_Invalid, ci ), _sm_ptr( 0L ), _const_ptr(&ref)
        {
        }

        virtual ~Implementation() {}

        void* get_ptr() { return &(*_sm_ptr); }
        const void* get_const_ptr() { return _const_ptr; }

      protected:
        T_SmartPtr<T> _sm_ptr;
        const T *_const_ptr;
        SLB_INSTANCE;
      };
    };

    template<template <class> class T_SmartPtr>
    struct SmartPtrSharedCopy
    {
      template<class T>
      class Implementation : public virtual InstanceBase
      {
      public:
        Implementation(ClassInfo *ci, T* ptr) : InstanceBase( I_Pointer, ci ), _sm_ptr(ptr)
        {
          _const_ptr = &(*_sm_ptr);
        }
        Implementation(ClassInfo *ci, const T *ptr ) : InstanceBase( I_Const_Pointer, ci), _const_ptr(ptr)
        {
        }

        Implementation(ClassInfo *ci, T &ref ) : InstanceBase( I_Reference, ci ), _sm_ptr( &ref )
        {
          _const_ptr = &(*_sm_ptr);
        }

        // copy constructor,  
        Implementation(ClassInfo *ci, const T &ref) : InstanceBase( I_Invalid, ci ), _sm_ptr( 0L ), _const_ptr(&ref)
        {
          T *obj = const_cast<T*>(&ref);
          _sm_ptr = obj;
        }

        virtual ~Implementation() {}

        void* get_ptr() { return &(*_sm_ptr); }
        const void* get_const_ptr() { return _const_ptr; }

      protected:
        T_SmartPtr<T> _sm_ptr;
        const T *_const_ptr;
        SLB_INSTANCE;
      };
    };

  
  } // end of Instance namespace


  struct SLB_EXPORT InstanceFactory
  {
    // create an Instance from a reference
    virtual InstanceBase *create_ref(Manager *m, void *ref) = 0;
    // create an Instance from a pointer
    virtual InstanceBase *create_ptr(Manager *m, void *ptr) = 0;
    // create an Instance from a const pointer
    virtual InstanceBase *create_const_ptr(Manager *m, const void *const_ptr) = 0;
    // create an Instance with copy
    virtual InstanceBase *create_copy(Manager *m, const void *ptr) = 0;

    virtual ~InstanceFactory();
  };

  template<class T, class TInstance >
  struct InstanceFactoryAdapter : public InstanceFactory
  {
    virtual InstanceBase *create_ref(Manager *m, void *v_ref)
    {
      T &ref = *reinterpret_cast<T*>(v_ref);
      ClassInfo *ci = m->getClass(_TIW(T));
      return new (Malloc(sizeof(TInstance))) TInstance(ci, ref);
    }

    virtual InstanceBase *create_ptr(Manager *m, void *v_ptr)
    {
      T *ptr = reinterpret_cast<T*>(v_ptr);
      ClassInfo *ci = m->getClass(_TIW(T));
      return new (Malloc(sizeof(TInstance))) TInstance(ci, ptr);
    }

    virtual InstanceBase *create_const_ptr(Manager *m, const void *v_ptr)
    {
      const T *const_ptr = reinterpret_cast<const T*>(v_ptr);
      ClassInfo *ci = m->getClass(_TIW(T));
      return new (Malloc(sizeof(TInstance))) TInstance(ci, const_ptr);
    }

    virtual InstanceBase *create_copy(Manager *m, const void *v_ptr)
    {
      const T &const_ref = *reinterpret_cast<const T*>(v_ptr);
      ClassInfo *ci = m->getClass(_TIW(T));
      return new (Malloc(sizeof(TInstance))) TInstance(ci, const_ref);
    }

    virtual ~InstanceFactoryAdapter() {}
  };
}


#endif



#ifndef __SLB_CLASS_INFO__
#define __SLB_CLASS_INFO__

//->#include "Export.hpp"
//->#include "Object.hpp"
//->#include "Instance.hpp"
//->#include "Table.hpp"
//->#include "ref_ptr.hpp"
//->#include "FuncCall.hpp"
//->#include "String.hpp"
//->#include "Property.hpp"
////->#include "ClassHelpers.hpp"
#include <typeinfo>
#include <vector>

struct lua_State;

namespace SLB {

  class SLB_EXPORT Namespace : public Table
  {
  public:
    Namespace( bool cacheable = true ) : Table("::", cacheable) {}
  protected:
    virtual ~Namespace() {}
    SLB_CLASS(Namespace, Table);
  };

  class SLB_EXPORT ClassInfo : public Namespace
  {
  public:
    typedef SLB_Map(TypeInfoWrapper, ClassInfo* ) BaseClassMap;
    
    const TypeInfoWrapper &getTypeid() const { return __TIW; }
    const String &getName() const      { return _name; }
    void setName(const String&);

    void push_ref(lua_State *L, void *ref);
    void push_ptr(lua_State *L, void *ptr);
    void push_const_ptr(lua_State *L, const void *const_ptr);
    void push_copy(lua_State *L, const void *ptr);
    void* get_ptr(lua_State*, int pos) const;
    const void* get_const_ptr(lua_State*, int pos) const;

    // Uses dynamic_cast to convert from Base to Derived
    template<class This, class Base>
    void dynamicInheritsFrom();

    // This version uses static cast instead of dynamic_cast
    template<class This, class Base>
    void staticInheritsFrom();

    template<class This, class Other>
    void convertibleTo( Other* (*func)(This*) );


    void setConstructor( FuncCall *constructor );
    void setInstanceFactory( InstanceFactory *);

    /** __index method will receive:
     *  - object
     *  - key */
    void setObject__index( FuncCall* );

    /** __index method will receive:
     *  - object
     *  - key
     *  - value */
    void setObject__newindex( FuncCall* );

    /** Here you can use setCache/getCache methods to
     * speed up indexing.
     *
     * __index method will receive:
     *  - [table] -> ClassInfo
     *  - key */
    void setClass__index( FuncCall* );

    /** Here you can use setCache/getCache methods to
     * speed up indexing.
     * __index method will receive:
     *  - [table] -> ClassInfo
     *  - key
     *  - value */
    void setClass__newindex( FuncCall* );

    /** __eq method will receive to objects, and should return
      * true or false if those objects are equal or not. */
    void set__eq( FuncCall* );

    //This is used by some default initializations...
    bool initialized() const { return _instanceFactory != 0; }

    bool isSubClassOf( const ClassInfo* );
    bool hasConstructor() const { return _constructor.valid(); }

    //--Private methods -(not meant to be used)-------------------
    void setHybrid() { _isHybrid = true; }
    FuncCall* getConstructor() { return _constructor.get(); }

    // to add properties
    void addProperty(const String &name, BaseProperty *prop)
    {
      _properties[name] = prop;
    }

    BaseProperty* getProperty(const String &key);

    // const getters
    const BaseClassMap& getBaseClasses() const { return _baseClasses; }
    const FuncCall* getConstructor() const { return _constructor.get(); }

  protected:
    // Class Info are crated using manager->getOrCreateClass()
    ClassInfo(Manager *m, const TypeInfoWrapper &);
    virtual ~ClassInfo();
    void pushImplementation(lua_State *);
    virtual int __index(lua_State*);
    virtual int __newindex(lua_State*);
    virtual int __call(lua_State*);
    virtual int __garbageCollector(lua_State*);
    virtual int __tostring(lua_State*);
    virtual int __eq(lua_State *L);

    Manager          *_manager;
    TypeInfoWrapper   __TIW;
    String            _name;
    InstanceFactory  *_instanceFactory;
    BaseClassMap      _baseClasses;
    BaseProperty::Map _properties;
    ref_ptr<FuncCall> _constructor;
    ref_ptr<FuncCall> _meta__index[2];    // 0 = class, 1 = object
    ref_ptr<FuncCall> _meta__newindex[2]; // 0 = class, 1 = object
    ref_ptr<FuncCall> _meta__eq; 
    bool _isHybrid;

  private:
    void pushInstance(lua_State *L, InstanceBase *instance);
    InstanceBase* getInstance(lua_State *L, int pos) const;

    friend class Manager;
    SLB_CLASS(ClassInfo, Namespace);
  };


  //--------------------------------------------------------------------
  // Inline implementations:
  //--------------------------------------------------------------------
  
    
  template<class D, class B>
  inline void ClassInfo::dynamicInheritsFrom()
  {
    _manager->template addConversor<D,B>();
    ClassInfo *ci = _manager->getOrCreateClass(_TIW(B));
    assert( (ci != this) && "Circular reference between ClassInfo classes");
    _baseClasses[ _TIW(B) ] = ci;
  }

  template<class D, class B>
  inline void ClassInfo::staticInheritsFrom()
  {
    _manager->template addStaticConversor<D,B>();
    ClassInfo *ci = _manager->getOrCreateClass(_TIW(B));
    assert( (ci != this) && "Circular reference between ClassInfo classes");
    _baseClasses[ _TIW(B) ] = ci;
  }

  template<class This, class Other>
  inline void ClassInfo::convertibleTo( Other* (*func)(This*) )
  {
    /* This is a pretty ugly cast, SLB Manager handles changes from one type to other through
       void*, here we are changing the function to receive two pointers from the expected types (when the
       origin will be void*) It should be completely safe, as a pointer is the same size no matter the type. 

       Anyway, it's a pretty ugly trick. */
    typedef void* (*T_void)(void*);
    T_void aux_f = reinterpret_cast<T_void>(func);

    /* Once we've forced the conversion of the function we can let the manager know how to change from
       this type to the other one */
    _manager->template addClassConversor<This,Other>(aux_f);
    ClassInfo *ci = _manager->getOrCreateClass(_TIW(Other));
    assert( (ci != this) && "Circular reference between ClassInfo classes");
    _baseClasses[ _TIW(Other) ] = ci;
  }

}


#endif



#ifndef __SLB_PRIVATE_FUNC_CALL__
#define __SLB_PRIVATE_FUNC_CALL__

//->#include "SPP.hpp"
//->#include "Allocator.hpp"
//->#include "PushGet.hpp"
//->#include "ClassInfo.hpp"
//->#include "Manager.hpp"
//->#include "lua.hpp"
#include <typeinfo>


namespace SLB {
namespace Private {

//----------------------------------------------------------------------------
//-- FuncCall Implementations ------------------------------------------------
//----------------------------------------------------------------------------
  template<class T>
  class FC_Function; //> FuncCall to call functions (C static functions)

  template<class C, class T>
  class FC_Method; //> FuncCall to call Class methods

  template<class C, class T>
  class FC_ConstMethod; //> FuncCall to call Class const methods

  template<class C>
  struct FC_DefaultClassConstructor; //> FuncCall to create constructors

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

  // SLB_INFO: Collects info of the arguments
  #define SLB_INFO_PARAMS(N) _Targs.push_back(\
      std::pair<TypeInfoWrapper, String>( _TIW(T##N), "") ); 
  #define SLB_INFO(RETURN, N) \
    _Treturn = _TIW(RETURN);\
    SPP_REPEAT(N,SLB_INFO_PARAMS ) \

  // SLB_GET: Generates Code to get N parameters 
  //
  //    N       --> Number of parameters
  //    START   --> where to start getting parameters
  //                n=0   means start from the top
  //                n>0   start at top+n (i.e. with objects first parameter is the object)
  //
  //    For each paramter a param_n variable is generated with type Tn
  #define SLB_GET_PARAMS(N, START) typename SLB::Private::Type<T##N>::GetType param_##N = SLB::Private::Type<T##N>::get(L,N + (START) );
  #define SLB_GET(N,START) \
    if (lua_gettop(L) != N + (START) ) \
    { \
      luaL_error(L, "Error number of arguments (given %d -> expected %d)", \
          lua_gettop(L)-(START), N); \
    }\
    SPP_REPEAT_BASE(N,SLB_GET_PARAMS, (START) ) \
    

  // FC_Method (BODY) 
  //    N       --> Number of parameters
  //
  // ( if is a const method )
  //    NAME    --> FC_Method   |  FC_ConstMethod
  //    CONST   --> /*nothing*/ |  const 
  // 
  // ( if returns or not a value)
  //    HEADER  --> class R    |  /*nothing*/
  //    RETURN  --> R          |  void 
  //
  //    ...     --> implementation of call function
  #define SLB_FC_METHOD_BODY(N,NAME, CONST, HEADER,RETURN, ...) \
  \
    template<class C HEADER SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    class NAME <C,RETURN (SPP_ENUM_D(N,T))> : public FuncCall { \
    public: \
      NAME( RETURN (C::*func)(SPP_ENUM_D(N,T)) CONST ) : _func(func) \
      {\
        SLB_INFO(RETURN, N) \
      }\
    protected: \
      int call(lua_State *L) \
      { \
        __VA_ARGS__ \
      } \
    private: \
      RETURN (C::*_func)(SPP_ENUM_D(N,T)) CONST; \
    }; \


  // FC_Method (implementation with return or not a value):
  // ( if is a const method )
  //    NAME    --> FC_Method   |  FC_ConstMethod
  //    CONST   --> /*nothing*/ |  const 
  #define SLB_FC_METHOD(N, NAME, CONST) \
    SLB_FC_METHOD_BODY(N, NAME, CONST, SPP_COMMA class R ,R, \
        CONST C *obj = SLB::get<CONST C*>(L,1); \
        if (obj == 0) luaL_error(L, "Invalid object for this method");\
        SLB_GET(N,1) \
        R value = (obj->*_func)(SPP_ENUM_D(N,param_)); \
        SLB::push<R>(L, value); \
        return 1; \
      ) \
    SLB_FC_METHOD_BODY(N, NAME, CONST, /*nothing*/ , void,  \
        CONST C *obj = SLB::get<CONST C*>(L,1); \
        if (obj == 0) luaL_error(L, "Invalid object for this method");\
        SLB_GET(N,1) \
        (obj->*_func)(SPP_ENUM_D(N,param_)); \
        return 0; \
      )

  // FC_Method (with const methods or not)
  #define SLB_REPEAT(N) \
    SLB_FC_METHOD(N, FC_ConstMethod,  const) /* With const functions */ \
    SLB_FC_METHOD(N, FC_Method, /* nothing */ ) /* with non const functions */

  SPP_MAIN_REPEAT_Z(MAX,SLB_REPEAT)
  #undef SLB_REPEAT


  // FC_Function (Body)
  //    N       --> Number of parameters
  //
  // ( if returns or not a value)
  //    HEADER  --> class R    |  /*nothing*/
  //    RETURN  --> R          |  void 
  //
  //    ...     --> implementation of call function
  #define SLB_FC_FUNCTION_BODY(N, HEADER, RETURN, ...) \
  \
    template< HEADER  SPP_ENUM_D(N, class T)> \
    class FC_Function< RETURN (SPP_ENUM_D(N,T))> : public FuncCall { \
    public: \
      FC_Function( RETURN (*func)(SPP_ENUM_D(N,T)) ) : _func(func) {\
        SLB_INFO(RETURN, N) \
      } \
    protected: \
      virtual ~FC_Function() {} \
      int call(lua_State *L) \
      { \
        __VA_ARGS__ \
      } \
    private: \
      RETURN (*_func)(SPP_ENUM_D(N,T)); \
    }; 
  
  #define SLB_FC_FUNCTION(N) \
    SLB_FC_FUNCTION_BODY( N, class R SPP_COMMA_IF(N), R, \
        SLB_GET(N,0) \
        R value = (_func)(SPP_ENUM_D(N,param_)); \
        SLB::push<R>(L, value); \
        return 1; \
    )\
    SLB_FC_FUNCTION_BODY( N, /* nothing */ , void, \
        SLB_GET(N,0) \
        (_func)(SPP_ENUM_D(N,param_)); \
        return 0; \
    )

  SPP_MAIN_REPEAT_Z(MAX,SLB_FC_FUNCTION)
  #undef SLB_FC_METHOD
  #undef SLB_FC_METHOD_BODY
  #undef SLB_FC_FUNCTION
  #undef SLB_FC_FUNCTION_BODY

  #define SLB_REPEAT(N) \
    template<class C SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    struct FC_DefaultClassConstructor<C(SPP_ENUM_D(N,T))> : public FuncCall\
    {\
    public:\
      FC_DefaultClassConstructor() {} \
    protected: \
      int call(lua_State *L) \
      { \
        ClassInfo *c = Manager::getInstance(L)->getClass(_TIW(C)); \
        if (c == 0) luaL_error(L, "Class %s is not avaliable! ", _TIW(C).name()); \
        SLB_GET(N, 0); \
        Private::Type<C*>::push(L, new (Malloc(sizeof(C))) C(SPP_ENUM_D(N,param_))); \
        return 1; \
      } \
    }; \

  SPP_MAIN_REPEAT(MAX,SLB_REPEAT)
  #undef SLB_REPEAT

  // For C() like constructors (empty constructors)
  template<class C>
  struct FC_DefaultClassConstructor<C()> : public FuncCall
  {
  public:
    FC_DefaultClassConstructor() {}
  protected:
    int call(lua_State *L)
    {
      ClassInfo *c = Manager::getInstance(L)->getClass(_TIW(C));
      if (c == 0) luaL_error(L, "Class %s is not avaliable! ", _TIW(C).name());
      Private::Type<C*>::push(L, new (Malloc(sizeof(C))) C);
      return 1;
    }
  };


  #undef SLB_GET
  #undef SLB_GET_PARAMS


}} // end of SLB::Private namespace  

#endif



#ifndef __SLB_FUNCCALL_INLINE__
#define __SLB_FUNCCALL_INLINE__

//->#include "Private_FuncCall.hpp"

namespace SLB {

  #define SLB_REPEAT(N) \
  \
  template<class C,class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
  inline FuncCall* FuncCall::create(R (C::*func)(SPP_ENUM_D(N,T)) ) \
  { \
    return createNonConst(func); \
  } \
  \
  template<class C,class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
  inline FuncCall* FuncCall::create(R (C::*func)(SPP_ENUM_D(N,T)) const ) \
  { \
    return createConst(func); \
  } \
  \
  template<class C,class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
  inline FuncCall* FuncCall::createConst(R (C::*func)(SPP_ENUM_D(N,T)) const ) \
  { \
    typedef Private::FC_ConstMethod<C,R(SPP_ENUM_D(N,T))> _type_;\
    return new (Malloc(sizeof(_type_))) _type_(func); \
  } \
  template<class C,class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
  inline FuncCall* FuncCall::createNonConst(R (C::*func)(SPP_ENUM_D(N,T)) ) \
  { \
    typedef Private::FC_Method<C,R(SPP_ENUM_D(N,T))> _type_;\
    return new (Malloc(sizeof(_type_))) _type_(func); \
  } \
  \
  template<class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
  inline FuncCall* FuncCall::create(R (*func)(SPP_ENUM_D(N,T)) ) \
  { \
    typedef Private::FC_Function<R(SPP_ENUM_D(N,T))> _type_;\
    return new (Malloc(sizeof(_type_))) _type_(func); \
  } \
  \
  template<class C SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
  inline FuncCall* FuncCall::defaultClassConstructor() \
  { \
    typedef Private::FC_DefaultClassConstructor<C(SPP_ENUM_D(N,T))> _type_;\
    return new (Malloc(sizeof(_type_))) _type_; \
  } \
  
  SPP_MAIN_REPEAT_Z(MAX,SLB_REPEAT)
  #undef SLB_REPEAT

}
#endif



#ifndef __SLB_TYPE__
#define __SLB_TYPE__

//->#include "lua.hpp"
//->#include "Debug.hpp"
//->#include "SPP.hpp"
//->#include "Manager.hpp"
//->#include "ClassInfo.hpp"


namespace SLB {
namespace Private {

  // Default implementation
  template<class T>
  struct Type
  {
    typedef T GetType;

    static ClassInfo *getClass(lua_State *L)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"getClass '%s'", _TIW(T).name());
      ClassInfo *c = SLB::Manager::getInstance(L)->getClass(_TIW(T));
      if (c == 0) luaL_error(L, "Unknown class %s", _TIW(T).name());
      return c;
    }

    static void push(lua_State *L,const T &obj)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(8,"Push<T=%s>(L=%p, obj =%p)", _TIW(T).name(), L, &obj);
      getClass(L)->push_copy(L, (void*) &obj);
    }

    static T get(lua_State *L, int pos)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(8,"Get<T=%s>(L=%p, pos = %i)", _TIW(T).name(), L, pos);
      T* obj = reinterpret_cast<T*>( getClass(L)->get_ptr(L, pos) );  
      SLB_DEBUG(9,"obj = %p", obj);
      return *obj;
    }

  };

  template<class T>
  struct Type<T*>
  {
    typedef T* GetType;
    static ClassInfo *getClass(lua_State *L)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"getClass '%s'", _TIW(T).name());
      ClassInfo *c = SLB::Manager::getInstance(L)->getClass(_TIW(T));
      if (c == 0) luaL_error(L, "Unknown class %s", _TIW(T).name());
      return c;
    }

    static void push(lua_State *L, T *obj)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"push '%s' of %p",
          _TIW(T).name(),
          obj);

      if (obj == 0)
      {
        lua_pushnil(L);
        return;
      }

      /*TODO Change this for TypeInfoWrapper?
      const std::type_info &t_T = _TIW(T);
      const std::type_info &t_obj = _TIW(*obj);
      
      assert("Invalid typeinfo!!! (type)" && (&t_T) );
      assert("Invalid typeinfo!!! (object)" && (&t_obj) );

      if (t_obj != t_T)
      {
        //Create TIW
        TypeInfoWrapper wt_T = _TIW(T);
        TypeInfoWrapper wt_obj = _TIW(*obj);
        // check if the internal class exists...
        ClassInfo *c = SLB::Manager::getInstance(L)->getClass(wt_obj);
        if ( c ) 
        {
          SLB_DEBUG(8,"Push<T*=%s> with conversion from "
            "T(%p)->T(%p) (L=%p, obj =%p)",
            c->getName().c_str(), t_obj.name(), t_T.name(),L, obj);
          // covert the object to the internal class...
          void *real_obj = SLB::Manager::getInstance(L)->convert( wt_T, wt_obj, obj );
          c->push_ptr(L, real_obj, fromConstructor);
          return;
        }
      }*/
      // use this class...  
      getClass(L)->push_ptr(L, (void*) obj);
    }

    static T* get(lua_State *L, int pos)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"get '%s' at pos %d", _TIW(T).name(), pos);
      return reinterpret_cast<T*>( getClass(L)->get_ptr(L, pos) );
    }

  };
  
  template<class T>
  struct Type<const T*>
  {
    typedef const T* GetType;
    static ClassInfo *getClass(lua_State *L)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"getClass '%s'", _TIW(T).name());
      ClassInfo *c = SLB::Manager::getInstance(L)->getClass(_TIW(T));
      if (c == 0) luaL_error(L, "Unknown class %s", _TIW(T).name());
      return c;
    }

    static void push(lua_State *L,const T *obj)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"push '%s' of %p", _TIW(T).name(), obj);
      if (obj == 0)
      {
        lua_pushnil(L);
        return;
      }
      /*
      if (_TIW(*obj) != _TIW(T))
      {
        //Create TIW
        TypeInfoWrapper wt_T = _TIW(T);
        TypeInfoWrapper wt_obj = _TIW(*obj);
        // check if the internal class exists...
        ClassInfo *c = SLB::Manager::getInstance(L)->getClass(wt_obj);
        if ( c ) 
        {
          SLB_DEBUG(8,"Push<const T*=%s> with conversion from "
            "T(%p)->T(%p) (L=%p, obj =%p)",
            c->getName().c_str(), typeid(*obj).name(), _TIW(T).name(),L, obj);
          // covert the object to the internal class...
          const void *real_obj = SLB::Manager::getInstance(L)->convert( wt_T, wt_obj, obj );
          c->push_const_ptr(L, real_obj);
          return;
        }
      }
      */
      getClass(L)->push_const_ptr(L, (const void*) obj);
    }

    static const T* get(lua_State *L, int pos)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"get '%s' at pos %d", _TIW(T).name(), pos);
      return reinterpret_cast<const T*>( getClass(L)->get_const_ptr(L, pos) );
    }

  };

  template<class T>
  struct Type<const T&>
  {
    typedef const T& GetType;
    static void push(lua_State *L,const T &obj)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"push '%s' of %p(const ref)", _TIW(T).name(), &obj);
      Type<const T*>::push(L, &obj);
    }

    static const T& get(lua_State *L, int pos)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"get '%s' at pos %d", _TIW(T).name(), pos);
      const T* obj = Type<const T*>::get(L,pos);
      //TODO: remove the _TIW(T).getName() and use classInfo :)
      if (obj == 0L) luaL_error(L, "Can not get a reference of class %s", _TIW(T).name());
      return *(obj);
    }

  };
  
  template<class T>
  struct Type<T&>
  {
    typedef T& GetType;
    static ClassInfo *getClass(lua_State *L)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"getClass '%s'", _TIW(T).name());
      ClassInfo *c = SLB::Manager::getInstance(L)->getClass(_TIW(T));
      if (c == 0) luaL_error(L, "Unknown class %s", _TIW(T).name());
      return c;
    }

    static void push(lua_State *L,T &obj)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"push '%s' of %p (reference)", _TIW(T).name(), &obj);
      getClass(L)->push_ref(L, (void*) &obj);
    }

    static T& get(lua_State *L, int pos)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(10,"get '%s' at pos %d", _TIW(T).name(), pos);
      return *(Type<T*>::get(L,pos));
    }

  };

  //--- Specializations ---------------------------------------------------

  template<>
  struct Type<void*>
  {
    typedef void* GetType;
    static void push(lua_State *L,void* obj)
    {
      SLB_DEBUG_CALL; 
      if (obj == 0) lua_pushnil(L);
      else
      {
        SLB_DEBUG(8,"Push<void*> (L=%p, obj =%p)",L, obj);
        lua_pushlightuserdata(L, obj);
      }
    }

    static void *get(lua_State *L, int pos)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(8,"Get<void*> (L=%p, pos=%i ) =%p)",L, pos, lua_touserdata(L,pos));
      if (lua_islightuserdata(L,pos)) return lua_touserdata(L,pos);
      //TODO: Check here if is an userdata and convert it to void
      return 0;
    }

  };

  // Type specialization for <char>
  template<>
  struct Type<char>
  {
    typedef char GetType;
    static void push(lua_State *L, char v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push char = %d",v);
      lua_pushinteger(L,v);
    }
    static char get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      char v = (char) lua_tointeger(L,p);
      SLB_DEBUG(6,"Get char (pos %d) = %d",p,v);
      return v;
    }
  };
  template<> struct Type<char&> : public Type<char> {};
  template<> struct Type<const char&> : public Type<char> {};

  // Type specialization for <char>
  template<>
  struct Type<unsigned char>
  {
    typedef unsigned char GetType;
    static void push(lua_State *L, unsigned char v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push unsigned char = %d",v);
      lua_pushinteger(L,v);
    }
    static unsigned char get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      unsigned char v = (unsigned char) lua_tointeger(L,p);
      SLB_DEBUG(6,"Get unsigned char (pos %d) = %d",p,v);
      return v;
    }
  };
  template<> struct Type<unsigned char&> : public Type<unsigned char> {};
  template<> struct Type<const unsigned char&> : public Type<unsigned char> {};

  // Type specialization for <short>
  template<>
  struct Type<short>
  {
    typedef short GetType;
    static void push(lua_State *L, short v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push short = %d",v);
      lua_pushinteger(L,v);
    }
    static short get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      short v = (short) lua_tointeger(L,p);
      SLB_DEBUG(6,"Get short (pos %d) = %d",p,v);
      return v;
    }
  };

  template<> struct Type<short&> : public Type<short> {};
  template<> struct Type<const short&> : public Type<short> {};

  // Type specialization for <unsigned short>
  template<>
  struct Type<unsigned short>
  {
    typedef unsigned short GetType;

    static void push(lua_State *L, unsigned short v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push unsigned short = %d",v);
      lua_pushinteger(L,v);
    }
    static unsigned short get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      unsigned short v = (unsigned short) lua_tointeger(L,p);
      SLB_DEBUG(6,"Get unsigned short (pos %d) = %d",p,v);
      return v;
    }
  };

  template<> struct Type<unsigned short&> : public Type<unsigned short> {};
  template<> struct Type<const unsigned short&> : public Type<unsigned short> {};

  // Type specialization for <int>
  template<>
  struct Type<int>
  {
    typedef int GetType;
    static void push(lua_State *L, int v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push integer = %d",v);
      lua_pushinteger(L,v);
    }
    static int get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      int v = (int) lua_tointeger(L,p);
      SLB_DEBUG(6,"Get integer (pos %d) = %d",p,v);
      return v;
    }
  };

  template<> struct Type<int&> : public Type<int> {};
  template<> struct Type<const int&> : public Type<int> {};

  // Type specialization for <unsigned int>
  template<>
  struct Type<unsigned int>
  {
    typedef unsigned int GetType;
    static void push(lua_State *L, unsigned int v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push unsigned integer = %d",v);
      lua_pushinteger(L,v);
    }
    static unsigned int get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      unsigned int v = static_cast<unsigned int>(lua_tointeger(L,p));
      SLB_DEBUG(6,"Get unsigned integer (pos %d) = %d",p,v);
      return v;
    }

  };

  template<> struct Type<unsigned int&> : public Type<unsigned int> {};
  template<> struct Type<const unsigned int&> : public Type<unsigned int> {};
  

  template<>
  struct Type<long>
  {
    typedef long GetType;
    static void push(lua_State *L, long v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push long = %ld",v);
      lua_pushinteger(L,v);
    }
    static long get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      long v = (long) lua_tointeger(L,p);
      SLB_DEBUG(6,"Get long (pos %d) = %ld",p,v);
      return v;
    }

  };

  template<> struct Type<long&> : public Type<long> {};
  template<> struct Type<const long&> : public Type<long> {};
  

  /* unsigned long == unsigned int */
  template<>
  struct Type<unsigned long>
  {
    typedef unsigned long GetType;
    static void push(lua_State *L, unsigned long v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push unsigned long = %lu",v);
      lua_pushnumber(L,v);
    }

    static unsigned long get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      unsigned long v = (unsigned long) lua_tonumber(L,p);
      SLB_DEBUG(6,"Get unsigned long (pos %d) = %lu",p,v);
      return v;
    }

  };

  template<> struct Type<unsigned long&> : public Type<unsigned long> {};
  template<> struct Type<const unsigned long&> : public Type<unsigned long> {};
  

  template<>
  struct Type<unsigned long long>
  {
    typedef unsigned long long GetType;
    static void push(lua_State *L, unsigned long long v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push unsigned long long = %llu",v);
      lua_pushnumber(L,(lua_Number) v);
    }

    static unsigned long long get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      unsigned long long v = (unsigned long long) lua_tonumber(L,p);
      SLB_DEBUG(6,"Get unsigned long long (pos %d) = %llu",p,v);
      return v;
    }
  };

  template<> struct Type<unsigned long long&> : public Type<unsigned long long> {};
  template<> struct Type<const unsigned long long&> : public Type<unsigned long long> {};
  
  // Type specialization for <double>
  template<>
  struct Type<double>
  {
    typedef double GetType;
    static void push(lua_State *L, double v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push double = %f",v);
      lua_pushnumber(L,v);
    }
    static double get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      double v = (double) lua_tonumber(L,p);
      SLB_DEBUG(6,"Get double (pos %d) = %f",p,v);
      return v;
    }

  };

  template<> struct Type<double&> : public Type<double> {};
  template<> struct Type<const double&> : public Type<double> {};
  
  // Type specialization for <float>
  template<>
  struct Type<float>
  {
    typedef float GetType;
    static void push(lua_State *L, float v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push float = %f",v);
      lua_pushnumber(L,v);
    }

    static float get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      float v = (float) lua_tonumber(L,p);
      SLB_DEBUG(6,"Get float (pos %d) = %f",p,v);
      return v;
    }

  };

  template<> struct Type<float&> : public Type<float> {};
  template<> struct Type<const float&> : public Type<float> {};
  
  
  // Type specialization for <bool>
  template<>
  struct Type<bool>
  {
    typedef bool GetType;
    static void push(lua_State *L, bool v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push bool = %d",(int)v);
      lua_pushboolean(L,v);
    }
    static bool get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      bool v = (lua_toboolean(L,p) != 0);
      SLB_DEBUG(6,"Get bool (pos %d) = %d",p,v);
      return v;
    }
  };


  template<> struct Type<bool&> : public Type<bool> {};
  template<> struct Type<const bool&> : public Type<bool> {};

  template<>
  struct Type<std::string>
  {
    typedef std::string GetType;
    static void push(lua_State *L, const std::string &v)
    {
      SLB_DEBUG_CALL; 
      SLB_DEBUG(6, "Push const std::string& = %s",v.c_str());
      lua_pushlstring(L, v.data(), v.size());
    }

    static std::string get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      size_t len;
      const char* v = (const char*) lua_tolstring(L,p, &len);
      SLB_DEBUG(6,"Get std::string (pos %d) = %s",p,v);
      return std::string(v, len);
    }

  };

  template<> struct Type<std::string&> : public Type<std::string> {};
  template<> struct Type<const std::string&> : public Type<std::string> {};


  // Type specialization for <const char*>
  template<>
  struct Type<const char*>
  {
    typedef const char* GetType;
    static void push(lua_State *L, const char* v)
    {
      SLB_DEBUG_CALL; 
      if (v)
      {
        SLB_DEBUG(6, "Push const char* = %s",v);
        lua_pushstring(L,v);
      }
      else
      {
        SLB_DEBUG(6, "Push const char* = NULL");
        lua_pushnil(L);
      }
    }

    static const char* get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      const char* v = (const char*) lua_tostring(L,p);
      SLB_DEBUG(6,"Get const char* (pos %d) = %s",p,v);
      return v;
    }

  };

  template<>
  struct Type<const unsigned char*>
  {
    typedef const unsigned char* GetType;
    static void push(lua_State *L, const unsigned char* v)
    {
      SLB_DEBUG_CALL; 
      if (v)
      {
        SLB_DEBUG(6, "Push const unsigned char* = %s",v);
        lua_pushstring(L,(const char*)v);
      }
      else
      {
        SLB_DEBUG(6, "Push const unsigned char* = NULL");
        lua_pushnil(L);
      }
    }

    static const unsigned char* get(lua_State *L, int p)
    {
      SLB_DEBUG_CALL; 
      const unsigned char* v = (const unsigned char*) lua_tostring(L,p);
      SLB_DEBUG(6,"Get const unsigned char* (pos %d) = %s",p,v);
      return v;
    }

  };

}} // end of SLB::Private

#endif



#ifndef __SLB_HYBRID__
#define __SLB_HYBRID__

//->#include "Export.hpp"
//->#include "Allocator.hpp"
//->#include "SPP.hpp"
//->#include "Manager.hpp"
//->#include "LuaCall.hpp"
//->#include "Value.hpp"
//->#include "ClassInfo.hpp"
//->#include "Instance.hpp"
//->#include "Mutex.hpp"
#include <typeinfo>
#include <map>
#include <vector>
#include <string>

struct lua_State;

namespace SLB {

  class HybridBase;

  struct SLB_EXPORT InvalidMethod : public std::exception
  {  
    InvalidMethod(const HybridBase*, const char *c);
    ~InvalidMethod() throw() {}
    const char* what() const throw() { return _what.c_str(); }
    std::string _what;
  };

  class SLB_EXPORT HybridBase {
  public:

    /** Returns the lua_State, this function will be valid if the object is
     * attached, otherwise will return 0 */
    virtual lua_State* getLuaState() const { return _lua_state; }

    /** Indicates where this instance will look for its hybrid methods; If you reimplement
     this method remember to call the parent (HybridBase) to set _lua_state properly and register
     itself there.*/
    virtual void attach(lua_State *);
    virtual bool isAttached() const { return (_lua_state != 0); }

    /** use this to release memory allocated by the hybrid object, inside
     * the lua_State.*/
    void unAttach(); 

    /** Use this function to register this class as hybrid, it will override
     * ClassInfo metamethods of class__index, class__newindex, object__index and
         * object__newindex. 
     * Note: if your class requires those methods contact me to see if it is possible
     * to do it, by the moment this is the only way this works */
    static void registerAsHybrid(ClassInfo *ci);

  protected:
    typedef SLB_Map(String, LuaCallBase *) MethodMap;

    HybridBase();
    virtual ~HybridBase();

    //-- Private data -----------------------------------------------------
    // [-1, (0|+1)]
    bool getMethod(const char *name) const;
    virtual ClassInfo* getClassInfo() const = 0;
    void clearMethodMap();


    mutable MethodMap _methods;
    mutable ref_ptr<Table> _subclassMethods;

    friend struct InternalHybridSubclass;
    friend struct InvalidMethod;
    //-- Private data -----------------------------------------------------

  private:
    lua_State * _lua_state;
    int _data; //< lua ref to internal data

    // pops a key,value from tom and sets as our method
    // [-2,0]
    static void setMethod(lua_State *L, ClassInfo *ci);

    static int call_lua_method(lua_State *L);
    static int class__newindex(lua_State *);
    static int class__index(lua_State *);
    static int object__index(lua_State *);
    static int object__newindex(lua_State *);

  protected:
    mutable Mutex _mutex;
  };

  template<class BaseClass, class T_CriticalSection = ActiveWaitCriticalSection >
  class Hybrid : public virtual HybridBase {
  public:
    Hybrid(Manager* mgr = Manager::defaultManager())
      : _mgr(mgr)
    {
      ClassInfo *c;
      c = _mgr->getOrCreateClass( _TIW(BaseClass) );
      if (!c->initialized())
      {
        // Give a default instance factory... that only is able
        // to handle push/get of pointers without handling 
        // construction, copy, delete, ...
        typedef InstanceFactoryAdapter< BaseClass,
          Instance::NoCopyNoDestroy::Implementation<BaseClass> > t_IFA;
        c->setInstanceFactory( new (Malloc(sizeof(t_IFA))) t_IFA);
      }
    }
  private:
    Manager* _mgr;
  protected:
    virtual ~Hybrid() {}
    ClassInfo* getClassInfo() const
    {
      return _mgr->getClass( _TIW(BaseClass) );
    }
    
  #define SLB_ARG_H(N) ,T##N arg_##N
  #define SLB_ARG(N) , arg_##N
  #define SLB_BODY(N) \
      \
      T_CriticalSection __dummy__lock(&_mutex); \
      LC *method = 0; \
      SLB_DEBUG(3,"Call Hybrid-method [%s]", name)\
      lua_State *L = getLuaState(); \
      {\
        SLB_DEBUG_CLEAN_STACK(L,0)\
        MethodMap::iterator it = _methods.find(name) ; \
        if (it != _methods.end()) \
        { \
          method = reinterpret_cast<LC*>(it->second); \
          SLB_DEBUG(4,"method [%s] was found %p", name,method)\
        } \
        else \
        { \
          if (getMethod(name)) \
          { \
            method = new (Malloc(sizeof(LC))) LC(L, -1);\
            lua_pop(L,1); /*method is stored in the luaCall*/\
            SLB_DEBUG(2,"method [%s] found in lua [OK] -> %p", name,method)\
            _methods[name] = method;\
          } \
          else \
          { \
            _methods[name] = 0L; \
            SLB_DEBUG(2,"method [%s] found in lua [FAIL!]", name)\
          }\
        }\
        if (!method) {\
          SLB_THROW(InvalidMethod(this, name));\
          SLB_CRITICAL_ERROR("Invalid method")\
        }\
      }\

  #define SLB_REPEAT(N) \
  \
    /* non const version */\
    template<class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    R LCall( const char *name SPP_REPEAT(N, SLB_ARG_H) ) \
    { \
      SLB_DEBUG_CALL;\
      typedef SLB::LuaCall<R(BaseClass* SPP_COMMA_IF(N) SPP_ENUM_D(N,T))> LC;\
      SLB_BODY(N) \
      return (*method)(static_cast<BaseClass*>(this) SPP_REPEAT(N, SLB_ARG) ); \
    } \
    /* const version */\
    template<class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    R LCall( const char *name SPP_REPEAT(N, SLB_ARG_H) ) const \
    { \
      SLB_DEBUG_CALL;\
      typedef SLB::LuaCall<R(const BaseClass* SPP_COMMA_IF(N) SPP_ENUM_D(N,T))> LC;\
      SLB_BODY(N) \
      return (*method)(static_cast<const BaseClass*>(this) SPP_REPEAT(N, SLB_ARG) ); \
    } \
    /* (VOID) non const version */\
    SPP_IF(N,template<SPP_ENUM_D(N, class T)>) \
    void void_LCall( const char *name SPP_REPEAT(N, SLB_ARG_H) ) \
    { \
      SLB_DEBUG_CALL;\
      typedef SLB::LuaCall<void(BaseClass* SPP_COMMA_IF(N) SPP_ENUM_D(N,T))> LC;\
      SLB_BODY(N) \
      (*method)(static_cast<BaseClass*>(this) SPP_REPEAT(N, SLB_ARG) ); \
    } \
    /* (VOID) const version */\
    SPP_IF(N, template<SPP_ENUM_D(N, class T)>) \
    void void_LCall( const char *name SPP_REPEAT(N, SLB_ARG_H) ) const \
    { \
      SLB_DEBUG_CALL;\
      typedef SLB::LuaCall<void(const BaseClass* SPP_COMMA_IF(N) SPP_ENUM_D(N,T))> LC;\
      SLB_BODY(N) \
      return (*method)(static_cast<const BaseClass*>(this) SPP_REPEAT(N, SLB_ARG) ); \
    } \

  SPP_MAIN_REPEAT_Z(MAX,SLB_REPEAT)
  #undef SLB_REPEAT
  #undef SLB_BODY
  #undef SLB_ARG
  #undef SLB_ARG_H
  };
}

#define HYBRID_method_0(name,ret_T) \
  ret_T name() { SLB_DEBUG_CALL; return LCall<ret_T>(#name); }
#define HYBRID_method_1(name,ret_T, T1) \
  ret_T name(T1 p1) { SLB_DEBUG_CALL; return LCall<ret_T,T1>(#name,p1); }
#define HYBRID_method_2(name,ret_T, T1, T2) \
  ret_T name(T1 p1,T2 p2) { SLB_DEBUG_CALL; return LCall<ret_T,T1,T2>(#name,p1,p2); }
#define HYBRID_method_3(name,ret_T, T1, T2, T3) \
  ret_T name(T1 p1,T2 p2, T3 p3) { SLB_DEBUG_CALL; return LCall<ret_T,T1,T2,T3>(#name,p1,p2, p3); }
#define HYBRID_method_4(name,ret_T, T1, T2, T3, T4) \
  ret_T name(T1 p1,T2 p2, T3 p3, T4 p4) {  SLB_DEBUG_CALL; return LCall<ret_T,T1,T2,T3,T4>(#name,p1,p2, p3,p4); }
#define HYBRID_method_5(name,ret_T, T1, T2, T3, T4,T5) \
  ret_T name(T1 p1,T2 p2, T3 p3, T4 p4, T5 p5) {  SLB_DEBUG_CALL; return LCall<ret_T,T1,T2,T3,T4,T5>(#name,p1,p2, p3,p4,p5); }

#define HYBRID_const_method_0(name,ret_T) \
  ret_T name() const {  SLB_DEBUG_CALL; return LCall<ret_T>(#name); }
#define HYBRID_const_method_1(name,ret_T, T1) \
  ret_T name(T1 p1) const {  SLB_DEBUG_CALL; return LCall<ret_T,T1>(#name,p1); }
#define HYBRID_const_method_2(name,ret_T, T1, T2) \
  ret_T name(T1 p1,T2 p2) const {  SLB_DEBUG_CALL; return LCall<ret_T,T1,T2>(#name,p1,p2); }
#define HYBRID_const_method_3(name,ret_T, T1, T2, T3) \
  ret_T name(T1 p1,T2 p2, T3 p3) const {  SLB_DEBUG_CALL; return LCall<ret_T,T1,T2,T3>(#name,p1,p2, p3); }
#define HYBRID_const_method_4(name,ret_T, T1, T2, T3, T4) \
  ret_T name(T1 p1,T2 p2, T3 p3, T4 p4) const {  SLB_DEBUG_CALL; return LCall<ret_T,T1,T2,T3,T4>(#name,p1,p2, p3,p4); }
#define HYBRID_const_method_5(name,ret_T, T1, T2, T3, T4,T5) \
  ret_T name(T1 p1,T2 p2, T3 p3, T4 p4, T5 p5) const {  SLB_DEBUG_CALL; return LCall<ret_T,T1,T2,T3,T4,T5>(#name,p1,p2, p3,p4,p5); }

#define HYBRID_void_method_0(name) \
  void name() { SLB_DEBUG_CALL; return void_LCall(#name); }
#define HYBRID_void_method_1(name, T1) \
  void name(T1 p1) { SLB_DEBUG_CALL; return void_LCall<T1>(#name,p1); }
#define HYBRID_void_method_2(name, T1, T2) \
  void name(T1 p1,T2 p2) { SLB_DEBUG_CALL; return void_LCall<T1,T2>(#name,p1,p2); }
#define HYBRID_void_method_3(name, T1, T2, T3) \
  void name(T1 p1,T2 p2, T3 p3) { SLB_DEBUG_CALL; return void_LCall<T1,T2,T3>(#name,p1,p2, p3); }
#define HYBRID_void_method_4(name, T1, T2, T3, T4) \
  void name(T1 p1,T2 p2, T3 p3, T4 p4) {  SLB_DEBUG_CALL; return void_LCall<T1,T2,T3,T4>(#name,p1,p2, p3,p4); }
#define HYBRID_void_method_5(name, T1, T2, T3, T4,T5) \
  void name(T1 p1,T2 p2, T3 p3, T4 p4, T5 p5) {  SLB_DEBUG_CALL; return void_LCall<T1,T2,T3,T4,T5>(#name,p1,p2, p3,p4,p5); }

#define HYBRID_const_void_method_0(name) \
  void name() const {  SLB_DEBUG_CALL; return void_LCall(#name); }
#define HYBRID_const_void_method_1(name, T1) \
  void name(T1 p1) const {  SLB_DEBUG_CALL; return void_LCall<T1>(#name,p1); }
#define HYBRID_const_void_method_2(name, T1, T2) \
  void name(T1 p1,T2 p2) const {  SLB_DEBUG_CALL; return void_LCall<T1,T2>(#name,p1,p2); }
#define HYBRID_const_void_method_3(name, T1, T2, T3) \
  void name(T1 p1,T2 p2, T3 p3) const {  SLB_DEBUG_CALL; return void_LCall<T1,T2,T3>(#name,p1,p2, p3); }
#define HYBRID_const_void_method_4(name, T1, T2, T3, T4) \
  void name(T1 p1,T2 p2, T3 p3, T4 p4) const {  SLB_DEBUG_CALL; return void_LCall<T1,T2,T3,T4>(#name,p1,p2, p3,p4); }
#define HYBRID_const_void_method_5(name, T1, T2, T3, T4,T5) \
  void name(T1 p1,T2 p2, T3 p3, T4 p4, T5 p5) const {  SLB_DEBUG_CALL; return void_LCall<T1,T2,T3,T4,T5>(#name,p1,p2, p3,p4,p5); }

#endif


#ifndef __SLB_CLASS__
#define __SLB_CLASS__

//->#include "SPP.hpp"
//->#include "Allocator.hpp"
//->#include "Export.hpp"
//->#include "Debug.hpp"
//->#include "ClassInfo.hpp"
//->#include "ClassHelpers.hpp"
//->#include "Manager.hpp"
//->#include "FuncCall.hpp"
//->#include "Value.hpp"
//->#include "Instance.hpp"
//->#include "Iterator.hpp"
//->#include "Hybrid.hpp"
//->#include "String.hpp"
#include <typeinfo>
#include <map>
#include <vector>

#include <iostream>

struct lua_State;

namespace SLB {
  
  struct ClassBase
  {
    ClassBase() {}
    virtual ~ClassBase() {}
  };

  template< typename T, typename W = Instance::Default >
  class Class : public ClassBase {
  public:
    typedef Class<T,W> __Self;

    Class(const char *name, Manager *m = Manager::defaultManager());
    Class(const Class&);
    Class& operator=(const Class&);

    __Self &rawSet(const char *name, Object *obj);

    template<typename TValue>
    __Self &set(const char *name, const TValue &obj)
    { return rawSet(name, (Object*) Value::copy(obj)); }

    template<typename TValue>
    __Self &set_ref(const char *name, TValue& obj)
    { return rawSet(name, Value::ref(obj)); }

    template<typename TValue>
    __Self &set_autoDelete(const char *name, TValue *obj)
    { return rawSet(name, Value::autoDelete(obj)); }

    __Self &set(const char *name, lua_CFunction func)
    { return rawSet(name, FuncCall::create(func)); }

    template<typename TEnum>
    __Self &enumValue(const char *name, TEnum obj);

    __Self &constructor();


    /** Declares a class as hybrid, this will imply that the __index
     * and __newindex methods will be overriden, see 
     * Hybrid::registerAsHybrid */
    __Self &hybrid()
    {
      dynamic_inherits<HybridBase>();
      HybridBase::registerAsHybrid( _class );
      return *this;
    }

    template<typename TBase>
    __Self &dynamic_inherits()
    { _class->dynamicInheritsFrom<T,TBase>(); return *this;}

    template<typename TBase>
    __Self &static_inherits()
    { _class->staticInheritsFrom<T,TBase>(); return *this;}

    template<typename TBase>
    __Self &inherits()
    { return static_inherits<TBase>(); }

    template<typename T_to>
    __Self &convertibleTo( T_to* (*func)(T*)  = &(ClassConversor<T,T_to>::defaultConvert) )
    {
      _class->convertibleTo<T,T_to>(func);
      return *this;
    }

    template<typename M>
    __Self &property(const char *name, M T::*member)
    {
      _class->addProperty(name, BaseProperty::create(member));
      return *this;
    }

    /* Class__index for (non-const)methods */
    template<class C, class R, class P>
    __Self &class__index( R (C::*func)(P) )
    {
      _class->setClass__index( FuncCall::create(func) ); return *this;
    }

    /* Class__index for const methods */
    template<class C, class R, class P>
    __Self &class__index( R (C::*func)(P) const )
    {
      _class->setClass__index( FuncCall::create(func) ); return *this;
    }

    /* Class__index for C functions */
    template<class R, class P>
    __Self &class__index( R (*func)(P) )
    {
      _class->setClass__index( FuncCall::create(func) ); return *this;
    }
    
    /* Class__index for lua_functions */
    __Self &class__index(lua_CFunction func)
    {
      _class->setClass__index( FuncCall::create(func) ); return *this;
    }

    /* Class__newindex for (non-const)methods */
    template<class C, class R, class K, class V>
    __Self &class__newindex( R (C::*func)(K,V) )
    {
      _class->setClass__newindex( FuncCall::create(func) ); return *this;
    }

    /* Class__newindex for const methods */
    template<class C, class R, class K, class V>
    __Self &class__newindex( R (C::*func)(K,V) const )
    {
      _class->setClass__newindex( FuncCall::create(func) ); return *this;
    }

    /* Class__newindex for C functions */
    template<class R, class K, class V>
    __Self &class__newindex( R (*func)(K,V) )
    {
      _class->setClass__newindex( FuncCall::create(func) ); return *this;
    }
    
    /* Class__newindex for lua_functions */
    __Self &class__newindex(lua_CFunction func)
    {
      _class->setClass__newindex( FuncCall::create(func) ); return *this;
    }

    /* Object__index for (non-const)methods */
    template<class C, class R, class P>
    __Self &object__index( R (C::*func)(P) )
    {
      _class->setObject__index( FuncCall::create(func) ); return *this;
    }

    /* Object__index for const methods */
    template<class C, class R, class P>
    __Self &object__index( R (C::*func)(P) const )
    {
      _class->setObject__index( FuncCall::create(func) ); return *this;
    }

    /* Object__index for C functions */
    template<class R, class P>
    __Self &object__index( R (*func)(P) )
    {
      _class->setObject__index( FuncCall::create(func) ); return *this;
    }
    
    /* Object__index for lua_functions */
    __Self &object__index(lua_CFunction func)
    {
      _class->setObject__index( FuncCall::create(func) ); return *this;
    }

    /* Object__newindex for (non-const)methods */
    template<class C, class R, class K, class V>
    __Self &object__newindex( R (C::*func)(K,V) )
    {
      _class->setObject__newindex( FuncCall::create(func) ); return *this;
    }

    /* Object__newindex for const methods */
    template<class C, class R, class K, class V>
    __Self &object__newindex( R (C::*func)(K,V) const )
    {
      _class->setObject__newindex( FuncCall::create(func) ); return *this;
    }

    /* Object__newindex for C functions */
    template<class R, class K, class V>
    __Self &object__newindex( R (*func)(K,V) )
    {
      _class->setObject__newindex( FuncCall::create(func) ); return *this;
    }
    
    /* Object__newindex for lua_functions */
    __Self &object__newindex(lua_CFunction func)
    {
      _class->setObject__newindex( FuncCall::create(func) ); return *this;
    }

    template<class T1, class T2>
    __Self &__eq( bool (*func)(T1,T2) )
    {
      _class->setObject__newindex( FuncCall::create(func) ); return *this;
    }

    __Self &__eq(lua_CFunction func)
    {
      _class->set__eq( FuncCall::create(func) ); return *this;
    }
    
    __Self &__add()
    { SLB_DEBUG_CALL; SLB_DEBUG(0, "NOT IMPLEMENTED!"); return *this; }

    __Self &__mult()
    { SLB_DEBUG_CALL; SLB_DEBUG(0, "NOT IMPLEMENTED!"); return *this; }

    template<class IT> /* IT == Iterator Traits */
    __Self &customIterator(  const char *name,
      typename IT::GetIteratorMember first,
      typename IT::GetIteratorMember end )
    {
      Iterator* it;
      StdIterator< IT >* sit;
      sit = new (Malloc(sizeof(StdIterator<IT>))) StdIterator<IT>(first, end);
      it = new (Malloc(sizeof(Iterator))) Iterator(sit);
      return rawSet(name, it );
    }

    template<typename C, typename T_Iterator>
    __Self &iterator(const char *name,
        T_Iterator (C::*first)(),
        T_Iterator (C::*end)() )
    {
      return customIterator< StdIteratorTraits<C, T_Iterator> >
        (name,first, end ) ;
    }

    template<typename C, typename T_Iterator>
    __Self &const_iterator(const char *name,
        T_Iterator (C::*first)() const,
        T_Iterator (C::*end)() const )
    {
      return customIterator< StdConstIteratorTraits<C, T_Iterator> >
        (name,first, end ) ;
    }

    // Metada
    __Self &comment(const String&);
    __Self &param(const String&);

    #define SLB_REPEAT(N) \
    \
      /* Methods */ \
      template<class C, class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      __Self &set(const char *name, R (C::*func)(SPP_ENUM_D(N,T)) ); \
      template<class C, class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      __Self &nonconst_set(const char *name, R (C::*func)(SPP_ENUM_D(N,T)) ); \
    \
      /* CONST Methods */ \
      template<class C, class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      __Self &set(const char *name, R (C::*func)(SPP_ENUM_D(N,T)) const ); \
      template<class C, class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      __Self &const_set(const char *name, R (C::*func)(SPP_ENUM_D(N,T)) const); \
    \
      /* C-functions  */ \
      template<class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      __Self &set(const char *name, R (func)(SPP_ENUM_D(N,T)) ); \
    \
      /* constructors */ \
      template<class T0 SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      __Self &constructor(); \
    \
      /* Constructors (wrappers to c-functions)  */ \
      template<class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
      __Self &constructor(R (func)(SPP_ENUM_D(N,T)) ); \

    SPP_MAIN_REPEAT_Z(MAX,SLB_REPEAT)
    #undef SLB_REPEAT

  protected:
    ClassInfo *_class;
    // For metadata
    Object *_lastObj;
    size_t _param;
    Manager* _mgr;

  };
  
  template<typename T, typename W>
  inline Class<T,W>::Class(const char *name, Manager *m)
    : _class(0), _lastObj(0), _param(0), _mgr(m)
  {
    SLB_DEBUG_CALL;
    // we expect to have a template "Implementation" inside W
    typedef typename W::template Implementation<T> Adapter;
    _class = m->getOrCreateClass( _TIW(T) );
    _class->setName( name );
    typedef InstanceFactoryAdapter< T, Adapter > t_IFA;
    _class->setInstanceFactory( new (Malloc(sizeof(t_IFA))) t_IFA );
    SLB_DEBUG(1, "Class declaration for %s[%s]", name, _TIW(T).name());
  }

  template<typename T, typename W>
  inline Class<T,W>::Class(const Class &c)
    : _class(c._class), _mgr(c._mgr)
  {
  }
  
  template<typename T, typename W>
  inline Class<T,W>& Class<T,W>::operator=(const Class &c)
  {
    _class = c._class;
    _mgr = c._mgr;
  }
  
  template<typename T, typename W>
  inline Class<T,W> &Class<T,W>::rawSet(const char *name, Object *obj)
  {
    _class->set(name, obj);
    _lastObj = obj;
    _param = 0;
    return *this;
  }
  
  template<typename T,  typename W>
  inline Class<T,W> &Class<T,W>::constructor()
  {
    _class->setConstructor( FuncCall::defaultClassConstructor<T>() );
    return *this;
  }

  template<typename T, typename W>
  template<typename TEnum>
  inline Class<T,W> &Class<T,W>::enumValue(const char *name, TEnum obj)
  {
    // "fake" Declaration of TEnum...
    ClassInfo *c = _mgr->getOrCreateClass( _TIW(TEnum) );
    if (!c->initialized())
    {
      struct Aux
      {
        static bool equal(TEnum a, TEnum b) { return a == b; }
      };
      // if it is not initialized then add a simple adapter for 
      // references.
      typedef InstanceFactoryAdapter< TEnum, SLB::Instance::Default::Implementation<TEnum> > t_IFA;
      c->setInstanceFactory( new (Malloc(sizeof(t_IFA))) t_IFA );
      c->set__eq( FuncCall::create( &Aux::equal ));
    }
    // push a reference
    return rawSet(name, Value::copy(obj));
  }

  template<typename T,  typename W>
  inline Class<T,W> &Class<T,W>::comment( const String &s )
  {
    if (_lastObj) _lastObj->setInfo(s);
    else _class->setInfo(s);
    return *this;
  }

  template<typename T,  typename W>
  inline Class<T,W> &Class<T,W>::param( const String &s )
  {
    //TODO: This should also work for constructors, and so on.
    if (_lastObj)
    {
      FuncCall *fc = slb_dynamic_cast<FuncCall>(_lastObj);
      if (fc)
      {
        size_t max_param = fc->getNumArguments();
        if (_param >= max_param)
        {
        std::cerr
          << "SLB_Warning: " << fc->getInfo() <<" to many parameters (total args=" << max_param << ")" 
          << "("  << _param << ", " << s << ")"
          << std::endl;
        }
        else
        {
          fc->setArgComment(_param, s);
        }
      }
      else
      {
        std::cerr
          << "SLB_Warning: Can not set param info to a non-funcCall object " 
          << "("  << _param << ", " << s << ")"
          << std::endl;
      }
    }
    _param++;
    return *this;
  }
  #define SLB_REPEAT(N) \
  \
    /* Methods */ \
    template<typename T, typename W>\
    template<class C, class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    inline Class<T,W> &Class<T,W>::set(const char *name, R (C::*func)(SPP_ENUM_D(N,T)) ){ \
      if (_TIW(T) != _TIW(C)) static_inherits<C>();\
      return rawSet(name, FuncCall::create(func)); \
    } \
    template<typename T, typename W>\
    template<class C, class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    inline Class<T,W> &Class<T,W>::nonconst_set(const char *name, R (C::*func)(SPP_ENUM_D(N,T)) ){ \
      if (_TIW(T) != _TIW(C)) static_inherits<C>();\
      return rawSet(name, FuncCall::create(func)); \
    } \
  \
    /* CONST Methods */ \
    template<typename T, typename W>\
    template<class C, class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    inline Class<T,W> &Class<T,W>::set(const char *name, R (C::*func)(SPP_ENUM_D(N,T)) const ){ \
      if (_TIW(T) != _TIW(C)) static_inherits<C>();\
      return rawSet(name, FuncCall::create(func)); \
    } \
    template<typename T, typename W>\
    template<class C, class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    inline Class<T,W> &Class<T,W>::const_set(const char *name, R (C::*func)(SPP_ENUM_D(N,T)) const ){ \
      if (_TIW(T) != _TIW(C)) static_inherits<C>();\
      return rawSet(name, FuncCall::create(func)); \
    } \
  \
    /* C-functions  */ \
    template<typename T, typename W> \
    template<class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    inline Class<T,W> &Class<T,W>::set(const char *name, R (func)(SPP_ENUM_D(N,T)) ){ \
      return rawSet(name, FuncCall::create(func)); \
    } \
  \
    /* constructor  */ \
    template<typename T, typename W> \
    template<class T0 SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    inline Class<T,W> &Class<T,W>::constructor(){ \
      FuncCall *fc = FuncCall::defaultClassConstructor<T,T0 SPP_COMMA_IF(N) SPP_ENUM_D(N,T)>();\
      _class->setConstructor( fc );\
      return *this; \
    } \
    /* Constructors (wrappers c-functions)  */ \
    template<typename T, typename W> \
    template<class R SPP_COMMA_IF(N) SPP_ENUM_D(N, class T)> \
    inline Class<T,W> &Class<T,W>::constructor( R (func)(SPP_ENUM_D(N,T)) ){ \
      _class->setConstructor( FuncCall::create( func ) ); \
      return *this;\
    } \
  \

  SPP_MAIN_REPEAT_Z(MAX,SLB_REPEAT)
  #undef SLB_REPEAT
}


#endif


#ifndef __SLB_CLASS_HELPERS__
#define __SLB_CLASS_HELPERS__

//->#include "SPP.hpp"
//->#include "FuncCall.hpp"

struct lua_State;

namespace SLB {

  template<class C>
  struct Operator
  {
    static C* defaultAdd (const C *a,  const C *b)  { return new (Malloc(sizeof(C))) C( (*a)+(*b) ); }  
    static C* defaultSub (const C *a,  const C *b)  { return new (Malloc(sizeof(C))) C( (*a)-(*b) ); }  
    static C* defaultMult(const C *a,  const C *b)  { return new (Malloc(sizeof(C))) C( (*a)*(*b) ); }  
    static C* defaultDiv (const C *a,  const C *b)  { return new (Malloc(sizeof(C))) C( (*a)/(*b) ); }  
  };


}


#endif



#ifndef __SLB_SCRIPT__
#define __SLB_SCRIPT__

//->#include "lua.hpp" 
//->#include "PushGet.hpp"
//->#include "Type.hpp"
#include<stdexcept>

namespace SLB {

  class ErrorHandler; // #include <SLB/Error.hpp>
  
  class SLB_EXPORT Script
  {  
  public:
    typedef void (*PrintCallback)(
        Script *,     // Script that produced the print call
        const char *, // string to print
        size_t        // size of the string to print
    );

    explicit Script(Manager *m = Manager::defaultManager());
    virtual ~Script();

    lua_State* getState();

    // Tries to load-and-execute the given file, it can throw an exception
    // If SLB was compiled with Exception support or can exit the program
    // with an error code. See safeDoFile for a safe call that doesn't fail.
    void doFile(const char *filename);

    // Executes the content of the given file, returns true if the call
    // was successful, false otherwise. It never throws, or exits the program,
    // If you need error control use a custom ErrorHandler
    bool safeDoFile(const char *filename);

    // Tries to load and execute the given chunk of code. This method can
    // throw exceptions or exit the program (depends on SLB_USE_EXCEPTIONS 
    // definition). If you ned a safe call that doesn't fail use safeDoString
    // method instead.
    void doString(
      const char *codeChunk,
      const char *where_hint ="[SLB]");

    // Executs the given code chunk and returns true if successful, false
    // otherwise. It never throws, or exits the program, if you need error
    // control use a custom ErrorHandler.
    bool safeDoString(
      const char *codeChunk,
      const char *where_hint ="[SLB]");

    // closes the current state, and will create a new state on the next
    // getState() call.
    void resetState() { close(); }

     /* ************************* WARNING *********************************
      * Sometines you need to manually call Garbage Collector(GC), to be sure
      * that all objects are destroyed. This is mandatory when using smartPointers
      * ,be very carefull. GC operations are really expensive, avoid calling GC
      * too frequently.
      * ************************* WARNING *********************************/
    void callGC();

    /// Returns the number of Kb of memory used by the script
    size_t memUsage();

    /// Pass a new ErrorHandler, the error handler will be part of the object
    /// and it will be destroyed when the object is destroyed. 
    void setErrorHandler(ErrorHandler *h);

    /// Returns the manager that this Script is using
    Manager* getManager() { return _manager; }
    const Manager* getManager() const { return _manager; }

    template<class T>
    void set(const char *name, T value)
    { SLB::setGlobal<T>(getState(), value, name);}

    template<class T>
    T get(const char *name)
    { return SLB::getGlobal<T>(getState(), name); }

    Table::Keys getKeys(const char* name) 
    {
        lua_getglobal(getState(),name);
        ClassInfo *ci = Manager::getInstance(getState())->getClass(getState(),-1);
        lua_pop(getState(),1); // remove the value
        if(!ci) return Table::Keys();
        return ci->getKeys();
    }

    static void* allocator(void *ud, void *ptr, size_t osize, size_t nsize);

    const char *getLastError() const { return _lastError.c_str(); }
    int getLastErrorLine() const { return _lastErrorLine; }

    // Changes the print callback. This callback will be called by the internal
    // print function from the scripts in order t
    void setPrintCallback( PrintCallback );

    template<class P1>
        void call(const std::string& f, P1 p1)
        {
            LuaCall<void(P1)> call(getState(), f.c_str());
            call(p1);
        }

    template<class P1, class P2>
        void call(const std::string& f, P1 p1, P2 p2)
        {
            LuaCall<void(P1, P2)> call(getState(), f.c_str());
            call(p1, p2);
        }

    template<class P1, class P2, class P3>
        void call(const std::string& f, P1 p1, P2 p2, P3 p3)
        {
            LuaCall<void(P1, P2, P3)> call(getState(), f.c_str());
            call(p1, p2, p3);
        }

        void call(const std::string& f)
        {
            LuaCall<void(void)> call(getState(), f.c_str());
            call();
        }

  protected:
    virtual void onNewState(lua_State * /*L*/) {}
    virtual void onCloseState(lua_State * /*L*/) {}
    virtual void onGC(lua_State * /*L*/) {}
    void setAllocator(lua_Alloc f, void *ud = 0);

    void close(); // will close lua_state

  private:
    Script(const Script &s);
    Script& operator=(const Script&);
    static int PrintHook(lua_State *);

    Manager *_manager;
    lua_State *_lua_state;
    PrintCallback _printCallback;
    lua_Alloc _allocator;
    void*     _allocator_ud;
    std::string _lastError;
    int      _lastErrorLine;
    ErrorHandler *_errorHandler;
  };

}

#endif



#ifndef __SLB_STATEFUL_HYBRID__
#define __SLB_STATEFUL_HYBRID__

//->#include "Hybrid.hpp" 
//->#include "Script.hpp" 

namespace SLB {
  
  /* S -> Requires to have a method "getState" and "close" */
  template<class T, class S = SLB::Script>
  class StatefulHybrid : public S, public Hybrid< T >
  {  
  public:
    StatefulHybrid(Manager *m = Manager::defaultManager()) :
      S(m), Hybrid<T>(m)
    {
    }
    virtual ~StatefulHybrid() { S::close(); }

    virtual bool isAttached() const;
    virtual lua_State *getLuaState() { return S::getState(); }

  protected:
    virtual void onNewState(lua_State *L) { HybridBase::attach( L ); S::onNewState(L); }
    virtual void onCloseState(lua_State *L) { HybridBase::unAttach(); S::onCloseState(L); }

  };
  
  template<class T, class S >
  inline bool StatefulHybrid<T,S>::isAttached() const
  {
    //StatefulHybrids are always attached (but as we use a lazy attachment, here
    // we have to simulate and do the attachment, that means throw away constness)
    
    StatefulHybrid<T,S> *me = const_cast< StatefulHybrid<T,S>* >(this);
    me->getState(); // That's enought to ensure the attachment
    return true;
  }

}

#endif
