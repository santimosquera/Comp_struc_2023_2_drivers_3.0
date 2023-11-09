/*
 * ring_buffer.h
 *
 *  Created on: Oct 26, 2023
 *      Author: santi
 */

#ifndef INC_RING_BUFFER_H_
#define INC_RING_BUFFER_H_

#include <stdint.h>

typedef struct {
	uint8_t *buffer;	//the * means is a pointer, to memory location
	uint16_t head;		//to write new data
	uint16_t tail;		//to read old data

	uint16_t capacity;
	uint8_t is_full;

} ring_buffer_t; 		// the "_t" refers to type

void ring_buffer_init(ring_buffer_t *ring_buffer, uint8_t *buffer, uint16_t capacity);
uint8_t ring_buffer_put(ring_buffer_t *ring_buffer,uint8_t data);
uint8_t ring_buffer_get (ring_buffer_t *ring_buffer, uint8_t*data);
uint16_t ring_buffer_size(ring_buffer_t *ring_buffer);

uint8_t ring_buffer_is_empty(ring_buffer_t *ring_buffer);

uint8_t ring_buffer_is_full(ring_buffer_t *ring_buffer);

void ring_buffer_reset(ring_buffer_t *ring_buffer);
#endif

