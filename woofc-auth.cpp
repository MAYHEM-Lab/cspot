extern "C"
{
	#include "woofc-auth.h"
	#include <czmq.h>
	#include "woofc-access.h"
}

#include <unordered_map>
#include <string>
#include <iostream>

struct woof_auth_ctx
{
	std::unordered_map<std::string, std::string> public_keys;
	zcert_t* client_cert = nullptr;

	~woof_auth_ctx()
	{
		if (client_cert)
		{
			zcert_destroy(&client_cert);
		}
	}
};

static woof_auth_ctx* global_ctx;

extern "C"
{
	void WoofAuthSetPrivateKeyFile(const char* path)
	{
		global_ctx->client_cert = zcert_load(path);
	}

	void WooFAuthSetPublicKey(const char* woofname, const char* pubkey)
	{
		char ip_str[25] = {};
		char ns[255] = {};
		int port;

		WooFNameSpaceFromURI(woofname,ns,sizeof(ns));
		WooFIPAddrFromURI(woofname,ip_str,sizeof(ip_str));
		auto err = WooFPortFromURI(woofname,&port);
		if (err < 0)
		{
			port = WooFPortHash(ns);
		}

		std::string endpoint = std::string(">tcp://") + ip_str + ":" + std::to_string(port);

		//std::cout << "putting key for: " << endpoint << '\n';
		global_ctx->public_keys.emplace(endpoint, pubkey);
	}

	void* WoofAuthGetPrivateKeyObject()
	{
		return global_ctx->client_cert;
	}

	const char* WooFAuthGetPublicKey(const char* endpoint)
	{
		//std::cout << "getting key for: " << endpoint << '\n';
		//return "UkCxC]?G.Pb]5HX61Sig!2c4XyHdy>O55kUiQGpU";
		auto iter = global_ctx->public_keys.find(endpoint);
		if (iter == global_ctx->public_keys.end())
		{
			return nullptr;
		}
		return iter->second.c_str();
	}

	void WooFAuthInit()
	{
		global_ctx = new woof_auth_ctx;
	}

	void WooFAuthDeinit()
	{
		delete global_ctx;
	}
}