/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors (fill in below):
// NMec:  Name:
// 
// 
// 
// Date:
//

#include "image8bit.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "instrumentation.h"

// The data structure
//
// An image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the 8-bit gray
// level of each pixel in the image.  The pixel array is one-dimensional
// and corresponds to a "raster scan" of the image from left to right,
// top to bottom.
// For example, in a 100-pixel wide image (img->width == 100),
//   pixel position (x,y) = (33,0) is stored in img->pixel[33];
//   pixel position (x,y) = (22,1) is stored in img->pixel[122].
// 
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Maximum value you can store in a pixel (maximum maxval accepted)
const uint8 PixMax = 255;

// Internal structure for storing 8-bit graymap images
struct image {
  int width;
  int height;
  int maxval;   // maximum gray value (pixels with maxval are pure WHITE)
  uint8* pixel; // pixel data (a raster scan)
};


// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
// 
// When one of these functions fails, it signals this by returning an error
// value such as NULL or 0 (see function documentation), and sets an internal
// variable (errCause) to a string indicating the failure cause.
// The errno global variable thoroughly used in the standard library is
// carefully preserved and propagated, and clients can use it together with
// the ImageErrMsg() function to produce informative error messages.
// The use of the GNU standard library error() function is recommended for
// this purpose.
//
// Additional information:  man 3 errno;  man 3 error;

// Variable to preserve errno temporarily
static int errsave = 0;

// Error cause
static char* errCause;

/// Error cause.
/// After some other module function fails (and returns an error code),
/// calling this function retrieves an appropriate message describing the
/// failure cause.  This may be used together with global variable errno
/// to produce informative error messages (using error(), for instance).
///
/// After a successful operation, the result is not garanteed (it might be
/// the previous error cause).  It is not meant to be used in that situation!
char* ImageErrMsg() { ///
  return errCause;
}


// Defensive programming aids
//
// Proper defensive programming in C, which lacks an exception mechanism,
// generally leads to possibly long chains of function calls, error checking,
// cleanup code, and return statements:
//   if ( funA(x) == errorA ) { return errorX; }
//   if ( funB(x) == errorB ) { cleanupForA(); return errorY; }
//   if ( funC(x) == errorC ) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Understanding such chains is difficult, and writing them is boring, messy
// and error-prone.  Programmers tend to overlook the intricate details,
// and end up producing unsafe and sometimes incorrect programs.
//
// In this module, we try to deal with these chains using a somewhat
// unorthodox technique.  It resorts to a very simple internal function
// (check) that is used to wrap the function calls and error tests, and chain
// them into a long Boolean expression that reflects the success of the entire
// operation:
//   success = 
//   check( funA(x) != error , "MsgFailA" ) &&
//   check( funB(x) != error , "MsgFailB" ) &&
//   check( funC(x) != error , "MsgFailC" ) ;
//   if (!success) {
//     conditionalCleanupCode();
//   }
//   return success;
// 
// When a function fails, the chain is interrupted, thanks to the
// short-circuit && operator, and execution jumps to the cleanup code.
// Meanwhile, check() set errCause to an appropriate message.
// 
// This technique has some legibility issues and is not always applicable,
// but it is quite concise, and concentrates cleanup code in a single place.
// 
// See example utilization in ImageLoad and ImageSave.
//
// (You are not required to use this in your code!)


// Check a condition and set errCause to failmsg in case of failure.
// This may be used to chain a sequence of operations and verify its success.
// Propagates the condition.
// Preserves global errno!
static int check(int condition, const char* failmsg) {
  errCause = (char*)(condition ? "" : failmsg);
  return condition;
}


