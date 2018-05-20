#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <iostream>
#include <getopt.h> // getopt_long
#include <stdlib.h> // abs


using namespace cv;
using namespace std;

const uint8_t FOOT_SIDE_MASK = 0x1;
const uint8_t FOOT_SIDE_LEFT = 0x0;
const uint8_t FOOT_SIDE_RIGHT = 0x1;

const uint8_t FOOT_PHOTO_MASK = 0x10;
const uint8_t FOOT_PHOTO_PLANTAR = 0x00;
const uint8_t FOOT_PHOTO_MEDIAL = 0x10;

uint8_t type = 0;
char* debug_filename_prepend = (char*)"";

void convolve5tap1d( double* src, double* dst, uint16_t length, const double* kernel )
{
  // why doesn't opencv have 1D convolution tools!? son of a...
  for ( int j=2; j<length-2; j++ ) {
    dst[j] = 0;
    for ( int i=0; i<5; i++ ) {
      dst[j] += kernel[i] * src[j+i-2];
    }
  }
}
// remove the leg from a foot-mask
double findFootWidth( Mat src )
{
  double scale;
  uint8_t *row;
  double *fg_frequency = new double[src.size().height];
  double *fg_frequency_derivative = new double[src.size().height];
  double *fg_frequency_acceleration = new double[src.size().height];
  uint16_t paper_width = 0;
  uint16_t foot_start_index = 0;
  uint16_t foot_width_index = 0;
  const double kernel[] = { -.25, -.25, 0, 0.25, 0.25 };
  double foot_width;

  // count the number of fg pixels
  for ( int j=0; j<src.size().height; j++ ) {
    row = src.ptr(j);
    fg_frequency[j] = 0;
    for ( int i=0; i<src.size().width; i++ )
      if ( row[i] )
        fg_frequency[j]++;
  }

  convolve5tap1d( fg_frequency, fg_frequency_derivative, src.size().height, kernel );
  convolve5tap1d( fg_frequency_derivative, fg_frequency_acceleration, src.size().height, kernel );
  for ( int j=10; j<src.size().height-10; j++ ) {
    fprintf( stderr, "%d:%.0f|%.1f\n", j, fg_frequency[j], fg_frequency_derivative[j] );
    // find the foot start
    if ( !foot_start_index && fg_frequency_derivative[j]<-2 ) {
      foot_start_index = j;
    }
  }
  fprintf( stderr, "\n\n" );
  // find the widest point
  uint16_t min_fg = fg_frequency[foot_start_index];
  for ( int j=foot_start_index; j<foot_start_index+200; j++ ) {
    if (fg_frequency[j] <= min_fg) {
      foot_width_index = j;
      min_fg = fg_frequency[j];
    }
  }

  // for debug, mark where the foot starts and the foot width are
  row = src.ptr(foot_start_index);
  for ( int i=0; i<src.size().width; i++ )
      row[i] = 0;
  row = src.ptr(foot_width_index);
  for ( int i=0; i<src.size().width; i++ )
      row[i] = 0;

  paper_width = fg_frequency[foot_start_index-5];
  foot_width = (paper_width-fg_frequency[foot_width_index]) / paper_width * 215.9; 

  fprintf( stdout, "{" );
  fprintf( stdout, "\"width\": %.1f", foot_width );
  fprintf( stdout, "}" );

  delete[] fg_frequency;
  delete[] fg_frequency_derivative;
  delete[] fg_frequency_acceleration;

  return foot_width;
}

void usage() {
  fprintf( stdout, "Usage: rulerless [OPTIONS]\n");
  fprintf( stdout, "  --input [input image]\n" );
}

int main( int argc, char** argv )
{
  char* input_path;

  // parse command line options
  int option_index = 0;
  int c;
  static struct option long_options[] = {
      {"input", required_argument, 0, 'i'},
      {"debug", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}
  };
  while (1) {
    c = getopt_long(argc, argv, "i:r:m:s:t:d:h", long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
      case 'i':
        input_path = optarg;
        break;
      case 'd':
        debug_filename_prepend = optarg;
        break;
      case 'h':
      case '?':
          usage();
          return 0;
        break;
    }
  }

  Mat input_image;
  Mat hsv_image;
  Mat greyscale_image;
  Mat seg_image;
  Mat paper_mask;

  char* mask_path = (char*)"rulerless_mask.png";

  Mat seg_mask;
  Mat bgdModel;
  Mat fgdModel;

  Mat kernel = getStructuringElement( MORPH_RECT, Size( 5, 5 ) );

  Mat debug_output;
  char debug_path[128];

  input_image = imread( input_path, CV_LOAD_IMAGE_COLOR );
  if ( !input_image.data ) {
    fprintf( stderr, "could not open input image %s\n", input_path );
    return -1;
  }

  cvtColor( input_image, greyscale_image, CV_BGR2GRAY );

  //equalizeHist( greyscale_image, greyscale_image );

  snprintf( debug_path, 128, "%s_input.jpg", debug_filename_prepend );
  imwrite( debug_path, greyscale_image );

  seg_image.create( input_image.size(), CV_8UC1 );

  // read mask
  paper_mask = imread( mask_path, CV_8UC1 );
  seg_mask = Mat( input_image.size(), CV_8UC1, GC_PR_BGD );

  // simple threshold segmentation
  seg_image.setTo( 0 );
  uint8_t thresh = 200;
  while ( !countNonZero(seg_image) ) {
    seg_image.setTo( 255, greyscale_image > thresh );
    thresh -= 10;
  }

  //erode( seg_image, seg_image, kernel );
  seg_mask.setTo( GC_FGD, seg_image == 255 );
  seg_mask.setTo( GC_BGD, paper_mask == 0 );

  // FIXME debug images should be a command line option
  input_image.copyTo( debug_output );
  convertScaleAbs( debug_output, debug_output, 0.25 );
  debug_output.setTo( 128 );
  input_image.copyTo( debug_output, seg_image==255 );
  snprintf( debug_path, 128, "%s_thresh.jpg", debug_filename_prepend );
  imwrite( debug_path, debug_output );

  grabCut( input_image, seg_mask, Rect(), bgdModel, fgdModel, 10, GC_INIT_WITH_MASK );

  seg_mask = seg_mask & 1;
  findFootWidth( seg_mask ); 

  input_image.copyTo( debug_output );
  convertScaleAbs( debug_output, debug_output, 0.25 );
  input_image.copyTo( debug_output, seg_mask & 1 );
  snprintf( debug_path, 128, "%s_seg.jpg", debug_filename_prepend );
  imwrite( debug_path, debug_output );

  return 0;
}

