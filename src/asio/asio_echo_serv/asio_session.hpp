
#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "common.h"

using namespace boost::system;

#define MAX_PACKET_SIZE	        65536
#define QUERY_COUNTER_INTERVAL  100

using namespace boost::asio;

namespace asio_test {

class asio_session : public std::enable_shared_from_this<asio_session>,
                     private boost::noncopyable {
private:
    enum { PACKET_SIZE = MAX_PACKET_SIZE };

    ip::tcp::socket socket_;
    std::uint32_t packet_size_;
    std::uint64_t query_count_;

    char data_[PACKET_SIZE];

public:
    asio_session(boost::asio::io_service & io_service, std::uint32_t packet_size)
        : socket_(io_service), packet_size_(packet_size), query_count_(0)
    {
        ::memset(data_, 0, sizeof(data_));
    }

    ~asio_session()
    {
        socket_.close();
    }

    void start()
    {
        g_client_count++;
        get_socket_recv_bufsize(socket_);
        set_socket_recv_bufsize(socket_, 65536);
        do_read_some();
    }

    void stop(bool delete_self = false)
    {
        g_client_count--;
        if (delete_self)
            delete this;
    }

    ip::tcp::socket & socket()
    {
        return socket_;
    }

    static boost::shared_ptr<asio_session> create_new(
        boost::asio::io_service & io_service, std::uint32_t packet_size) {
        return boost::shared_ptr<asio_session>(new asio_session(io_service, packet_size));
    }

private:
    int get_socket_recv_bufsize(const ip::tcp::socket & socket) const
    {
        boost::asio::socket_base::receive_buffer_size recv_bufsize_option;
        socket.get_option(recv_bufsize_option);

        std::cout << "receive_buffer_size: " << recv_bufsize_option.value() << " bytes" << std::endl;
        return recv_bufsize_option.value();
    }

    int set_socket_recv_bufsize(ip::tcp::socket & socket, int buffer_size) const
    {
        boost::asio::socket_base::receive_buffer_size recv_bufsize_option(buffer_size);
        socket.set_option(recv_bufsize_option);

        std::cout << "set_socket_recv_buffer_size(): " << buffer_size << " bytes" << std::endl;
        return get_socket_recv_bufsize(socket);
    }

    void do_read()
    {
        //auto self(this->shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(data_, packet_size_),
            [this](boost::system::error_code ec, std::size_t bytes_received)
            {
                if ((uint32_t)bytes_received != packet_size_) {
                    std::cout << "asio_session::do_read(): async_read(), bytes_received = "
                              << bytes_received << " bytes." << std::endl;
                }
                if (!ec) {
                    // A successful request, can be used to statistic qps
                    do_write();
                }
                else {
                    // Write error log
                    std::cout << "asio_session::do_read() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_write()
    {
        //auto self(this->shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_, packet_size_),
            [this](boost::system::error_code ec, std::size_t bytes_written)
            {
                if (!ec) {
                    if ((uint32_t)bytes_written != packet_size_) {
                        std::cout << "asio_session::do_write(): async_write(), bytes_written = "
                                  << bytes_written << " bytes." << std::endl;
                    }

                    do_read();
                    // If get a circle of ping-pong, we count the query one time.
#if 0
                    g_query_count++;
#else
                    query_count_++;
                    if (query_count_ >= QUERY_COUNTER_INTERVAL) {
                        g_query_count.fetch_add(QUERY_COUNTER_INTERVAL);
                        query_count_ = 0;
                    }
#endif
                }
                else {
                    // Write error log
                    std::cout << "asio_session::do_write() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_read_some()
    {
        socket_.async_read_some(boost::asio::buffer(data_, packet_size_),
            [this](boost::system::error_code ec, std::size_t bytes_received)
            {
                if ((uint32_t)bytes_received != packet_size_) {
                    std::cout << "asio_session::do_read_some(): async_read(), bytes_received = "
                              << bytes_received << " bytes." << std::endl;
                }
                if (!ec) {
                    // A successful request, can be used to statistic qps
                    do_write_some();
                }
                else {
                    // Write error log
                    std::cout << "asio_session::do_read() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_write_some()
    {
        //auto self(this->shared_from_this());
        socket_.async_write_some(boost::asio::buffer(data_, packet_size_),
            [this](boost::system::error_code ec, std::size_t bytes_written)
            {
                if (!ec) {
                    if ((uint32_t)bytes_written != packet_size_) {
                        std::cout << "asio_session::do_write_some(): async_write(), bytes_written = "
                                  << bytes_written << " bytes." << std::endl;
                    }

                    do_read_some();
                    // If get a circle of ping-pong, we count the query one time.
#if 0
                    g_query_count++;
#else
                    query_count_++;
                    if (query_count_ >= QUERY_COUNTER_INTERVAL) {
                        g_query_count.fetch_add(QUERY_COUNTER_INTERVAL);
                        query_count_ = 0;
                    }
#endif
                }
                else {
                    // Write error log
                    std::cout << "asio_session::do_write() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }
};

} // namespace asio_test
