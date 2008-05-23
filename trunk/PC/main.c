#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

void show_usage()
{
     printf("Usage: save_convert inputsave outputsave\n");
     exit(0);
}

int main(int argc, char **argv) {
    int fsize = 0;
    int i = 0;
    char *buffer = NULL;
    short *buffer_short = NULL;
    FILE *in;
    FILE *out;

    if(argc!=3)
        show_usage();
    
    in = fopen(argv[1],"rb");
    if(!in)
    {
        printf("Error opening %s\n",argv[1]);
        exit(1);
    }
    out = fopen(argv[2],"wb");
    if(!out)
    {
        printf("Error creating %s\n",argv[2]);
        exit(1);
    }
    
    fseek(in, 0, SEEK_END);
    fsize = ftell(in);
    fseek(in, 0, SEEK_SET);
    
    buffer = (char*)malloc(fsize);
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
    
    buffer_short = (short*)malloc(fsize);
    memcpy(buffer_short,buffer,fsize);
    
    unsigned short temp_short = 0;
    for(i=0; i<fsize/2; i+=2)
    {
        temp_short = buffer_short[i];
        buffer_short[i]   = buffer_short[i+1];
        buffer_short[i+1] = temp_short;
    }
    
    fwrite(buffer_short,1, fsize, out);
        
    fclose(in);
    fclose(out);
    
    return 0;
}
