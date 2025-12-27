/* TODO list
 * [ ] Make key unique
 * [x] ht_remove()
 * [ ] test ht_remove()
 * [x] ht_update()
 * [ ] test ht_update()
 * [ ] test ht_update(), specifically its behaivor with size, if size have changed or not etc...
 * [ ] ht_clear()
 * [x] define grow and shrink factors
 * [ ] test hash table with     k: int      v: int
 * [ ] test hash table with     k: float    v: int
 * [ ] test hash table with     k: int      v: float
 * [ ] test hash table with     k: int      v: str
 * [ ] test hash table with     k: int      v: ptr -> str
 * [ ] test hash table with     k: ptr->str v: *
 * [ ] test hash table with     k: *        v: struct
 * [ ] test hash table with     k: *        v: ptr -> struct
 * [ ] test hash table with     k: *        v: ptr
 * [ ] test hash table with     k: struct   v: *
 * [ ] test hash table with     k: ptr      v: *
 * [ ] test hash table with     k: ptr      v: ptr
 * [ ] TEST TEST TEST!...
 *
 */
#ifndef _CASH_TABLE_H
#define _CASH_TABLE_H

//#define CASH_TABLE_DEBUG

#define HT_DONT_GROW 0x1
#define HT_DONT_SHRINK 0x2
#define HT_STATIC (HT_DONT_GROW | HT_DONT_SHRINK)
#define HT_DYNAMIC 0x0


#define DEFAULT_CAPACITY BUFSIZ
#define TABLE_GROW_FACTOR 1.6f
#define TABLE_SHRINK_FACTOR 0.5f

#define GROW_LOAD_FACTOR 0.6f
#define SHRINK_LOAD_FACTOR 0.15f

#define HT_PTR_IS_DATA 0

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

//#define _CASH_TABLE_IMPLEMENTATION

typedef struct {
    void* key;
    size_t key_size;        // if `key_size` == 0 (HT_PTR_IS_DATA flag), ht will treat `key` as DATA ITSELF, 
                            // otherwise it will treat `key` as PTR TO ACTUAL DATA
    void* value;
    size_t value_size;      // if `value_size` == 0 (HT_PTR_IS_DATA flag), ht will treat `value` as DATA ITSELF, 
                            // otherwise it will treat `value` as PTR TO ACTUAL DATA
    bool is_occupied;
    bool is_deleted;        // tombstone if is_deleted == 1
} HashData;

typedef struct {
    size_t capacity;
    int count;

	int flags;
    HashData* hash_data;

    uint64_t (*hash_func) (const void* key);
    int (*cmp_func) (const void* a, const void* b);
} HashTable;



#ifdef CASH_TABLE_DEBUG
#warning "DEBUG enabled"
void d_print_ht_info(HashTable* ht);
#define DEBUG_PRINT_HT_INFO(ht_ptr) d_print_ht_info
#else 
#define DEBUG_PRINT_HT_INFO(ht_ptr)		// NOP
#endif

HashTable* init_ht_heap(int capacity, int flags, uint64_t (*hash_func) (const void* key), int (*compare_func) (const void* a, const void* b));
HashTable init_ht_stack(int capacity, int flags, uint64_t (*hash_func) (const void* key), int (*compare_func) (const void* a, const void* b));

bool clear_ht(HashTable* ht);

bool ht_add(HashTable* ht, void* key, size_t key_size, void* value, size_t value_size);
bool ht_add_kdata_vptr(HashTable* ht, void* key, size_t key_size, void* value);
bool ht_add_kptr_vdata(HashTable* ht, void* key, void* value, size_t value_size);
bool ht_add_kptr_vptr(HashTable* ht, void* key, void* value);

bool ht_update(HashTable* ht, void* key, void* new_value, size_t new_value_size);
bool ht_update_ptr(HashTable* ht, void* key, void* new_value);

void* ht_find(HashTable* ht, void* key);
bool ht_remove(HashTable* ht, void* key);

HashData* get_ht_array(HashTable* ht);

static void ht_n_cpy(HashTable* ht, HashData* dst, HashData* src, size_t n, size_t new_size);
uint64_t default_hash(const void *key);
int default_compare_func(const void* a, const void* b);

static void clear_ht_data(HashTable* ht, int64_t capacity);

