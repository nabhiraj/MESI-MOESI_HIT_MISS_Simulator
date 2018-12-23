unsigned long long get_blockAddr(unsigned long long address){
		address=address>>BLOCK_OFFSET;
		return address;
	}
	unsigned long long get_tag(unsigned long long blockaddress,int setbit){
		blockaddress=blockaddress>>setbit;
		return blockaddress;
	}
	unsigned long long get_set(unsigned long long blockaddress,int setbits){
		blockaddress=blockaddress & ((1<<(setbits))-1);
		return blockaddress;
	}