/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Rob Fowler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _INCLUDED_ESP_1WIRE_H_
#define _INCLUDED_ESP_1WIRE_H_

#define CONVERT_T 0x44
#define READ_SCRATCH_PAD 0xBE
#define WRITE_SCRATCH_PAD 0x4E
#define COPY_SCRATCH_PAD 0x48
#define RECALL_E  0xB8
#define READ_POWER_SUPPLY 0xB4
#define READ_ROM 0x33
#define MATCH_ROM 0x55
#define SEARCH_ROM 0xF0
#define SKIP_ROM 0xCC

uint8_t ds_reset(pmap_t *pmp);
pmap_t  *ds_init(int gpio);
void ds_write(pmap_t *pmp, uint8_t v, int power);
uint8_t ds_read(pmap_t *pmp);
#endif // _INCLUDED_ESP_1WIRE_H_
