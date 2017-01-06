// asm.c -- assembler for the mint virtual machine
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "vm.h"

static void* emalloc(size_t size)
{
	void* mem = malloc(size);
	if(!mem) { fprintf(stderr, "Out of memory!\n"); exit(1); }
	return mem;
}

static void* erealloc(void* mem, size_t newSize)
{
	void* newMem = realloc(mem, newSize);
	if(!newMem) { fprintf(stderr, "Out of memory!\n"); exit(1); }
	return newMem;
}

static char* estrdup(const char* string)
{
	char* newString = emalloc(strlen(string) + 1);
	strcpy(newString, string);
	return newString;
}

typedef struct _LinkedListNode
{
	struct _LinkedListNode* next;
	void* data;
} LinkedListNode;

typedef struct _LinkedList
{
	LinkedListNode* head;
	LinkedListNode* tail;
	int length;
} LinkedList;

void InitList(LinkedList* list)
{
	list->head = list->tail = NULL;
	list->length = 0;
}

int AddNode(LinkedList* list, void* data)
{
	LinkedListNode* newNode = emalloc(sizeof(LinkedListNode));
	newNode->next = NULL;
	newNode->data = data;
	
	if(!list->length)
		list->head = list->tail = newNode;
	else
	{
		list->tail->next = newNode;
		list->tail = newNode;
	}
	
	++list->length;
	
	return list->length - 1;
}

void FreeList(LinkedList* list)
{
	if(!list) return;
	
	if(list->length)
	{
		LinkedListNode* curr;
		LinkedListNode* next;
		
		curr = list->head;
		while(curr)
		{
			next = curr->next;
			
			if(curr->data)
				free(curr->data);
			
			free(curr);
			
			curr = next;
		}
	}

	free(list);
}

static LinkedList* StringTable;

int AddString(const char* string)
{
	LinkedListNode* node = StringTable->head;
	
	int nodeIndex;
	for(nodeIndex = 0; nodeIndex < StringTable->length; ++nodeIndex)
	{
		if(strcmp(node->data, string) == 0) 
			return nodeIndex;
		
		node = node->next;
	}
	
	char* stringCopy = estrdup(string);
	return AddNode(StringTable, stringCopy);
}

static LinkedList* NumberTable;

int AddNumber(double value)
{
	LinkedListNode* node = NumberTable->head;
	
	int nodeIndex;
	for(nodeIndex = 0; nodeIndex < NumberTable->length; ++nodeIndex)
	{
		if(*((double*)node->data) == value)
			return nodeIndex;
			
		node = node->next;
	}
	
	double* number = emalloc(sizeof(double));
	*number = value;
	return AddNode(NumberTable, number);
}

typedef struct _LabelNode
{
	char* name;
	int entryPoint;
	int index;
	char defined;
} LabelNode;

static LinkedList* LabelTable;

LabelNode* RegisterLabel(char* name)
{
	LinkedListNode* node = LabelTable->head;
	
	int nodeIndex;
	for(nodeIndex = 0; nodeIndex < LabelTable->length; ++nodeIndex)
	{
		if(strcmp(((LabelNode*)node->data)->name, name) == 0)
			return node->data;
		
		node = node->next;
	}
	
	LabelNode* label = emalloc(sizeof(LabelNode));
	label->name = estrdup(name);
	label->index = LabelTable->length;
	label->defined = 0;
	AddNode(LabelTable, label);
	return label;
}

static LinkedList* GlobalTable;

int RegisterGlobal(char* name)
{
	LinkedListNode* node = GlobalTable->head;

	int nodeIndex;
	for(nodeIndex = 0; nodeIndex < GlobalTable->length; ++nodeIndex)
	{
		if(strcmp((char*)node->data, name) == 0)
			return nodeIndex;

		node = node->next;
	}

	char* copy = estrdup(name);
	AddNode(GlobalTable, copy);
	return GlobalTable->length - 1; 
}

int GetGlobalIndex(char* name)
{
	LinkedListNode* node = GlobalTable->head;

	int nodeIndex;
	for(nodeIndex = 0; nodeIndex < GlobalTable->length; ++nodeIndex)
	{
		if(strcmp((char*)node->data, name) == 0)
			return nodeIndex;

		node = node->next;
	}

	return -1;
}

