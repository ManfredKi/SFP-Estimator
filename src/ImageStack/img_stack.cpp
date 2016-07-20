#include <QDebug>
#include <QString>

#include <iostream>
#include <algorithm>
#include <string>   // memset
#include <cassert>
#include <cmath>

#include "img_stack.hpp"


// non-visible implementation classes for TIFF access

/// base class of all image-file accessors
class img_file_accessor
{
  public:
    virtual ~img_file_accessor(){}
    virtual bool writeable() = 0;
    virtual bool good() = 0;
    virtual std::string const& get_mode() = 0;
    virtual std::string const& get_path() = 0;
    
    virtual int img_count() = 0;

    virtual image16_ref get_image(int img) = 0;
    virtual void append_image(image16_ref const& image) = 0;
    virtual void append_as_8bit_image(image16_ref const& image, int shift = 0) = 0;
    virtual void append_new_16bit_image(uint16_t * data, int width, int length) = 0;

};


/// accessor for TIFF containers, supports reading and writing
class tiff_file_accessor : public img_file_accessor
{
  public:
    tiff_file_accessor(std::string path, std::string mode);
    ~tiff_file_accessor();
    
    bool good();
    bool writeable();
    std::string const& get_mode();
    std::string const& get_path();
    
    int img_count();

    image16_ref get_image(int img);
    void append_image(image16_ref const& image);
    void append_as_8bit_image(image16_ref const& image, int shift = 0);
 	void append_new_16bit_image(uint16_t * data, int width, int length);

  private:
    static void TIFFWarningHandler(const char* module, const char* fmt, va_list ap);
  
    TIFF *tiff_;              ///< tiff file handle
    std::string path_;        ///< path of the tiff file
    std::string mode_;        ///< opening mode of the tiff file
    bool good_;               ///< indicated wether tiff file could be opened
};


// public

/// Create an empty image
/** @param length length of the image in pixels
    @param width  width of the image in pixels
    @param bits_per_pixel range of a pixel value in bits
    @param img Number of the image in the stack
**/
image16_ref::image16_ref(int length, int width, int bits_per_pixel,  int img)
  : length_(length), width_(width), scanline_size_(width * sizeof(uint16)),
    bits_per_pixel_(bits_per_pixel), ref_count_(new int), dir_number_(img)
{ 
  uint16_t **data = new uint16_t*[length];
  uint16_t *data_rows = new uint16_t[length * width];
  
  for(int row = 0; row < (int) length; row++) {
    data[row] =  &data_rows[row * width];
    memset(data[row], '\0', scanline_size_);
  }
  
  assert(scanline_size_ == width * (int) sizeof(uint16_t));
  
  data_ = data;
  *ref_count_ = 1;
}


/// Copy an image reference without copying the image
/** @param image The reference to copy
**/
image16_ref::image16_ref(image16_ref const& image)
{ 
  if(data_ == image.data_) return;

  data_ = image.data_;
  length_ = image.length_;
  width_ = image.width_;
  scanline_size_ = image.scanline_size_;
  bits_per_pixel_ = image.bits_per_pixel_;
  dir_number_ = image.dir_number_;
  ref_count_ = image.ref_count_;

  (*ref_count_)++;
}

image16_ref::image16_ref()
   : length_(128), width_(128), scanline_size_(128 * sizeof(uint16)),
  bits_per_pixel_(16), ref_count_(new int), dir_number_(1)
{
    uint16_t **data = new uint16_t*[length_];
    uint16_t *data_rows = new uint16_t[length_ * width_];

    for(int row = 0; row < (int) length_; row++) {
      data[row] =  &data_rows[row * width_];
      memset(data[row], '\0', scanline_size_);
    }

    assert(scanline_size_ == width_ * (int) sizeof(uint16_t));

    data_ = data;
    *ref_count_ = 1;
}


/// Image destructor, delete data if last reference to it gets destructed
image16_ref::~image16_ref()
{
  (*ref_count_)--;  
  
  if(*ref_count_ == 0) {
    delete[] data_[0];
    delete[] data_;
    delete ref_count_;
    ref_count_ = nullptr;
    data_ = nullptr;
  }
  
}


/// Copy an image by copying its data (deep copy)
image16_ref image16_ref::copy()
{ 
  
  uint16_t **data = new uint16_t*[length_];
  uint16_t *data_rows = new uint16_t[length_ * width_];
  for(int row = 0; row < length_; row++) {
    data[row] = &data_rows[row * width_];
    memcpy(data[row], data_[row], scanline_size_);
  }
  
  assert(scanline_size_ == width_ * (int) sizeof(uint16_t));
  
  return image16_ref(data, length_, width_, scanline_size_, bits_per_pixel_, dir_number_);
}


