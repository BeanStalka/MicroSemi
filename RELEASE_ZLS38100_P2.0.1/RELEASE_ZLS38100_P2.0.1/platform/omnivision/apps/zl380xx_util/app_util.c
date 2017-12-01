#include "typedefs.h"
#include "chip.h"
#include "hbi.h"
#include "app_util.h"
#define HEADER_STRING(s) #s
#define HEADER(name) HEADER_STRING(name)
//#define DBG_FREAD

#ifdef LOAD_FWR_STATIC 
#ifdef FWR_C_FILE
#include HEADER(FWR_C_FILE)
#else
# error "firmware c file not defined"
#endif
#endif

#ifdef LOAD_CFGREC_STATIC
#ifdef CFGREC_C_FILE
#include HEADER(CFGREC_C_FILE)
#endif
#endif
#ifndef HBI_BUFFER_SIZE
#define HBI_BUFFER_SIZE 1024
#endif
#ifdef printf
#undef printf
#define printf debug_printf
#endif

static user_buffer_t image[HBI_BUFFER_SIZE];

static uint32_t AsciiHexToHex(const char * str, unsigned char len)
{
    uint32_t val = 0;
    char c;
    unsigned char i = 0;

    for (i = 0; i< len; i++)    
    {
        c = *str++; 


        if (c >= '0' && c <= '9')
        {
            val <<= 4;
            val += c & 0x0F;
            continue;
        }

        c &= 0xDF;
        if (c >= 'A' && c <= 'F')
        {
            val <<= 4;
            val += (c & 0x07) + 9;
            continue;
        }
    }
    return val;
}

#ifdef LOAD_CFGREC_STATIC
hbi_status_t ldcfgrec(hbi_handle_t handle,void *arg)
{
   hbi_status_t status;
   int i=0,j=0,k=0;
   size_t nread;
   user_buffer_t *pTmp=image;
   reg_addr_t reg;
   cfgrec_t *pCfgRec = (cfgrec_t *)arg;
   dataArr  *pImage = (dataArr  *)(pCfgRec->pImage);
   if(pImage == NULL)
   {
      printf("Invalid Input\n");
      return HBI_STATUS_INVALID_ARG;
   }

   while(i<(pCfgRec->ImageLen))
   {
      reg=pImage->reg;

      for(j=0,k=0;j<(pCfgRec->configBlockSize);j++,k+=2)
      {
         pTmp[k] = ((pImage->value[j]) >> 8)&0xFF;
         pTmp[k+1] = (pImage->value[j]) & 0xFF;
      }

      status = HBI_write(handle,reg,(user_buffer_t *)pTmp,(pCfgRec->configBlockSize)*2);
      if(status != HBI_STATUS_SUCCESS)
      {
         printf("HBI write failed\n");
         return status;
      }
      i++;pImage++;
  }
   return status;
}
#else
#define CFGREC_BLOCK_SIZE 256
#define CFGREC_SIZE       0x1000
#define dbg_fread(arg1,...) { }  

struct { 
    reg_addr_t reg;
    unsigned char buf[CFGREC_SIZE];
    size_t size;
}cfgrec;