#endif  //_CASH_TABLE_H

#ifdef CASH_TABLE_IMPLEMENTATION
//#undef _CASH_TABLE_IMPLEMENTATION

#define ht_strlen(str) (strlen(str) + 1)

#define WRITE_ELEMENT(arr_ptr, index, elem_size, element) \
    memcpy((void*)((char*)arr_ptr + index * elem_size),   \
            element, elem_size);                          \

#define GET_ELEM_ADR(arr_ptr, index, elem_size)     (void*)((char*)arr_ptr + index * elem_size)


// djb2 hash alorithm
uint64_t default_hash(const void *key)
{
    uint64_t hash = 5381;
    int32_t c;
    char* c_key = (char*)key;
    while((c = *(c_key)++))
        hash = (hash << 5) + hash + c; // hash * 33 + c
    c_key = NULL;

    return hash;
}

int default_compare_func(const void* a, const void* b)
{
#ifdef CASH_TABLE_DEBUG
    printf("d_ a: %p\tb : %p\n", a, b);
#endif
    if(a == NULL && b == NULL)
        return 0;
    else if(a == NULL)
        return strcmp("", (char*)b);
    else if(b == NULL)
        return strcmp((char*)a, "");
#ifdef CASH_TABLE_DEBUG
    printf("d_ *a = %s,\t*b = %s\n\n", (char*)a, (char*)b);
#endif
    return strcmp((char*)a, (char*)b);
}

// readds all elemets, the way it is added in ht_add() function (using hash)
//#define ht_n_cpy(ht_ptr, dst_keys_ptr, dst_values_ptr, key_type, elem_type, n, new_size) 
static void ht_n_cpy(HashTable* ht, HashData* dst, HashData* src, size_t n, size_t new_size)
{ 
    //assert("Keys or values array is not initialized" && dst_keys_ptr != NULL && dst_values != NULL %% ht_ptr != NULL) 
    if(src == NULL || dst == NULL) {
        fprintf(stderr, "ht_n_cpy(): src or dst arrays must not be NULL\n");
        abort();
    }

    for(int i = 0; i < n; i++) { 
       HashData current_elem = src[i];
       // iterate until not empy cell in array is found
       if(current_elem.is_occupied == 1) { 
            // then hash the key
            uint64_t index = ht->hash_func(current_elem.key) % new_size; 
            // loop through the list, until ther is an empty cell  
            while(dst[index].is_occupied == 1)
                index = (index + 1) % new_size;  

            // copy elements of HashData struct
            dst[index].key = current_elem.key;
            dst[index].key_size = current_elem.key_size;
            dst[index].value = current_elem.value;
            dst[index].value_size = current_elem.value_size;
            dst[index].is_occupied = 1;
            dst[index].is_deleted = 0;      // clears all the tomstones
       } 
    } 
}
// shallow clean, actual data int `key` and `value` remains untouched, only being reassigned
static void clear_ht_data(HashTable* ht, int64_t capacity)
{
	assert(ht != NULL && "`ht` must not be NULL");
    assert("HashTable data capacity must be > 0" && capacity > 0);
    HashData* ht_arr = ht->hash_data;
    free(ht_arr);
}

HashData* get_ht_array(HashTable* ht)
{
	assert(ht != NULL && "`ht` must not be NULL");
    return ht->hash_data;
}