/// Return data array of the image
uint16_t *const *image16_ref::get_data()
{
  return data_;
}


/// Return constant array of the data
uint16_t const* const *image16_ref::get_data() const
{
  return data_;
}


/// Return the length of the image
/** @return The length of the image in pixels
**/
int image16_ref::get_length() const
{
  return length_;
}


/// Return the length of the image
/** @return The length of the image in pixels
**/
int image16_ref::get_width() const
{
  return width_;
}


/// Return the directory number of the image in the tiff stack
/** @return The directory number of the image in the tiff stack
**/
int image16_ref::get_dir_number() const
{
  return dir_number_;
}


/// Return the size of one line in bytes of the image
/** @return The size of one line in bytes of the image
**/
int image16_ref::get_scanline_size() const
{
  return scanline_size_;
}


/// Return the number of bytes used to store one pixel
/** @return The number of bytes used to store one pixel
**/
int image16_ref::get_bytes_per_pixel() const
{
  return scanline_size_ / width_;
}


/// Retunr the range of a pixel value in bits
int image16_ref::get_bits_per_pixel() const
{
  return bits_per_pixel_;
}


/// Assignment operator
image16_ref& image16_ref::operator=(image16_ref const& image)
{
  if(data_ == image.data_) return *this;
  
  (*ref_count_)--;
  if(*ref_count_ == 0) {
    delete[] data_[0];
    delete[] data_;
    delete ref_count_;
  }

  data_ = image.data_;
  length_ = image.length_;
  width_ = image.width_;
  scanline_size_ = image.scanline_size_;
  bits_per_pixel_ = image.bits_per_pixel_;
  dir_number_ = image.dir_number_;
  ref_count_ = image.ref_count_;

  (*ref_count_)++;
  
  return *this;
}


/// Shift all pixel values to the left
/** @param shift shift amount
**/
image16_ref& image16_ref::operator<<=(int shift)
{
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] <<= shift;
    }
  }
  
  return *this;
}


/// Shift all pixel values to the right
/** @param shift shift amount
**/
image16_ref& image16_ref::operator>>=(int shift)
{
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] >>= shift;
    }
  }
  
  return *this;
}


/// Subtract an image from this image
/** @param image The image to subtract
**/
image16_ref& image16_ref::operator-=(image16_ref const& image)
{
  assert(length_ == image.length_ && width_ == image.width_);
  
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] = std::max(data_[row][col] - image.data_[row][col],0);
    }
  }
  
  return *this;
}


/// Add an image to this image
/** @param image The image to add
**/
image16_ref& image16_ref::operator+=(image16_ref const& image)
{
  assert(length_ == image.length_ && width_ == image.width_);
  
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] = data_[row][col] + image.data_[row][col];
    }
  }
  
  return *this;
}

void image16_ref::print(int size)
{
    assert(size <= length_ && size <= width_);

    for(int row = 0; row < size; row++) {
      QString out;
      for(int col = 0; col < size; col++) {
          out += QString::number(data_[row][col])+ "\t";
      }
      qDebug() << "\n";
    }

}


/// Multiply every pixel value by a factor
/** @param d Factor to multiply each pixel value with
**/
image16_ref& image16_ref::operator*=(double d)
{
    
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] = (uint16_t) (data_[row][col] * d);
    }
  }
  
  return *this;
}


/// Subtract background from image, update background, and return mean background
/** @param bg Image of the background
    @param img_weight Defines how much of the current image will form the updated background
    @return Mean value of picture
**/
int image16_ref::subtr_and_update_bg(image16_ref const& bg, float img_weight)
{
  assert(length_ == bg.length_ && width_ == bg.width_);
  assert(img_weight >= 0 && img_weight <= 1);
  
  int meanbg = 0;
  
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      uint16_t img_data = data_[row][col];
      uint16_t bg_data = bg.data_[row][col];
      int diff_data = std::min<int>( (img_data - bg_data) , (int)sqrt((double)img_data) ) ; // subtract background and test if change is bigger than sigma of noise
      data_[row][col] =  std::max( img_data - bg_data , 0 );
             
      bg.data_[row][col] = (uint16_t) (bg_data  + diff_data * img_weight);      // update background
      meanbg += img_data;
    }
  }
  meanbg /= (length_*width_);
  
  return meanbg;
}


