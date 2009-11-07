#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

void show_usage(char **argv)
{
     printf("Usage: %s inputsave outputsave\n",argv[0]);
     exit(-1);
}

int main(int argc, char **argv) {
    int fsize = 0;
    int i = 0;

    if(argc!=3)
        show_usage(argv);
    
    printf("PC->GC/Wii Save swapper\n\n");
    printf("Opening input file ..\n");
    FILE *in = fopen(argv[1],"rb");
    if(!in)
    {
        printf("Error opening input file %s\n",argv[1]);
        exit(-1);
    }
    printf("Creating output file ..\n");
    FILE *out = fopen(argv[2],"wb");
    if(!out)
    {
        printf("Error creating output file %s\n",argv[2]);
        fclose(in);
        exit(-1);
    }
    
    fseek(in, 0, SEEK_END);
    fsize = ftell(in);
    fseek(in, 0, SEEK_SET);
    if(!fsize)
    {
      printf("File size error\n");
      fclose(in);
      fclose(out);
      exit(-1);
    }
    printf("Swapping ..\n");
    char *buffer = (char*)malloc(fsize);
    if(buffer == NULL)
    {
        printf("Error allocating %i bytes for save!\n",fsize);
        exit(1);
    }
    
    fread(buffer, 1, fsize, in);
    unsigned char temp_byte = 0;
    for(i=0; i<fsize; i+=2)
    {
        temp_byte = buffer[i];
        buffer[i]   = buffer[i+1];
        buffer[i+1] = temp_byte;
    }
    
    short *buffer_short = (short*)malloc(fsize);
    memcpy(buffer_short,buffer,fsize);
    
    unsigned short temp_short = 0;
    for(i=0; i<fsize/2; i+=2)
    {
        temp_short = buffer_short[i];
        buffer_short[i]   = buffer_short[i+1];
        buffer_short[i+1] = temp_short;
    }
    printf("Writing swapped file ..\n\n");
    fwrite(buffer_short,1, fsize, out);    
    
    fclose(in);
    fclose(out);
    free(buffer);
    free(buffer_short);
    printf("done !!\n");
    
    return 0;
}
