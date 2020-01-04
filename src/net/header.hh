#ifndef HEADER_HH
#define HEADER_HH

#include <cstdio>
// #include <stdint.h>

/* total length of a data packet header is 24 bytes */
struct Header{
  uint16_t connection_id_;
  uint32_t source_state_;
  uint32_t target_state_;
  uint32_t frame_no_;
  uint16_t fragment_no_;
  uint16_t fragments_in_this_frame_;
  uint32_t time_since_last_; /* microseconds */

  // we need to specify how many bytes are in the payload.
  uint16_t payload_length_;

  /* construct outgoing Packet */
  Header( const uint16_t connection_id,
          const uint32_t source_state,
          const uint32_t target_state,
          const uint32_t frame_no,
          const uint16_t fragment_no,
          const uint16_t time_since_last)
    : connection_id_( connection_id ),
    source_state_( source_state ),
    target_state_( target_state ),
    frame_no_( frame_no ),
    fragment_no_( fragment_no ),
    fragments_in_this_frame_( 0 ), /* temp value */
    time_since_last_( time_since_last ),
    payload_length_( 0 ) /* temp value*/
    {}
  
  /* construct incoming Packet */
  Header( const Chunk & str )
   : connection_id_( str( 0, 2 ).le16() ),
    source_state_( str( 2, 4 ).le32() ),
    target_state_( str( 6, 4 ).le32() ),
    frame_no_( str( 10, 4 ).le32() ),
    fragment_no_( str( 14, 2 ).le16() ),
    fragments_in_this_frame_( str( 16, 2 ).le16() ),
    time_since_last_( str( 18, 4 ).le32() ),
    payload_length_( str( 22,2 ).le16() )
    {}

  /* allow copy constrctor */
  Header(const Header & other)
    : connection_id_( other.connection_id_ ),
      source_state_( other.source_state_ ),
      target_state_( other.target_state_ ),
      frame_no_( other.frame_no_ ),
      fragment_no_( other.fragment_no_ ),
      fragments_in_this_frame_( other.fragments_in_this_frame_ ),
      time_since_last_( other.time_since_last_ ),
      payload_length_( other.payload_length_ )
    {}
  
  /* construct a invalid packet header */
  Header()
    : connection_id_(),
      source_state_(),
      target_state_(),
      frame_no_(),
      fragment_no_(),
      fragments_in_this_frame_(),
      time_since_last_(),
      payload_length_()
  {}

  void set_payload_length( uint16_t payload_length ){payload_length_ = payload_length;}
  // void set_fragments_in_this_frame( const uint16_t x ) {fragments_in_this_frame_ = x}
};

#endif