/// Subtract a value from all pixel values
/** @param subtrahend The value to subtract from each pixel value
**/
image16_ref& image16_ref::operator-=(int subtrahend)
{
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] = std::max(data_[row][col] - subtrahend, 0);
    }
  }
  
  return *this;
}


/// Apply a 2D high-pass filter to the image
/** @param fir_radius The radius of the square where the average is calculated from
**/
image16_ref& image16_ref::apply_highpass(int fir_radius)
{  
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] = sum_halo(row, col, fir_radius);
    }
  }
  
  return *this;
}

/// Apply a 2D high-pass filter to a second image
/** @param fir_radius The radius of the square where the average is calculated from
    @param fir_img The radius of the square where the average is calculated from
**/
image16_ref& image16_ref::apply_highpass(int fir_radius, image16_ref *fir_img)
{
  assert(length_ == fir_img->length_ && width_ == fir_img->width_);

  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      fir_img->data_[row][col] = sum_halo(row, col, fir_radius);
    }
  }

  return *this;
}

/// Print a histogram of all pixel values to stdout
int image16_ref::histogram()
{
  const int HISTSIZE = 5000;
  int *values = new int[HISTSIZE];
  memset(values, '\0', HISTSIZE );
  int maxvalue=0;


  for(int i=0;i<length_;i++)
  {
    for(int j=0;j<width_;j++)
    {
      if(data_[i][j]>=HISTSIZE)
      {
        maxvalue=HISTSIZE;
        values[HISTSIZE-1]++;
      } else {
        values[data_[i][j]]++;
        if (data_[i][j]>maxvalue)
          maxvalue=data_[i][j];
      }
    }
  }  
  /* output */
  for(int i=0;i <= maxvalue; i++)
  {
    std::cout << i << "\t" << values[i] << std::endl;
  }
  delete values;
  
  return maxvalue;
}

/// Calculate and subtract the mean pixel value from the image
/** @return The mean pixel value
**/
int image16_ref::substract_meanvalue()
{

  int meanbg = 0;
  
  for(int i=0; i<length_; i++)
  {
    for(int j=0; j<width_; j++)
    {
      meanbg += data_[i][j];
    }  
  }
  meanbg = meanbg / (length_ * width_);

  for(int i=0; i<length_; i++)
  {
    for(int j=0; j<width_; j++)
    {
      data_[i][j] = std::max( data_[i][j] - meanbg, 0);
      //data_[i][j] -= value;    
    }  
  }   
  
  return (int) meanbg;
}


/// Calculate the mean pixel value from the image
/** @return The mean pixel value
**/
int image16_ref::calc_meanvalue()
{
 int value = 0;
  
  for(int i=0; i<length_; i++)
  {
    for(int j=0; j<width_; j++)
    {
      value += data_[i][j];
    }  
  }
  value = value / (length_ * width_);
  
  return value;
}


/// Output the image as CSV
/**  @param out The output stream to print the values to
**/
void image16_ref::write_as_csv(std::ostream &out)
{
  for(int row = 0; row < length_; row++)
  {
    for(int col = 0; col < width_; col++)
    {
      out << data_[row][col] << "; ";
    }
    out << std::endl;
  }
}



// private
/// Create an image reference from a tiff container
image16_ref::image16_ref(uint16_t *const *data, int length, int width,
                           int scanline_size, int bits_per_pixel, int dir_number)
  : data_(data), length_(length), width_(width), scanline_size_(scanline_size),
    bits_per_pixel_(bits_per_pixel), ref_count_(new int), dir_number_(dir_number)
{           
  *ref_count_ = 1;
}

/// Calculate the sum of all pixel values in a halo
int image16_ref::sum_halo(int row, int col, int fir_radius)
{
  int halo_acc = 0;
  int halo_count = 0;
  
  for(int row_halo = row - fir_radius; row_halo < row + fir_radius + 1; row_halo++) {
    for(int col_halo = col - fir_radius; col_halo < col + fir_radius + 1; col_halo++) {
      if(row_halo > 0 && row_halo < length_ && col_halo > 0 && col_halo < width_) {
        halo_acc += data_[row_halo][col_halo];
        halo_count++;
      }
    }
  }
  
  return halo_acc/halo_count;
}


/********************** img_stack **********************************/
// public