/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) { ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  // Name other counters here...
  InstrName[1] = "pixel reads";
  InstrName[2] = "pixel writes";
  
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
// Add more macros here...
#define PIXRD InstrCount[1]
#define PIXWR InstrCount[2]

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!

//Para arredondar os píxeis desfocados para o uint8 mais próximo em vez de truncar.
int arred(float f) {
	int i = 0;
	while(f >= 1) {
		f--;
		i++;
	}
	if(f >= 0.5) {
		i++;
	}
	
	return i;
}


/// Image management functions

/// Create a new black image.
///   width, height : the dimensions of the new image.
///   maxval: the maximum gray level (corresponding to white).
/// Requires: width and height must be non-negative, maxval > 0.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCreate(int width, int height, uint8 maxval) { ///
  assert (width >= 0);
  assert (height >= 0);
  assert (0 < maxval && maxval <= PixMax);
  // Insert your code here!
  Image img = (Image)malloc(sizeof(struct image));
  if(img == NULL) {
	  errno = ENOMEM; //Será que deixo o próprio malloc definir o errno?
	  errCause = "Falha ao alocar memória para a nova imagem\n"; //Será que uso o check para definir o errCause?
	  return NULL;
  }
  
  uint8* pixel = (uint8*)malloc(width*height);
  if(pixel == NULL) {
	  errno = ENOMEM; //Será que deixo o próprio malloc definir o errno?
	  errCause = "Falha ao alocar memória para os píxeis da nova imagem\n"; //Será que uso o check para definir o errCause?
	  return NULL;
  }
  
  img->width = width;
  img->height = height;
  img->maxval = maxval;
  img->pixel = pixel;
  
  return img;
}

/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail, and should preserve global errno/errCause.
void ImageDestroy(Image* imgp) { ///
  assert (imgp != NULL);
  // Insert your code here!
  free(*imgp);
  *imgp = NULL;
}


/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE* f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Load a raw PGM file.
/// Only 8 bit PGM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageLoad(const char* filename) { ///
  int w, h;
  int maxval;
  char c;
  FILE* f = NULL;
  Image img = NULL;

  int success = 
  check( (f = fopen(filename, "rb")) != NULL, "Open failed" ) &&
  // Parse PGM header
  check( fscanf(f, "P%c ", &c) == 1 && c == '5' , "Invalid file format" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &w) == 1 && w >= 0 , "Invalid width" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &h) == 1 && h >= 0 , "Invalid height" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d", &maxval) == 1 && 0 < maxval && maxval <= (int)PixMax , "Invalid maxval" ) &&
  check( fscanf(f, "%c", &c) == 1 && isspace(c) , "Whitespace expected" ) &&
  // Allocate image
  (img = ImageCreate(w, h, (uint8)maxval)) != NULL &&
  // Read pixels
  check( fread(img->pixel, sizeof(uint8), w*h, f) == w*h , "Reading pixels" );
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (!success) {
    errsave = errno;
    ImageDestroy(&img);
    errno = errsave;
  }
  if (f != NULL) fclose(f);
  return img;
}

/// Save image to PGM file.
/// On success, returns nonzero.
/// On failure, returns 0, errno/errCause are set appropriately, and
/// a partial and invalid file may be left in the system.
int ImageSave(Image img, const char* filename) { ///
  assert (img != NULL);
  int w = img->width;
  int h = img->height;
  uint8 maxval = img->maxval;
  FILE* f = NULL;

  int success =
  check( (f = fopen(filename, "wb")) != NULL, "Open failed" ) &&
  check( fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0, "Writing header failed" ) &&
  check( fwrite(img->pixel, sizeof(uint8), w*h, f) == w*h, "Writing pixels failed" ); 
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (f != NULL) fclose(f);
  return success;
}


/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img) { ///
  assert (img != NULL);
  return img->width;
}

/// Get image height
int ImageHeight(Image img) { ///
  assert (img != NULL);
  return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img) { ///
  assert (img != NULL);
  return img->maxval;
}

/// Pixel stats
/// Find the minimum and maximum gray levels in image.
/// On return,
/// *min is set to the minimum gray level in the image,
/// *max is set to the maximum.
void ImageStats(Image img, uint8* min, uint8* max) { ///
  assert (img != NULL);
  // Insert your code here!
  int img_size = img->width*img->height;
  
  //O que fazer se o tamanho da imagem for zero?
  *min = *max = img->pixel[0];
  
  if(img_size == 1)
	return;
  
  //Achar os píxeis de valor mínimo e máximo
  uint8 amin = *min, amax = *max;
  for(int i = 1; i < img_size; i++) {
	  if(img->pixel[i] < amin)
		  amin = img->pixel[i];
	  else if(img->pixel[i] > amax)
		  amax = img->pixel[i];
  }
  
  *min = amin;
  *max = amax;
}

