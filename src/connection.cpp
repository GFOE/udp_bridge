#include "udp_bridge/connection.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

namespace udp_bridge
{

Connection::Connection(std::string const &host, uint16_t port):m_host(host),m_port(port)
{
    struct addrinfo hints = {0}, *addresses;
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    
    std::string port_string = std::to_string(port);
    
    int ret = getaddrinfo(host.c_str(), port_string.c_str(), &hints, &addresses);
    if(ret != 0)
        throw std::runtime_error(gai_strerror(ret));
    
    int error {0};
    for (struct addrinfo *address = addresses; address != nullptr; address = address->ai_next)
    {
        m_socket = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if(m_socket == -1)
        {
            error = errno;
            continue;
        }
        
        if(connect(m_socket, address->ai_addr, address->ai_addrlen) == 0)
        {
            unsigned int s = sizeof(m_send_buffer_size);
            getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (void*)&m_send_buffer_size, &s);

            m_send_buffer_size = 500000;
            setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &m_send_buffer_size, sizeof(m_send_buffer_size));
            getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (void*)&m_send_buffer_size, &s);
            break;
        }
        
        error = errno;
        close(m_socket);
        m_socket = -1;
    }
    freeaddrinfo(addresses);
    if(m_socket == -1)
        throw std::runtime_error(strerror(error));
}
    
    
Connection::~Connection()
{
    close(m_socket);
}

void Connection::send(std::vector<uint8_t> const &data)
{
  int tries = 0;
  while (true)
  {
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(m_socket, &writefds);
    timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    int ret = select(m_socket+1, nullptr, &writefds, nullptr, &timeout);
    if(ret > 0)
    {
      int e = ::send(m_socket, data.data(), data.size(), 0);
      if(e == -1)
        throw(ConnectionException(strerror(errno)));
    }
    if(ret == 0)
      throw(ConnectionException("Timeout"));
    else if( errno == EAGAIN && tries < 2000)
    {
      tries += 1;
      usleep(500);
    }
    else
      throw(ConnectionException(std::to_string(errno) +": "+ strerror(errno)));
  }
}

std::string Connection::str() const
{
    std::stringstream ret;
    ret << m_host << ":" << m_port;
    return ret.str();
}

std::string Connection::label(bool allowEmpty) const
{
    if(!allowEmpty && m_label.empty())
      return str();
    return m_label;
}

void Connection::setLabel(const std::string &label)
{
    m_label = label;
}

int Connection::sendBufferSize() const
{
    return m_send_buffer_size;
}

std::shared_ptr<Connection> ConnectionManager::getConnection(std::string const &host, uint16_t port)
{
    for(auto c: m_connections)
        if(c->m_host == host && c->m_port == port)
            return c;
    m_connections.push_back(std::shared_ptr<Connection>(new Connection(host,port)));
    return m_connections.back();
}

std::shared_ptr<Connection> ConnectionManager::getConnection(std::string const &label)
{
    for(auto c: m_connections)
      if(c->label() == label || c->str() == label)
        return c;
    return std::shared_ptr<Connection>();
}

const std::vector<std::shared_ptr<Connection> > & ConnectionManager::connections() const
{
    return m_connections;
}

} // namespace udp_bridge