static LinkedList* ExternTable;

int RegisterExtern(char* name)
{
	LinkedListNode* node = ExternTable->head;
	
	int nodeIndex;
	for(nodeIndex = 0; nodeIndex < ExternTable->length; ++nodeIndex)
	{
		if(strcmp((char*)node->data, name) == 0)
			return nodeIndex;

		node = node->next;
	}
	
	char* copy = estrdup(name);
	AddNode(ExternTable, copy);
	return ExternTable->length - 1;
}

int GetExternIndex(char* name)
{
	LinkedListNode* node = ExternTable->head;

	int nodeIndex;
	for(nodeIndex = 0; nodeIndex < ExternTable->length; ++nodeIndex)
	{
		if(strcmp((char*)node->data, name) == 0)
			return nodeIndex;

		node = node->next;
	}

	return -1;
}

enum 
{
	OPT_A0,
	
	OPT_A4,
	OPT_A4L,
	OPT_A4G,
	
	OPT_A14,
	OPT_A14L
};

typedef struct 
{
	char name[12];
	Word type;
	Word value;
} Opcode;
Opcode Opcodes[] = {
	{ "array", OPT_A4, OP_CREATE_ARRAY },

	{ "add", OPT_A0, OP_ADD },
	{ "sub", OPT_A0, OP_SUB },
	{ "mul", OPT_A0, OP_MUL },
	{ "mod", OPT_A0, OP_DIV },
	{ "or", OPT_A0, OP_OR },
	{ "and", OPT_A0, OP_AND },
	{ "lt", OPT_A0, OP_LT },
	{ "lte", OPT_A0, OP_LTE },
	{ "gt", OPT_A0, OP_GT },
	{ "gte", OPT_A0, OP_GTE },
	{ "equ", OPT_A0, OP_EQU },
	{ "nequ", OPT_A0, OP_NEQU },

	{ "setindex", OPT_A0, OP_SETINDEX },
	{ "getindex", OPT_A0, OP_GETINDEX },

	{ "set", OPT_A4G, OP_SET },
	{ "get", OPT_A4G, OP_GET },

	{ "write", OPT_A0, OP_WRITE },
	{ "read", OPT_A0, OP_READ },

	{ "goto", OPT_A4L, OP_GOTO },
	{ "gotoz", OPT_A4L, OP_GOTOZ },

	{ "call", OPT_A14L, OP_CALL },
	{ "ret", OPT_A0, OP_RETURN },
	{ "retval", OPT_A0, OP_RETURN_VALUE },

	{ "callf", OPT_A4, OP_CALLF },

	{ "getlocal", OPT_A4, OP_GETLOCAL },
	{ "setlocal", OPT_A4, OP_SETLOCAL },

	{ "halt", OPT_A0, OP_HALT } 
};

Opcode* GetOpcodeByName(const char* name)
{
	for(int i = 0; i < NUM_OPCODES; ++i)
	{
		if(strcmp(Opcodes[i].name, name) == 0)
			return &Opcodes[i];
	}

	return NULL;
}

const char* GetNameByCode(Word code)
{
	for(int i = 0; i < NUM_OPCODES; ++i)
	{
		if(Opcodes[i].value == code)
			return Opcodes[i].name;
	}

	return NULL;	
}

enum 
{
	TOK_INSTR,
	TOK_GLOBAL,
	TOK_EXTERN,
	TOK_LABEL_REF,
	TOK_NUMBER,
	TOK_STRING,
	TOK_LABEL_DEF,
	TOK_EOF
};

#define LEXEME_LENGTH	256
char Lexeme[LEXEME_LENGTH];
int Line = 1;

int StoreUntilSpace(FILE* in)
{
	int i = 0;
	int c = getc(in);

	while(!isspace(c) && c != EOF)
	{
		// TODO: Check for token exceeding maximum length
		Lexeme[i++] = c;
		c = getc(in);
	}

	Lexeme[i] = '\0';
	return c;
}

