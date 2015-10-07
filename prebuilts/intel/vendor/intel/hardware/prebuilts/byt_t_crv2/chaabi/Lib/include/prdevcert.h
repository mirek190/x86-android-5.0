/**********************************************************************
 * Copyright 2009 - 2012 (c) Intel Corporation. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * Author:    zhang wenjie (wenjie.z.zhang@intel.com)
 **********************************************************************/

#ifndef __PRDEVCERT
#define __PRDEVCERT

int sep_ext_genkeypair(unsigned char *signpubkey, unsigned int *signkeylen,
		       unsigned char *encpubkey, unsigned int *enckeylen,
		       unsigned char *signkeycontainer,
		       unsigned int *singkeycontainerlen,
		       unsigned char *enckeycontainer,
		       unsigned int *enckeycontainerlen);

int sep_ext_signdevcert(unsigned char *devcert, unsigned int len,
			unsigned char *signatureofcert,
			unsigned int *signaturelen);
#endif /* __PRDEVCERT */