/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y) { ///
  assert (img != NULL);
  return (0 <= x && x < img->width) && (0 <= y && y < img->height);
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h) { ///
  assert (img != NULL);
  // Insert your code here!
  //Se o canto inferior direito da área retangular estiver dentro da imagem, então toda a área retangular estará dentro da imagem
  return(ImageValidPos(img, x+w-1, y+h-1));
}

/// Pixel get & set operations

/// These are the primitive operations to access and modify a single pixel
/// in the image.
/// These are very simple, but fundamental operations, which may be used to 
/// implement more complex operations.

// Transform (x, y) coords into linear pixel index.
// This internal function is used in ImageGetPixel / ImageSetPixel. 
// The returned index must satisfy (0 <= index < img->width*img->height)
static inline int G(Image img, int x, int y) {
  int index;
  // Insert your code here!
  index = y*img->width + x;
  assert (0 <= index && index < img->width*img->height);
  return index;
}

/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (read)
  PIXRD++;
  return img->pixel[G(img, x, y)];
} 

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (store)
  PIXWR++;
  img->pixel[G(img, x, y)] = level;
} 


/// Pixel transformations

/// These functions modify the pixel levels in an image, but do not change
/// pixel positions or image geometry in any way.
/// All of these functions modify the image in-place: no allocation involved.
/// They never fail.


/// Transform image to negative image.
/// This transforms dark pixels to light pixels and vice-versa,
/// resulting in a "photographic negative" effect.
void ImageNegative(Image img) { ///
  assert (img != NULL);
  // Insert your code here!
  uint8 maxval = img->maxval;
  for(int i = 0; i < img->width*img->height; i++)
    img->pixel[i] = maxval - img->pixel[i];
}

/// Apply threshold to image.
/// Transform all pixels with level<thr to black (0) and
/// all pixels with level>=thr to white (maxval).
void ImageThreshold(Image img, uint8 thr) { ///
  assert (img != NULL);
  assert (img != NULL);
    img->width = ImageWidth(img); //nao sei se aqui é assim ou int width
    img->height = ImageHeight(img);
    img->maxval = ImageMaxval(img);

    for ( uint32_t  i = 0; i < img->height; i++) {
        for (uint32_t  j = 0; j < img->width; j++) {
            uint8 pixel = ImageGetPixel(img, j, i);

            if (pixel < thr) {
                ImageSetPixel(img, j, i, 0);
            } else {
                ImageSetPixel(img, j, i, img->maxval);
            }
        }
    }
}

/// Brighten image by a factor.
/// Multiply each pixel level by a factor, but saturate at maxval.
/// This will brighten the image if factor>1.0 and
/// darken the image if factor<1.0.
void ImageBrighten(Image img, double factor) { ///
  assert (img != NULL);
   assert (factor >= 0.0);
  
    for (uint32_t i = 0; i < img->height; i++) {
        for (uint32_t j = 0; j < img->width; j++) {
            uint8_t pixel = ImageGetPixel(img, j, i);
            uint8_t brightenedPixel = (uint8_t)((double)pixel * factor);

            if (brightenedPixel > img->maxval) {
                brightenedPixel = img->maxval;
            }

            ImageSetPixel(img, j, i, brightenedPixel);
        }
    }
}


/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
/// 
/// Success and failure are treated as in ImageCreate:
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

// Implementation hint: 
// Call ImageCreate whenever you need a new image!

/// Rotate an image.
/// Returns a rotated version of the image.
/// The rotation is 90 degrees clockwise.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageRotate(Image img) { ///
  assert (img != NULL);
   int width =ImageWidth(img);
  int height = ImageHeight(img);

  Image rotated_img = ImageCreate(height, width, img->maxval);
  if (!rotated_img) return NULL; // error already set
   if(rotated_img= NULL){
      /*errno = ENOMEM;
	    errCause = "Falha ao alocar memória para a imagem resultante do crop\n";*/
      return NULL;
   }
   for (int i = 0; i < width; ++i) {
    for(int j = 0; j < height; ++j){
      int rotated_i = j;
      int rotated_j = width -1 -i;
      ImageSetPixel(rotated_img, rotated_i,rotated_j, ImageGetPixel(img, i, j));
    }
   }
   return rotated_img;
}

