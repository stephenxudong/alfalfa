#include <fstream>
#include <algorithm>
#include <iostream>
#include <string>

#include "dct.hh"
#include "frame.hh"
#include "player.hh"
#include "vp8_raster.hh"
#include "decoder.hh"
#include "display.hh"
#include "macroblock.hh"

using namespace std;

KeyFrame make_empty_frame( unsigned int width, unsigned int height ) {
  BoolDecoder data { { nullptr, 0 } };
  KeyFrame frame { true, width, height, data };
  frame.parse_macroblock_headers( data, ProbabilityTables {} );
  return frame;
}

template <class FrameHeaderType2, class MacroblockHeaderType2>
void copy_macroblock( Macroblock<FrameHeaderType2, MacroblockHeaderType2> & target,
		      const Macroblock<FrameHeaderType2, MacroblockHeaderType2> & source )
{
  target.segment_id_update_ = source.segment_id_update_;
  target.segment_id_ = source.segment_id_;
  target.mb_skip_coeff_ = source.mb_skip_coeff_;
  target.header_ = source.header_;
  target.has_nonzero_ = source.has_nonzero_;

  /* copy contents */
  target.Y2_ = source.Y2_;
  target.Y_.copy_from( source.Y_ );
  target.U_.copy_from( source.U_ );
  target.V_.copy_from( source.V_ );
}

template <>
void copy_frame( KeyFrame & target, const KeyFrame & source,
		 const Optional<Segmentation> & segmentation __attribute((unused)) )
{
  target.show_ = source.show_;
  target.header_ = source.header_;

  assert( target.display_width_ == source.display_width_ );
  assert( target.display_height_ == source.display_height_ );
  assert( target.macroblock_width_ == source.macroblock_width_ );
  assert( target.macroblock_height_ == source.macroblock_height_ );

  /*
  {
    const Quantizer frame_quantizer( target.header_.quant_indices );
    const auto segment_quantizers = target.calculate_segment_quantizers( segmentation );

    target.Y_.forall_ij( [&] ( YBlock & target_block, const unsigned int col, const unsigned int row ) {
	target_block = source.Y_.at( col, row );
      } );
  }
  */

  assert( source.macroblock_headers_.initialized() );
  assert( target.macroblock_headers_.initialized() );

  assert( source.macroblock_headers_.get().width() == target.macroblock_headers_.get().width() );
  assert( source.macroblock_headers_.get().height() == target.macroblock_headers_.get().height() );

  /* copy each macroblock */
  target.macroblock_headers_.get().forall_ij( [&] ( KeyFrameMacroblock & target_macroblock,
						    const unsigned int col,
						    const unsigned int row ) {
						copy_macroblock( target_macroblock,
								 source.macroblock_headers_.get().at( col, row ) );
					      } );

  target.relink_y2_blocks();
}

vector<uint8_t> loopback( const Chunk & chunk, const uint16_t width, const uint16_t height )
{
    vector<uint8_t> serialized_new_frame;

    UncompressedChunk temp_uncompressed_chunk { chunk, width, height };
    if ( not temp_uncompressed_chunk.key_frame() ) {
      throw runtime_error( "not a keyframe" );
    }

    DecoderState temp_state { width, height };
    KeyFrame frame = temp_state.parse_and_apply<KeyFrame>( temp_uncompressed_chunk );
    vector<uint8_t> serialized_old_frame = frame.serialize( temp_state.probability_tables );

    KeyFrame temp_new_frame = make_empty_frame( width, height );
    copy_frame( temp_new_frame, frame,
		temp_state.segmentation );

    assert( temp_new_frame == frame );

    serialized_new_frame = temp_new_frame.serialize( temp_state.probability_tables );

    if ( serialized_new_frame != serialized_old_frame ) {
      throw runtime_error( "roundtrip failure" );
    }

    return serialized_new_frame;
}

int main( int argc, char *argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort();
    }

    if ( argc != 2 ) {
      cerr << "Usage: " << argv[ 0 ] << " <video>" << endl;
      return EXIT_FAILURE;
    }

    const IVF ivf( argv[ 1 ] );

    FramePlayer player { ivf.width(), ivf.height() };
    VideoDisplay display { player.example_raster() };

    for ( unsigned int frame_number = 0; frame_number < ivf.frame_count(); frame_number++ ) {
      const vector<uint8_t> serialized_new_frame = loopback( ivf.frame( frame_number ),
							     ivf.width(), ivf.height() );
      const auto maybe_raster = player.decode( serialized_new_frame );
      if ( maybe_raster.initialized() ) {
	display.draw( maybe_raster.get() );
      }
    }
  } catch ( const exception & e ) {
    print_exception( argv[ 0 ], e );
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
