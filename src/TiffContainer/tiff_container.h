#ifndef TIFF_IMAGE_HPP
#define TIFF_IMAGE_HPP

#include <string>
#include <tiffio.h>
#include <ostream>

#include <stdint.h>
#include <iostream>
#include <fstream>
#include "tiff_container.h"

class tiff_image16_ref {
  friend class tiff_container;
  
  public:
    tiff_image16_ref(int length, int width, int bits_per_pixel, int dir_number);
    tiff_image16_ref(tiff_image16_ref const& image);
    ~tiff_image16_ref();
    
    tiff_image16_ref copy();
    uint16_t *const *get_data();
    uint16_t const *const *get_data() const;
    
    int get_length() const;
    int get_width() const;
    int get_scanline_size() const;
    int get_bytes_per_pixel() const;
    int get_bits_per_pixel() const;
    int get_dir_number() const;
    
    tiff_image16_ref& operator=(tiff_image16_ref const& image);
    
    tiff_image16_ref& operator<<=(int shift); 
    tiff_image16_ref& operator>>=(int shift);
    tiff_image16_ref& operator*=(double d);
    tiff_image16_ref& operator-=(tiff_image16_ref const& image);

    tiff_image16_ref& operator+=(tiff_image16_ref const& image);

    tiff_image16_ref& operator-=(int subtrahend);
    
    tiff_image16_ref& apply_highpass(int fir_radius);

    int subtr_and_update_bg(tiff_image16_ref const& bg, float img_weight = 1.0/16);
    
    int substract_meanvalue();
    int calc_meanvalue();
    
    int histogram();
    
    void write_as_csv(std::ostream &out);
    
  private:
    tiff_image16_ref(uint16_t *const *data, int length, int width,
                 int scanline_size, int bits_per_pixel, int dir_number);
    
    int sum_halo(int row, int col, int fir_radius, int& halo_count);
                 
    uint16_t *const *data_;        ///< data array of image
    int length_;                ///< length of the image
    int width_;                 ///< width of the image
    int scanline_size_;         ///< size of a line in bytes
    int bits_per_pixel_;        ///< bits used to store a pixel value
    int *ref_count_;            ///< nr of references that point to same data
    int dir_number_;            ///< number of the image in its tiff container
};


class tiff_container {
  public:
    tiff_container(char * path, char *  mode);
    ~tiff_container();
    
    bool good();
    bool writeable();
    const char * const get_mode();
    const char * const  get_path();
    
    int get_dir_count();
    
    tiff_image16_ref get_image(int dir);
    void append_image(tiff_image16_ref const& image);
    void append_as_8bit_image(tiff_image16_ref const& image, int shift = 0);
	void append_new_16bit_image(uint16_t * data, int width, int length);
	void save_stack_as_tif(uint16_t * data_stack, int width, int length, int dir_count );

  private:
    
    static void TIFFWarningHandler(const char* module, const char* fmt, va_list ap);
    TIFF *tiff_;              ///< tiff file handle
    const char * const  path_;        ///< path of the tiff file
    const char * const  mode_;        ///< opening mode of the tiff file
    bool good_;               ///< indicated wether tiff file could be opened
    
};

#endif