/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageMirror(Image img) { ///
  assert (img != NULL);
   Image mirrored_img = ImageCreate(img->width, img->height, img->maxval);
    if (mirrored_img == NULL) {
        errno = ENOMEM;
	      errCause = "Falha ao alocar memória para a imagem resultante do crop\n";
        return NULL;
    }
    for (uint32_t i = 0; i < img->height; i++) {
        for (uint32_t j= 0; j < img->width; j++) {

            // coordenadas img 1(i, j)
            img->pixel = ImageGetPixel(img, i, j);

            // mudar as coordenadas (j, i) na img espelhada para o pixel value da img original
            ImageSetPixel(mirrored_img, j, i, img->pixel);
        }
    }

    return mirrored_img;
}

/// Crop a rectangular subimage from img.
/// The rectangle is specified by the top left corner coords (x, y) and
/// width w and height h.
/// Requires:
///   The rectangle must be inside the original image.
/// Ensures:
///   The original img is not modified.
///   The returned image has width w and height h.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCrop(Image img, int x, int y, int w, int h) { ///
  assert (img != NULL);
  assert (ImageValidRect(img, x, y, w, h));
  // Insert your code here!
  Image ret = ImageCreate(w, h, img->maxval);
  if(ret == NULL) {
	  /*errno = ENOMEM;
	  errCause = "Falha ao alocar memória para a imagem resultante do crop\n";*/
	  return NULL;
  }
  
  //Copiar os píxeis da zona adequada de img para a nova imagem
  for(int ay = y; ay < y+h; ay++) {
	  for(int ax = x; ax < x+w; ax++) {
		  //~ ret->pixel[i++] = img->pixel[G(img, ax, ay)];
		  ImageSetPixel(ret, ax, ay, ImageGetPixel(img, ax, ay));
	  }
  }
  
  return ret;
}


/// Operations on two images

/// Paste an image into a larger image.
/// Paste img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
void ImagePaste(Image img1, int x, int y, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidRect(img1, x, y, img2->width, img2->height));
    for (int i = 0; i < img2->height; i++) {
        for (int j = 0; j < img2->width; j++) {

            // Get the pixel value from the second image at coordinates (i, j)
           uint8_t pixel = ImageGetPixel(img2, i, j);

            // Set the pixel value at coordinates (x + j, y + i) in the first image to the pixel value from the second image
            ImageSetPixel(img1, x + j, y + i, pixel);
        }
    }
}

/// Blend an image into a larger image.
/// Blend img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
/// alpha usually is in [0.0, 1.0], but values outside that interval
/// may provide interesting effects.  Over/underflows should saturate.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidRect(img1, x, y, img2->width, img2->height));
    //~ assert (ImageValidRect(img1, x, y, img2->width, img2->height));
  
    //~ // Iterate over each pixel in the second image
    //~ for (int i = 0; i < img2->height; i++) {
        //~ for (int j = 0; j < img2->width; j++) {

            //~ //coordenadas da segunda img(i, j)
            //~ uint8_t pixel2 = ImageGetPixel(img2, i, j);

            //~ //coordenadas da primeira img (x + j, y + i)
            //~ uint8_t pixel1 = ImageGetPixel(img1, x + j, y + i);
            //~ uint8_t blendedPixel = BlendPixels(pixel1, pixel2, alpha);
		
            //~ //mudar coordenadas da 1  imagem (x + j, y + i) para "blended pixel value"
            //~ ImageSetPixel(img1, x + j, y + i, blendedPixel);
        //~ }
    //~ }
}

/// Compare an image to a subimage of a larger image.
/// Returns 1 (true) if img2 matches subimage of img1 at pos (x, y).
/// Returns 0, otherwise.
int ImageMatchSubImage(Image img1, int x, int y, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidPos(img1, x, y));
  // Insert your code here!
  //Se a imagem 2 nem couber dentro da 1 na posição desejada, não é match
  if(!ImageValidRect(img1, x, y, img2->width, img2->height))
	return 0;
  
  //Criar a sub-imagem para comparação
  Image sub = ImageCrop(img1, x, y, img2->width, img2->height);
  
  //Comparar a imagem 2 com a sub-imagem
  int match = 1;
    for(int ay = y; ay < y+img2->height; ay++) {
	  for(int ax = x; ax < x+img1->width; ax++) {
		  if(ImageGetPixel(img2, ax, ay) != ImageGetPixel(sub, ax, ay)) {
			  match = 0;
			  break;
		  }
	  }
    }
  return match;
}

