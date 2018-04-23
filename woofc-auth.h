#pragma once

void WooFAuthInit();
void WooFAuthDeinit();

/**
 * Reads the private key from the given file
 */
void WoofAuthSetPrivateKeyFile(const char* path);

void WooFAuthSetPublicKey(const char* woofname, const char* pubkey);

void* WoofAuthGetPrivateKeyObject();
const char* WooFAuthGetPublicKey(const char* woofname);