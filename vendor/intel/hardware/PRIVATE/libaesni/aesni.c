/*
 * Copyright (C) 2010-2013 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include "iaesni.h"
#include "AES.h"

#define BLOCK_SIZE (16)
/**
 * Function to check if hardware supports AESNI
 */
JNIEXPORT jint JNICALL Java_com_android_org_bouncycastle_crypto_paddings_PaddedBufferedBlockCipher_checkAesNI(JNIEnv *env, jobject thisObj)
{
   return check_for_aes_instructions();
}
/**
 * Function to call into AESNI assembly module
 */
JNIEXPORT jint JNICALL Java_com_android_org_bouncycastle_crypto_paddings_PaddedBufferedBlockCipher_aesNI
  (JNIEnv *env, jobject thisObj, jbyteArray inArray, jbyteArray keyArray, jbyteArray outArray,jbyteArray init_vector, jint buffLen, jint Enc, jint Keylen)
{
   //IV and input buffer
   UCHAR *IV= NULL;
   UCHAR *vector = NULL;
   UCHAR *key = NULL;
   UCHAR *result = NULL;

   //Function pointer array for AES functions
   static void (*operation_ptr[3][2])(UCHAR *vector,UCHAR *testResult,UCHAR *key,size_t numBlocks,UCHAR *IV) = {
       {intel_AES_dec128_CBC,intel_AES_enc128_CBC},
       {intel_AES_dec192_CBC,intel_AES_enc192_CBC},
       {intel_AES_dec256_CBC,intel_AES_enc256_CBC}};

   //Operation must be in block size increments
   //AES CBC type operations must be in block size multiples
   // Enc - Valid range check
   if((buffLen%BLOCK_SIZE!=0) || ((Enc != 0) && (Enc != 1)))
   {
       return 0;
   }

   //Copy contents to local buffer
   vector= (UCHAR*)(*env)->GetByteArrayElements(env, inArray, NULL);
   if(vector == NULL)
   {
       buffLen = 0;
       goto out;
   }
   IV= (UCHAR*)(*env)->GetByteArrayElements(env,init_vector,NULL);
   if(IV == NULL)
   {
       buffLen = 0;
       goto out;
   }
   key= (UCHAR*)(*env)->GetByteArrayElements(env,keyArray,NULL);
   if(key == NULL)
   {
       buffLen = 0;
       goto out;
   }

   result = (UCHAR*)malloc(buffLen);
   if(result == NULL)
   {
       buffLen = 0;
       goto out;
   }
   //Number of blocks of Data
   int numBlocks=buffLen/BLOCK_SIZE;
   //Initialize result array
   memset(result,0xee,buffLen);

   int index= ((Keylen>>3)-2) & (0x3);
   if(index > 2)
   {
       buffLen = 0;
       goto out;
   }

   //Call AESNI assembly functions to perform our operation switch according to keysize(128,192,256)
   (*operation_ptr[index][Enc])(vector, result, key, numBlocks, IV);

    (*env)->SetByteArrayRegion(env, outArray, 0 , buffLen, (jbyte*)result);

    //Free memory
out:
    if (result) free(result);

    if (vector) (*env)->ReleaseByteArrayElements(env,inArray,vector,0);

    if (key) (*env)->ReleaseByteArrayElements(env,keyArray,key,0);

    if (IV) {
        (*env)->ReleaseByteArrayElements(env,init_vector,IV,0);
    }

    return buffLen;
}

