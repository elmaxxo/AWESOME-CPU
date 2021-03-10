#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
#include <time.h>
#include <errno.h>
 
#include "comands.h" 
#include "../CommonFiles/text.h"
#include "../CommonFiles/errors.h" 
#include "byte_code.h"
#include "label.h"
#include "../CommonFiles/listing.h"


static const size_t DEFAUTL_BYTE_CODE_SIZE = 1024;              
static const size_t GROW_COEFFICIENT       = 2;
static const char   DELIM[]                = " \t\n\r\0";  
static const char   NO_DELIM_FIELDS[]      = "[#]";

typedef double arg_t;

using namespace std;


/*
  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  @todo writeCommand with flags@
  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/


struct Assembler      
    {
    ByteCode byte_code;
    Label *label[NHASH];
    
    FILE *listing;
    };


void       assemblerCtor (Assembler *asm_ptr, const char *listing_file_name);
void       assemblerDtor (Assembler *asm_ptr);
Assembler *newAssembler  (const char *listing_file_name);
Assembler *dellAssembler (Assembler *asm_ptr); 
 
int      translateFile       (Assembler *asm_ptr, const char *file_name);
int      translateCode       (Assembler *asm_ptr, Text *code);
void     writeArgument       (Assembler *asm_ptr, const void *arg, size_t arg_size);
void     writeData           (Assembler *asm_ptr, const void *value, size_t value_size);
void     writeCommand        (Assembler *asm_ptr, const Command *cmd, cmd_t cmd_type = 0x00);
Command *identifyCommand     (const char* str);
void     writeByteCode       (Assembler *asm_ptr, const char *file_name);
bool     enoughSpaseForValue (Assembler *asm_ptr, size_t value_size);
Label   *addLabel            (Assembler *asm_ptr, const char *label);

void trycatch_assemblerLabelCommandProcessing (Assembler *asm_ptr, Token **tok, Text *code); 
void          assemblerLabelCommandProcessing (Assembler *asm_ptr, Token *asm_label);
Errors assemblerPushPopCommandsProcessing (Assembler *asm_ptr, Command *cmd, Token **tok, Text *code);
Errors assemblerJumpCommandProcessing  (Assembler *asm_ptr, Command *jmp_cmd, Token **tok, Text *code);

int translateMemoryAccesByRegister (Token *tok, unsigned char *arg_buf);
int translateMemoryAccesByNumber (Token *tok, unsigned char *arg_buf);
int translateRegisterArgument (Token *tok, unsigned char *arg_buf);
int transateNumberArgument (Token *tok, unsigned char *arg_buf);
int translateArgument (Token *tok, unsigned char *arg_buf);
cmd_t identifyArgumentType (const Token *tok);

#define NELEMS( array ) (sizeof (array) / sizeof (array[0]))

#endif