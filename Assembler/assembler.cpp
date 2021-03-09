#include "assembler.h"

static bool err = false;

Assembler *newAssembler (const char *listing_file_name)
    {
    CATCH (!listing_file_name, NULL_FILE_NAME_PTR)

    Assembler *asm_ptr = (Assembler *)calloc (1, sizeof (*asm_ptr));
    CATCH (!asm_ptr, NULL_ASSEMBLER_PTR)

    assemblerCtor (asm_ptr, listing_file_name);

    qsort (ASSEMBLER_COMMANDS, NELEMS (ASSEMBLER_COMMANDS), 
           sizeof (ASSEMBLER_COMMANDS[0]), commandCompare);

    return asm_ptr;
    }

void assemblerCtor (Assembler *asm_ptr, const char *listing_file_name)
    {                 
    CATCH (!asm_ptr,           NULL_ASSEMBLER_PTR)
    CATCH (!listing_file_name, NULL_FILE_NAME_PTR)

    byteCodeCtor (&asm_ptr->byte_code, DEFAUTL_BYTE_CODE_SIZE);

    asm_ptr->listing = fopen (listing_file_name, "wb");
    CATCH (!asm_ptr->listing, NULL_LISTING_FILE_PTR)
    }

int translateFile (Assembler *asm_ptr, const char *file_name)
    {
    VERIFY_ASSEMBLER
    CATCH (!file_name, NULL_FILE_NAME_PTR);
    
    NEW_ASSEMBLER_LISTING_BLOCK ("%p, %s", (const void *)asm_ptr, file_name)

    Text src_code (file_name, asm_ptr->listing);
    src_code.fillStringsAfter (';', ' ');
    src_code.tokenizeText (DELIM, NO_DELIM_FIELDS, NULL_TERMINATED);

    int err = translateCode (asm_ptr, &src_code); 
    if (!err)
        {
        asm_ptr->byte_code.pos = 0;
        err = translateCode (asm_ptr, &src_code);
        }
    return err;           
    }

int translateCode (Assembler *asm_ptr, Text *code)
    {                              
    VERIFY_ASSEMBLER
    CATCH (!code, NULL_CODE_PTR)

    NEW_ASSEMBLER_LISTING_BLOCK ("%p, %p", (const void *)asm_ptr, (const void *)code)

    static bool are_all_labels_procesed = false;  
    for (Token *token = &code->tokens[0]; token; token = code->getNextToken (token)) 
        {
        Command *asm_com = identifyCommand (token->str);

        arg_t arg     = 0;
        char *end     = NULL;
        char  reg     = 0;
        cmd_t cmd = (cmd_t)asm_com->value;
        switch (cmd)
            {
            case CMD_PUSH:
                writeCommand (asm_ptr, asm_com);
              
                token = code->getNextToken (token);

                arg = strtod (token->str, &end);
                if (end != token->str)     //process the number
                    writeArgument (asm_ptr, &arg, sizeof (arg));

                else
                    {
                    reg = getRegisterNum (token->str);
                    if (reg >= 0) 
                        {
                        setCommandFlag (asm_ptr, REGISTER_FLAG);
                        writeArgument  (asm_ptr, &reg, sizeof (reg));
                        }
                    else 
                        {
                        TRANSLIATION_ERROR ("cant't process an argument %s", token->str)
                        token = code->getLastLineToken (token);
                        err = true;
                        }
                    }
                break;

            case CMD_POP:
                writeCommand (asm_ptr, asm_com);

                token = code->getNextToken(token);

                reg = getRegisterNum (token->str);
                if (reg >= 0)
                    writeArgument (asm_ptr, &reg, sizeof (reg));
                else
                    {
                    TRANSLIATION_ERROR ("cant't process an argument %s", token->str)
                    token = code->getLastLineToken (token);
                    err = true;
                    }
                 break;
                 
            case CMD_JB:
            case CMD_JBE:
            case CMD_JA:
            case CMD_JAE:
            case CMD_JE:
            case CMD_JNE:
            case CMD_JMP:
            case CMD_CALL:
                {
                int cmd_err = assemblerJumpCommandProcessing  (asm_ptr, asm_com,  &token, code);
                if (cmd_err && are_all_labels_procesed)
                    {
                    TRANSLIATION_ERROR ("invalid label %s", token->str);
                    token = code->getLastLineToken (token);
                    err = true;
                    }
                break;    
                }

            case CMD_LABEL:
                trycatch_assemblerLabelCommandProcessing (asm_ptr, &token, code);
                break;   

            case CMD_RET:
            case CMD_ADD:
            case CMD_SUB:
            case CMD_MULT:
            case CMD_DIV:
            case CMD_NEG:
            case CMD_SQRT:
            case CMD_SIN:
            case CMD_COS:
            case CMD_OUT:
            case CMD_IN:
            case CMD_DUMP:
            case CMD_HET:
            case CMD_END:
                writeCommand (asm_ptr, asm_com);
                break;

            default: 
                TRANSLIATION_ERROR ("unknown token \"%s\"", token->str);
                token = code->getLastLineToken (token);
                err = true;
            }
        }

    are_all_labels_procesed = true;

    return err;
    }

