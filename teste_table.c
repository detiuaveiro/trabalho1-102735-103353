#include <stdio.h>

//~ long int summed_area(Image img, long int* table_cache, int x, int y) {
  //~ if(table_cache[G(img, x, y)] != -1) {
	  //~ return table_cache[G(img, x, y)];
  //~ }
  
  //~ long int ret = summed_area(img, table_cache, x-1, y) + summed_area(img, table_cache, x, y-1) - summed_area(img, table_cache, x-1, y-1) + ImageGetPixel(img, x, y);
  //~ table_cache[G(img, x, y)] = ret;
  //~ return ret;
//~ }

int main(void) {
  int window_area = 9;
  
  float fator[window_area];
  for(int i = 0; i < window_area; fator[i++] = 1.0/(i+1));
  
  for(int i = 0; i < window_area; printf("%f\n", fator[i++]));
  
  return 0;
}
