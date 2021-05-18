
#include <windows.h>
#include "SPAD-correct.h"
#include "SPAD-correct_internal.h"


void SPAD_bin_by_2(USHORT* histogram, int width, int height, int timebins, int* new_width, int* new_height)
{
	int w = width / 2;
	int h = height / 2;

	for (int y = 0; y < h; y++) {
		// rows (col start pts)
		USHORT* row_ptr = histogram + 2 * y * width * timebins;    // row to get data from
		USHORT* row_ptr_dn = row_ptr + width * timebins;           // and the row down from that

		USHORT* row_ptr_binned = histogram + y * w * timebins;     // row to put data into

		for (int x = 0; x < w; x++) {
			// pixels (transient start pts)
			USHORT* tran1_ptr = row_ptr + 2 * x * timebins;
			USHORT* tran2_ptr = tran1_ptr + timebins;
			USHORT* tran3_ptr = row_ptr_dn + 2 * x * timebins;
			USHORT* tran4_ptr = tran3_ptr + timebins;

			USHORT* tran_ptr_binned = row_ptr_binned + x * timebins;

			for (int t = 0; t < timebins; t++) {

				*tran_ptr_binned = *tran1_ptr + *tran2_ptr + *tran3_ptr + *tran4_ptr;

				// Move all ptrs aong 1 since we are not binning in time! phew!
				tran1_ptr++;
				tran2_ptr++;
				tran3_ptr++;
				tran4_ptr++;
				tran_ptr_binned++;
			}
		}
	}

	if (new_width != NULL) *new_width = w;
	if (new_height != NULL) *new_height = h;
}

void SPAD_bin(USHORT* histogram, int width, int height, int timebins, int bin_size, int* new_width, int* new_height)
{
	int w = width;
	int	h = height;

	while (bin_size > 1) {
		SPAD_bin_by_2(histogram, width, height, timebins, &w, &h);
		width = w;
		height = h;
		bin_size = bin_size / 2;
	}

	if (new_width != NULL) *new_width = w;
	if (new_height != NULL) *new_height = h;

}