void trycatch_assemblerLabelCommandProcessing (Assembler *asm_ptr, Token **tok, Text *code) 
{
    VERIFY_ASSEMBLER
    CATCH (!code, NULL_CODE_PTR)
    CATCH (!tok, NULL_PTR)

    Token *token = *tok;
    try
        {
        assemblerLabelCommandProcessing (asm_ptr, *tok);
        }
    catch (exception &ex)  
        {
            TRANSLIATION_ERROR ("same label names(%s) for differen pointers", token->str);
            *tok = code->getLastLineToken (token);
            err = true;
        }    
}

void assemblerLabelCommandProcessing (Assembler *asm_ptr, Token *asm_label)
    {
    VERIFY_ASSEMBLER
    assert (asm_label);

    auto tmp = asm_label->str [asm_label->size - 1];
    asm_label->str [asm_label->size - 1] = '\0';

    Label *res = addLabel (asm_ptr, asm_label->str);
    asm_label->str [asm_label->size - 1] = tmp;  

    if (!res)
        throw exception();
    }

Errors assemblerJumpCommandProcessing  (Assembler *asm_ptr, Command *jmp_cmd, Token **tok, Text *code)
    {
    VERIFY_ASSEMBLER
    CATCH (!jmp_cmd, NULL_PTR)
    CATCH (!tok,     NULL_PTR)
    CATCH (!code,    NULL_CODE_PTR)

    writeCommand (asm_ptr, jmp_cmd);

    *tok = code->getNextToken (*tok);
    Label *find = findName (asm_ptr->label [strHash ((*tok)->str)], (*tok)->str);
                
    if (!find)
        {
        asm_ptr->byte_code.pos += sizeof (find->pos);
        return UNKNOWN_LABEL;
        }
    writeArgument (asm_ptr, &find->pos, sizeof (find->pos));   
    return NOT_ERROR; 
    }

Command* identifyCommand (const char* str)
    {
    CATCH (!str, NULL_PTR)

    if (isLabel (str)) return &LABEL;

    Command cmd    = {str, CMD_UNKNOWN};
    Command *tmp[] = { &cmd };

    Command **result = 
    (Command **)bsearch (tmp, ASSEMBLER_COMMANDS, NELEMS (ASSEMBLER_COMMANDS), 
                         sizeof (ASSEMBLER_COMMANDS[0]), commandCompare);
    
    if (!result)
        return &UNKNOWN;
    
    return *result;
    }

