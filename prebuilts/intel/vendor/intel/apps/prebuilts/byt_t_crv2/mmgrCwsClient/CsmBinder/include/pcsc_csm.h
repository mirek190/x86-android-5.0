/*
 * Copyright (C) Intel Corporation 2013
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PCSC_CSM_H
#define PCSC_CSM_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <pthread.h>

/* PCSC-Lite stub */

/* imported from wintypes.h*/
#ifndef BYTE
typedef unsigned char BYTE;
#endif

typedef unsigned long DWORD;
typedef unsigned long *PDWORD;

typedef const void *LPCVOID;
typedef long LONG;
typedef const char *LPCSTR;

typedef const BYTE *LPCBYTE;
typedef BYTE *LPBYTE;
typedef DWORD *LPDWORD;
typedef char *LPSTR;


/* imported from pcsclite.h */
typedef long SCARDCONTEXT; /**< \p hContext returned by SCardEstablishContext() */
typedef long SCARDHANDLE; /**< \p hCard returned by SCardConnect() */
typedef SCARDCONTEXT *LPSCARDCONTEXT;
typedef SCARDHANDLE *LPSCARDHANDLE;

#define SCARD_PROTOCOL_T0		0x0001	/**< T=0 active protocol. */
#define SCARD_PROTOCOL_T1		0x0002	/**< T=1 active protocol. */

#define SCARD_S_SUCCESS			((LONG)0x00000000) /**< No error was encountered. */
#define SCARD_E_INVALID_PARAMETER	((LONG)0x80100004) /**< One or more of the supplied parameters could not be properly interpreted. */
#define SCARD_E_NO_SMARTCARD		((LONG)0x8010000C) /**< The operation requires a Smart Card, but no Smart Card is currently in the device. */
#define SCARD_E_NOT_READY		((LONG)0x80100010) /**< The reader or smart card is not ready to accept commands. */
#define SCARD_F_COMM_ERROR		((LONG)0x80100013) /**< An internal communications error has been detected. */
#define SCARD_SHARE_SHARED		0x0002	/**< Shared mode only */

#define SCARD_LEAVE_CARD		0x0000	/**< Do nothing on close */
#define SCARD_UNPOWER_CARD		0x0002	/**< Power down on close */


/** Protocol Control Information (PCI) */
typedef struct
{
	unsigned long dwProtocol;	/**< Protocol identifier */
	unsigned long cbPciLength;	/**< Protocol Control Inf Length */
} SCARD_IO_REQUEST, *PSCARD_IO_REQUEST, *LPSCARD_IO_REQUEST;

typedef const SCARD_IO_REQUEST *LPCSCARD_IO_REQUEST;

extern SCARD_IO_REQUEST g_rgSCardT0Pci, g_rgSCardT1Pci;

#define SCARD_PCI_T0	(&g_rgSCardT0Pci) /**< protocol control information (PCI) for T=0 */
#define SCARD_PCI_T1	(&g_rgSCardT1Pci) /**< protocol control information (PCI) for T=1 */

#define SCARD_SCOPE_SYSTEM		0x0002	/**< Scope in system */

/* imported from winscard.h */

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef PCSC_API
#define PCSC_API
#endif
	PCSC_API LONG SCardEstablishContext(DWORD dwScope,
		/*@null@*/ LPCVOID pvReserved1, /*@null@*/ LPCVOID pvReserved2,
		/*@out@*/ LPSCARDCONTEXT phContext);

	PCSC_API LONG SCardReleaseContext(SCARDCONTEXT hContext);

	PCSC_API LONG SCardConnect(SCARDCONTEXT hContext,
		LPCSTR szReader,
		DWORD dwShareMode,
		DWORD dwPreferredProtocols,
		/*@out@*/ LPSCARDHANDLE phCard, /*@out@*/ LPDWORD pdwActiveProtocol);

	PCSC_API LONG SCardTransmit(SCARDHANDLE hCard,
		LPCSCARD_IO_REQUEST pioSendPci,
		LPCBYTE pbSendBuffer, DWORD cbSendLength,
		/*@out@*/ LPSCARD_IO_REQUEST pioRecvPci,
		/*@out@*/ LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);

	PCSC_API LONG SCardListReaders(SCARDCONTEXT hContext,
		/*@null@*/ /*@out@*/ LPCSTR mszGroups,
		/*@null@*/ /*@out@*/ LPSTR mszReaders,
		/*@out@*/ LPDWORD pcchReaders);

	PCSC_API LONG SCardBeginTransaction(SCARDHANDLE hCard);

	PCSC_API LONG SCardEndTransaction(SCARDHANDLE hCard, DWORD dwDisposition);

	PCSC_API LONG SCardDisconnect(SCARDHANDLE hCard, DWORD dwDisposition);

#ifdef __cplusplus
}
#endif

#endif /* PCSC_CSM_H */

