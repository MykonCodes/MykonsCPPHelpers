#include "IDTM.h"


IDTM_IMPL(Shape,)
	virtual void PrintStr() { std::cout << "ShapeBase" << std::endl; }
};

IDTM_IMPL_CLASS(CircleShape, Shape)
public:
	virtual void PrintStr() override { std::cout << "I am a CircleShape" << std::endl; }
};

IDTM_IMPL_CLASS(ElipseShape, Shape)
public:
	virtual void PrintStr() override { std::cout << "I am a ElipseShape" << std::endl; }
};


class ObjectBase {
public:
	int myFancyValue = 99;
};

class SomeInterface {
protected:
	virtual void DoStuff() = 0;
};

IDTM_IMPL(Object, : public ObjectBase COMMA public SomeInterface)
	virtual void PrintStr() { std::cout << "ShapeBase" << std::endl; }
	virtual void DoStuff() override { std::cout << "Did stuff." << std::endl; }
};

//Specify a custom IDTM_MakeNew by adding the IDTM_CustomThunk param
IDTM_IMPL_CLASS(FooObject, Object, IDTM_CustomThunk)
public:
	FooObject(int myBetterValue) {
		myFancyValue = myBetterValue;
	}
	virtual void PrintStr() override { std::cout << "I am a FooObject" << std::endl; }
	static Object* IDTM_MakeNew() { 
		std::cout << "Overwrote the MakeNew function of FooObject by adding IDTM_CustomThunk" << std::endl;
		return new FooObject(111);
	} 
	virtual void DoStuff() override { std::cout << "Did foo stuff. Fancy Value: " << myFancyValue << std::endl; }
};

IDTM_IMPL_CLASS(BarObject, Object)
public:
	virtual void PrintStr() override { std::cout << "I am a BarObject" << std::endl; }
	virtual void DoStuff() override { std::cout << "Did bar stuff. Fancy Value: " << myFancyValue << std::endl; }
};





int main()
{

	Shape::GetIDTM()["ElipseShape"]()->PrintStr();
	Shape::GetIDTM()["CircleShape"]()->PrintStr();
	Object::GetIDTM()["FooObject"]()->DoStuff();
	Object::GetIDTM()["BarObject"]()->DoStuff();

	return 0;
}

/**
  Output:
  I am a ElipseShape
  I am a CircleShape
  Overwrote the MakeNew function of FooObject by adding IDTM_CustomThunk
  Did foo stuff. Fancy Value: 111
  Did bar stuff. Fancy Value: 99
**/
