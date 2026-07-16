#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "chibihash64.h"

static void *hash_array = NULL;
static size_t hash_array_entry_count = 0;

static uint64_t chibihash64_wrapper(const char *input, size_t len)
{
	return chibihash64((void *)input, (ptrdiff_t)len, 0xFFFFFFFFFFFFFFFFULL);
}

static void table_insert(const char *str)
{
	if (!str)
		return;

	uint64_t hash = chibihash64_wrapper((const char *)str, strlen(str));
	if (!hash)
		return;

	if (!hash_array)
		goto skip_anti_dup;

	size_t i;
	for (i = 0; i < hash_array_entry_count; i++) {
		if (((uint64_t *)hash_array)[i] == hash) {
			printf("duplicate hash: 0x%llx\n", hash);
			return;
		}
	}

skip_anti_dup:
	;

	uintptr_t tail = (hash_array_entry_count) * sizeof(uint64_t);

	// now expand this array
	hash_array = realloc(hash_array, (hash_array_entry_count + 1) * sizeof(uint64_t));
	if (!hash_array)
		return;

	memcpy((void *)((uintptr_t)hash_array + tail), &hash, sizeof(uint64_t));
	
	hash_array_entry_count++;
}

int main(int argc, char *argv[], char *envp[])
{
	if (!argv[1])
		return 1;

	table_insert("abcd");
	table_insert("efgh");
	table_insert("ijkl");
	table_insert("ijkl"); //
	table_insert("mnop");
	table_insert("qrst");
	table_insert("uvwx");
	table_insert("yzab");
	table_insert("cdef");
	table_insert("ghij");
	table_insert("klmn");

	uint64_t input_hash = chibihash64_wrapper(argv[1], strlen(argv[1]));
	if (!input_hash)
		return 1;

	size_t i;
	bool found = false;
	for (i = 0; i < hash_array_entry_count; i++)
	{
		if (((uint64_t *)hash_array)[i] == input_hash) {
			found = true;
			break;
		}
	}

	if (found)
		printf("found: %s hash: 0x%llx == 0x%llx\n", argv[1], input_hash, ((uint64_t *)hash_array)[i]);
	else
		printf("not found: %s hash: 0x%llx\n", argv[1], input_hash);


	return 0;
}
