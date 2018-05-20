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

// sum the diff between two mats of same dims
uint64_t diff( Mat a, Mat b )
{
  Mat diff;
  absdiff( a, b, diff );
  return sum( diff ).val[0];
}

// fudge out the top of an image
void fudgeIt( Mat src, Mat dst, uint16_t gradient_offset, uint16_t gradient_height )
{
  src.copyTo( dst );
  double scale;
  uint8_t *row;
  for ( int j=0; j<gradient_offset; j++ )
  {
    row = dst.ptr(j);
    for ( int i=0; i<dst.size().width; i++ )
      row[i] = 0;
  }
  for ( int j=0; j<gradient_height; j++ )
  {
    row = dst.ptr(gradient_offset+j);
    scale = 1.0/(sqrt(gradient_height)-sqrt(j));
    if ( scale > 1.0 ) break;
    for ( int i=0; i<dst.size().width; i++ )
    {
      row[i] *= scale;
    }
  }
}
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
void amputateLeg( Mat src, Mat dst )
{
  double scale;
  uint8_t *row;
  double *fg_frequency = new double[dst.size().height];
  double *fg_frequency_derivative = new double[dst.size().height];
  double *fg_frequency_acceleration = new double[dst.size().height];
  double max_derivative = 2;
  uint16_t max_deriative_index = 0;

  src.copyTo( dst );

  // count the number of fg pixels
  for ( int j=0; j<dst.size().height; j++ ) {
    row = dst.ptr(j);
    fg_frequency[j] = 0;
    for ( int i=0; i<dst.size().width; i++ )
      if ( row[i] )
        fg_frequency[j]++;
  }

  // figure out where we want to cut
  const double kernel[] = { -.25, -.25, 0, 0.25, 0.25 };
  convolve5tap1d( fg_frequency, fg_frequency_derivative, dst.size().height, kernel );
  //convolve5tap1d( fg_frequency_derivative, fg_frequency_acceleration, dst.size().height, kernel );
  // turns out there's only a small area on the image that we're interested in
  for ( int j=100; j<dst.size().height/2; j++ ) {
    if ( fg_frequency_derivative[j] > max_derivative ) {
      max_derivative = fg_frequency_derivative[j];
      max_deriative_index = j;
      break;
    }
    fprintf( stderr, "%.0f:%.1f ", fg_frequency[j], fg_frequency_derivative[j] );
  }
  fprintf( stderr, "\n\n" );

  fudgeIt(src, dst, 100, max_deriative_index-100);

  delete[] fg_frequency;
  delete[] fg_frequency_derivative;
  delete[] fg_frequency_acceleration;
}


