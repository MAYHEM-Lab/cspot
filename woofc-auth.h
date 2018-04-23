#pragma once

void WooFAuthInit();
void WooFAuthDeinit();

/**
 * Reads the private key from the given file
 */
void SetPrivateKeyFile(const char* path);

void SetWooFPublicKey(const char* woofname, const char* pubkey);

void* GetPrivateKeyObject();
const char* WooFAuthGetPublicKey(const char* woofname);