hbi_status_t ldcfgrec(hbi_handle_t handle,void *arg)
{
    hbi_status_t status;
    unsigned char line[CFGREC_BLOCK_SIZE];
    File *fp=NULL;
    int bytestoread = CFGREC_BLOCK_SIZE;
    int numbytesread = 0;
    int eof=0;
    int index=0,i;

    u16 value;
    reg_addr_t reg;

    printf("ldcfgrec() ..\n");

    /*make sure sd-card has  image with the name given in file_fopen call */
    if(file_fopen(fp, &sh_efs.myFs, arg, MODE_READ)!= 0)
    {
        printf("file open failed\n");
        return HBI_STATUS_INTERNAL_ERR;
    }

    printf("1- Opening file - done....\n");

    /*read and format the data accordingly*/
    int unprocessed=0;
    
    reg_addr_t prev=0;
    
    memset(&cfgrec,0,sizeof(cfgrec));
    do {
//        printf("bytestoread %d\n",bytestoread);
        if(!bytestoread)
            break;
      
         numbytesread = file_read(fp,bytestoread,&line[unprocessed]);

         if(numbytesread < bytestoread)
         {
            printf("EOF !!\n");
            eof=1;
         }
         i=0;
         bytestoread=CFGREC_BLOCK_SIZE;
         numbytesread+=unprocessed;
         unprocessed=0;
         dbg_fread("line %s, len %d\n",line,numbytesread);
         while(i < numbytesread)
         {
            int j;
            
            
             dbg_fread(" current indx %d\n",i);

             /* get 1 line */
             index = i;
             while(i < numbytesread && line[i++]!='\n');
             if(line[i-1]!='\n'/*i>=numbytesread*/) 
             {
                 unprocessed = i-index;
                 dbg_fread("unprocessed %d\n",unprocessed);
                /*copy unprocessed data to beginning of buffer */
                for(j=0;j<unprocessed;j++)
                    line[j]=line[index+j];
                bytestoread -= unprocessed;
                break;
            }

            if(line[index]==';')
            {
                continue;
            }
            
            if(line[index]!='\n')
            {
                /* read one line */

                while(memcmp(&line[index],"0x",2))
                    index++;

                index+=2;
                j=index;
                while(line[j++] != ',');

                reg = AsciiHexToHex(&line[index],j-index);

                dbg_fread("value at loc %d\n",j);
                index = j;
                
                while(memcmp(&line[index],"0x",2))
                    index++;

                index+=2;
                j=index;
                while(line[j++] != ',');

                value = AsciiHexToHex(&line[index],j-index);

                if(!prev)
                {
                    cfgrec.reg = reg;
                }
                else if(cfgrec.size >= CFGREC_BLOCK_SIZE|| (reg != prev+2))
                {
                    /* non-contigous*/
                    printf("Writing @ 0x%x, len %d to device\n",cfgrec.reg,cfgrec.size);
                   status = HBI_write(handle,cfgrec.reg,cfgrec.buf,cfgrec.size);
                    cfgrec.reg = reg;
                    cfgrec.size=0;
                }

                cfgrec.buf[cfgrec.size++]=value >> 8;
                cfgrec.buf[cfgrec.size++]=value & 0xff;

                dbg_fread("prev = 0x%x, reg = 0x%x, value = 0x%x, "\
                           "cfgrec.buf[%d]:0x%x\n", prev, reg, value,
                                                cfgrec.size,
                                                *((uint16_t*)&(cfgrec.buf[cfgrec.size])));                
                prev=reg;
            }
            /* continue to read rest of lines */
        }
    } while (eof == 0);
    
    printf("Writing @ 0x%x, val 0x%x to device\n",cfgrec.reg,*(cfgrec.buf));

    /* write last entry */
    status = HBI_write(handle,cfgrec.reg,cfgrec.buf,cfgrec.size);   


    file_fclose(fp);

    return status;
}