// use EnhancedCorrelationCoefficient maximization
Mat useECC( Mat a, Mat b )
{
  // Define the motion model
  const int warp_mode = MOTION_AFFINE;

  // Storage for warped image.
  Mat b_aligned;

  // helper mats
  float t_image_plantar[4][4] = {{1,0,0,640},{0,1,0,360},{0,0,1,0},{0,0,0,1}};
  float t_world_plantar[4][4] = {{1,0,0,-640},{0,1,0,-360},{0,0,1,0},{0,0,0,1}};
  // for the medial image, I need to translate by the offset of the render
  // TODO fix this, seems a bit off
  float t_image_medial[4][4] = {{1,0,0,0},{0,1,0,640},{0,0,1,360},{0,0,0,1}};
  float t_world_medial[4][4] = {{1,0,0,0},{0,1,0,-640},{0,0,1,-360},{0,0,0,1}};
  Mat translate_image;
  Mat translate_world;
  float n_scale;
  Mat normalize_scale = Mat::eye(4, 4, CV_32F);

  Mat output_transform = Mat(4, 4, CV_32F);

  // Set a 2x3 or 3x3 warp matrix depending on the motion model.
  Mat warp_matrix;
  if ( warp_mode == MOTION_HOMOGRAPHY )
    warp_matrix = Mat::eye(3, 3, CV_32F);
  else
    warp_matrix = Mat::eye(2, 3, CV_32F);

  // Define termination criteria
  int number_of_iterations = 100;
  double termination_eps = 1e-5;
  TermCriteria criteria (TermCriteria::COUNT+TermCriteria::EPS, number_of_iterations, termination_eps);

  // Run the ECC algorithm. The results are stored in warp_matrix.
  try {
    findTransformECC(
      a,
      b,
      warp_matrix,
      warp_mode,
      criteria
    );
  } catch(Exception e) {
    fprintf( stderr, "%s\n", e.msg.c_str() );
    exit(-1);
  }

  if (warp_mode != MOTION_HOMOGRAPHY) {
    // Use warpAffine for Translation, Euclidean and Affine
    warpAffine(b, b_aligned, warp_matrix, a.size(), INTER_LINEAR + WARP_INVERSE_MAP);
  } else {
    // Use warpPerspective for Homography
    warpPerspective (b, b_aligned, warp_matrix, a.size(), INTER_LINEAR + WARP_INVERSE_MAP);
  }

  // invert the warp matrix
  Mat inverse_warp_matrix = Mat::eye(4, 4, CV_32F);

  for ( int i=0; i<2; i++ ) {
    uint8_t j=i;
    uint8_t dim_offset = 0;
    if ( type & FOOT_PHOTO_MEDIAL ) {
      j += 1;
      dim_offset = 1;
    }
    inverse_warp_matrix.at<float>(j,0+dim_offset) = warp_matrix.at<float>(i, 0);
    inverse_warp_matrix.at<float>(j,1+dim_offset) = warp_matrix.at<float>(i, 1);
    inverse_warp_matrix.at<float>(j,3) = warp_matrix.at<float>(i, 2);
  }

  invert( inverse_warp_matrix, inverse_warp_matrix );

  if ( type & FOOT_PHOTO_MEDIAL ) {
    // set x scaling to average of scaling of y and z
    inverse_warp_matrix.at<float>(0,0) = (inverse_warp_matrix.at<float>(1,1) + inverse_warp_matrix.at<float>(2,2)) / 2.0;
    n_scale = 1.0 / inverse_warp_matrix.at<float>(0,0);

    translate_image = Mat(4, 4, CV_32F, t_image_medial);
    translate_world = Mat(4, 4, CV_32F, t_world_medial);
  } else {
    // set z scaling to average of scaling of x and y
    inverse_warp_matrix.at<float>(2,2) = (inverse_warp_matrix.at<float>(0,0) + inverse_warp_matrix.at<float>(1,1)) / 2.0;
    n_scale = 1.0 / inverse_warp_matrix.at<float>(2,2);

    translate_image = Mat(4, 4, CV_32F, t_image_plantar);
    translate_world = Mat(4, 4, CV_32F, t_world_plantar);
  }

  // multiply in the offsets and scaling
  normalize_scale.at<float>(0,0) = n_scale;
  normalize_scale.at<float>(1,1) = n_scale;
  normalize_scale.at<float>(2,2) = n_scale;
  output_transform = normalize_scale * translate_world * inverse_warp_matrix * translate_image;

  // compute heuristics
  const uint64_t segment_pixel_count_mean = 101290;
  const double segment_pixel_count_sdev = 15064.0844978;
  double segment_energy = abs(countNonZero(a)-segment_pixel_count_mean)/segment_pixel_count_sdev;

  fprintf( stdout, "{" );
    fprintf( stdout, "\"segment_energy\": %.2f,", segment_energy );
    fprintf( stdout, "\"register_energy\": %lu,", diff(a, b_aligned) );
    fprintf( stdout, "\"warp\": [" );
    for ( int i=0; i<4; i++ ) {
      if ( i ) fprintf( stdout, "," );
      fprintf( stdout, "[" );
      for ( int j=0; j<4; j++ ) {
        if ( j ) fprintf( stdout, "," );
        fprintf( stdout, "%.2f", output_transform.at<float>(i, j) );
      }
      fprintf( stdout, "]" );
    }
    fprintf( stdout, "]" );
  fprintf( stdout, "}" );

  return b_aligned;
}

