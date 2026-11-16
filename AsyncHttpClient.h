#include <cstdio>
#include <cstring>
#include <string>
#include "pico/stdlib.h"
#include "lwip/dns.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

class AsyncHttpClient 
{
    public:
        enum class State { IDLE, RESOLVING, CONNECTING, SENDING, RECEIVING, DONE, ERROR };

        State state = State::IDLE;
        std::string response;

        bool get(const char* host, const char* path, uint16_t port = 80);

    private:
        std::string host;
        std::string path;
        uint16_t port = 80;
        ip_addr_t remote_ip{};
        struct tcp_pcb* pcb = nullptr;
        static void on_dns_found(const char* name, const ip_addr_t* ipaddr, void* arg);
        void connect_tcp();
        static err_t on_connected(void* arg, struct tcp_pcb* tpcb, err_t err);
        static err_t on_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err);
        static void on_err(void* arg, err_t err);
        void close_pcb();
};