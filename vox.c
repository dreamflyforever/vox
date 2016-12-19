/*  Filename: vox.c
    Description:  Kind of like the sox program.  It converts voice
          file formats.  Converts 16 bit and 8 bit raw voice files to
          the Dialogic or Oki ADPCM (foo.vox or foo.32K) file format. 
          Of course, the files have to sampled at the right amount for
          them to work with Dialogic hardware.
          Files sampled at 8-kHz are converted to the 32K.
          Files sampled at 6053 Hz are converted to the 24K -- normal
          vox format.

    Usage: vox [-b 8|-b 16] infile outfile

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

int read12(int, int, short *, int );    /* program to read and convert
                                        * data to 12 bit format.
                                        */

int main (int argc, char **argv)
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
        fprintf( stderr, "%s: USAGE: vox [-b 8|-b 16] infile outfile\n",
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
    * Read the input file and convert the samples to 12 bit -- 
    * which is not support by Sound Blaster hardware, but is needed
    * to accurately implement the Dialogic ADPCM algorithm.
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
    adpcm=(char*) calloc (buffer_size/2,sizeof(unsigned char));
    /* Check that memory was allocated correctly */
    if (adpcm==NULL) { 
        fprintf (stderr,"%s: Malloc Error",argv[0] );
        exit(1);  
    }    
    /*
    * Need different read commands for 8 bit and 16 bit data.
    * Read the data; continue until end of file
    */

    while ((n=read12(infile, sample_size, buffer12, buffer_size))>0 ) {
        /*
        * Convert data to Dialogic ADPCM format
        * Note that two ADPCM samples are stored in (8 bit) char,
        * because the ADPCM samples are only 4 bits.
        */
        j = 0;
        for(i=0; i<n/2; i++) {
            adpcm[i] = adpcm_encode( buffer12[j++], &coder_stat )<<4;
            if( j > n )  /* only true for last sample when n is odd */
                adpcm[i] |= adpcm_encode(0, &coder_stat);
            else
                adpcm[i] |= adpcm_encode(buffer12[j++], &coder_stat);
        }
        /*
        * now write the output file.
        */
        n /= 2;
        if(write(outfile, adpcm, n*sizeof(unsigned char)) < 0) {
            fprintf (stderr,"Error in writing file.");	
            exit(1);
        }
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
* program to read and convert data to 12 bit format.
*/

int read12(int infile,int sample_size, short *buffer12,int buffer_size)
{
    int i, n;
    short j, sign;
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
        * Read the next block of data;
        */
        if ((n=read(infile,buffer8,buffer_size*sizeof(unsigned char)))<0 ) {
            fprintf (stderr,"Error in reading file.");	
            exit(1);
        }
        /* 
        * Convert the 8 bit samples to 12 bit
        * Need subtract 128 first because it is a unsigned char.
        */
        for (i=0; i<n; i++) {
            buffer12[i] = (short)buffer8[i] - 128;
            buffer12[i] *= 16;
        }
        free( buffer8 );
    }
    else {  /* now read the 16 bit data */
        /* 
        * Read the next block of data;
        * Can use the 12-bit buffer to read the 16 bit data.
        */
        if ((n=read(infile,buffer12,buffer_size*sizeof(short)))<0 ) {
            fprintf (stderr,"Error in reading file.");	
            exit(1);
        }
        /* 
        * Convert the 16 bit samples to 12 bit.
        * Note that n is the number of bytes read, which is twice
        * the number samples read because sizeof(short) == 2.
        */
        n /= 2;
        for (i=0; i<n; i++) {
            buffer12[i] /= 16;
        }
    }
    return(n);
}