/// Locate a subimage inside another image.
/// Searches for img2 inside img1.
/// If a match is found, returns 1 and matching position is set in vars (*px, *py).
/// If no match is found, returns 0 and (*px, *py) are left untouched.
int ImageLocateSubImage(Image img1, int* px, int* py, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  // Insert your code here!
  //Se a imagem 2 for maior que a 1, não é match
  if((img2->width > img1->width) || (img2->height > img1->height)) {
	  return 0;
  }
  
  for(int y = 0; y <= img1->height-img2->height; y++) {
	  for(int x = 0; x <= img1->width-img2->width; x++) {
		  //Para cada píxel de img1, ver se a subimagem que nele começa e tem as dimensões de img2 é igual a img2
		  if(ImageMatchSubImage(img1, x, y, img2)) {
			  *px = x;
			  *py = y;
			  return 1;
		  }
	  }
  }
  
  return 0;
}


/// Filtering

/// Blur an image by a applying a (2dx+1)x(2dy+1) mean filter.
/// Each pixel is substituted by the mean of the pixels in the rectangle
/// [x-dx, x+dx]x[y-dy, y+dy].
/// The image is changed in-place.
//Tabela que para cada píxel (x, y) associa a soma dos valores de todos os píxeis do retângulo que vai de (0, 0)  a (x, y)
unsigned long int* build_summed_area_table(Image img) {
	//Pode produzir erros
	unsigned long int* table = (unsigned long int*)malloc(img->width*img->height*sizeof(unsigned long int));
	
	for(int y = 0; y < img->height; y++) {
		for(int x = 0; x < img->width; x++) {
			//Primeira linha ou coluna
			if((y == 0) || (x == 0)) {
				//Primeiro píxel
				if((y == 0) && (x == 0)) {
					table[G(img, x, y)] = ImageGetPixel(img, x, y);
				}
				//Primeira linha
				else if(y == 0) {
					table[G(img, x, 0)] = table[G(img, x-1, 0)] + ImageGetPixel(img, x, 0);
				}
				//Primeira coluna
				else {
					table[G(img, 0, y)] = table[G(img, 0, y-1)] + ImageGetPixel(img, 0, y);
				}
			}
			//Demais píxeis
			else {
				table[G(img, x, y)] = table[G(img, x-1, y)] + table[G(img, x, y-1)] - table[G(img, x-1, y-1)] + ImageGetPixel(img, x, y);
			}
		}
	}
	
	return table;
}

void ImageBlur_naive_sem_borda(Image img, int dx, int dy) { ///
  // Insert your code here!
  //Clonar imagem
  Image img_cpy = ImageCrop(img, 0, 0, img->width, img->height);
  
  int soma;
  float valor;
  for(int y = dy; y < img->height-dy; y++) {
	  for(int x = dx; x < img->width-dx; x++) {
		  //Para cada pixel da imagem (somente os que causam a janela a ficar dentro da imagem)
		  soma = 0;
		  for(int i = y-dy; i <= y+dy; i++) {
			  for(int j = x-dx; j <= x+dx; j++) {
				  //Somar os valores dos pixeis da janela
				  soma += ImageGetPixel(img_cpy, j, i);
			  }
		  }
		  //Dividir pelo número de píxeis da janela
		  valor = (float)soma / ((2*dx+1)*(2*dy+1));
		  ImageSetPixel(img, x, y, (uint8)arred(valor));
	  }
  }
  
  ImageDestroy(&img_cpy);
}