int GetToken(FILE* in)
{
	static int last = ' ';

	while(isspace(last))
		last = getc(in);

	if(isalpha(last) || last == '_')
	{
		ungetc(last, in);

		last = StoreUntilSpace(in);
	
		if(strcmp(Lexeme, "global") == 0) return TOK_GLOBAL;
		if(strcmp(Lexeme, "extern") == 0) return TOK_EXTERN;

		Opcode* op = GetOpcodeByName(Lexeme);
		
		if(!op) return TOK_LABEL_REF;
		return TOK_INSTR;
	}
	else if(last == '$')
	{
		last = StoreUntilSpace(in);

		char buf[LEXEME_LENGTH];
		sprintf(buf, "%i", (int)strtol(Lexeme, NULL, 16));
		Lexeme[0] = '\0';
		strcpy(Lexeme, buf);
		
		return TOK_NUMBER;
	}
	else if(isdigit(last) || last == '-')
	{
		ungetc(last, in);

		last = StoreUntilSpace(in);
		return TOK_NUMBER;
	}
	else if(last == '"')
	{
		// NOTE: This is pretty much the same as 'StoreUntilSpace' function but stores until '"'; refactor?
		
		int i = 0;
		int c = getc(in);

		while(c != '"')
		{
			// TODO: Check for token exceeding max length
			Lexeme[i++] = c;
			c = getc(in);
		}

		Lexeme[i] = '\0';
		last = getc(in);		// eat '"'
		
		return TOK_STRING;
	}
	else if(last == '.')
	{
		last = StoreUntilSpace(in);
		
		if(GetOpcodeByName(Lexeme))
		{
			fprintf(stderr, "Cannot have label with the same name as a mnemonic ('%s')\n", Lexeme);
		   	exit(1);	
		}

		return TOK_LABEL_DEF;
	}
	else if(last == '#')
	{
		while(last != '\n' && last != '\r' && last != EOF)
			last = getc(in);
		return GetToken(in);
	}
	else if(last == EOF)
		return TOK_EOF;

	fprintf(stderr, "Unexpected character %c\n", last);
	exit(1);
	
	return -1;
}

int CurTok = -1;
int GetNextToken(FILE* in)
{
	CurTok = GetToken(in);
	return CurTok;
}


Word* CodeBuffer = NULL;
size_t CodeBufferCapacity = 0;
size_t CodeBufferLength = 0;
int EntryPoint = 0;
int NumGlobals = 0;
char ListPcs = 0;
int NumExterns = 0;

void BeginCode()
{
	EntryPoint = 0;
	NumGlobals = 0;
	NumExterns = 0;

	CodeBuffer = NULL;
	CodeBufferCapacity = 1;
	CodeBufferLength = 0;

	StringTable = emalloc(sizeof(LinkedList));
	InitList(StringTable);

	NumberTable = emalloc(sizeof(LinkedList));
	InitList(NumberTable);

	LabelTable = emalloc(sizeof(LinkedList));
	InitList(LabelTable);

	GlobalTable = emalloc(sizeof(LinkedList));
	InitList(GlobalTable);
	
	ExternTable = emalloc(sizeof(LinkedList));
	InitList(ExternTable);
}

void GenCode(Word code)
{
	if(CodeBufferLength + 1 >= CodeBufferCapacity)
	{
		CodeBufferCapacity *= 2;
		CodeBuffer = erealloc(CodeBuffer, CodeBufferCapacity * sizeof(Word));
	}

	CodeBuffer[CodeBufferLength++] = code;

	if(ListPcs)
	{
		const char* name = GetNameByCode(code);
		if(name)
			printf("%s at %i\n", name, CodeBufferLength - 1);
	}
}

void GenInt(int value)
{
	Word* vp = (Word*)(&value);

	for(int i = 0; i < sizeof(int) / sizeof(Word); ++i)
		GenCode(*vp++);
}

