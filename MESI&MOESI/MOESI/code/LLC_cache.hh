class Bus;
class LLC_cache{
public:
	unsigned long long hits;
	unsigned long long miss;
	 Bus* bus;
	 Set* set[LLC_number_of_sets];
//--------------member functions--------------------------------------
	LLC_cache(Bus* b);
	void touch(unsigned long long baddr,int operation);

};