img_stack::img_stack(std::string path, std::string mode)
{
  this->accessor_ = NULL;
  std::string extension = path.substr(path.find_last_of('.') + 1);
  
  std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
  if(extension.compare("tif") == 0 || extension.compare("tiff") == 0) {
    this->accessor_ = new tiff_file_accessor(path, mode);
  } else {
    std::cerr << "unknown file format " << extension << std::endl;
  }
}


img_stack::~img_stack()
{
  delete accessor_;
}


bool img_stack::good()
{
  return accessor_ && accessor_->good();
}

bool img_stack::writeable()
{
  return accessor_ && accessor_->writeable();
}

std::string const& img_stack::get_mode()
{
  assert(good());
  return accessor_->get_mode();
}

std::string const& img_stack::get_path()
{
  assert(good());
  return accessor_->get_path();
}

int img_stack::img_count()
{
  assert(good());
  return accessor_->img_count();
}

image16_ref img_stack::get_image(int img)
{
  assert(good());
  return accessor_->get_image(img);
}

void img_stack::append_image(image16_ref const& image)
{
  assert(good());
  assert(accessor_->writeable());
  accessor_->append_image(image);
}

void img_stack::append_as_8bit_image(image16_ref const& image, int shift)
{
  assert(good());
  assert(accessor_->writeable());
  accessor_->append_as_8bit_image(image, shift);
}

void img_stack::append_new_16bit_image(uint16_t * data, int width, int length)
{
  assert(good());
  assert(accessor_->writeable());
  accessor_->append_new_16bit_image(data, width, length);
}

/// Create an image that is the mean average of all images with begin <= img < end
image16_ref img_stack::average_img(int begin, int end)
{ 
  assert(good());
  assert(begin < end);
  
  image16_ref average = accessor_->get_image(begin);
  
  for(int i = begin + 1; i < end; i++){
    average += this->get_image(i);   
  }
  
  average *= (2.0/(end - begin));
  
  return average;

}

/// Create an image that is the mean average of all images with begin <= img < end with subtracted meanvalue
image16_ref img_stack::overview_img(int begin, int end)
{
  assert(good());
  assert(begin < end);

  image16_ref average = accessor_->get_image(begin);
  average.substract_meanvalue();
  for(int i = begin + 1; i < end; i++){
    image16_ref next =  this->get_image(i);
    next.substract_meanvalue();
    average += next;
  }

  average *= (2.0/(end - begin));

  return average;

}

/********************** img_file_accessor **********************************/
// purely virtual class, no implementation

/********************** tiff_file_accessor **********************************/
// public

/// Create a tiff container object from a tiff file
tiff_file_accessor::tiff_file_accessor(std::string path, std::string mode)
  : path_(path), mode_(mode)
{
  TIFFSetWarningHandler(&TIFFWarningHandler);
  tiff_ = TIFFOpen(path.c_str(), mode.c_str());
  if(!tiff_) {
    good_ = false;
  }
  good_ = true;
}

/// Close a tiff container
tiff_file_accessor::~tiff_file_accessor()
{
  if(good_) {
    TIFFClose(tiff_);
  }

}

/// Indicate wheter the tiff container could be opened
/** @return true iff tiff file could be read and object is valie
**/
bool tiff_file_accessor::good()
{
  return good_;
}

/// Return wheter the images in the container can be saved after modifications
/** true iff the images in the container can be saved after modifications
**/
bool tiff_file_accessor::writeable()
{
  return mode_.find('w') != std::string::npos;
}

/// Return the opening mode of the tiff file
/** @return the opening mode of the tiff file
**/
std::string const& tiff_file_accessor::get_mode()
{
  return mode_;
}

/// Get the path of the tiff file
/** @return the path of the tiff file
**/
std::string const& tiff_file_accessor::get_path()
{
  return path_;
}

/// Get the number of images in the tiff container
/** @return the number of images in the tiff container
**/
int tiff_file_accessor::img_count()
{
  int count = 0;
  TIFFSetDirectory(tiff_, 0);
  
  do {
    count++;
  } while (TIFFReadDirectory(tiff_));
  
  TIFFSetDirectory(tiff_, 0);
  return count;
}

