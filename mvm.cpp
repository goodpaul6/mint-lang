#include <vector>
#include <string>
#include <algorithm>
#include <cstdarg>

typedef unsigned char Word;

enum ObjectType
{
	OBJ_NUMBER,
	OBJ_STRING
};

struct Object
{
	bool marked;
	ObjectType type;
	
	double number;
	std::string string;
};

enum
{
	OP_PUSH_NUMBER,
	OP_PUSH_STRING,
	OP_PUSH_ARRAY,

	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_OR,
	OP_AND,
	OP_LT,
	OP_LTE,
	OP_GT,
	OP_GTE,
	OP_EQU,
	OP_NEQU,
	OP_NEG,
	OP_LOGICAL_NOT,
	
	OP_SETINDEX,
	OP_GETINDEX,

	OP_SET,
	OP_GET,
	
	OP_WRITE,
	OP_READ,
	
	OP_GOTO,
	OP_GOTOZ,

	OP_CALL,
	OP_RETURN,
	OP_RETURN_VALUE,

	OP_CALLF,

	OP_GETLOCAL,
	OP_SETLOCAL,
	
	OP_HALT,

	NUM_OPCODES
};

#define STACK_SIZE 2048
#define INDIR_SIZE 256

struct VM
{
	std::vector<Object> objects;
	int maxObjects;
	
	std::vector<std::string> strings;
	std::vector<double> numbers;
	
	int numGlobals;
	
	Object* stack[STACK_SIZE];
	int stackSize;
	
	int indirStack[INDIR_SIZE];
	int indirSize;
	
	std::vector<Word> code;
	int fp, ip;

	bool paused;

	VM() :
	numGlobals(0),
	stackSize(0),
	indirSize(0),
	fp(0), ip(0),
	paused(false),
	maxObjects(2)
	{
	}
	
	void error(const char* format, ...)
	{
		va_list l;
		va_start(l, format);
		vfprintf(stderr, format, l);
		va_end(l);
		
		paused = true;
	}
	
	void addNum(double num)
	{
		numbers.push_back(num);
	}
	
	void addString(const std::string& str)
	{
		strings.push_back(str);
	}
	
	Object* newObject(ObjectType type)
	{
		if(objects.size() >= maxObjects) gc(); 
		
		objects.emplace_back();
		
		Object* obj = &objects.back();
		obj->type = type;
		
		return obj;
	}
	
	void markAll()
	{
		if(stackSize < numGlobals) error("globals were removed from the stack\n");
		
		for(int i = 0; i < stackSize; ++i)
		{	
			if(stack[i])
				stack[i]->marked = true;
		}
	}
	
	void sweep()
	{
		auto newEnd = std::remove_if(objects.begin(), objects.end(), [](Object& obj) 
		{ 
			if(obj.marked) 
				obj.marked = false; 
			return !obj.marked; 
		});
		objects.erase(newEnd, objects.end());
	}
	
	void gc()
	{
		markAll();
		sweep();
		maxObjects = objects.size() * 2;
	}
	
	void push(Object* obj)
	{
		if(stackSize >= STACK_SIZE) error("Stack overflow\n");
		stack[stackSize++] = obj;
	}
	
	Object* pop()
	{
		if(stackSize <= 0) error("Stack underflow\n");
		return stack[--stackSize];
	}
	
	void pushNumber(double value)
	{
		Object* obj = newObject(OBJ_NUMBER);
		obj->number = value;
		
		push(obj);
	}
	
	void pushString(const std::string& value)
	{
		Object* obj = newObject(OBJ_NUMBER);
		obj->string = value;
		
		push(obj);
	}
	
	double popNumber()
	{
		Object* obj = pop();
		if(obj->type != OBJ_NUMBER)
			error("Expected number but received something else\n");
		return obj->number;
	}
	
	const std::string& popString()
	{
		Object* obj = pop();
		if(obj->type != OBJ_STRING)
			error("Expected string but received something else\n");
		return obj->string;
	}
	
	int readInt()
	{
		int value;
		Word* vp = (Word*)(&value);
		
		for(int i = 0; i < sizeof(int) / sizeof(Word); ++i)
			*vp++ = code[ip++];
		
		return value;
	}
	
	void step()
	{
		if(ip < 0) return;
		
		switch(code[ip])
		{
			case OP_PUSH_NUMBER:
			{
				++ip;
				int index = readInt();
				
				pushNumber(numbers[index]);
			} break;
			
			case OP_PUSH_STRING:
			{
				++ip;
				int index = readInt();
				
				pushString(strings[index]);
			} break;
			
			case OP_WRITE:
			{
				Object* obj = pop();
				if(obj->type == OBJ_NUMBER)
					printf("%g\n", obj->number);
				else if(obj->type == OBJ_STRING)
					printf("%s\n", obj->string.c_str());
				
				++ip;
			} break;
			
			case OP_HALT:
			{
				ip = -1;
			} break;
		}
	}
	
	void run()
	{
		ip = 0;
		
		while(ip >= 0)
			step();
	}
	
	void addCode(Word w)
	{
		code.push_back(w);
	}
	
	void addInt(int v)
	{
		Word* wp = (Word*)(&v);
		for(int i = 0; i < sizeof(int) / sizeof(Word); ++i)
			addCode(*wp++);
	}
};

int main(int argc, char* argv[])
{
	VM vm;
	
	vm.addNum(1000);
	
	for(int i = 0; i < 1000; ++i)
	{
		vm.addCode(OP_PUSH_NUMBER);
		vm.addInt(0);
	}
	
	vm.addCode(OP_WRITE);
	vm.addCode(OP_HALT);
	
	vm.run();
}
