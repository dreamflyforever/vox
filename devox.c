/*  Filename: devox.c
    Description:  Kind of like the program sox.  It converts voice
          file formats.  Converts 
          the Dialogic or Oki ADPCM (foo.vox or foo.32K) file format
          to 16 bit and 8 bit raw voice files.

    Usage: devox [-b 8|-b 16] infile outfile

        The -b is for 8 or 16 bit files (reference to input files)
        Default is 8 bits.
*/

#include <unistd.h>           /* needed for getopt */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "adpcm.h"

void write12(int, int, short *, int );    /* program to write and convert
                                        * data to 12 bit format.
                                        */
int
main (int argc, char **argv)
{
    int c, i, j; 
    extern char *optarg;
    extern int optind;
    int infile, outfile, n;
    int buffer_size, sample_size;
    short *buffer12;
    char *adpcm;
    struct adpcm_status coder_stat;

    /*
     * Process the arguments.
     */
    sample_size = 1;   /* default to 8 bit */
    while ((c = getopt(argc, argv, "b:")) != -1) {
        switch (c) {
        case 'b':
            /*
            * set bits per sample to 8 or 16 - sample_size to
            * 1 or 2 bytes.
            */
            switch (atoi(optarg)) {
                case 8:
                    sample_size = 1;
                    break;
                case 16:
                    sample_size = 2;
                    break;
                default:
                    fprintf(stderr, "Wrong bit specification, 8 bit/sample used.\n");
                    sample_size = 1;
                    break;
            }
            break;
        default:
            /*
            * set bits per sample to 8 - sample_size to 1 byte.
            */
            sample_size = 1;
            break;
        }
    }
    
    /*
     * Process extra arguments. (infile outfile)
     */
    if (argc - optind != 2) {
        fprintf( stderr, "%s: USAGE: devox [-b 8|-b 16] infile outfile\n",
               argv[0]);
        exit(1);
    }

    /*
    * Open the input file for reading.
    */
    if ((infile = open(argv[optind], O_RDONLY)) < 0) {
        perror(argv[optind]);
        exit(1);
    }
    
    /*
    * Open the output file for writing.
    */
    if ((outfile = open(argv[++optind], O_WRONLY | O_CREAT, 0666)) < 0) {
        perror(argv[optind]);
        exit(1);
    }
    
    /* 
    * convert the 12 bit samples linear to the final format of either
    * 8 or 16 bit and write the output file.
    */
    /*
    * Allocate memory for the buffer of 12 bit data.
    */
    buffer_size = 1024;
    buffer12=(short*) calloc (buffer_size,sizeof(short));
    /* Check that memory was allocated correctly */
    if (buffer12==NULL) { 
        fprintf (stderr,"%s: Malloc Error",argv[0] );
        exit(1);  
    }
    /*
    * Initialize the coder.
    */
    adpcm_init( &coder_stat );
    /*
    * Allocate memory for the buffer of ADPCM samples.
    */
    adpcm=(char*) calloc (buffer_size/2,sizeof(char));
    /* Check that memory was allocated correctly */
    if (adpcm==NULL) { 
        fprintf (stderr,"%s: Malloc Error",argv[0] );
        exit(1);  
    }    
    /*
    * Read the data; continue until end of file
    */
    while ((n=read(infile, adpcm, buffer_size*sizeof(char)/2))>0 ) {
        /*
        * Convert data to linear  format
        * Note that two ADPCM samples are stored in (8 bit) char,
        * because the ADPCM samples are only 4 bits.
        */
        j = 0;
        for(i=0; i<n; i++) {
            buffer12[j++] = adpcm_decode( (adpcm[i]>>4)&0x0f, &coder_stat );
            buffer12[j++] = adpcm_decode( adpcm[i]&0x0f, &coder_stat );
        }
        /*
        * now convert from 12 bit to either 8 or 16 and write the output file.
        */
        write12(outfile, sample_size, buffer12, n*2);
    }
    /*
    * free allocated memory
    */
    free( buffer12 );
    free( adpcm );

    /*
    *  Close the input and output files
    */
    close(infile);
    close(outfile);

    exit(0);
}

/*
* program to convert data from 12 bit format and write it.
*/

void write12(int outfile,int sample_size, short *buffer12,int buffer_size)
{
    int i;
    unsigned char *buffer8;	

    /*
    * The 8 bit case first.
    * The second bit of sample_size indicates whether 8 or 16 bit, hence the
    * bitwise operation in the condition.
    */
    if (!(sample_size>>1)) {
        /* 
        * Allocate memory for the buffer.
        */
        buffer8=(unsigned char*) calloc (buffer_size,sizeof(unsigned char));
        /* Check that memory was allocated correctly */
        if (buffer8==NULL) { 
            fprintf (stderr,"Malloc Error" );
            exit(1);  
        }
        /* 
        * Convert the 12 bit samples to 8 bit
        * Need to add 128 because it is a unsigned char.
        */
        for (i=0; i<buffer_size; i++) {
            //buffer12[i] /= 16;
            buffer12[i] /= 32;
            buffer8[i] = (char)(buffer12[i] + 128);
        }    
        /* 
        * Write the next block of data;
        */
        if (write(outfile,buffer8,buffer_size*sizeof(char))<0 ) {
            fprintf (stderr,"Error in writing file.");	
            exit(1);
        }
        free( buffer8 );
    }
    else {  /* now write the 16 bit data */
       
        /* 
        * Convert the 12 bit samples to 16 bit.
        */
        for (i=0; i<buffer_size; i++) {
            buffer12[i] *= 16;
            // Compiler should implement at a shift left 4 bits.
            // The following was a temporary work around for a
            // clipping problem. -- believed to be fixed in version
            // 1.1 (see web page and adpcm.c line 99). TLB 3/30/04
            //buffer12[i] *= 8;
        }
        /* 
        * write the next block of data;
        * Can use the 12-bit buffer for the 16 bit data.
        */
        if (write(outfile,buffer12,buffer_size*sizeof(short))<0 ) {
            fprintf (stderr,"Error in writing file.");	
            exit(1);
        }
    }
    return;
}
