#include <getopt.h>

#include <cstdlib>
#include <random>
#include <unordered_map>
#include <utility>
#include <tuple>
#include <queue>
#include <deque>
#include <thread>
#include <condition_variable>
#include <future>
#include <map>
#include <spdlog/spdlog.h>

#include "socket.hh"
#include "packet.hh"
#include "poller.hh"
#include "optional.hh"
#include "player.hh"
#include "display.hh"
#include "paranoid.hh"
#include "procinfo.hh"

using namespace std;
using namespace std::chrono;
using namespace PollerShortNames;

struct Connection
{
  TCPSocket socket;
  Connection(TCPSocket && sock) : socket(move(sock)){};
};

void init_server_socket(TCPSocket& socket, const std::string& congestion_control )
{
  socket.set_blocking(false);
  socket.set_reuseaddr();
  socket.set_congestion_control(congestion_control);
  cerr << "server listening" << endl;
  socket.listen(1);
}

int main( int argc, char *argv[] )
{
  /* check the command-line arguments */
  if ( argc < 1 ) { /* for sticklers */
    abort();
  }
  /* choose a random connection_id */
  const uint16_t connection_id = 1337; // ezrand();
  cerr << "Connection ID: " << connection_id << endl;

  /* construct Socket for incoming  datagrams */
  TCPSocket socket;
  cout << "port is " << argv[ optind ] << endl;

  socket.bind( Address( "0.0.0.0", 8888 ) );
  /* Listen */
  init_server_socket(socket, "cubic");

  Poller poller;

  std::map<uint64_t, Connection> connections_ {};

  static uint64_t id = 0;

  poller.add_action( Poller::Action( socket, Direction::In,
    [&]()
    {
      /* wait for next peer socket */
      auto client = socket.accept();
      cerr << "connceted" <<endl;
      client.set_blocking(false);
      auto client_addr = client.peer_address();
      cerr << "Peer is " << client_addr.ip() << ":" << client_addr.port() << endl;

      const uint64_t conn_id = id++;
      connections_.emplace(conn_id, move(client));
      Connection & conn = connections_.at(conn_id);
       /* we are interested in the client socket*/
      poller.add_action( Poller::Action( conn.socket, Direction::In,
        [&, conn_id]()
        {
          Connection & conn = connections_.at(conn_id);
          cerr << "read frames from sender" << endl;
          /* wait for next TCP message */
          const auto new_fragment = conn.socket.recv();
           /* parse into Packet */
          const Packet packet { new_fragment.payload };
          spdlog::info("Recved packets, frame num: {}, segment num: {}", packet.frame_no(), packet.fragment_no());

          cerr << "send videoACK to sender" << endl;
          // explictly inform the sender
          AckPacket( connection_id, packet.frame_no(), packet.fragment_no(),
                    10, uint32_t(0),
                    {uint32_t(1)} ).send( conn.socket );

          return ResultType::Continue;
        })
      );
      return ResultType::Continue;
    } ,[&]() { return not socket.eof(); }
    )
  );

  /* handle events */
  while ( true ) {
    const auto poll_result = poller.poll( -1 );
    if ( poll_result.result == Poller::Result::Type::Exit ) {
      return poll_result.exit_status;
    }
  }

  return EXIT_SUCCESS;
}
