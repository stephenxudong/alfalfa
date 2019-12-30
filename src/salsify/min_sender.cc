#include <getopt.h>

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <limits>
#include <thread>
#include <future>
#include <algorithm>
#include <unordered_map>
#include <iomanip>
#include <cmath>
//logging
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "exception.hh"
#include "finally.hh"
#include "paranoid.hh"
#include "yuv4mpeg.hh"
#include "encoder.hh"
#include "socket.hh"
#include "packet.hh"
#include "poller.hh"
#include "socketpair.hh"
#include "camera.hh"
#include "pacer.hh"
#include "procinfo.hh"

using namespace std;
using namespace std::chrono;
using namespace PollerShortNames;

int main( int argc, char *argv[] )
{
  /* check the command-line arguments */
  if ( argc < 1 ) { /* for sticklers */
    abort();
  }
  /* buffer for the output of encoder */
  std::deque<std::string> queue_ {};

  /* construct Socket for outgoing datagrams */
  TCPSocket socket;
  socket.connect( Address( argv[ optind ], argv[ optind + 1 ] ) );
  spdlog::info("Socket connnected to {}:{}", argv[ optind ], argv[ optind + 1 ]);
  socket.set_timestamps();
  // send packets with ccp
  socket.set_congestion_control("bbr");
  spdlog::debug("Created sender socket with CCP");
  //socket.set_blocking(false);

  /* get connection_id */
  // const uint16_t connection_id = paranoid::stoul( argv[ optind + 2 ] );

  Poller poller;

  /* new ack from receiver */
  poller.add_action( Poller::Action( socket, Direction::In,
    [&]()
    {
      spdlog::info("recv packets");

      // we only need the payload
      auto packet = socket.recv();

      /* why would this callback be called ?*/
      if (packet.payload == ""){
        cerr << "Warning: " << "recv empty data" << endl;
        // socket.noeof();
        return ResultType::Cancel;
      }

      // AckPacket ack( packet.payload );

      // if ( ack.connection_id() != connection_id ) {
      //   /* this is not an ack for this session! */
      //   return ResultType::Continue;
      // }
      return ResultType::Continue;

    })
  );

  /* outgoing packet ready to leave the buffer */
  poller.add_action( Poller::Action( socket, Direction::Out, [&]() {
        /* pop packet from buffer */
        spdlog::info("Send frame now");
        while ( not queue_.empty() ) {
          spdlog::info("Current Packet size: {}", queue_.front().size());
          socket.send( queue_.front() );
          queue_.pop_front();
        }
        return ResultType::Continue;
        /* when we are interested in this action*/
      }, [&]() { return not queue_.empty(); } ) );

  /* handle events */
  int round = 0;
  size_t sizet = 1;
  while ( true ) {
    std::vector<uint8_t> payload(1442*10, uint8_t(11));
    string s = "HELLO";
    spdlog::info( "Push encoded frame to send buffer");
    for ( int i = 0; i < 10; i++ ) {
      Packet packet = {payload, uint16_t(1337), uint32_t(1), uint32_t(1), uint32_t(round), uint16_t(i), uint16_t(101010), sizet};
      packet.set_fragments_in_this_frame(10);
      /* we don't need pacer since we send the packet with TCP*/
      spdlog::info( "Push encoded frame to send buffer, times: {}, index: {}", round, i);
      queue_.push_back( packet.to_string() );
    }
    round += 1;
    if (round > 10) return 1;
    // when queue
    const auto poll_result = poller.poll(3000);
    if ( poll_result.result == Poller::Result::Type::Exit ) {
      if ( poll_result.exit_status ) {
        cerr << "Connection error." << endl;
      }

      return poll_result.exit_status;
    }
  }

  return EXIT_FAILURE;
}
