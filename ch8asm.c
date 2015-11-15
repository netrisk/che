#include <stdio.h>

typedef struct ch8asm_t{
    size_t lineno; /**< current line number is being processed */
    char* line;/**< current line is being processed */ 
    ssize_t linelen; /**< current line length */
}ch8asm_t;


static int ch8asm_process_line(ch8asm_t* ch8asm){
    if( ch8asm->linelen <= 0 )
        return 0;

    return 0;
}

int main(int argc,char** argv){
   ch8asm_t ch8asm;
   FILE* asm_file;

   if( argc < 2 ) {
        fprintf(stderr,"ERROR:Specifify the assembler file\n");
        return -1;
   }
   asm_file = fopen(argv[1],"r");
   if( NULL == asm_file ){
       fprintf(stderr,"Error openeing the file\n");
       return -2;
   }
   ch8asm.lineno = 0;
   ch8asm.line = NULL;
   size_t linecap=0;
   
   while( (ch8asm.linelen = getline(&ch8asm.line,&linecap,asm_file)) > 0 ){
        fprintf(stderr,"line=%s",ch8asm.line);
        if( 0 != ch8asm_process_line(&ch8asm) ) {
            fprintf(stderr,"Error processing line %zu\n",ch8asm.lineno);
        }
   }
   fclose(asm_file);
   return 0;
}
