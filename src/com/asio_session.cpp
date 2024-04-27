#include "asio_session.hpp"
#include <stdexcept>

namespace com
{

asio_session::asio_session(boost::asio::local::stream_protocol::socket sock)
: socket_(std::move(sock))
{

}

asio_session::~asio_session()
{
    close();
}

void asio_session::connect_to(std::string const & endpoint, accept_handler handler)
{
    boost::asio::local::stream_protocol::endpoint ep(endpoint);
    auto self(shared_from_this());

    socket_.async_connect(ep,[self, handler](auto err) mutable {
        if (err)
        {
            self.reset();
        }

        handler(self);
    });
}


void asio_session::send(std::string const & message)
{
    if (write_queue.empty()) {
        // ToDo start writing
    }
    else {
        write_queue.emplace(message);
    }
}

void asio_session::set_on_close(close_handler handler)
{
    if (on_close)
    {
        throw std::runtime_error("handler already set");
    }

    on_close = std::move(handler);

    if(!socket_.is_open())
    {
        on_close();
    }
}

void asio_session::set_on_message(message_handler handler)
{
    if (on_message)
    {
        throw std::runtime_error("handler already set");        
    }

    on_message = std::move(handler);

    read_header();
    // ToDo begin read
}

void asio_session::close()
{
    if (socket_.is_open())
    {
        socket_.close();
        if (on_close) {
            on_close();
        }
    }
}

void asio_session::read_header()
{
    auto self(shared_from_this());
    boost::asio::async_read(socket_, boost::asio::buffer(message_to_read.header, 4),
        [this, self](auto err, auto) {
            if (err)
            {
                close();
                return;
            }

            size_t length;
            if (!message_to_read.parse_header(length))
            {
                close();
                return;
            }
            
            read_payload(length);
        });

}

void asio_session::read_payload(size_t length)
{
    message_to_read.payload.resize(length);

    auto self(shared_from_this());
    boost::asio::async_read(socket_, boost::asio::buffer(message_to_read.payload, length),
        [this, self](auto err, auto) {
            if (err)
            {
                close();
                return;
            }

            on_message(message_to_read.payload);
            read_header();
    });
}

}