/// Get an image from the tiff container
/** @param img The number of the image
    @return a refernce to the requested image
**/
image16_ref tiff_file_accessor::get_image(int img)
{ 
  TIFFSetDirectory(tiff_, img);
  
  tsize_t scanline_size = TIFFScanlineSize(tiff_);
  
  uint32 length;
  TIFFGetField(tiff_,  TIFFTAG_IMAGELENGTH, &length);
  
  uint32 width;
  TIFFGetField(tiff_,  TIFFTAG_IMAGEWIDTH, &width);
  
  uint16 bits_per_pixel;
  TIFFGetField(tiff_, TIFFTAG_BITSPERSAMPLE, &bits_per_pixel);
  
  assert(scanline_size == (tsize_t) (sizeof(uint16_t) * width));
  
  /*
  uint16_t **data = new uint16_t*[length];
  for(int row = 0; row < (int) length; row++) {
    data[row] = new uint16_t[width];
    TIFFReadScanline(tiff_, data[row], row);
  }*/
  
  
  uint16_t **data = new uint16_t*[length];
  uint16_t *data_rows = new uint16_t[length * width];
  
  for(int row = 0; row < (int) length; row++) {
    data[row] =  &data_rows[row * width];
    TIFFReadScanline(tiff_, data[row], row);
  }
  
  // rely on return value optimization here,
  // copy-constructor should not be called
  return image16_ref(data, length, width, scanline_size, bits_per_pixel, img);
}

/// Append an image to the end of the tiff container
/** @param image the image to append
**/
void tiff_file_accessor::append_image(image16_ref const& image)
{
  int length = image.get_length();

  TIFFSetField(tiff_, TIFFTAG_IMAGEWIDTH, image.get_width());      // set the width of the image
  TIFFSetField(tiff_, TIFFTAG_IMAGELENGTH, image.get_length());    // set the height of the image
  TIFFSetField(tiff_, TIFFTAG_SAMPLESPERPIXEL, 1);                 // set number of channels per pixel
  TIFFSetField(tiff_, TIFFTAG_BITSPERSAMPLE, 16);                  // set the size of the channels
  TIFFSetField(tiff_, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);   // set the origin of the image
  TIFFSetField(tiff_, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tiff_, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff_, image.get_scanline_size()));

  for(int row = 0; row < length; row++) {
    TIFFWriteScanline(tiff_, (tdata_t) image.get_data()[row], row);
  }
  
  TIFFWriteDirectory(tiff_);
  
}

/// Append an image to the end of the tiff container after converting it to 8 bits
/** @param image the image to append
    @param shift the amount of pixels the image should be shifted to the left before appending
**/
void tiff_file_accessor::append_as_8bit_image(image16_ref const& image, int shift)
{
  int length = image.get_length();
  int width  = image.get_width();
  
  TIFFSetField(tiff_, TIFFTAG_IMAGEWIDTH, image.get_width());  // set the width of the image
  TIFFSetField(tiff_, TIFFTAG_IMAGELENGTH, image.get_length());    // set the height of the image
  TIFFSetField(tiff_, TIFFTAG_SAMPLESPERPIXEL, 1);   // set number of channels per pixel
  TIFFSetField(tiff_, TIFFTAG_BITSPERSAMPLE, 8);    // set the size of the channels
  TIFFSetField(tiff_, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);    // set the origin of the image
  TIFFSetField(tiff_, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tiff_, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff_, width * sizeof(uint8)));
  
  uint8 *row_8bit = new uint8[width];
  
  for(int row = 0; row < length; row++) {
    for(int col = 0; col < width; col++) {
      row_8bit[col] = image.get_data()[row][col] >> shift;
    }
    TIFFWriteScanline(tiff_, (tdata_t) row_8bit, row);
  }
  
  delete[] row_8bit;
  
  TIFFWriteDirectory(tiff_);
}

void tiff_file_accessor::append_new_16bit_image(uint16_t * data, int width, int length)
{

  TIFFSetField(tiff_, TIFFTAG_IMAGEWIDTH, width);      // set the width of the image
  TIFFSetField(tiff_, TIFFTAG_IMAGELENGTH,length);    // set the height of the image
  TIFFSetField(tiff_, TIFFTAG_SAMPLESPERPIXEL, 1);                 // set number of channels per pixel
  TIFFSetField(tiff_, TIFFTAG_BITSPERSAMPLE, 16);                  // set the size of the channels
  TIFFSetField(tiff_, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);   // set the origin of the image
  TIFFSetField(tiff_, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tiff_, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff_, width*2));

  for(int row = 0; row < length; row++) {
    TIFFWriteScanline(tiff_, (tdata_t) &data[row*width], row);
  }
  
  TIFFWriteDirectory(tiff_);
  
}


void tiff_file_accessor::TIFFWarningHandler(const char* /*module*/, const char* /*fmt*/, va_list /*ap*/)
{
  /* do nothing*/
}
