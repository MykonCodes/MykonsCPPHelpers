/**
  By MykonCodes.
  ID to Type allocation helper. 
  Inspired by Andrei Alexandrescus "Modern C++ Design" I adressed the issue of an dependency-free, easy to extent solution
  to allocating objects based on a string identifier. 
  This implementation let's you spawn an instance of any class (implemented with the below described method) by passing the class name.
  So far so underwhelming. However, Implementing a new derived class to support IDTM requires not one line of additional code. 
  To implement IDTM, use the macro IDTM_IMPL with the base class name and any optional parent classes.
  IDTM_IMPL(Shape) [OR] IDTM_IMPL(Shape, : public ShapeBase COMMA public ShapeBaseBase)
  continue and close the "Shape"-class body below.
  
  To implement a IDTM supportive class, add
  IDTM_IMPL_CLASS(CircleShape, Shape)
  continue and close the"CircleShape"-class body below.
  
  After that, you can call
  Shape* myShape = Shape::GetIDTM()["CircleShape"](); 
  anywhere to allocate a CircleShape, also in the very first line in main();
  
  If you are not interested in all the macro behaviour, you should be able to extract all the logic behind the concept to use it without
  macros.
  
  This code snippet should be easily extendable to support any kind of static function, not just allocating instances.

  The gist of all of this is basically using a CRTP construct and one neat trick that allows you to call any function for any class on static initialization
  without any need to call it somewhere manually or to instantiate the class, that is assigning a lambda or function to a static member that then executes that
  function in it's constructor. Just be careful, static initialization is "random" (not guaranteed to be consistent in order), so I use
  lazy initialization for the actual map, that grants that the map is actually initialized already (upon first call).
  That trick I utilized to make deriving IDTM classes automatically register their MakeNew function, so you don't have to remember
  anything when implementing new derived classes you want to have that functionality.
  
**/

#pragma once

#include <functional>
#include <map>
#include <iostream>
#include <string>


#define PPCAT_NX(A, B) A ## B
#define DECON(a) a
#define COMMA ,

#define IDTM_CustomThunk CUSTOMTHUNK

#define IDTM_IMPL_CRTP(_base, customThunk) \
struct PPCAT_NX(_base,IDTMExecStatic) { PPCAT_NX(_base,IDTMExecStatic)(std::function<void()> func) { func(); } }; \
template<class Derived> \
struct PPCAT_NX(_base,CRTP) { \
	static PPCAT_NX(_base,IDTMExecStatic) execStatic;\
	PPCAT_NX(_base,CRTP)() { PPCAT_NX(_base,CRTP)<Derived>::execStatic; }\
};\
template<class Derived>\
PPCAT_NX(_base,IDTMExecStatic) PPCAT_NX(_base, CRTP)<Derived>::execStatic = []() {\
	DECON(_base)::GetIDTM()[PPCAT_NX(Derived::,IDTM_ObjectID)] = PPCAT_NX(&Derived::, IDTM_MakeNew);\
};\

#define IDTM_IMPL(_baseType, _derive)\
IDTM_IMPL_CRTP(_baseType) \
class _baseType _derive\
{ \
public:\
 typedef std::map<std::string, std::function<class DECON(_baseType)*()>> PPCAT_NX(_baseType, IDTMSpawnMap);  \
static PPCAT_NX(_baseType, IDTMSpawnMap)& GetIDTM() { \
	static PPCAT_NX(_baseType, IDTMSpawnMap) spawnMap; \
	return spawnMap; \
} \


#define IDTM_IMPL_CLASS_STATIC_(_type, _base)\
static PPCAT_NX(_base,*) IDTM_MakeNew() {return new _type();} \

//Ensures execStatic is not optimized away and thus ensures that IDTM_MakeNew is implemented
#define IDTM_IMPL_CLASS_STATIC_CUSTOMTHUNK(_type, _base) PPCAT_NX(_type,()){ execStatic; }

#define IDTM_IMPL_CLASS_STATIC_SWITCH(_type, _base, bCustomThunk) PPCAT_NX(IDTM_IMPL_CLASS_STATIC_,bCustomThunk)(_type, _base)

#define IDTM_IMPL_CLASS(_type, _base, customThunk) class _type; class PPCAT_NX(_type, Impl) : public _base, public PPCAT_NX(_base,CRTP)<_type> \
{ \
public: \
	static std::string IDTM_ObjectID; \
	static std::string GetIDTM_ObjectID(){return IDTM_ObjectID;} \
}; \
\
std::string PPCAT_NX(_type, Impl)::IDTM_ObjectID = #_type; \
\
class _type : public PPCAT_NX(_type, Impl){ \
public: \
IDTM_IMPL_CLASS_STATIC_SWITCH(_type, _base, customThunk) \
private: \