// COPIES data specified by `value` ptr into HashData->value field, calculating index by hash(key)
bool ht_add(HashTable* ht, void* key, size_t key_size, void* value, size_t value_size)
{ 
	assert(ht != NULL && "`ht` must not be NULL");
	if (ht->flags & HT_DONT_GROW) {
		if(ht->count+1 >= ht->capacity) {
			fprintf(stderr, "Hash table (%p) overflow\n", ht);
            DEBUG_PRINT_HT_INFO(ht);        
			exit(1);	// TODO maybe just return false, dont exit
		}
	} 
	else {
		// grow array if {GROW_LOAD_FACTOR}% of the table is occupied
		// or if count+1 >= capacity -1 (just to be sure nothing goes wrong)
		if(ht->count+1 >= ht->capacity - 1 || 
				((float)(ht->count+1) / (float)ht->capacity >= GROW_LOAD_FACTOR)) { 
			size_t new_capacity = ht->capacity * TABLE_GROW_FACTOR; 
			void* temp_data_arr = malloc(sizeof(HashData) * new_capacity); 
			memset(temp_data_arr, 0, sizeof(HashData) * new_capacity);

			/* readd all elements */ 
			ht_n_cpy(ht, temp_data_arr, ht->hash_data, ht->capacity, new_capacity); 

			clear_ht_data(ht, ht->capacity);

			ht->hash_data = temp_data_arr; 
			temp_data_arr = NULL;

			ht->capacity = new_capacity; 
		} 
	}
    uint64_t index = ht->hash_func(key) % ht->capacity;  
    // search until an empty element is found
    while(ht->hash_data[index].is_occupied != 0) {
        if(ht->cmp_func(ht->hash_data[index].key, key) == 0) {    // if same key is already in the hash table, just return false
                                                                  // TODO maybe just call ht_update()
            return false;
        }
        index = (index + 1) % ht->capacity;
    }

    HashData* current_element = &ht->hash_data[index];

    // just write a pointer if the size is HT_PTR_IS_DATA
    if(key_size == HT_PTR_IS_DATA) {
        current_element->key = key;
    } else {    // otherwise allocate memory with `size` and copy `key` into HashData->key
        current_element->key = malloc(key_size);
        if(current_element->key == NULL) {
            fprintf(stderr, "ht_add(): malloc failed\n");
            abort();
        }
        memcpy(current_element->key, key, key_size);
    }
    current_element->key_size = key_size;

    // just write a pointer if the size is HT_PTR_IS_DATA
    if(value_size == HT_PTR_IS_DATA) {
        current_element->value = value;
    } else {    // otherwise allocate memory with `size` and copy `value` into HashData->value
        current_element->value = malloc(value_size);
        if(current_element->value == NULL) {
            fprintf(stderr, "ht_add(): malloc failed\n");
            abort();
        }
        memcpy(current_element->value, value, value_size);
    }
    current_element->value_size = value_size;

    current_element->is_occupied = 1;
    current_element->is_deleted = 0;

    ht->count += 1;
    return true;
} 

// adds value POINTER to an `value` field WITHOUT copying anything, calculating index by hash(key)
// useful when you need to store a pointer itself, not the data in it
//
// key: pointer to actual data      value: pointer is a DATA ITSELF
bool ht_add_kdata_vptr(HashTable* ht, void* key, size_t key_size, void* value)
{ 
    return ht_add(ht, key, key_size, value, HT_PTR_IS_DATA);
} 

// key: pointer is a DATA ITSELF    value: pointer to an actual data
bool ht_add_kptr_vdata(HashTable* ht, void* key, void* value, size_t value_size)
{
    return ht_add(ht, key, HT_PTR_IS_DATA, value, value_size);
}

// key: pointer is DATA ITSELF      value: pointer is DATA ITSELF
bool ht_add_kptr_vptr(HashTable* ht, void* key, void* value)
{
    return ht_add(ht, key, HT_PTR_IS_DATA, value, HT_PTR_IS_DATA);
}

