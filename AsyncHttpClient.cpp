#include "AsyncHttpClient.h"

//Constructor
bool AsyncHttpClient::get(const char* host, const char* path, uint16_t port) 
{
    if (state != State::IDLE && state != State::DONE && state != State::ERROR) 
    {
        return false; // Request already in progress
    }

    this->host = host;
    this->path = path;
    this->port = port;
    this->response.clear();
    this->state = State::RESOLVING;

    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(host, &remote_ip, &AsyncHttpClient::on_dns_found, this);
    cyw43_arch_lwip_end();

    if (err == ERR_OK) 
    {
        // IP was already cached in DNS table
        connect_tcp();
    } 
    else if (err != ERR_INPROGRESS) 
    {
        state = State::ERROR;
        return false;
    }
        return true;
}

void AsyncHttpClient::on_dns_found(const char* name, const ip_addr_t* ipaddr, void* arg) 
{
    auto* self = static_cast<AsyncHttpClient*>(arg);
    if (ipaddr != nullptr) 
    {
        self->remote_ip = *ipaddr;
        self->connect_tcp();
    } 
    else 
    {
        self->state = State::ERROR;
    }
}

void AsyncHttpClient::connect_tcp() 
{
    state = State::CONNECTING;
    pcb = tcp_new_ip_type(IP_GET_TYPE(&remote_ip));
    if (!pcb) {
        state = State::ERROR;
        return;
    }

    tcp_arg(pcb, this);
    tcp_recv(pcb, &AsyncHttpClient::on_recv);
    tcp_err(pcb, &AsyncHttpClient::on_err);

    err_t err = tcp_connect(pcb, &remote_ip, port, &AsyncHttpClient::on_connected);
    if (err != ERR_OK) 
    {
        close_pcb();
        state = State::ERROR;
    }
}

err_t AsyncHttpClient::on_connected(void* arg, struct tcp_pcb* tpcb, err_t err) 
{
    auto* self = static_cast<AsyncHttpClient*>(arg);
    if (err != ERR_OK) 
    {
        self->state = State::ERROR;
        return err;
    }

    self->state = State::SENDING;
    std::string req = "GET " + self->path + " HTTP/1.1\r\n"
                        "Host: " + self->host + "\r\n"
                        "Connection: close\r\n\r\n";

    err_t write_err = tcp_write(tpcb, req.c_str(), req.size(), TCP_WRITE_FLAG_COPY);
    if (write_err == ERR_OK) 
    {
        tcp_output(tpcb);
        self->state = State::RECEIVING;
    } 
    else 
    {
        self->close_pcb();
        self->state = State::ERROR;
    }
    return ERR_OK;
}

err_t AsyncHttpClient::on_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) 
{
    auto* self = static_cast<AsyncHttpClient*>(arg);
    if (p == nullptr) 
    {
        // Remote host closed connection (end of response)
        self->close_pcb();
        self->state = State::DONE;
        return ERR_OK;
    }

    // Copy incoming data chunks
    for (struct pbuf* q = p; q != nullptr; q = q->next) 
    {
        self->response.append(static_cast<char*>(q->payload), q->len);
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p); // Crucial: always free pbuf to prevent memory leaks
    return ERR_OK;
}

void AsyncHttpClient::on_err(void* arg, err_t err) 
{
    auto* self = static_cast<AsyncHttpClient*>(arg);
    self->pcb = nullptr; // lwIP already freed the pcb on error
    self->state = State::ERROR;
}

void AsyncHttpClient::close_pcb() 
{
    if (pcb) 
    {
        tcp_arg(pcb, nullptr);
        tcp_recv(pcb, nullptr);
        tcp_err(pcb, nullptr);
        tcp_close(pcb);
        pcb = nullptr;
    }
}