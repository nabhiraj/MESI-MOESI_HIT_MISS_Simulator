#include<map> 
class L1_cache;
class Bus;
class L2_cache{
public:
	int core_id;
	unsigned long long hits;
	unsigned long long miss;
	unsigned long long coherent_miss;
	map<unsigned long long,unsigned long long> coherent_table;
	//diffrenct transistion count//
	unsigned long long MtoO;
	unsigned long long MtoI;
	unsigned long long EtoM;
	unsigned long long EtoS;
	unsigned long long EtoI;
	unsigned long long StoI;
	unsigned long long StoM;
	unsigned long long OtoM;
	unsigned long long OtoI;

	Set* set[L2_number_of_sets];
	 L1_cache* l1;
	 Bus* bus;
	 L2_cache(int core);//constructor.
	 //---------------member functions-------------------------------
	void attach_l1(L1_cache* temp);
	void attach_bus(Bus* temp);
	void invalidate(unsigned long long baddr);
	bool is_present_in_state(unsigned long long baddr,int state);
	void set_state(unsigned long long baddr,int s);
	void touch(unsigned long long baddr,int operation);
};