void EndCode(FILE* out, char summarize)
{
	if(summarize)
	{
		printf("===========================\n");
		printf("Summary:\n");
		printf("Entry Point: %i\n", EntryPoint);
		printf("Length: %i\n", CodeBufferLength);
		printf("Number of globals: %i\n", NumGlobals);
		printf("Number of externs: %i\n", NumExterns);
		printf("Number of functions: %i\n", LabelTable->length);
		printf("Number of number constants: %i\n", NumberTable->length);
		printf("Number of string constants: %i\n", StringTable->length);
		printf("===========================\n");
	}
	
	fwrite(&EntryPoint, sizeof(int), 1, out);

	fwrite(&CodeBufferLength, sizeof(int), 1, out);
	fwrite(CodeBuffer, sizeof(Word), CodeBufferLength, out);

	fwrite(&NumGlobals, sizeof(int), 1, out);

	fwrite(&LabelTable->length, sizeof(int), 1, out);
	for(LinkedListNode* node = LabelTable->head; node != NULL; node = node->next)
	{
		LabelNode* label = (LabelNode*)node->data;
		if(!label->defined)
		{
			fprintf(stderr, "Undefined reference to label '%s'\n", label->name);
			exit(1);
		}

		fwrite(&label->entryPoint, sizeof(int), 1, out);
	}
	
	fwrite(&ExternTable->length, sizeof(int), 1, out);
	for(LinkedListNode* node = ExternTable->head; node != NULL; node = node->next)
	{
		char* name = node->data;
		int length = strlen(name);
		fwrite(&length, sizeof(int), 1, out);
		fwrite(name, sizeof(char), length, out);
	}

	fwrite(&NumberTable->length, sizeof(int), 1, out);
	for(LinkedListNode* node = NumberTable->head; node != NULL; node = node->next)
	{
		double* pnum = node->data;
		fwrite(pnum, sizeof(double), 1, out);
	}

	fwrite(&StringTable->length, sizeof(int), 1, out);
	for(LinkedListNode* node = StringTable->head; node != NULL; node = node->next)
	{
		char* string = node->data;
		int length = strlen(string);
		fwrite(&length, sizeof(int), 1, out);
		fwrite(string, sizeof(char), length, out);
	}

	for(LinkedListNode* node = LabelTable->head; node != NULL; node = node->next)
	{
		LabelNode* label = (LabelNode*)node->data;
		free(label->name);
	}

	FreeList(LabelTable);
	FreeList(NumberTable);
	FreeList(StringTable);
	FreeList(GlobalTable);
	FreeList(ExternTable);

	free(CodeBuffer);
}

void CompileOp(Opcode* op, FILE* in, FILE* out)
{		
	if(op->type == OPT_A0)
		GenCode(op->value);
	
	if(op->type == OPT_A4)
	{
		GenCode(op->value);

		GetNextToken(in);
		
		int iArg;
		
		if(op->value == OP_CALLF)
		{
			if(CurTok != TOK_LABEL_REF)
			{
				fprintf(stderr, "Expected name of extern function after %s\n", op->name);
				exit(1);
			}
			
			iArg = GetExternIndex(Lexeme);
		}
		else if(CurTok != TOK_NUMBER)
		{
			fprintf(stderr, "Expected number after %s\n", op->name);
			exit(1);
		}
		else
			iArg = (int)strtod(Lexeme, NULL);

		GenInt(iArg);
	}

	if(op->type == OPT_A4L)
	{
		GenCode(op->value);

		GetNextToken(in);
		if(CurTok != TOK_LABEL_REF)
		{
			fprintf(stderr, "Expected label reference after %s\n", op->name);
			exit(1);
		}

		int idx = RegisterLabel(Lexeme)->index;
		GenInt(idx);
	}

	if(op->type == OPT_A4G)
	{
		GenCode(op->value);

		GetNextToken(in);
		if(CurTok != TOK_LABEL_REF)
		{
			fprintf(stderr, "Expected global name after %s\n", op->name);
			exit(1);
		}

		int gIndex = GetGlobalIndex(Lexeme);
		if(gIndex < 0)
		{
			fprintf(stderr, "Attempted to reference undeclared global %s\n", Lexeme);
			exit(1);
		}

		GenInt(gIndex);
	}

	if(op->type == OPT_A14)
	{
		GenCode(op->value);

		GetNextToken(in);

		int iArg;
		if(CurTok == TOK_STRING)
		{	
			GenCode(1);
			iArg = AddString(Lexeme);
		}
		else 
		{
			if(CurTok != TOK_NUMBER)
			{
				fprintf(stderr, "Push instruction only accepts strings or numbers\n");
				exit(1);
			}

			GenCode(0);
			iArg = AddNumber(strtod(Lexeme, NULL));
		}

		GenInt(iArg);
	}

	if(op->type == OPT_A14L)
	{
		GenCode(op->value);

		GetNextToken(in);

		if(CurTok != TOK_NUMBER)
		{
			if(CurTok == TOK_LABEL_REF)
			{
				GenCode(0);
				LabelNode* node = RegisterLabel(Lexeme);
				GenInt(node->index);
				return;
			}
			else 
			{
				fprintf(stderr, "Invalid use of %s instruction\n", op->name);
				exit(1);
			}
		}

		Word nargs = (Word)strtol(Lexeme, NULL, 10);
		GenCode(nargs);

		GetNextToken(in);

		if(CurTok != TOK_LABEL_REF)
		{
			fprintf(stderr, "%s instruction takes label reference as second parameter\n", op->name);
			exit(1);
		}

		LabelNode* node = RegisterLabel(Lexeme);
		GenInt(node->index);
	}
}

