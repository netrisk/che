#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "che_log.h"

#define CH8ASM_DBG

#define CH8ASM_MAX_LINE_LEN         80
#define CH8ASM_MAX_LABEL_LEN        20
#define CH8ASM_MAX_INSTRUCTION_LEN  5
#define CH8ASM_MAX_LABELS           128 


#define CH8ASM_SET_OPCODE(_op,_val) _op |= (uint16_t)(_val) << 12

#define CH8ASM_SET_RECORD(_op,_rec) _op |= ((uint16_t)(_rec) << 8)

#define CH8ASM_SET_VALUE(_op,_val) _op |= _val

#define CH8ASM_SET_ADDRES(_op,_address) _op |= (_address & 0x0FFF)


typedef struct label_node_t
{
    char label[CH8ASM_MAX_LABEL_LEN+1];
    uint16_t address;
}label_node_t;

typedef struct record_instruction_params_t 
{
    uint8_t record;
    uint8_t value;
}record_instruction_params_t;

typedef struct instruction_params_t
{
    union {
        record_instruction_params_t rec;
        uint16_t address;
    };
}instruction_params_t;

typedef struct ch8asm_t
{
    size_t lineno; /**< current line number is being processed */
    uint16_t address; /**< current operation address */
    char line[CH8ASM_MAX_LINE_LEN+1];/**< current line is being processed */ 
    label_node_t labels[CH8ASM_MAX_LABELS]; /* keep the track of the labels */
    uint8_t labelsno; /**< number of labels in use */
    char instruction[CH8ASM_MAX_INSTRUCTION_LEN+1];/**< current instruction */
    instruction_params_t parameters; /**< parameters for current instruction */
}ch8asm_t;

   
ch8asm_t ch8asm; /* assembler object */

static void ch8asm_uint16be_write(uint16_t num,uint8_t* serialized)
{
    serialized[0] = (num & 0xFF00) >> 8;
    serialized[1] = (num & 0x00FF);
}

static char *strrstr(const char *haystack, const char *needle)
{
    if (*needle == '\0')
        return (char *) haystack;

    char *result = NULL;
    for (;;) {
        char *p = strstr(haystack, needle);
        if (p == NULL)
            break;
        result = p;
        haystack = p + 1;
    }

    return result;
}

static char *ltrim(char *str)
{
    // Trim leading space
    while(isspace(*str)) str++;

    if(*str == 0)  // All spaces?
        return str;
    
    return str;
}


static void ch8asm_dump_labels(ch8asm_t* ch8asm)
{
    uint8_t i;

    che_log("----- List of labels: -----");
    for( i = 0; i < ch8asm->labelsno; i++ ){
        che_log("Label=%s at address:0x%x",
                ch8asm->labels[i].label,
                ch8asm->labels[i].address);
    }
}

static int ch8asm_parse_parameters(const char* instruction,char* parameters,
                                   instruction_params_t* parsed_params)
{
    int ret = 0;

    #ifdef CH8ASM_DBG
    che_log("instruction=%s parameters=%s",instruction,parameters);
    #endif /* CH8ASM_DBG */
    parameters = ltrim(parameters); 
    if( 0 == strcmp(instruction,"MOV") ) {
        parsed_params->rec.record = parameters[1]-'0';
        parameters = strstr(parameters,",");
        parameters++;
        parsed_params->rec.value = strtoul(parameters,NULL,10);
        #ifdef CH8ASM_DBG
        che_log("MOV command. Register=%u value=%u",parsed_params->rec.record,
                parsed_params->rec.value);
        #endif /* CH8ASM_DBG */
    } else if( 0 == strcmp(instruction,"SNE") ) {
        parsed_params->rec.record = parameters[1]-'0';
        parameters = strstr(parameters,",");
        parameters++;
        parsed_params->rec.value = strtoul(parameters,NULL,10);
        #ifdef CH8ASM_DBG
        che_log("SNE command. Register=%u value=%u",parsed_params->rec.record,
                parsed_params->rec.value);
        #endif /* CH8ASM_DBG */
    } else if( 0 == strcmp(instruction,"JMP") ) {
        char* suffix = strstr(parameters,"h");
        uint8_t base;
        if( suffix != NULL ) {
            base = 16;
            *suffix = '\0';
        } else {
            base = 10;
        }
        parsed_params->address = strtoul(parameters,NULL,base);
        #ifdef CH8ASM_DBG
        che_log("JMP command.address=%u",parsed_params->address);
        #endif /* CH8ASM_DBG */

    } else {
        che_log("Instruction [%s] not recognized",instruction);
        ret = -1; /* not recognized */
    }
    return ret;
}

/**
 * @brief Process current line
 *
 * @param ch8asm che chip8 assembler
 *
 * @return 0 if success,-1 if error,1 if line is a comment or a blank line
 */
