# First example of assembler
START: MOV V0,0
#Next line will be skipped
SNE V0,0
MOV V1,fh
MOV V1,0
#now next line will not be skipped an we return to START
SNE V1,1
JMP 200h
