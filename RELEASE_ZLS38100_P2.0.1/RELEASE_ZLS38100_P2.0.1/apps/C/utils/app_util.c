#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "typedefs.h"
#include "chip.h"
#include "hbi.h"
#include "app_util.h"
#define HEADER_STRING(s) #s
#define HEADER(name) HEADER_STRING(name)
//#define DBG_FREAD


#ifndef HBI_BUFFER_SIZE
#define HBI_BUFFER_SIZE 1024
#endif

#undef DBG
#undef DBG2
static user_buffer_t image[HBI_BUFFER_SIZE];


#if defined(LOAD_FWR_STATIC) || defined(LOAD_CFGREC_STATIC)

hbi_status_t vproc_load_image(hbi_handle_t handle, uint8_t *pArg) {

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
   
   memcpy(data.pData,pArg,HBI_BUFFER_SIZE);

   /*Firmware image is organised into chunks of fixed length and this information
     is embedded in image header. Thus first read image header and 
     then start reading chunks and loading on to device
   */
   data.size = HBI_BUFFER_SIZE;

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

   printf("\nSending image data ...\n");

   /* Somehow direct memcpy from buffer is  giving me some memory issues.
   thus using pointer to pass on data from global buffer.
   */
   
//   tmp = pArg;
   /* This loops covers all firmware loading */
   for(i=hdr.hdr_len;i<fwr_len;i+=block_size)
   {
      memcpy((void *)(data.pData),(const void *)(pArg+i),block_size);

//      printf("Writing image from buffer 0x%x len %d\n",data.pData, data.size);
      if (hdr.image_type == HBI_IMG_TYPE_FWR)
      {
          status = HBI_set_command(handle,HBI_CMD_LOAD_FWR_FROM_HOST,&data);
      } else if(hdr.image_type == HBI_IMG_TYPE_CR) 
      {      
          status = HBI_set_command(handle,HBI_CMD_LOAD_CFGREC_FROM_HOST,&data);
      } else {
          printf("Error %d:Unrecognized image type %d\n", hdr.image_type);
          status = HBI_STATUS_INVALID_ARG;  
      }      
      if (status != HBI_STATUS_SUCCESS) 
      {
          printf("Error %d:HBI_set_command(HBI_CMD_LOAD_FWR_FROM_HOST)\n", status);
          return status;
      }
   }
   if (hdr.image_type == HBI_IMG_TYPE_FWR)
   {
       status = HBI_set_command(handle,HBI_CMD_LOAD_FWR_COMPLETE,NULL);
       if (status != HBI_STATUS_SUCCESS) {
           printf("Error %d:HBI_set_command(HBI_CMD_LOAD_FWR_COMPLETE)\n", status);
           return status;
       }
   }
   printf("Image loaded into Device\n");  

   return status;
}
#else
hbi_status_t vproc_load_image(hbi_handle_t handle, uint8_t *pArg) {

   hbi_status_t   status = HBI_STATUS_SUCCESS;
   size_t         len;
   int            i;
   int             c;
   char            *inpath;
   FILE            *file=NULL;
   FILE           *dst=NULL;
   hbi_data_t      data;
   uint32_t        block_size;
   hbi_img_hdr_t   hdr;
   size_t          fwr_len;
   
   /* init to null on safer side*/
   data.pData=image;

#ifdef DBG_FREAD
   dst = fopen("test.bin","wb");
   if(dst==NULL)
      printf("couldn't open dump file\n");
#endif

   file=fopen((char *)pArg,"rb");
   if(file == NULL)
   {
      printf("Error : couldn't open an input file %s\n",(char *)pArg);
      if(dst != NULL)
         fclose(dst);
      return HBI_STATUS_RESOURCE_ERR;
   }

   fread(data.pData,1,HBI_BUFFER_SIZE,file);
   fseek(file,0,SEEK_SET);

   /*Firmware image is organised into chunks of fixed length and this information
     is embedded in image header. Thus first read image header and 
     then start reading chunks and loading on to device
   */
   data.size = HBI_BUFFER_SIZE;
   
   status = HBI_get_header(&data,&hdr);
   if(status != HBI_STATUS_SUCCESS)
   {
      printf("HBI_get_header() err 0x%x \n",status);
      if(file != NULL)
         fclose(file);
      if(dst != NULL)
         fclose(dst);
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
         fclose(file);
      if(dst != NULL)
         fclose(dst);

      return HBI_STATUS_RESOURCE_ERR;
   }

   printf("\nSending image data ...\n");

   /* skip header from file.
      Re-adjust file pointer to start of actual data. 
    */
   fseek(file,hdr.hdr_len,SEEK_CUR);
   len=0;
   while(len < fwr_len)
   {
      fread(data.pData,1,block_size,file);
      len+=block_size;

      if(dst!=NULL)
         fwrite(data.pData,1,block_size,dst);

      if (hdr.image_type == HBI_IMG_TYPE_FWR)
      {
          status = HBI_set_command(handle,HBI_CMD_LOAD_FWR_FROM_HOST,&data);
      } else if(hdr.image_type == HBI_IMG_TYPE_CR)
      {
          status = HBI_set_command(handle,HBI_CMD_LOAD_CFGREC_FROM_HOST,&data);
      } else {
          printf("Error %d:Unrecognized image type %d\n", hdr.image_type);
          status = HBI_STATUS_INVALID_ARG;  
      }      
      if (status != HBI_STATUS_SUCCESS) 
      {
          printf("Error %d:HBI_set_command(HBI_CMD_LOAD_FWR_FROM_HOST)\n", status);
          if(file !=NULL)
             fclose(file);
           if(dst!=NULL)
             fclose(dst);
          return status;
      }
   }

   if (hdr.image_type == HBI_IMG_TYPE_FWR)
   {
       status = HBI_set_command(handle,HBI_CMD_LOAD_FWR_COMPLETE,NULL);
       if (status != HBI_STATUS_SUCCESS) {
           printf("Error %d:HBI_set_command(HBI_CMD_START_FWR)\n", status);
           if(file !=NULL)
              fclose(file);
            if(dst!=NULL)
              fclose(dst);
           return status;
       }
    }
   printf("Image loaded into Device\n");  

   if(file !=NULL)
      fclose(file);
   if(dst!=NULL)
      fclose(dst);

   return HBI_STATUS_SUCCESS;
}
#endif
