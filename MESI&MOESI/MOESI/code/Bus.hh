class L2_cache;
class LLC_cache;
class Bus{
public:
	 L2_cache* l2cache[NUM_OF_CORE];
	 LLC_cache* llc;
	 //-----------member function-------------------------
	void attach_l2(L2_cache* temp,int i);
	void attach_llc(LLC_cache* temp);
	void touch(unsigned long long baddr,int operation);
	bool share_handler(unsigned long long baddr,int core);//for read
	void inform_handler(unsigned long long baddr,int core);//for write
	void invalid(unsigned long long baddr);
	void invalid(unsigned long long baddr,int j);//invalidate all core but j
};