void usage() {
  fprintf( stdout, "Usage: register [OPTIONS]\n");
  fprintf( stdout, "  --input [input image]\n" );
  fprintf( stdout, "  --register [registration target]\n" );
  fprintf( stdout, "  --side [left|right]\n" );
  fprintf( stdout, "  --type [plantar|medial]\n" );
}

int main( int argc, char** argv )
{
  char* input_path;
  char* register_path;
  char* mask_path = (char*)"bottom_mask.png";

  // parse command line options
  int option_index = 0;
  int c;
  static struct option long_options[] = {
      {"input", required_argument, 0, 'i'},
      {"register", required_argument, 0, 'r'},
      {"mask", required_argument, 0, 'm'},
      {"side", required_argument, 0, 's'},
      {"type", required_argument, 0, 't'},
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
      case 'r':
        register_path = optarg;
        break;
      case 'm':
        mask_path = optarg;
        break;
      case 's':
        if ( !strncmp(optarg, "right", 5) )
          type |= FOOT_SIDE_RIGHT;
        break;
      case 't':
        if ( !strncmp(optarg, "medial_sagittal", 9) )
          type |= FOOT_PHOTO_MEDIAL;
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
  Mat register_image;
  Mat input_mask;
  Mat seg_image;

  Mat seg_mask;
  Mat bgdModel;
  Mat fgdModel;

  Mat warp;

  Mat debug_output;
  char debug_path[128];

  input_image = imread( input_path, CV_LOAD_IMAGE_COLOR );
  if ( !input_image.data ) {
    fprintf( stderr, "could not open input image %s\n", input_path );
    return -1;
  }

  snprintf( debug_path, 128, "%s_input.jpg", debug_filename_prepend );
  imwrite( debug_path, input_image );

  if ( type & FOOT_SIDE_RIGHT ) {
    flip(input_image, input_image, 1);
  }

  register_image = imread( register_path, CV_LOAD_IMAGE_COLOR );
  if( !register_image.data ) {
    fprintf( stderr, "could not open registration target %s\n", register_path );
    return -1;
  }
  cvtColor(register_image, register_image, CV_BGR2GRAY);
  // FIXME blender rendered register images values are inverted
  register_image = abs(255 - register_image);
  // FIXME blender medial images are rotated
  if ( type & FOOT_PHOTO_MEDIAL ) {
    transpose(register_image, register_image);  
    amputateLeg(register_image, register_image);
    flip(register_image, register_image, 1);
  }

  seg_image.create( input_image.size(), CV_8UC1 );

  // rect not used
  Rect rect( 1, 1, input_image.size().width-2, input_image.size().height-2 );

  // read bottom mask
  input_mask = imread( mask_path, CV_8UC1 );
  seg_mask = Mat( input_image.size(), CV_8UC1, GC_PR_BGD );
  imwrite( "debug_mask.jpg", input_mask );
  seg_mask.setTo( GC_FGD, input_mask == 255 );
  seg_mask.setTo( GC_BGD, input_mask == 0 );

  // grabcut segmentation
  grabCut( input_image, seg_mask, rect, bgdModel, fgdModel, 10, GC_INIT_WITH_MASK );

  seg_image.setTo( 0 );
  seg_image.setTo( 255, seg_mask & 1 );

  if ( type & FOOT_PHOTO_MEDIAL ) {
    amputateLeg(seg_image, seg_image);
  }

  // FIXME debug images should be a command line option
  input_image.copyTo( debug_output );
  convertScaleAbs( debug_output, debug_output, 0.25 );
  input_image.copyTo( debug_output, seg_image==255 );
  snprintf( debug_path, 128, "%s_seg.jpg", debug_filename_prepend );
  imwrite( debug_path, debug_output );

  warp = useECC( seg_image, register_image );

  // Show final result
  input_image.copyTo( debug_output );
  convertScaleAbs( debug_output, debug_output, 0.25 );
  input_image.copyTo( debug_output, warp==255 );
  snprintf( debug_path, 128, "%s_warped.jpg", debug_filename_prepend );
  imwrite( debug_path, debug_output );

  return 0;
}

