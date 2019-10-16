//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <functional>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/filesystem.hpp>

using boost::asio::ip::tcp;

static const boost::filesystem::path g_keyPath =
    R"(E:\dev_dir\VS_Project\Gym\ThirdParty\boost_1_70_0\libs\asio\example\cpp11\ssl)";

class session : public std::enable_shared_from_this<session>
{
public:
    session(tcp::socket socket, boost::asio::ssl::context &context)
        : socket_(std::move(socket), context)
    {}

    void start() { do_handshake(); }

private:
    void do_handshake()
    {
        auto self(shared_from_this());
        socket_.async_handshake(boost::asio::ssl::stream_base::server,
                                [this, self](const boost::system::error_code &error) {
                                    if (!error) {
                                        do_read();
                                    }
                                });
    }

    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_),
                                [this, self](const boost::system::error_code &ec,
                                             std::size_t length) {
                                    if (!ec) {
                                        do_write(length);
                                    }
                                });
    }

    void do_write(std::size_t length)
    {
        auto self(shared_from_this());
        static const char TestSSL[] = "#!-agent ack ";
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(TestSSL, sizeof(TestSSL) - 1),
                                 [this, self](const boost::system::error_code &ec,
                                              std::size_t /*length*/) {
                                     if (!ec) {
                                         do_read();
                                     }
                                 });
    }

    boost::asio::ssl::stream<tcp::socket> socket_;
    char data_[1024];
};

class server
{
public:
    server(boost::asio::io_context &io_context, unsigned short port)
        : acceptor_(io_context, tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), port))
        , context_(boost::asio::ssl::context::method::tlsv13)
    {
        //context_.set_options(boost::asio::ssl::context::default_workarounds);
        context_.set_verify_mode(boost::asio::ssl::context_base::verify_peer |
                                 boost::asio::ssl::context_base::verify_fail_if_no_peer_cert);
        context_.set_password_callback(
            [](std::size_t max_length,
               boost::asio::ssl::context::password_purpose purpose) -> std::string {
                return "dingant";
            });

        context_.load_verify_file(R"(E:\dev_dir\linux_project\dingant\etc\ssl_key\cacert.pem)");
        context_.use_certificate_chain_file(
            R"(E:\dev_dir\linux_project\dingant\etc\ssl_key\usercert.pem)");
        context_.use_private_key_file(R"(E:\dev_dir\linux_project\dingant\etc\ssl_key\userkey.pem)",
                                      boost::asio::ssl::context::pem);

        do_accept();
    }

private:
    std::string get_password() const { return "test"; }

    void do_accept()
    {
        acceptor_.async_accept([this](const boost::system::error_code &error, tcp::socket socket) {
            if (!error) {
                std::make_shared<session>(std::move(socket), context_)->start();
            }

            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    boost::asio::ssl::context context_;
};

int main(int argc, char *argv[])
{
    boost::ignore_unused(argc, argv);
    try {
        boost::asio::io_context io_context;

        server s(io_context, 1514);

        io_context.run();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