static int ch8asm_process_line(ch8asm_t* ch8asm)
{
    char* line;
    uint8_t i;

    /* skip left blanks and coments line */
    line = ltrim(ch8asm->line);
    if( line[0] == '\0' ) { /* blank line */
        #ifdef CH8ASM_DBG
        che_log("Discarding blank line");
        #endif /* CH8ASM_DBG */
        return 1;   
    }
    if( line[0] == '#' ) {
        #ifdef CH8ASM_DBG
        che_log("discarding a comment");
        #endif /* CH8ASM_DBG */
        return 1; /*it is a commnet */
    }
    /* parse label */
    char* label = strstr(line,":");
    if( NULL != label ) {
       for(i=0 ;line < label;line++,i++){
            ch8asm->labels[ch8asm->labelsno].label[i] = *line;
       }
       ch8asm->labels[ch8asm->labelsno++].address = ch8asm->address;
       line++;
    }

    /* parse instruction */
    line = ltrim(line);
    i = 0;
    while( *line != ' ' && *line != '\0' ) {
        ch8asm->instruction[i++] = *line;
        line++;
    }

    ch8asm->instruction[i] = '\0';
    if( '\0' != *line ) {
        line++; /* skip the blank */
        if( 0 != ch8asm_parse_parameters(ch8asm->instruction,line,&ch8asm->parameters) ) {
            che_log("Error parsing parameters for instruction:%s at line %zu",
                    ch8asm->instruction,ch8asm->lineno);
            return -1;
        }
    }

    return 0;
}

static int ch8asm_translate(ch8asm_t* ch8asm,FILE* obj_file)
{
    int res = 0;
    uint16_t opcode = 0;
    uint8_t output[2];

    if( '\0' == ch8asm->instruction[0] )
        return 0;

    if( 0 == strcmp(ch8asm->instruction,"MOV") ) {
        CH8ASM_SET_OPCODE(opcode,6);
        CH8ASM_SET_RECORD(opcode,ch8asm->parameters.rec.record);
        CH8ASM_SET_VALUE(opcode,ch8asm->parameters.rec.value);
    } else if( 0 == strcmp(ch8asm->instruction,"SNE") ) {
        CH8ASM_SET_OPCODE(opcode,3);
        CH8ASM_SET_RECORD(opcode,ch8asm->parameters.rec.record);
        CH8ASM_SET_VALUE(opcode,ch8asm->parameters.rec.value);
    } else if( 0 == strcmp(ch8asm->instruction,"JMP") ) {
        CH8ASM_SET_OPCODE(opcode,1);
        CH8ASM_SET_ADDRES(opcode,ch8asm->parameters.address);
    } else {
        che_log("Unknown instruction:[%s]",ch8asm->instruction);
        res = -1;
    }
    if( res != -1 ) {
        ch8asm_uint16be_write(opcode,output);
        fwrite(output,sizeof(uint16_t),1,obj_file);
    }
    return res;
}

int main(int argc,char** argv){
   FILE* asm_file;
   FILE* obj_file;
   char obj_filename[256];

   if( argc < 2 ) {
        che_log("ERROR:Specifify the assembler file");
        return -1;
   }
   strcpy(obj_filename,argv[1]);
   char* dot = strrstr(obj_filename,".");
   dot++;
   strcpy(dot,"ch8");

   asm_file = fopen(argv[1],"r");
   obj_file = fopen(obj_filename,"w+");
   che_log("Output object file will be:%s",obj_filename);
   if( NULL == asm_file ){
       che_log("Error openeing the file:%s",argv[1]);
       return -1;
   }
   if( NULL == obj_file ) {
       che_log("Error openeing the file:%s",obj_filename);
       return -1;
   }
   memset(&ch8asm,0,sizeof(ch8asm));
   ch8asm.lineno = 1;
   ch8asm.address = 0x0200;
   
   while( fgets(ch8asm.line,sizeof(ch8asm.line),asm_file) != NULL ){
       int res;
       #ifdef CH8ASM_DBG
       che_log("line %zu=%s",ch8asm.lineno,ch8asm.line);
       #endif /* CH8ASM_DBG */
       
       res = ch8asm_process_line(&ch8asm);
       if( -1 == res ) {
           che_log("Error processing line %zu",ch8asm.lineno);
           return -1;
       }
       if( 1 == res ) { /* empty line */
            continue;
       }
       res = ch8asm_translate(&ch8asm,obj_file);
       if( 0 != res ) {
           che_log("Error %d while translating",res);
       }
       ch8asm.lineno++;
       ch8asm.address+=2;
   }
   fclose(asm_file);
   fclose(obj_file);
   ch8asm_dump_labels(&ch8asm);
   return 0;
}
