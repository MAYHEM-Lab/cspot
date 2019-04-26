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

#include <cwpack.h>
#include <caps/emsha_signer.hpp>
#include <caps/request.hpp>
#include <cppspot/capabilities.hpp>
#include <nonstd/variant.hpp>
#include <tos/print.hpp>

#include <common/inet/tcp_ip.hpp>
#include "uriparser2/uriparser2.h"

namespace cspot
{
	enum class msg_tag : uint8_t
    {
        put = 1,
        get_el_sz = 2,
        get = 3,
        get_tail = 4,
        get_latest_seqno = 5,
        get_done = 6,
        repair = 7,
        get_log = 8,
        get_log_size = 9
    };
}

void tos_force_get_failed(void*)
{
	std::cerr << "force get failed\n";
	std::abort();
}

extern "C"
{
#include "woofc-access.h"
#include "woofc.h"
void WooFFree(WOOF *wf);
}

using namespace tos;

struct sock_stream
{
	sock_stream(int fd) : m_fd(fd) {}
	~sock_stream()
	{
		close(m_fd);
	}

	span<char> read(span<char> buf)
	{
		auto ret = ::read(m_fd, buf.data(), buf.size());
		return buf.slice(0, ret);
	}

	size_t write(span<const char> buf)
	{
		return ::write(m_fd, buf.data(), buf.size());
	}

	sock_stream* operator->() { return this; }
	sock_stream& operator*() { return *this; }

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


namespace cspot
{
    struct put_req
    {
        char woof[16];
        std::vector<uint8_t> data;
        char handler[12];
        uint32_t host;
        event_seq_t host_seq;
    };

    using reqs = mpark::variant<mpark::monostate, put_req>;

    cspot::cap_t req_to_tok(const reqs& r)
    {
        struct
        {
            cap_t operator()(const mpark::monostate&) { return cap_t{}; }
            cap_t operator()(const put_req&) { return cap_t{woof_id_t{1}, perms::put}; }
        } vis;

        return mpark::visit(vis, r);
    }

    enum class parse_error
    {
        unexpected
    };

    tos::expected<put_req, parse_error> parse_put_req(cw_unpack_context& uc)
    {
        put_req res;
        {
            cw_unpack_next(&uc);
            if (uc.item.type != cwpack_item_types::CWP_ITEM_STR)
            {
                return tos::unexpected(parse_error::unexpected);
            }

            auto tmp = parse_container<std::string>(uc.item.as.str);
            std::strncpy(res.woof, tmp.data(), std::size(res.woof));
        }

        {
            cw_unpack_next(&uc);
            if (uc.item.type != cwpack_item_types::CWP_ITEM_STR)
            {
                return tos::unexpected(parse_error::unexpected);
            }

            auto tmp = parse_container<std::string>(uc.item.as.str);
            std::strncpy(res.handler, tmp.data(), std::size(res.handler));
        }

        cw_unpack_next(&uc);
        if (uc.item.type != cwpack_item_types::CWP_ITEM_POSITIVE_INTEGER)
        {
            return tos::unexpected(parse_error::unexpected);
        }

        res.host = uc.item.as.u64;

        cw_unpack_next(&uc);
        if (uc.item.type != cwpack_item_types::CWP_ITEM_POSITIVE_INTEGER)
        {
            return tos::unexpected(parse_error::unexpected);
        }

        res.host_seq = {(uint16_t)uc.item.as.u64};

        cw_unpack_next(&uc);
        if (uc.item.type != cwpack_item_types::CWP_ITEM_BIN)
        {
            return tos::unexpected(parse_error::unexpected);
        }

        res.data = parse_container<std::vector<uint8_t>>(uc.item.as.bin);

        return res;
    }

    reqs parse_req(tos::span<const uint8_t> buf)
    {
        cw_unpack_context uc;
        cw_unpack_context_init(&uc, (void*)buf.data(), buf.size(), nullptr);

        cw_unpack_next(&uc);
        if (uc.item.type != cwpack_item_types::CWP_ITEM_ARRAY) {
            return mpark::monostate{};
        }

        cw_unpack_next(&uc);
        if (uc.item.type != cwpack_item_types::CWP_ITEM_POSITIVE_INTEGER) {
            return mpark::monostate{};
        }

        uint8_t tag = uc.item.as.u64;

        printf("got req %d\n", int(tag));

        switch (cspot::msg_tag(tag))
        {
            case cspot::msg_tag::put:
                return force_get(parse_put_req(uc));
                break;
            case cspot::msg_tag::get_el_sz:
                //handle_el_sz(ep, uc);
                break;
            case cspot::msg_tag::get:
                //handle_get(ep, uc);
                break;
            case cspot::msg_tag::get_log:
                //handle_get_log(ep, uc);
                break;
            case cspot::msg_tag::get_log_size:
                //handle_get_log_size(ep, uc);
                break;
            default:
                break;
        }
        return mpark::monostate{};
    }
}

auto client_key = []{
    caps::emsha::signer signer("sekkret");

    return caps::mkcaps({
        cspot::mkcap(cspot::root_cap, cspot::perms::root) }, signer);
}();

caps::emsha::signer signer("sekret");
auto proc = caps::req_deserializer<cspot::cap_t>(signer, cspot::parse_req, cspot::satisfies);

template <class Ep>
struct req_handlers
{
    void operator()(const mpark::monostate&) {
        ep.write("unknown error");
    }
    void operator()(const cspot::put_req& put)
    {
        auto err = WooFPut(put.woof, put.handler, (void*)put.data.data());
        tos::println(ep, "putting in", put.woof, put.handler, int(put.data.size()), int(err));

        char b[200];
        tos::msgpack::packer p {b};
        p.insert(uint32_t(err));
        auto res = p.get();

        auto h = signer.hash(res);

        auto t = clone(*client_key);
        t->signature = get_req_sign(*t, signer, 1000, h);

        ep.write({res.data(), res.size()});
        caps::serialize(ep, *t);
    }

