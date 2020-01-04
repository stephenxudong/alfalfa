/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Copyright 2013-2018 the Alfalfa authors
                       and the Massachusetts Institute of Technology

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

      1. Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.

      2. Redistributions in binary form must reproduce the above copyright
         notice, this list of conditions and the following disclaimer in the
         documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#ifndef SOCKET_HH
#define SOCKET_HH

#include <functional>

#include "address.hh"
#include "file_descriptor.hh"
#include "header.hh"

/* class for network sockets (UDP, TCP, etc.) */
class Socket : public FileDescriptor
{
private:
  /* get the local or peer address the socket is connected to */
  Address get_address( const std::string & name_of_function,
		       const std::function<int(int, sockaddr *, socklen_t *)> & function ) const;

protected:
  /* default constructor */
  Socket( const int domain, const int type );

  /* construct from file descriptor */
  Socket( FileDescriptor && s_fd, const int domain, const int type );

  /* get and set socket option */
  template <typename option_type>
  socklen_t getsockopt( const int level, const int option, option_type & option_value ) const;

  template <typename option_type>
  void setsockopt( const int level, const int option, const option_type & option_value );

public:
  /* bind socket to a specified local address (usually to listen/accept) */
  void bind( const Address & address );

  /* connect socket to a specified peer address */
  void connect( const Address & address );

  /* accessors */
  Address local_address( void ) const;
  Address peer_address( void ) const;

  /* allow local address to be reused sooner, at the cost of some robustness */
  void set_reuseaddr( void );
};

/* UDP socket */
class UDPSocket : public Socket
{
public:
  UDPSocket() : Socket( AF_INET, SOCK_DGRAM ) {}

  struct received_datagram {
    Address source_address;
    uint64_t timestamp_us;
    std::string payload;
  };

  /* receive datagram, timestamp, and where it came from */
  received_datagram recv( void );

  /* send datagram to specified address */
  void sendto( const Address & peer, const std::string & payload );

  /* send datagram to connected address */
  void send( const std::string & payload );

  /* turn on timestamps on receipt */
  void set_timestamps( void );
};

/* TCP socket */
class TCPSocket : public Socket
{
protected:
  /* constructor used by accept() and SecureSocket() */
  TCPSocket( FileDescriptor && fd ) : Socket( std::move( fd ), AF_INET, SOCK_STREAM ) {}
public:
  TCPSocket() : Socket( AF_INET, SOCK_STREAM ) {}

  /* parse packet */
  struct received_datagram {
    /* in tcp, we don't care about the peer's address*/
    // Address source_address;
    uint64_t timestamp_us;
    Header header;
    std::string payload;
  };

  struct received_ackgram {
    /* in tcp, we don't care about the peer's address*/
    // Address source_address;
    uint64_t timestamp_us;
    std::string payload;
  };

   /* send datagram to connected address */
  void send( const std::string & payload );

  /* receive datagram, timestamp, and where it came from */
  received_ackgram recv_ack( void );

  received_datagram recv_data( void );

  /* mark the socket as listening for incoming connections*/
  void listen( const int backlog = 16);

  /* accept a new incoming connection */
  TCPSocket accept( void );

  Address original_dest( void ) const;

  /* set the current congestion control algorithm */
  void set_congestion_control( const std::string & cc);

  std::string get_congestion_control() const;

  /* turn on timestamps on receipt */
  void set_timestamps( void );

  // TCPInfo get_tcp_info() const;
  uint64_t timestamp_us();


};
#endif /* SOCKET_HH */