void writeData (Assembler *asm_ptr, const void *value, size_t value_size)
    {
    VERIFY_ASSEMBLER
    CATCH (!value, NULL_PTR)

    NEW_ASSEMBLER_LISTING_BLOCK ("%p, %p, %zu", (const void *)asm_ptr, (const void *)value, value_size)

    VERIFY_ASSEMBLER
    
    if (!enoughSpaseForValue (asm_ptr, value_size))
        {
        size_t new_size = asm_ptr->byte_code.size * GROW_COEFFICIENT;
        byteCodeResize (&asm_ptr->byte_code, new_size);
        }
    
    bool copy_error = !memcpy (asm_ptr->byte_code.data + asm_ptr->byte_code.pos, value, value_size);
    CATCH (copy_error, COPY_ERROR)
    asm_ptr->byte_code.pos += value_size;

    ASSEMBLER_LISTING ("data dump %s with size %zu", memoryDump (value, value_size).c_str(), value_size);
    }

void writeCommand (Assembler *asm_ptr, const Command *cmd)
    {
    VERIFY_ASSEMBLER
    CATCH (!cmd, NULL_PTR)

    const cmd_t command = cmd->value;

    ASSEMBLER_LISTING ("gets command %s (%d)", cmd->string, cmd->value);

    writeData (asm_ptr, &command, sizeof (command)); 
    }

void writeArgument (Assembler *asm_ptr, const void *arg, size_t arg_size)
    {                                                         
    VERIFY_ASSEMBLER
    CATCH (!arg, NULL_PTR)
    
    ASSEMBLER_LISTING ("get argument %lg with size %zu", (arg_size > 1) ? *(double *)arg : (double)*(byte_t *)arg, arg_size)

    writeData (asm_ptr, arg, arg_size);
    }

byte_t getRegisterNum (const char *reg)
    {
    CATCH (!reg, NULL_PTR)

    if (reg[0] == 'r' && reg[2] == 'x')
        if (isalpha (reg[1]))
            return reg[1] - 'a';

    return - 1;
    }

Label *addLabel (Assembler *asm_ptr, const char *label)
    {
    VERIFY_ASSEMBLER
    CATCH (!label, NULL_PTR)
    return addLabel (&asm_ptr->label [strHash (label)], newLabel (label, asm_ptr->byte_code.pos));
    }

void writeByteCode (Assembler *asm_ptr, const char *file_name)
    {
    VERIFY_ASSEMBLER
    CATCH (!file_name, NULL_FILE_NAME_PTR)
    
    NEW_ASSEMBLER_LISTING_BLOCK ("%p, %s", (const void *)asm_ptr, file_name)

    VERIFY_ASSEMBLER
    CATCH (!file_name, NULL_FILE_NAME_PTR);

    FILE *file = fopen (file_name, "wb");
    CATCH (!file, NULL_FILE_PTR)

    byte_t *bcode =  asm_ptr->byte_code.data;
    fwrite (bcode,  sizeof (bcode[0]), asm_ptr->byte_code.pos + 1, file);

    fclose (file);
    file = NULL;
    }

void setCommandFlag (Assembler *asm_ptr, byte_t flag)
    {
    VERIFY_ASSEMBLER

    ByteCode *bcode = &asm_ptr->byte_code;
    bcode->data [bcode->pos - 1] |= flag;
    }

bool enoughSpaseForValue (Assembler *asm_ptr, size_t value_size)
    {
    VERIFY_ASSEMBLER
    
    return asm_ptr->byte_code.pos + value_size < asm_ptr->byte_code.size;
    }
                                         
void assemblerDtor (Assembler *asm_ptr)
    {
    VERIFY_ASSEMBLER

    byteCodeDtor (&asm_ptr->byte_code);

    fclose (asm_ptr->listing);
    asm_ptr->listing = NULL;
    }

Assembler *dellAssembler (Assembler *asm_ptr)
    {                                    
    VERIFY_ASSEMBLER

    assemblerDtor (asm_ptr);
    free (asm_ptr);

    return (Assembler *)NULL;
    } 
