class L2_cache;
class L1_cache{
public:
	unsigned long long hits;
	unsigned long long miss;
	Set* set[L1_number_of_sets];
	 L2_cache* l2cache;
	 
	//-----------------------------------------constructor----------------------------------------------------
	L1_cache();
	//-----------------------------------------  methods-------------------------------------------------------
	void invalid(unsigned long long baddr);
	void attach_l2(L2_cache* c);
	void touch(unsigned long long baddr,int operation);
};