#endif
#ifdef LOAD_FWR_STATIC
hbi_status_t ldfwr(hbi_handle_t handle,void *arg) {

   hbi_status_t   status = HBI_STATUS_SUCCESS;
   size_t         len;
   int            i;
   void           *tmp=NULL;
   int            c;
   hbi_data_t     data;
   uint32_t       block_size;
   hbi_img_hdr_t hdr;
   size_t        fwr_len;

   data.pData=image;

   memcpy(data.pData,buffer,HBI_BUFFER_SIZE);

   /*Firmware image is organised into chunks of fixed length and this information
     is embedded in image header. Thus first read image header and 
     then start reading chunks and loading on to device
   */
   data.size = HBI_BUFFER_SIZE;

   printf("Calling HBI_get_header()\n");

   status = HBI_get_header(&data,&hdr);
   if(status != HBI_STATUS_SUCCESS)
   {
      printf("HBI_get_header() err 0x%x \n",status);
      HBI_close(handle);
      HBI_term();
      return -1;
   }

   /* length is in unit of 16-bit words */
   block_size = (hdr.block_size)*2;
   data.size = block_size;
   fwr_len = hdr.img_len;

   if(block_size > HBI_BUFFER_SIZE)
   {
      printf("Insufficient buffer size. please recompiled with increased HBI_BUFFER_SIZE\n");
      return HBI_STATUS_RESOURCE_ERR;
   }

   printf("\nStart firmware load ...\n");

   /* Somehow direct memcpy from buffer is  giving me some memory issues.
   thus using pointer to pass on data from global buffer.
   */
   tmp = buffer;
   
   /* This loops covers all firmware loading */
   for(i=hdr.hdr_len;i<fwr_len;i+=block_size)
   {
      memcpy((void *)(data.pData),(const void *)(tmp+i),block_size);

//      printf("Writing image from buffer 0x%x len %d\n",data.pData, data.size);
      status = HBI_set_command(handle,HBI_CMD_LOAD_FWR_FROM_HOST,&data);
      if (status != HBI_STATUS_SUCCESS) 
      {
          printf("Error %d:HBI_set_command(HBI_CMD_LOAD_FWR_FROM_HOST)\n", status);
          return status;
      }
   }

   status = HBI_set_command(handle,HBI_CMD_LOAD_FWR_COMPLETE,NULL);
   if (status != HBI_STATUS_SUCCESS) {
       printf("Error %d:HBI_set_command(HBI_CMD_LOAD_FWR_COMPLETE)\n", status);
       return status;
   }
   printf("Firmware loaded into Device\n");  

   return status;
}
#else
hbi_status_t ldfwr(hbi_handle_t handle,void *arg) {

   hbi_status_t   status = HBI_STATUS_SUCCESS;
   size_t         len;
   File *file=NULL;
   File *dst=NULL;
   hbi_data_t      data;
   uint32_t        block_size;
   hbi_img_hdr_t   hdr;
   size_t          fwr_len;
   
   /* init to null on safer side*/
   data.pData=image;

#ifdef DBG_FREAD
   if(file_fopen(dst, &sh_efs.myFs, "test.bin", MODE_READ)!= 0)
      printf("couldn't open dump file\n");
#endif

   if(file_fopen(file, &sh_efs.myFs, (char *)arg, MODE_READ)!= 0)
   {
      printf("Error : couldn't open an input file %s\n",(char *)arg);
      if(dst != NULL)
         file_fclose(dst);
      return HBI_STATUS_RESOURCE_ERR;
   }

   file_read(file,HBI_BUFFER_SIZE,data.pData);
   file_setpos(file,0);

   /*Firmware image is organised into chunks of fixed length and this information
     is embedded in image header. Thus first read image header and 
     then start reading chunks and loading on to device
   */
   data.size = HBI_BUFFER_SIZE;

   printf("Calling HBI_get_header()\n");
   
   status = HBI_get_header(&data,&hdr);
   if(status != HBI_STATUS_SUCCESS)
   {
      printf("HBI_get_header() err 0x%x \n",status);
      if(file != NULL)
         file_fclose(file);
      if(dst != NULL)
         file_fclose(dst);
      return status;
   }

   /* length is in unit of 16-bit words */
   block_size = (hdr.block_size)*2;
   data.size = block_size;
   fwr_len = hdr.img_len;

   if(block_size > HBI_BUFFER_SIZE)
   {
      printf("Insufficient buffer size. please recompiled with increased HBI_BUFFER_SIZE\n");
      
      if(file != NULL)
         file_fclose(file);
      if(dst != NULL)
         file_fclose(dst);

      return HBI_STATUS_RESOURCE_ERR;
   }


   /* skip header from file.
      Re-adjust file pointer to start of actual data. 
    */
   
   file_setpos(file,hdr.hdr_len);
   len=0;

   printf("\nStart firmware load ...\n");

   while(len < fwr_len)
   {
      file_read(file,block_size,data.pData);
      len+=block_size;

      if(dst!=NULL)
         file_write(dst,block_size,data.pData);

//      printf("Writing image from buffer 0x%x len %d\n",data.pData, data.size);

      status = HBI_set_command(handle,HBI_CMD_LOAD_FWR_FROM_HOST,&data);
      if (status != HBI_STATUS_SUCCESS) 
      {
          printf("Error %d:HBI_set_command(HBI_CMD_LOAD_FWR_FROM_HOST)\n", status);
          if(file !=NULL)
             file_fclose(file);
           if(dst!=NULL)
             file_fclose(dst);
          return status;
      }

   }

   status = HBI_set_command(handle,HBI_CMD_LOAD_FWR_COMPLETE,NULL);
   if (status != HBI_STATUS_SUCCESS) {
       printf("Error %d:HBI_set_command(HBI_CMD_START_FWR)\n", status);
       if(file !=NULL)
          file_fclose(file);
        if(dst!=NULL)
         file_fclose(dst);
       return status;
   }

   printf("Firmware loaded into Device\n");  

   if(file !=NULL)
      file_fclose(file);
   if(dst!=NULL)
      file_fclose(dst);

   return HBI_STATUS_SUCCESS;
}
#endif
