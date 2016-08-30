/*
 * Copyright © 2005-2007 Jens Gulden
 * Copyright © 2011-2011 Diego Elio Pettenò
 * Copyright © 2016 Jerome Flesch
 *
 * Pypillowfight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * Pypillowfight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <values.h>

#include <Python.h>

#include "util.h"

/*!
 * \brief Algorithm 'grayfilter' from unpaper, partially rewritten.
 */

#define SCAN_SIZE 50
#define SCAN_STEP 20
#define THRESHOLD 0.5
#define BLACK_THRESHOLD 0.33

#define BLACK_MAX (WHITE * (1.0 - BLACK_THRESHOLD))
#define THRESHOLD_ABS (WHITE * THRESHOLD)

/**
 * Returns the average lightness of a rectangular area.
 */
static int lightness_rect(int x1, int y1, int x2, int y2, const struct bitmap *img) {
	int x;
	int y;
	int total = 0;
	const int count = (x2 - x1 + 1) * (y2 - y1 + 1);

	for (x = x1; x < x2; x++) {
		for (y = y1; y < y2; y++) {
			total += GET_PIXEL_LIGHTNESS(img, x, y);
		}
	}
	return total / count;
}

static void grayfilter_main(const struct bitmap *in, struct bitmap *out)
{
	int left;
	int top;
	int right;
	int bottom;
	int count;
	int lightness;

	memcpy(out->pixels, in->pixels, sizeof(union pixel) * in->size.x * in->size.y);

	left = 0;
	top = 0;
	right = SCAN_SIZE - 1;
	bottom = SCAN_SIZE - 1;

	while (1) {
		count = count_pixels_rect(left, top, right, bottom, BLACK_MAX, out);
		if (count == 0) {
			lightness = lightness_rect(left, top, right, bottom, out);
			if ((WHITE - lightness) < THRESHOLD_ABS) {
				clear_rect(out, left, top, right, bottom);
			}
		}
		if (left < out->size.x) {
			left += SCAN_STEP;
			right += SCAN_STEP;
		} else { // end of row
			if (bottom >= out->size.y) // has been last row
				return;
			// next row
			left = 0;
			right = SCAN_SIZE - 1;
			top += SCAN_STEP;
			bottom += SCAN_STEP;
		}
	}
}

static PyObject *grayfilter(PyObject *self, PyObject* args)
{
	int img_x, img_y;
	Py_buffer img_in, img_out;
	struct bitmap bitmap_in;
	struct bitmap bitmap_out;

	if (!PyArg_ParseTuple(args, "iiy*y*",
				&img_x, &img_y,
				&img_in,
				&img_out)) {
		return NULL;
	}

	assert(img_x * img_y * 4 /* RGBA */ == img_in.len);
	assert(img_in.len == img_out.len);

	bitmap_in = from_py_buffer(&img_in, img_x, img_y);
	bitmap_out = from_py_buffer(&img_out, img_x, img_y);

	memset(bitmap_out.pixels, 0xFFFFFFFF, img_out.len);
	grayfilter_main(&bitmap_in, &bitmap_out);

	PyBuffer_Release(&img_in);
	PyBuffer_Release(&img_out);
	Py_RETURN_NONE;
}

static PyMethodDef grayfilter_methods[] = {
	{"grayfilter", grayfilter, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL},
};

#if PY_VERSION_HEX < 0x03000000

PyMODINIT_FUNC
init_grayfilter(void)
{
    PyObject* m = Py_InitModule("_grayfilter", grayfilter_methods);
}

#else

static struct PyModuleDef grayfilter_module = {
	PyModuleDef_HEAD_INIT,
	"_grayfilter",
	NULL /* doc */,
	-1,
	grayfilter_methods,
};

PyMODINIT_FUNC PyInit__grayfilter(void)
{
	return PyModule_Create(&grayfilter_module);
}

#endif