//  Writes new value to a value filed of a struct with a specified key , calculating index by hash(key)
//  if the `new_value_size` == 0 (HT_PTR_IS_DATA flag), `key` treated as a pointer that needs to be written itself, not the data in it,
//  meaning memory will not be allocated to a value field
//  and nothing will be copied from `key`. It will be just assigned HashData->value = new_value
//
//  On succes ht_update() will return true
//  On failure, if an element with specified key was not found, ht_update() will return false
bool ht_update(HashTable* ht, void* key, void* new_value, size_t new_value_size)
{ 

	assert(ht != NULL && "`ht` must not be NULL");
    uint64_t index = ht->hash_func(key) % ht->capacity;
    uint64_t start_index = index;

    HashData *needle = &ht->hash_data[index];

    // loop on the array while current element is either empty or not equal to the passed key
    while(needle->is_occupied == 0 || ht->cmp_func(needle->key, key) != 0) {
        index = (index + 1) % ht->capacity;
        // this will idicate that search completed the whole loop in the array,
        // which means no element was found
        if((needle->is_occupied == 0 && needle->is_deleted == 0) || index == start_index)
            return false;
        needle = &ht->hash_data[index];
    }


    // if passed new_value_size is 0 (HT_PTR_IS_DATA flag), `new_value` pointer is a data itself, so in that case, just assign it 
    // to value field
    // However if it is not 0, `new_value` is actual value, so allocate new memory and copy data into it
    if (new_value_size == HT_PTR_IS_DATA) {
        // size 0 (HT_PTR_IS_DATA flag) indicates that it is a pointer
        if(needle->value_size != HT_PTR_IS_DATA) {
            free(needle->value);
        }
        needle->value = new_value;
    }
    // if size didnt change, no need to allocate memory again
    // just memset() to 0 and copy new value in here
    else if(new_value_size == needle->value_size) {
            memset(needle->value, 0, needle->value_size);
            memcpy(needle->value, new_value, new_value_size);
        }   
    else {  //but if size have changed, then free existing memory if it is not a pointer, allocate new memory  and copy into it

        // size 0 (HT_PTR_IS_DATA flag) indicates that it is a pointer
        if(needle->value_size != HT_PTR_IS_DATA) {
            free(needle->value);
        }
        needle->value = malloc(new_value_size);
        if(needle->value == NULL) {
            fprintf(stderr, "ht_update(): malloc failed\n");
            abort();
        }
        memcpy(needle->value, new_value, new_value_size);
    }
    
    needle->value_size = new_value_size;
    needle->is_occupied = 1;
    needle->is_deleted = 0;

    return true;
} 

// Assignes a pointer to a `value` field of a HashData struct
//
//  On succes ht_update_ptr() will return true
//  On failure, if an element with specified key was not found, ht_update_ptr() will return false
bool ht_update_ptr(HashTable* ht, void* key, void* new_value)
{
    // just calls ht_update with size 0, see ht_update() description 
    return ht_update(ht, key, new_value, HT_PTR_IS_DATA);
}

void* ht_find(HashTable* ht, void* key)
{
	assert(ht != NULL && "`ht` must not be NULL");
    uint64_t index = ht->hash_func(key) % ht->capacity;
    uint64_t start_index = index;

    HashData *temp = &(ht->hash_data)[index];

    // loop on the array while current element is either empty or not equal to the passed key
    while(temp->is_occupied == 0 || ht->cmp_func(temp->key, key) != 0) {
        index = (index + 1) % ht->capacity;
        // this will idicate that search completed the whole loop in the array,
        // or when elemen is_occupied == 0 and is_deleted == 0, which means nothing could be 
        // inserted after that element in linear probing alorightm
        // which means no element was found
        if((temp->is_occupied == 0 && temp->is_deleted == 0) || index == start_index)
            return NULL;
        temp = &ht->hash_data[index];
    
    }
    return temp->value;
}

bool ht_remove(HashTable* ht, void* key)
{
	assert(ht != NULL && "`ht` must not be NULL");
    uint64_t index = ht->hash_func(key) % ht->capacity;
    uint64_t start_index = index;

    HashData* needle = &ht->hash_data[index];
    // look for the element
    while(needle->is_occupied == 0 || ht->cmp_func(needle->key, key) != 0) {
        index = (index + 1) % ht->capacity;
        // this will idicate that search completed the whole loop in the array,
        // or when elemen is_occupied == 0 and is_deleted == 0, which means nothing could be 
        // inserted after that element in linear probing alorightm
        // which means no element was found
        if((needle->is_occupied == 0 && needle->is_deleted == 0) || index == start_index)
            return false;
        needle = &ht->hash_data[index];
    }

    if(needle->value_size != HT_PTR_IS_DATA) {
        free(needle->value);
    }
    // TODO maybe in a future allow to store ptr as a key
    if(needle->key_size != HT_PTR_IS_DATA)
        free(needle->key);

    memset(needle, 0, sizeof(HashData));    // clear struct
    needle->is_deleted = 1;     // make it tombstone

    // check so that we dont decrement ot negative `count` values
    if(ht->count > 0)
        ht->count--;

	if(!(ht->flags & HT_DONT_SHRINK)) {
		// shrink array if only {SHRINK_LOAD_FACTOR}% of the table is occupied
		if((float)ht->count / (float)ht->capacity <= SHRINK_LOAD_FACTOR) { 
			size_t new_capacity = ht->capacity * TABLE_SHRINK_FACTOR; 

			if (new_capacity < 10)
				new_capacity = 10;
	 

			HashData* temp_data_arr = malloc(sizeof(HashData) * new_capacity); 
			memset(temp_data_arr, 0, sizeof(HashData) * new_capacity);

			/* readd all elements */ 
			ht_n_cpy(ht, temp_data_arr, ht->hash_data, ht->capacity, new_capacity); 

			clear_ht_data(ht, ht->capacity);

			ht->hash_data = temp_data_arr; 
			temp_data_arr = NULL;

			ht->capacity = new_capacity; 
		} 
	}

    return true;      
}

