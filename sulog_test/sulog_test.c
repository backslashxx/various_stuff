#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>


struct sulog_entry {
	uint8_t symbol;
	uint32_t uid; // mebbe u16?
} __attribute__((packed));

struct sulog_entry_rcv_ptr {
	uint64_t int_ptr; // send index here
	uint64_t buf_ptr; // send buf here
};

#define SULOG_ENTRY_MAX 100
#define SULOG_BUFSIZ SULOG_ENTRY_MAX * (sizeof (struct sulog_entry))

int main()
{
	uint64_t latest_index = 0;
	char sulog_buf[SULOG_BUFSIZ] = {0};

	struct sulog_entry_rcv_ptr sbuf = {0};
	
	sbuf.int_ptr = (uint64_t)&latest_index;
	sbuf.buf_ptr = (uint64_t)sulog_buf;

	syscall(SYS_reboot, 0xDEADBEEF, 99999, 0, &sbuf);
	
	printf("next index: %lu\n", latest_index);
	printf("latest entry: %lu\n", latest_index - 1);

	int start = latest_index;

	int i = 0;
	while (i < SULOG_ENTRY_MAX) {
		// this way we make it so that latest index is highest index
		// modulus due to this overflowinbf entry_max
		int idx = (start + i) % SULOG_ENTRY_MAX;

		struct sulog_entry *entry_ptr = (struct sulog_entry *)(sulog_buf + idx * sizeof(struct sulog_entry) );

		printf("index (reordered): %d sym: %c uid: %u\n", i, entry_ptr->symbol, entry_ptr->uid);
		
		i++;
	}
	return 0;



}
