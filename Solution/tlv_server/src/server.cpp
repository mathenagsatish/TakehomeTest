#include "server.hpp"
#include "logger.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <iomanip>
#include <optional>
#include <sstream>
#include <vector>

// ---------------- Constants ----------------
static constexpr size_t HEADER_SIZE = 6;
static constexpr size_t MAX_VALUE_SIZE = 4 * 1024 * 1024; // 4MB
static constexpr int MAX_CLIENTS = 1000;
static constexpr int IDLE_TIMEOUT_SEC = 60;

// ---------------- TLV Types ----------------
enum class TlvType : uint16_t
{
    Hello = 0xE110,
    Data = 0xDA7A,
    Goodbye = 0x0B1E
};

static std::optional<std::string> typeToString(uint16_t t)
{
    switch (static_cast<TlvType>(t))
    {
    case TlvType::Hello:
        return "Hello";
    case TlvType::Data:
        return "Data";
    case TlvType::Goodbye:
        return "Goodbye";
    default:
        return std::nullopt;
    }
}

// ---------------- Implementation ----------------

TLVServer::TLVServer(uint16_t port) : port_(port) {}
TLVServer::~TLVServer() { stop(); }

void TLVServer::close_fd(int &fd)
{
    if (fd >= 0)
    {
        ::close(fd);
        fd = -1;
    }
}

std::string TLVServer::address_string(const struct sockaddr_in &client_addr)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    uint16_t client_port = ntohs(client_addr.sin_port);
    std::ostringstream oss;
    oss << "[" << client_ip << ":" << client_port << "]";
    return oss.str();
}

void TLVServer::handle_client(int client_fd, const struct sockaddr_in &client_addr)
{
    ILogger &log = Logger::instance();
    std::string peer = address_string(client_addr);

    {
        std::lock_guard<std::mutex> lk(sockets_mtx);
        client_fds.insert(client_fd);
    }

    // Cleanup lambda to close fd and remove from set
    auto cleanup = [&]()
    {
        std::lock_guard<std::mutex> lk(sockets_mtx);
        client_fds.erase(client_fd);
        close_fd(client_fd);
    };

    // Set receive timeout
    timeval tv{IDLE_TIMEOUT_SEC, 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Main loop of client handler
    while (running && client_fd >= 0)
    {
        uint8_t header[HEADER_SIZE];
        ssize_t n = recv(client_fd, header, HEADER_SIZE, MSG_WAITALL);

        if (n == 0)
        {
            cleanup();
            return;
        }

        // handle errors
        if (n < 0)
        {
            if (errno == EINTR && running)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                log.log(peer + " [Timeout] Closing after idle " + std::to_string(IDLE_TIMEOUT_SEC) + "s");
            }
            else
            {
                log.log(peer + " [Error] recv(header): " + std::string(strerror(errno)));
            }
            cleanup();
            return;
        }

        // parse header
        uint16_t type;
        uint32_t len;
        std::memcpy(&type, header, sizeof(type));
        std::memcpy(&len, header + 2, sizeof(len));
        type = ntohs(type);
        len = ntohl(len);

        if (len > MAX_VALUE_SIZE)
        {
            log.log(peer + " [Error] LENGTH too large");
            cleanup();
            return;
        }

        // receive value (if any)
        std::vector<uint8_t> value(len);
        if (len > 0)
        {
            n = recv(client_fd, value.data(), len, MSG_WAITALL);
            if (n <= 0)
            {
                log.log(peer + " [Error] recv(value): " + (n == 0 ? "peer closed" : std::string(strerror(errno))));
                cleanup();
                return;
            }
        }

        // log message
        std::ostringstream oss;
        oss << peer << " [" << (typeToString(type).value_or("Invalid"))
            << "] [" << len << "]";
        if (len > 0)
        {
            oss << " [";
            size_t nbytes = std::min<size_t>(4, len);
            for (size_t i = 0; i < nbytes; i++)
            {
                if (i)
                    oss << " ";
                oss << "0x" << std::hex << std::uppercase << std::setfill('0') 
					<< std::setw(2) << (int)value[i] << std::dec;
            }
            oss << "]";
        }
        else
        {
            oss << " []";
        }

        log.log(oss.str());

        // Goodbye → close connection
        if (type == static_cast<uint16_t>(TlvType::Goodbye))
        {
            cleanup();
            return;
        }
    }

    cleanup();
}

bool TLVServer::start()
{
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        if (errno == EMFILE)
        {
            std::cerr << "FD limit reached (Too many open files). Skipping client.\n";
        }
        return false;
    }
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    if (bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) != 0)
        return false;
    if (listen(listen_fd, SOMAXCONN) != 0)
        return false;
    return true;
}

void TLVServer::run()
{
    try
    {
        ILogger &log = Logger::instance();
        log.log("Server listening on port " + std::to_string(port_));

        while (running)
        {
            sockaddr_in client_addr{};
            socklen_t len = sizeof(client_addr);
            int cfd = ::accept(listen_fd, (sockaddr *)&client_addr, &len);
            if (cfd < 0)
            {
                if (errno == EMFILE || errno == ENFILE)
                {
                    log.log("Too many open files, rejecting connection");
                }
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    continue;
                }
                else if (errno == EINTR)
                {
                    if (running)
                        continue;
                }
                else if (errno == EBADF || errno == EINVAL)
                {
                    // happens during shutdown
                    break;
                }
                else
                {
                    log.log("accept error: " + std::string(strerror(errno)));
                }
                continue;
            }
            if (active_clients >= MAX_CLIENTS)
            {
                log.log("Reject new client, too many connections");
                ::close(cfd);
                continue;
            }
            active_clients++;

            // Spawn thread to handle client
            std::thread([this, cfd, client_addr]()
                        {
                handle_client(cfd,client_addr);
                active_clients--; })
                .detach();
        }
    }
    catch (std::exception &e)
    {
        // Happens when server is stopped while blocking on accept().
        std::cerr << "Exception in server run loop:" << e.what() << std::endl;
    }
}

void TLVServer::stop()
{
    std::string str = "Closing server, Active clients: " + std::to_string(active_clients.load());
    std::cout << str << std::endl;
    running = false;
    close_fd(listen_fd);
    std::lock_guard<std::mutex> lk(sockets_mtx);
    for (int fd : client_fds)
        ::close(fd);
    client_fds.clear();
}