// clear HashTable data
// NOTE that clear_ht does NOT free HashTable structure itself, it is up to user to do that
// WARNING if you store pointers to data in HashTable, make sure you freed them before calling clear_ht()
bool clear_ht(HashTable* ht)
{
    int64_t size = ht->capacity; 
    int64_t count = ht->count; 
    if(size <= 0 || count <= 0)
        return false;

    for(int64_t i = 0; i < ht->capacity; i++) {
        HashData *temp = &ht->hash_data[i];
        if(temp->is_occupied) {
            if(temp->key_size != HT_PTR_IS_DATA) {
                free(temp->key);
            }
            temp->key = NULL;

            if(temp->value_size != HT_PTR_IS_DATA) {
                free(temp->value);
            }
            temp->value = NULL;
        }
    }
    free(ht->hash_data);
    ht->capacity = 0;       // < probably dont need that
    ht->count = 0;          // <
    return true;
}

#ifdef CASH_TABLE_DEBUG
void d_print_ht_info(HashTable* ht)
{
    printf("ht: %pcapacity: %lu\n"
	"count:%d\n"
	"flags: %d, HT_DONT_GROW: %d, HT_DONT_SHRINK: %d"
	"hash func: %p, cmp_func: %p\n",
            ht, ht->capacity, ht->count, ht->flags, ht->flags % HT_DONT_GROW,
			ht->flags & HT_DONT_SHRINK, ht->hash_func, ht->cmp_func);
}
#endif
    

// TODO make similar function for initialization on a stack
 HashTable* init_ht_heap(int capacity, int flags, uint64_t (*hash_func) (const void* key), int (*compare_func) (const void* a, const void* b)) 
 {    
        assert("Hash table capacity must be > 0" && (capacity > 0));    
    
        // alloc arrays
        HashTable* ht = (HashTable*)malloc(sizeof(HashTable));    
        if(ht == NULL)
            return NULL;

        ht->hash_data = malloc(sizeof(HashData) * capacity);
        //TODO make it consistent with init_ht_stack() function
        if(ht->hash_data == NULL) {
            free(ht);
            return NULL;
        }
        // set them to 0
        memset(ht->hash_data, 0, sizeof(HashData) * capacity);     

        ht->capacity = capacity;    
		ht->flags = flags;
        ht->count = 0;    

        ht->hash_func = hash_func == NULL ? default_hash : hash_func;
        ht->cmp_func = compare_func == NULL ? default_compare_func : compare_func;

        return ht;
} 

HashTable init_ht_stack(int capacity, int flags, uint64_t (*hash_func) (const void* key), int (*compare_func) (const void* a, const void* b)) 
 {    
        assert("Hash table capacity must be > 0" && (capacity > 0));    
    
        // alloc arrays
        HashTable ht = (HashTable) {
            .capacity = (size_t)capacity,    // TODO maybe it is a bad idea to cast 'int' to 'size_t'
			.flags = flags,			// TODO check flags
            .count = 0,    
            .hash_data = malloc(sizeof(HashData) * capacity),    

            .hash_func = hash_func == NULL ? default_hash : hash_func,
            .cmp_func = compare_func == NULL ? default_compare_func : compare_func,
        };
        //TODO make it consistent with init_ht_heap() function
        if(ht.hash_data == NULL) {
            fprintf(stderr, "malloc failed\n");
            abort();
        }
        
        // set them to 0
        memset(ht.hash_data, 0, sizeof(HashData) * capacity);     
        return ht;
} 

#endif  //CASH_TABLE_IMPLEMENTATION
