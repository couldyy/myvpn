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

#elif defined(USE_U16)
#define elem_t uint16_t

#elif defined(USE_U32)
#define elem_t uint32_t

#elif defined(USE_U64)
#define elem_t uint64_t

#else
#define elem_t uintptr_t
#endif


#define BITS_IN_BYTE 8

#define ELEMENT_SIZE (sizeof(elem_t) * BITS_IN_BYTE)

#define ELEM_T_MAX_VALUE ((1 << (ELEMENT_SIZE-1)) + (ELEMENT_SIZE - 1))

typedef struct {
    elem_t *items;
    size_t size;
    size_t bitcount;
} Bit_array;

void print_bitarray(Bit_array* bit_array);

// allocates array of 'bit_count' bits (bitcount / sizeof(elem_t) bytes) and initializes array with 'init_value'
// on success returns 0, on error -1
int init_bit_array(Bit_array* bit_array, size_t bit_count, uint8_t init_value);

// On success returns 'Bit_array' structure
// on failure - aborts
Bit_array init_bit_array_stack(size_t bit_count, uint8_t init_value);

// On success returns address to 'Bit_array' structure
// on failure - NULL
Bit_array* init_bit_array_heap(size_t bit_count, uint8_t init_value);

// sets bit at 'index' to 1st bit of 'value'
// on success return updated value of bit, on error -1
int set_bit(Bit_array* bit_array, size_t index, elem_t value);

// toggles bit at 'index' 
// on success 0 is returned, on error abort() 
int toggle_bit(Bit_array* bit_array, size_t index);

// gets bit value at 'index' 
// on success bit value is returned, on error abort() 
int get_bit(Bit_array* bit_array, size_t index);

// TODO implement cache (maybe just store 'last_found' bit or byte and start iterating from it (should also % index so
//      that iteration would loop, and stop either when empty elemnt was found or reached starting position
//      which would lead to a shitty performance on full or nearly full array, but for that case can set some flags in 
//      'Bit_array' structure

// searches 'bit_array' for 1st bit of 'needle'
// on succes returns index of first found bit, on error -1
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
            return -1;
            //abort();  //TODO maybe just abort
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

// On success returns address to 'Bit_array' structure
// on failure - NULL
Bit_array* init_bit_array_heap(size_t bit_count, uint8_t init_value)
{
    Bit_array* bit_array = malloc(sizeof(Bit_array));
    if(init_bit_array(bit_array, bit_count, init_value) < 0)
        return NULL;
    return bit_array;
}

// On success returns 'Bit_array' structure
// on failure - aborts
Bit_array init_bit_array_stack(size_t bit_count, uint8_t init_value)
{
    Bit_array bit_array;
    if(init_bit_array(&bit_array, bit_count, init_value) < 0)
        abort();
    return bit_array;
}

// allocates array of 'bit_count' aligned bits and initializes 'bit_count' bits with 1st bit of 'init_value'
int init_bit_array(Bit_array* bit_array, size_t bit_count, uint8_t init_value)
{
    assert(bit_array != NULL);
    assert(bit_count > 0);
    
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