void Assemble(FILE* in, FILE* out, char summarize)
{
	BeginCode();

	GetNextToken(in);

	while(CurTok != TOK_EOF)
	{
		if(CurTok == TOK_GLOBAL)
		{
			GetNextToken(in);
			RegisterGlobal(Lexeme);
			++NumGlobals;
		}
		else if(CurTok == TOK_EXTERN)
		{
			GetNextToken(in);
			RegisterExtern(Lexeme);
			++NumExterns;
		}
		else if(CurTok == TOK_LABEL_DEF)
		{
			if(strcmp(Lexeme, "_main") == 0)
				EntryPoint = CodeBufferLength;

			LabelNode* node = RegisterLabel(Lexeme);
			node->entryPoint = CodeBufferLength;
			node->defined = 1;
		}
		else if(CurTok == TOK_NUMBER)
			GenInt(AddNumber(strtod(Lexeme, NULL)));
		else if(CurTok == TOK_STRING)
			GenInt(AddString(Lexeme));
		else if(CurTok == TOK_LABEL_REF)
		{
			int index = GetGlobalIndex(Lexeme);
			if(index < 0)
			{
				fprintf(stderr, "Referenced undeclared global '%s'\n", Lexeme);
				exit(1);
			}
			
			
			GenInt(index);
		}
		else
		{
			Opcode* op = GetOpcodeByName(Lexeme);
			CompileOp(op, in, out);
		}

		GetNextToken(in);
	}
	GenCode(OP_HALT);

	EndCode(out, summarize);
}

int main(int argc, char* argv[])
{
	if(argc > 1)
	{
		const char* outputFileName = "out.t";
		char shouldSummarize = 0;
		char showTemp = 0;

		FILE* mergedFile = fopen("temp.tasm", "w");
		if(!mergedFile)
		{
			fprintf(stderr, "Failed to generate merge file!\n");
			return 1;
		}

		for(int i = 1; i < argc; ++i)
		{
			if(strcmp(argv[i], "-o") == 0)
				outputFileName = argv[++i];
			else if(strcmp(argv[i], "-sum") == 0)
				shouldSummarize = 1;
			else if(strcmp(argv[i], "-smf") == 0)
				showTemp = 1;
			else if(strcmp(argv[i], "-lpc") == 0)
				ListPcs = 1;
			else 
			{
				FILE* in = fopen(argv[i], "r");
				if(!in)
				{
					fprintf(stderr, "Failed to open file '%s' for reading\n", argv[i]);
					return 1;
				}

				int c;
				while((c = getc(in)) != EOF)
					putc(c, mergedFile);
				putc('\n', mergedFile);

				fclose(in);
			}
		}

		fclose(mergedFile);
		FILE* in = fopen("temp.tasm", "r");
		if(!in)
		{
			fprintf(stderr, "Failed to open merge file for reading!\n");
			return 1;
		}

		if(showTemp)
		{
			int c;
			while((c = getc(in)) != EOF)
				putc(c, stdout);
			putc('\n', stdout);
			rewind(in);
		}

		FILE* out = fopen(outputFileName, "wb");
		if(!out)
		{
			fprintf(stderr, "Failed to open file '%s' for output\n", outputFileName);
			return 1;
		}

		Assemble(in, out, shouldSummarize);
		fclose(in);
		remove("temp.tasm");

		fclose(out);
		return 0;
	}
	else 
	{
		fprintf(stderr, "Missing command line arguments\n");
		return 1;
	}
}
