//
//  main.c
//  zerozip
//
//  Created by hetima on 2014/03/28.
//  Copyright (c) 2014 hetima. All rights reserved.
//

#include <stdio.h>
#import <sys/stat.h>
#import "unzip.h"
#import "zip.h"

#define WRITEBUFFERSIZE (8192*8)


int fileExists(const char *path)
{
    struct stat st;
    int result=lstat(path, &st);
    return (result==0);
}

int zerozip(const char *fromFile, const char *toFile)
{
    int err;
    uLong i;
    unzFile uf=unzOpen(fromFile);
    zipFile zf;
    unz_global_info gi;
    int zip64 = 0;
    
    err = unzGetGlobalInfo (uf, &gi);
    if (err!=UNZ_OK){
        unzClose(uf);
        return err;
    }
    
    uInt size_buf = WRITEBUFFERSIZE;
    void* buf = (void*)malloc(size_buf);
    
    zf = zipOpen64(toFile, 0);

    for (i=0; i<gi.number_entry; i++){
        char filename_inzip[2048];
        unz_file_info file_info;

        err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
        if (err!=UNZ_OK){
            //printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
            break;
        }
        if ((file_info.flag & 1) != 0){
            continue;
        }
        
        zip_fileinfo zi;
        zi.dosDate = file_info.dosDate;
        zi.internal_fa = file_info.internal_fa;
        zi.external_fa = file_info.external_fa;
        
        err = unzOpenCurrentFile(uf);
        if (err!=UNZ_OK){
            continue;
        }
        
        err = zipOpenNewFileInZip3_64(zf, filename_inzip, &zi,
                                      NULL,0,NULL,0,NULL /* comment*/,
                                      0,
                                      0,0,
                                      /* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
                                      -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
                                      NULL, 0, zip64);
        if (err!=UNZ_OK) break;
        
        int size_read;


        do{
            size_read = unzReadCurrentFile(uf,buf,size_buf);
            if(size_read<0){
                err = ZIP_ERRNO;
                break;
            }
            if(size_read>0){
                err = zipWriteInFileInZip(zf,buf,size_read);
            }
            
        }while ((err == ZIP_OK) && (size_read>0));

        
        unzCloseCurrentFile(uf);
        err = unzGoToNextFile(uf);
        if (err!=UNZ_OK){
            //printf("error %d with zipfile in unzGoToNextFile\n",err);
            
            if (err==UNZ_END_OF_LIST_OF_FILE) {
                err=UNZ_OK;
            }
            
            break;
        }
    }
    

    zipClose(zf,NULL);
    unzClose(uf);
    
    free(buf);
    
    return err;
}

int main(int argc, const char * argv[])
{

    if (argc != 3) {
        printf("usage zerozip source-zipfile new-uncompressed-zipfile\n");
        return 0;
    }
    if (!fileExists(argv[1])) {
        printf("error: source file does not exist.\n");
        return 0;
    }
    if (fileExists(argv[2])) {
        printf("error: destination file exists.\n");
        return 0;
    }
    
    int err = zerozip(argv[1],argv[2]);
    if (err!=UNZ_OK){
        printf("error: %d\n",err);
    }else{
        printf("finished.\n");
    }
    return 0;
}