    Ep& ep;
};

void handle_sock(int sock_fd, sockaddr_in addr)
{
	sock_stream sock{sock_fd};

    char buffer[200];
    auto res = sock.read(buffer);
    auto x = proc(res);

    if (!x)
    {
    	std::cerr << "auth fail " << int(force_error(x)) << '\n';
    	return;
    }

    auto& req = force_get(x);

	req_handlers<sock_stream> handle{sock};

	mpark::visit(handle, req);
}

using cap_ptr = caps::token_ptr<cspot::cap_t, caps::emsha::signer>;
cap_ptr cap;
uint32_t seq = 0;

struct woof_addr_t
{
    tos::ipv4_addr_t addr;
    tos::port_num_t port;
    char woof_name[16];
};
enum class addr_parse_errors
{
    arg_too_short,
    not_a_woof_uri,
    empty_host
};	

tos::port_num_t WooFPortHash2(const char *ns)
{
    uint64_t h = 5381;
    uint64_t a = 33;
    for(size_t i = 0; i < strlen(ns); i++) {
        h = ((h*a) + ns[i]);
    }
    return {uint16_t(50000 + (h % 10000))};
}

bool WooFNameFromURI(const URI& u, tos::span<char> wf, tos::span<char> ns)
{
    auto uri = &u;
    int i = strlen(uri->path);
    int j = 0;
    /*
     * if last character in the path is a '/' this is an error
     */
    if(uri->path[i] == '/') {
        return false;
    }
    while(i >= 0) {
        if(uri->path[i] == '/') {
            i++;
            if(i <= 0) {
                return false;
            }
            if(j > wf.size()) { /* not enough space to hold path */
                return false;
            }
            /*
             * pull off the end as the name of the woof
             */
            strncpy(wf.data(), &(uri->path[i]), wf.size());
            std::copy(uri->path, uri->path + i - 1, ns.begin());
            ns[i - 1] = 0;
            //strncpy(ns.data(), uri->path, tos::std::min<size_t>(ns.size(), i - 1));
            return true;
        }
        i--;
        j++;
    }
    return false;
}

tos::expected<woof_addr_t, addr_parse_errors> parse_addr(tos::span<const char> addr) {
    URI uri(addr.data());
    woof_addr_t res;
    char ns_name[32];
    WooFNameFromURI(uri, res.woof_name, ns_name);
    res.addr = tos::parse_ip({ uri.host, strlen(uri.host) });
    res.port = WooFPortHash2(ns_name);
    return res;
}

extern "C" unsigned long WooFMsgPut(const char *woof_name, const char *hand_name, void *element, unsigned long el_size) 
{
    std::cerr << "got put\n";
	std::vector<char> body(1024);
	msgpack::packer bodyp{body};

	auto putreq = bodyp.insert_arr(5);
	putreq.insert(uint8_t(cspot::msg_tag::put));
	putreq.insert(woof_name);
	putreq.insert(hand_name);
	putreq.insert(uint8_t(1)); // unused
	putreq.insert(uint8_t(1)); // unused
	putreq.insert(span<const uint8_t>((const uint8_t*)element, el_size));

    std::cerr << "req ok\n";

	auto req = bodyp.get();

	auto reqseq = ++seq;
	auto req_hash = signer.hash(req);

	auto c = clone(*cap);
	auto req_sign = caps::get_req_sign(*c, signer, reqseq, req_hash);
	c->signature = req_sign;
	std::vector<uint8_t> cap_buf(512);
	tos::omemory_stream cap_str{cap_buf};
	caps::serialize(cap_str, *c);
    std::cerr << "cap ok\n";

	std::vector<char> buf(1024);
	msgpack::packer p{buf};
	auto arr = p.insert_arr(3);
	arr.insert(cap_str.get().as_bytes());
	arr.insert(req.as_bytes());
	arr.insert(reqseq);

	auto res = p.get();

	auto addr = force_get(parse_addr({woof_name, strlen(woof_name)}));

    std::cerr << addr.port.port << " : " << addr.woof_name << '\n';
    std::cerr << "calling socket\n";
	auto sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock <= 0)
	{
		perror("socket error");
		return -1;
	}

	sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	std::memcpy(&serv_addr.sin_addr.s_addr, &addr.addr, 4);
	serv_addr.sin_port = htons(addr.port.port - 10'000); // msgpack port is 10.000 less than the regular port

    std::cerr << "calling connect\n";
    if (connect(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting");
        return -1;
    }

    sock_stream ss{sock};

    ss->write(res);

    std::cerr << "wrote\n";

    std::vector<char> buffer(512);
    auto d = ss->read(buffer);

    return 100;
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

        cap = caps::mkcaps({
            cspot::cap_t{ cspot::woof_id_t{1}, cspot::perms::get | cspot::perms::put }
            }, signer);

        struct
        {
            int write(tos::span<const char> buf)
            {
                for (auto& c : buf)
                {
                    fprintf(stderr, "%02x", (unsigned int)(c));
                }
                return buf.size();
            }

            auto operator->() { return this; }
        } x;

        std::cerr << '\n';
        caps::serialize(x, *cap);
        std::cerr << '\n';

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
