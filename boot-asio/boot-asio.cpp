//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

static const unsigned short SERVER_PORT = 27015;
static const int MAX_PACKET_SIZE = 65536;

class Client : public std::enable_shared_from_this<Client> {
public:
    Client(tcp::socket socket) : socket_(std::move(socket)), lenCompleted(false), offset(0), packetLen(0) {
    }

    void start() {
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());

        char* ptr;
        int bytes;
        if (lenCompleted == false) {
            ptr = (char *) & packetLen;
            bytes = 4;
        }
        else {
            ptr = data_;
            bytes = packetLen;
        }

        socket_.async_read_some(boost::asio::buffer(ptr + offset, bytes - offset),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    offset += length;
                    if (lenCompleted == false) {
                        if (offset == 4) {
                            packetLen = ntohl(packetLen);
                            lenCompleted = true;
                            offset = 0;
                            std::cout << "Received length info: " << packetLen << std::endl;
                        }
                    }
                    else {
                        if (offset == packetLen) {
                            std::cout << "Received packet " << packetLen << " bytes" << std::endl;
                            lenCompleted = false;
                            offset = 0;
                        }
                    }
                    do_read();
                }
            });
    }

    tcp::socket socket_;
    enum { max_length = 65536 };
    char data_[max_length];
    bool lenCompleted;
    int packetLen;
    int offset;
};

void do_accept(tcp::acceptor& acceptor) {
    acceptor.async_accept(
        [&](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Client>(std::move(socket))->start();
            }
            else {
                std::cerr << "Error " << ec << std::endl;
            }

            do_accept(acceptor);
        });
}


int main(int argc, char* argv[]) {
    boost::asio::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), SERVER_PORT));
    do_accept(acceptor);
    io_context.run();

    return 0;
}