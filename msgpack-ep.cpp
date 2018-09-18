#include "msgpack-ep.h"
#include <iostream>
#include <thread>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cwpack.hpp>

#include <string>
#include <vector>

#include <memory>

extern "C"
{
#include "woofc-access.h"
#include "woofc.h"
void WooFFree(WOOF *wf);
}

struct sock_raii
{
	sock_raii(int fd) : m_fd(fd) {}
	~sock_raii()
	{
		close(m_fd);
	}
private:
	int m_fd;
};

template <class Container, class From>
Container parse_container(From& f)
{
	using el_t = typename Container::value_type;
	return Container { (const el_t*)f.start, (const el_t*)f.start + f.length };
}

void handle_put(int sock_fd, cw_unpack_context& uc)
{
    cw_unpack_next(&uc);
    if (uc.item.type != cwpack_item_types::CWP_ITEM_STR)
    {
    	std::cerr << "should've been a string\n";
    	return;
    }

    auto wf = parse_container<std::string>(uc.item.as.str);

    cw_unpack_next(&uc);
    if (uc.item.type != cwpack_item_types::CWP_ITEM_STR)
    {
    	std::cerr << "should've been a string\n";
    	return;
    }

    auto handler = parse_container<std::string>(uc.item.as.str);

    cw_unpack_next(&uc);
    if (uc.item.type != cwpack_item_types::CWP_ITEM_BIN)
    {
    	std::cerr << "should've been a binary array\n";
    	return;
    }

    auto buf = parse_container<std::vector<uint8_t>>(uc.item.as.bin);

    auto err = WooFPut(wf.c_str(), handler.c_str(), buf.data());

	char b[32];
	tos::msgpack::packer p {b};
	p.insert(uint32_t(err));
	auto res = p.get();

	write(sock_fd, res.data(), res.size());
}

using woof_ptr = std::unique_ptr<WOOF, decltype(&WooFFree)>;

size_t get_elem_sz(const char* woof)
{
	auto wf = woof_ptr(WooFOpen(woof), WooFFree);
	if (!wf) return -1;
	return wf->shared->element_size;
}

void handle_el_sz(int sock_fd, cw_unpack_context& uc)
{
    cw_unpack_next(&uc);
    if (uc.item.type != cwpack_item_types::CWP_ITEM_STR)
    {
    	std::cerr << "should've been a string\n";
    	return;
    }

    auto wf_name = parse_container<std::string>(uc.item.as.str);

    std::cerr << "sz of " << wf_name << '\n';

	char buf[32];
	tos::msgpack::packer p {buf};
	p.insert(uint32_t(get_elem_sz(wf_name.c_str())));
	auto res = p.get();

	write(sock_fd, res.data(), res.size());
}

void handle_get(int sock_fd, cw_unpack_context& uc)
{
	cw_unpack_next(&uc);
    if (uc.item.type != cwpack_item_types::CWP_ITEM_STR)
    {
    	std::cerr << "should've been a string\n";
    	return;
    }

    auto wf = parse_container<std::string>(uc.item.as.str);

	cw_unpack_next(&uc);
    if (uc.item.type != cwpack_item_types::CWP_ITEM_POSITIVE_INTEGER)
    {
    	std::cerr << "should've been an unsigned int (woof seq)\n";
    	return;
    }

    uint64_t seq = uc.item.as.u64;

    std::cerr << "woof get from " << wf << " at " << seq << '\n';

    auto elem_sz = get_elem_sz(wf.c_str());

    std::vector<uint8_t> buf(elem_sz);
    auto res = WooFGet(wf.c_str(), buf.data(), seq);

    std::vector<char> packet_buf(elem_sz + 4);
    tos::msgpack::packer p({packet_buf.data(), size_t(packet_buf.size())});
    auto arr = p.insert_arr(2);
    arr.insert(uint32_t(res));
    arr.insert(tos::span<const uint8_t>{ buf.data(), size_t(buf.size()) });

	auto r = p.get();

	write(sock_fd, r.data(), r.size());
}

void handle_sock(int sock_fd, sockaddr_in addr)
{
	sock_raii closer{sock_fd};

	while (true)
	{
		char buffer[512];
	    auto len = read(sock_fd, buffer, 512);
	    if (len <= 0) return;

		cw_unpack_context uc;
	    cw_unpack_context_init (&uc, buffer, len, nullptr);

	    cw_unpack_next(&uc);
	    if (uc.item.type != cwpack_item_types::CWP_ITEM_ARRAY)
	    {
	    	std::cerr << "should've been an array\n";
	    	return;
	    }

	    cw_unpack_next(&uc);
	    if (uc.item.type != cwpack_item_types::CWP_ITEM_POSITIVE_INTEGER)
	    {
	    	std::cerr << "should've been a tag\n";
	    	return;
	    }

	    uint8_t tag = uc.item.as.u64;

	    switch (tag)
	    {
	    	case WOOF_MSG_PUT:
	    		handle_put(sock_fd, uc);
	    		break;
	    	case WOOF_MSG_GET_EL_SIZE:
	    		handle_el_sz(sock_fd, uc);
	    		break;
    		case WOOF_MSG_GET:
	    		handle_get(sock_fd, uc);
	    		break;
	    	default:
	    		std::cerr << "non implemented tag: " << int(tag) << "!\n";
	    		break;
	    }
	}
}

extern "C"
{
	void* WooFMsgPackThread(void* arg)
	{
		uint16_t port = *((int*)arg) - 10000;

		auto sock = socket(AF_INET, SOCK_STREAM, 0);

		if (sock <= 0)
		{
			perror("socket error");
			return nullptr;
		}

		sockaddr_in address;

		address.sin_family = AF_INET;
    	address.sin_addr.s_addr = INADDR_ANY;
    	address.sin_port = htons(port);

		if (bind(sock, (sockaddr *)&address, sizeof address) < 0)
		{
			perror("bind error");
			return nullptr;
		}

		if (listen(sock, 3) < 0)
		{
			perror("listen error");
			return nullptr;
		}

		std::cerr << "listening on " << port << '\n';

		while(true)
		{
			sockaddr_in cli_addr;
			socklen_t addrlen = sizeof cli_addr;
			auto new_sock = accept(sock, (sockaddr*)&cli_addr, &addrlen);
			std::thread t{[=]{
				handle_sock(new_sock, cli_addr);
			}};
			t.detach();
		}
	}
}