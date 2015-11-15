#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslimits.h>

#define CH8ASM_DBG

#define CH8ASM_MAX_LINE_LEN         80
#define CH8ASM_MAX_LABEL_LEN        20
#define CH8ASM_MAX_INSTRUCTION_LEN  5
#define CH8ASM_MAX_LABELS           128 

typedef struct label_node_t
{
    char label[CH8ASM_MAX_LABEL_LEN+1];
    uint16_t address;
}label_node_t;

typedef struct instruction_params_t
{
    uint8_t record;
    uint8_t value;
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

static void ch8asm_uint16be_write(uint16_t num,uint16_t* serialized)
{
    *serialized = (num>>8) | (num<<8);
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
    char *end;

    // Trim leading space
    while(isspace(*str)) str++;

    if(*str == 0)  // All spaces?
        return str;
    
    return str;
}


static void ch8asm_dump_labels(ch8asm_t* ch8asm)
{
    uint8_t i;

    for( i = 0; i < ch8asm->labelsno; i++ ){
        fprintf(stdout,"Label=%s at address:0x%x\n",ch8asm->labels[i].label,ch8asm->labels[i].address);
    }
}

static int ch8asm_parse_parameters(const char* instruction,char* parameters,
                                   instruction_params_t* parsed_params)
{
    int ret = 0;

    #ifdef CH8ASM_DBG
    fprintf(stdout,"instruction=%s parameters=%s\n",instruction,parameters);
    #endif /* CH8ASM_DBG */
    parameters = ltrim(parameters); 
    if( 0 == strcmp(instruction,"MOV") ) {

        parsed_params->record = parameters[1]-'0';
        parameters = strstr(parameters,",");
        parameters++;
        parsed_params->value = strtoul(parameters,NULL,10);
        #ifdef CH8ASM_DBG
        fprintf(stdout,"MOV command. REgister=%u value=%u\n",parsed_params->record,parsed_params->value);
        #endif /* CH8ASM_DBG */
    } else {
        fprintf(stderr,"Instruction [%s] not recognized\n",instruction);
        ret = -1; /* not recognized */
    }
    return ret;
}

static int ch8asm_process_line(ch8asm_t* ch8asm)
{
    char* line;
    uint8_t i;

    if( ch8asm->line[0] == '\0' ) /* blank line */
        return 0;

    /* skip left blanks and coments line */
    line = ltrim(ch8asm->line);
    if( line[0] == '#' ) {
        #ifdef CH8ASM_DBG
        fprintf(stdout,"discarding a comment\n");
        #endif /* CH8ASM_DBG */
        return 0; /*it is a commnet */
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
            fprintf(stderr,"Error parsing parameters for instruction:%s at line %zu\n",
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

    if( '\0' == ch8asm->instruction[0] )
        return 0;

    if( 0 == strcmp(ch8asm->instruction,"MOV") ) {
        opcode |= 0x6000;
        opcode |= ((uint16_t)(ch8asm->parameters.record) << 8);
        opcode |= ch8asm->parameters.value;
        ch8asm_uint16be_write(opcode,&opcode);
        fwrite(&opcode,sizeof(uint16_t),1,obj_file);
    } else {
        res = -1;
    }
    return res;
}

int main(int argc,char** argv){
   FILE* asm_file;
   FILE* obj_file;
   char obj_filename[PATH_MAX+1];

   if( argc < 2 ) {
        fprintf(stderr,"ERROR:Specifify the assembler file\n");
        return -1;
   }
   strcpy(obj_filename,argv[1]);
   char* dot = strrstr(obj_filename,".");
   dot++;
   strcpy(dot,"obj");

   asm_file = fopen(argv[1],"r");
   obj_file = fopen(obj_filename,"w+");
   fprintf(stdout,"Output object file will be:%s\n",obj_filename);
   if( NULL == asm_file ){
       fprintf(stderr,"Error openeing the file:%s\n",argv[1]);
       return -1;
   }
   if( NULL == obj_file ) {
       fprintf(stderr,"Error openeing the file:%s\n",obj_filename);
       return -1;
   }
   memset(&ch8asm,0,sizeof(ch8asm));
   ch8asm.lineno = 1;
   ch8asm.address = 0x0200;
   
   while( fgets(ch8asm.line,sizeof(ch8asm.line),asm_file) != NULL ){
       int res;
       #ifdef CH8ASM_DBG
       fprintf(stdout,"line %zu=%s",ch8asm.lineno,ch8asm.line);
       #endif /* CH8ASM_DBG */
       if( 0 != ch8asm_process_line(&ch8asm) ) {
           fprintf(stderr,"Error processing line %zu\n",ch8asm.lineno);
           return -1;
       }
       res = ch8asm_translate(&ch8asm,obj_file);
       if( 0 != res ) {
           fprintf(stderr,"Error %d while translating\n",res);
       }
       ch8asm.lineno++;
       ch8asm.address+=2;
   }
   fclose(asm_file);
   fclose(obj_file);
   ch8asm_dump_labels(&ch8asm);
   return 0;
}
