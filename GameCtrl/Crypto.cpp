// Crypto.cpp : Defines the cryptographic functions needed by the application.
//

#include "stdafx.h"
#include "GameCtrl.h"

#include <windows.h>
#include <stdio.h>
#include <bcrypt.h>


#ifndef STATUS_SUCCESS
	#define STATUS_SUCCESS 0
#endif
#ifndef STATUS_NOT_FOUND
	#define STATUS_NOT_FOUND 0xC0000225
#endif


#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)

static BCRYPT_ALG_HANDLE hAesAlg = NULL;
static PUCHAR pbKeyObject = NULL;
static BCRYPT_KEY_HANDLE hKey = NULL;
static PBYTE pbIV = NULL;
static DWORD cbBlockLen = 0;


//! from:
//! https://docs.microsoft.com/fr-fr/windows/desktop/SecCNG/encrypting-data-with-cng
//!

static const BYTE rgbIV[] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static const BYTE rgbAES128Key[] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

BOOL initEncryption()
{
	// Open an algorithm handle.
	NTSTATUS status = BCryptOpenAlgorithmProvider(&hAesAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
	if (STATUS_SUCCESS != status)
	{
		CheckError("Algorithme non trouvé:", status);
		return FALSE;
	}

	// Calculate the size of the buffer to hold the KeyObject.
	DWORD cbKeyObject = 0;
	DWORD cbData = 0;
	status = BCryptGetProperty(hAesAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbKeyObject, sizeof(DWORD), &cbData, 0);
	if (STATUS_SUCCESS != status)
	{
		CheckError("Taille de la clé de l'algorithme non définie:", status);
		return FALSE;
	}

	// Allocate the key object on the heap.
	pbKeyObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbKeyObject);
	if (NULL == pbKeyObject)
	{
		CheckError("Echec d'allocation mémoire:", GetLastError());
		return FALSE;
	}

	// Calculate the block length for the IV.
	status = BCryptGetProperty(hAesAlg, BCRYPT_BLOCK_LENGTH, (PBYTE)&cbBlockLen, sizeof(DWORD), &cbData, 0);
	if (STATUS_SUCCESS != status)
	{
		CheckError("Taille des blocs de l'algorithme non définie:", status);
		return NULL;
	}

	// Determine whether the cbBlockLen is not longer than the IV length.
	if (cbBlockLen > sizeof(rgbIV))
	{
		CheckError("Taille du vecteur d'initialisation incompatible:", -1);
		return NULL;
	}

	// Allocate a buffer for the IV.
	// The buffer is consumed during the encrypt/decrypt process.
	pbIV = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbBlockLen);
	if (NULL == pbIV)
	{
		CheckError("Echec d'allocation mémoire:", GetLastError());
		return NULL;
	}

	status = BCryptSetProperty(hAesAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
	if (STATUS_SUCCESS != status)
	{
		CheckError("Impossible de configurer le mode enchaîne de l'algorithme:", status);
		return NULL;
	}

	// Generate the key from supplied input key bytes.
	
	status = BCryptGenerateSymmetricKey(hAesAlg, &hKey,
										pbKeyObject, cbKeyObject,
										(PBYTE)rgbAES128Key, sizeof(rgbAES128Key),
										0);
	if (STATUS_SUCCESS != status)
	{
		CheckError("Impossible de générer une clé symétrique:", status);
		return NULL;
	}

	return TRUE;
}

BOOL closeEncryption()
{
	if (NULL == hAesAlg)
		return FALSE;

	NTSTATUS status = BCryptCloseAlgorithmProvider(hAesAlg, 0);
	if (STATUS_SUCCESS != status)
	{
		CheckError("Algorithme corrompu", status);
		return FALSE;
	}
	else
		hAesAlg = NULL;
	if (NULL != hKey)
	{
		status = BCryptDestroyKey(hKey);
		if (STATUS_SUCCESS != status)
		{
			CheckError("Algorithme corrompu", status);
			return NULL;
		}
		else
			hKey = NULL;
	}
	if (NULL != pbKeyObject)
	{
		HeapFree(GetProcessHeap(), 0, pbKeyObject);
		pbKeyObject = NULL;
	}
	if (NULL != pbIV)
	{
		HeapFree(GetProcessHeap(), 0, pbIV);
		pbIV = NULL;
	}

	return TRUE;
}

unsigned char* encrypt(const unsigned char*data, DWORD &dataSize)
{
	if ((NULL == hAesAlg) || (NULL == data) || (0 == dataSize))
		return NULL;

	memcpy(pbIV, rgbIV, cbBlockLen);
	//
	// Get the output buffer size.
	//
	DWORD cbCipherText = 0;
	NTSTATUS status = BCryptEncrypt(hKey, (PUCHAR)data, dataSize, NULL,
									pbIV, cbBlockLen, NULL, 0,
									&cbCipherText, BCRYPT_BLOCK_PADDING);
	if (STATUS_SUCCESS != status)
	{
		CheckError("Echec du calcul de la taille du chiffrement", status);
		return NULL;
	}

	PBYTE pbCipherText = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbCipherText);
	if (NULL == pbCipherText)
	{
		CheckError("Echec d'allocation mémoire:", GetLastError());
		return NULL;
	}
	else
		memset(pbCipherText, 0, cbCipherText);

	// Use the key to encrypt the plaintext buffer.
	// For block sized messages, block padding will add an extra block.
	DWORD cbData = 0;
	status = BCryptEncrypt(	hKey, (PUCHAR)data, dataSize, NULL,
							pbIV, cbBlockLen, pbCipherText, cbCipherText,
							&cbData, BCRYPT_BLOCK_PADDING);
	if (STATUS_SUCCESS != status)
	{
		CheckError("Echec de chiffrement", GetLastError());
		return NULL;
	}

	dataSize = cbCipherText;
	return pbCipherText;
}


unsigned char* decrypt(const unsigned char*data, DWORD &dataSize)
{
	if ((NULL == hAesAlg) || (NULL == data) || (0 == dataSize))
		return NULL;

	memcpy(pbIV, rgbIV, cbBlockLen);
	//
	// Get the output buffer size.
	//
	DWORD cbPlainText = 0;
	NTSTATUS status = BCryptDecrypt(hKey, (PUCHAR)data, dataSize, NULL,
									pbIV, cbBlockLen, NULL, 0,
									&cbPlainText, BCRYPT_BLOCK_PADDING);
	if (STATUS_SUCCESS != status)
	{
		CheckError("Echec du calcul de la taille du chiffrement", status);
		return NULL;
	}

	PBYTE pbPlainText = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbPlainText);
	if (NULL == pbPlainText)
	{
		CheckError("Echec d'allocation mémoire:", GetLastError());
		return NULL;
	}
	else
		memset(pbPlainText, 0, cbPlainText);

	// Use the key to encrypt the plaintext buffer.
	// For block sized messages, block padding will add an extra block.
	DWORD cbData = 0;
	status = BCryptDecrypt(hKey, (PUCHAR)data, dataSize, NULL,
						   pbIV, cbBlockLen, pbPlainText, cbPlainText,
						   &cbData, BCRYPT_BLOCK_PADDING);
	if (STATUS_SUCCESS != status)
	{
		CheckError("Echec de chiffrement", GetLastError());
		return NULL;
	}

	dataSize = cbPlainText;
	return pbPlainText;
}