void ImageBlur_opt(Image img, int dx, int dy) {
  int window_width = 2*dx+1;
  int window_height = 2*dy+1;
  int window_area = window_width*window_height;
  
  //Para cada tamanho possível da janela, um fator multiplicativo diferente (está assim para não dividir dentro do ciclo)
  float fator[window_area];
  for(int i = 0; i < window_area; fator[i++] = 1.0/(i+1));
  
  float valor;
  unsigned long int* summed_area_table = build_summed_area_table(img);
  
  //~ for(int i = 0; i < img->height; i++) {
	//~ for(int j = 0; j < img->width; j++) {
		//~ printf("%ld ", summed_area_table[G(img, j, i)]);
	//~ }
	//~ printf("\n");
  //~ }
  
  for(int y = 0; y < img->height; y++) {
	  //Limites da janela
	  int yt = (y-dy) < 0 ? 0:(y-dy);
	  int yb = (y+dy) > (img->height-1) ? (img->height-1):(y+dy);
	  int cw_height = yb-yt+1;
	  
	  for(int x = 0; x < img->width; x++) {
		  //Para cada píxel da imagem
		  //Limites da janela (interseção da janela (dx x dy) com a imagem)
		  int xl = (x-dx) < 0 ? 0:(x-dx);
		  int xr = (x+dx) > (img->width-1) ? (img->width-1):(x+dx);
		  int cw_width = xr-xl+1;
		  
		  //Fator para esta janela (interseção da janela (dx x dy) com a imagem)
		  int cw_area = cw_width*cw_height;
		  float cw_fator = fator[cw_area-1];
		  
		  //Cálculo da soma dos valores dos píxeis da interseção da janela com a imagem
		  //Interseção da janela com a imagem toca a margem esquerda ou superior da imagem
		  if((xl == 0) || (yt == 0)) {
			  //Interseção da janela com a imagem toca o canto superior esquerdo da imagem
			  if((xl == 0) && (yt == 0)) {
				  valor = summed_area_table[G(img, xr, yb)];
			  }
			  //Interseção da janela com a imagem toca a margem superior, mas não a esquerda da imagem
			  else if(xl > 0) {
				  valor = summed_area_table[G(img, xr, yb)] - summed_area_table[G(img, xl-1, yb)];
			  }
			  //Interseção da janela com a imagem toca a margem esquerda, mas não a superior da imagem
			  else {
				  valor = summed_area_table[G(img, xr, yb)] - summed_area_table[G(img, xr, yt-1)];
			  }
		  }
		  //Janela não toca a margem esquerda nem superior da imagem
		  else {
			  valor = summed_area_table[G(img, xr, yb)] - summed_area_table[G(img, xl-1, yb)] - summed_area_table[G(img, xr, yt-1)] + summed_area_table[G(img, xl-1, yt-1)];
		  }
		  
		  //Média dos valores somados
		  ImageSetPixel(img, x, y, (uint8)arred(valor*cw_fator));
		  
		  //~ printf("(%d, %d):\n %ld\t%ld\n%ld\t%ld\n\n%f -> %d\n\n\n", x, y, summed_area_table[G(img, x-dx-1, y-dy-1)], summed_area_table[G(img, x+dx, y-dy-1)], summed_area_table[G(img, x-dx-1, y+dy)], summed_area_table[G(img, x+dx, y+dy)], valor, (uint8)arred(valor*cw_fator));
		  //~ printf("(%d, %d):\n%f -> %d\t(fator = %f)\n\n\n", x, y, valor, (uint8)arred(valor*cw_fator), cw_fator);
		  
	  }
  }

  unsigned long int** p = &summed_area_table;
  free(*p);
  *p = NULL;
	
	
  //~ long int* table_cache = (long int*)malloc(img->width*img->height*sizeof(long int));
  //~ for(int i = 0; i < img->height; i++) {
	  //~ for(int j = 0; j < img->width; j++) {
		  //~ table_cache[G(img, j, i)] = -1;
	  //~ }
  //~ }
  
  //~ summed_area(img, table_cache, img->width-1, img->height-1);
  //~ for(int i = 0; i < img->height; i++) {
	  //~ for(int j = 0; j < img->width; j++) {
		  //~ printf("%ld ", table_cache[G(img, j, i)]);
	  //~ }
	  //~ printf("\n");
  //~ }
  
  //~ long int** p = &table_cache;
  //~ free(*p);
  //~ *p = NULL
  
}

void ImageBlur(Image img, int dx, int dy) { ///
  // Insert your code here!
  //~ ImageBlur_naive_sem_borda(img, dx, dy);
  ImageBlur_opt(img, dx, dy);
  
}

