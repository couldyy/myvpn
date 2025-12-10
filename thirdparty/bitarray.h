#ifndef BITARRAY_H
#define BITARRAY_H
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#if defined(USE_U8)
#define elem_t uint8_t

#elif defined(USE_U32)
#define elem_t uint32_t

#else
#define elem_t uint64_t
#endif

#define BITS_IN_BYTE 8

#define ELEMENT_SIZE (sizeof(elem_t) * BITS_IN_BYTE)

typedef struct {
    elem_t *items;
    size_t size;
    size_t bitcount;
} Bit_array;

void print_bitarray(Bit_array* bit_array);
int init_bit_array(Bit_array* bit_array, size_t bit_count, uint8_t init_value);
int set_bit(Bit_array* bit_array, size_t index, elem_t value);
int toggle_bit(Bit_array* bit_array, size_t index);
int get_bit(Bit_array* bit_array, size_t index);
ssize_t find_first_bit(Bit_array* bit_array, uint8_t needle);

#endif // BITARRAY_H

#ifdef BITARRAY_IMPLEMENTATION

// TODO return type mismatch with 'bitcount' field, but need to somehow indicate error
ssize_t find_first_bit(Bit_array* bit_array, uint8_t needle)
{
    assert(bit_array != NULL);
    elem_t needle_norm = needle & 1;
    //printf("needle %d\n", needle_norm);
    const elem_t max = -1;        // since elem_t is unsigned, -1 gives all '1s' in number;
    size_t arr_size = bit_array->size;

    for(size_t i = 0; i < arr_size; i++) {
        elem_t current_elem = bit_array->items[i];
        //printf("%u ", current_elem);
        switch(current_elem) {
            case 0:
                switch(needle_norm) {
                    case 0:
                        return i * ELEMENT_SIZE;
                        break;
                    case 1:
                        continue;
                        break;
                    default:
                        fprintf(stderr, "invalid 'needle': %d\n", needle_norm);
                        exit(1);
                }
                break;
            case ((elem_t)-1):
                switch(needle_norm) {
                    case 0:
                        continue;
                        break;
                    case 1:
                        return i * ELEMENT_SIZE;
                        break;
                    default:
                        fprintf(stderr, "invalid 'needle': %d\n", needle_norm);
                        exit(1);
                }
                break;
            default:
                for(size_t j = 0; j < ELEMENT_SIZE; j++) {
                    if(((current_elem >> j) & 1) == needle_norm) {
                        return i*ELEMENT_SIZE+j;
                    }
                }
                break;
        }
    }
    return -1;
}

void print_bitarray(Bit_array* bit_array)
{
    assert(bit_array != NULL);
    for(size_t i = 0; i < bit_array->bitcount; i++) {
        printf("%d%s", get_bit(bit_array, i), (((i+1) % (sizeof(elem_t) * 8) == 0)) ? " " : "");
    }
    printf("\n");
}

int set_bit(Bit_array* bit_array, size_t index, elem_t value)
{
    assert(bit_array != NULL);
    assert(index < bit_array->bitcount);
    elem_t norm_value = value & 1;

    size_t elem = index / ELEMENT_SIZE;
    elem_t bit = index % ELEMENT_SIZE;
    elem_t mask = 1 << bit;
    switch(norm_value) {
        case 0:
            bit_array->items[elem] &= ~mask; 
            break;
        case 1:
            bit_array->items[elem] |= mask; 
            break;
        default:
            fprintf(stderr, "Invalid 'value' : %d\n", norm_value);
            exit(1);
            break;
    }
    return norm_value; 
}

int toggle_bit(Bit_array* bit_array, size_t index)
{
    assert(bit_array != NULL);
    assert(index < bit_array->bitcount);
    size_t elem = index / ELEMENT_SIZE;
    elem_t bit = index % ELEMENT_SIZE;
    elem_t mask = 1 << bit;

    bit_array->items[elem] ^= mask; 

    //return (bit_array->[elem] >> bit) & 1;    // return flipped bit (probably bad on performace)
    return 0;
}

int get_bit(Bit_array* bit_array, size_t index)
{
    assert(bit_array != NULL);
    assert(index < bit_array->bitcount);
    size_t elem = index / ELEMENT_SIZE;
    elem_t bit = index % ELEMENT_SIZE;
    return ((bit_array->items[elem]) >> bit) & 1;
}

// allocates array of 'bit_count' aligned bits and initializes 'bit_count' bits with 1st bit of 'init_value'
int init_bit_array(Bit_array* bit_array, size_t bit_count, uint8_t init_value)
{
    assert(bit_array != NULL && bit_count > 0);
//    if(bit_array == NULL || bit_count < 1) 
//        return -1;

    elem_t last_byte_bits = bit_count % ELEMENT_SIZE;
    size_t arr_size = bit_count / ELEMENT_SIZE + (last_byte_bits == 0 ? 0 : 1);
    bit_array->items = malloc(sizeof(elem_t) * arr_size);
    if(bit_array->items == NULL) {
        fprintf(stderr, "malloc() failed: %s\n", strerror(errno));
        return -1;
    }
    bit_array->size = arr_size;
    bit_array->bitcount = bit_count;
    // init whole array
    if(init_value & 1) { // if init with 1
        for(int i = 0; i < arr_size; i++) {
            bit_array->items[i] = (elem_t)-1;   //since elem_t should be unsigned, -1 gives all 1s
        }
    }
    else {
        memset(bit_array->items, init_value, arr_size);
    }
   
    if(last_byte_bits != 0) {
        elem_t bits_mask = (1 << last_byte_bits) - 1;
        // if init with '1'
        if(init_value & 1) {
            // becuase last elem is already initialized with memset, we need to AND with bits_mask to remove unused bits
            bit_array->items[arr_size - 1] &= bits_mask; 
        }
        else {  //if init with '0'
            // becuase last elem is already initialized with memset, we need to AND with bits_mask to remove unused bits
            bit_array->items[arr_size - 1] &= (~bits_mask); 
        }
    }

    return 0;

}
#endif //BITARRAY_IMPLEMENTATION

