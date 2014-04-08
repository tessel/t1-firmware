extern unsigned _bss;
extern unsigned _ebss;

extern unsigned _ramtext;
extern unsigned _eramtext;
extern unsigned _ramtext_loadaddr;

extern unsigned _data;
extern unsigned _edata;
extern unsigned _data_loadaddr;

void main(unsigned r0);

void _reset_handler(unsigned r0) {
	// Zero .bss
	unsigned* p = &_bss;
	while (p < &_ebss) {
		*p++ = 0;
	}

	if (&_ramtext != &_ramtext_loadaddr) {
		// Copy .ramtext to RAM
		unsigned* s=&_ramtext_loadaddr;
		unsigned* d=&_ramtext;
		while (d<&_eramtext) {
			*d++ = *s++;
		}
	}

	if (&_data != &_data_loadaddr) {
		// Copy .data to RAM
		unsigned* s=&_data_loadaddr;
		unsigned* d=&_data;
		while (d<&_edata) {
			*d++ = *s++;
		}
	}

	main(r0);

	while(1) {};
}