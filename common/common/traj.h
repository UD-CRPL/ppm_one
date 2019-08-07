
#include "supply.h"
#include "bmrb.h"
#include  "aa.h"
#include "pdb.h"

#ifndef TRAJ
#define TRAJ

class CTraj 
{
private:

protected:
		int nres;
		int natom,nframe;
		double noedistance(vector<int> *, vector<int> *);
		double noedistance_frame(vector<int> *att1, vector<int> *att2,int n);

public:
		vector<double> x;
		vector<double> y;
		vector<double> z;
		vector<string> atomname;
		vector<double> rmsf;

		CTraj(void);
		~CTraj(void);

		inline void setnatom(int in){natom=in;};
		inline void setnres(int in){nres=in;};
		inline int getnframe(void) {return nframe;};
		virtual int loadcoor(string);
		int appendcoor(string);
		int set_range(int,int);
		virtual void clear(void);
		void dis_matrix(vector<int> *,vector<double> *);
		void getdihe(vector<dihe_group> *, vector<double> *);
		void getangle(vector<struct dihe_group> *, vector<double> *);
		void getring(vector<ring_group> *, vector<struct methyl_group> *, vector<double_five> *, enum methyl);
		void getring(vector<ring_group> *, vector<struct proton>* , vector<struct double_five> *);
		void getring(vector<ring_group> *, vector<struct proton>* , vector< vector<struct double_five> > *);
		void getring(vector<ring_group> *, vector<struct nh_group>* , vector<struct double_five> *ring_effect);
		void getring(vector<ring_group> *, vector<struct nh_group>* , vector< vector<struct double_five>  > *ring_effect);
		void getani(vector<ani_group> *, vector<struct nh_group>* , vector<double_four> *);
		void getani(vector<ani_group> *, vector<struct nh_group>* , vector< vector<double_four> > *);
		void getani(vector<ani_group> *, vector<struct methyl_group>* , vector<double_four> *, enum methyl);
		void getani(vector<ani_group> *, vector<struct proton>* , vector<double_four> *);
		void getani(vector<ani_group> *, vector<struct proton>* , vector< vector<double_four> > *);
		void getring_bb(vector<ring_group> *, vector<struct bb_group> *, vector<double_five> *,enum bb_carbon);
		void gethbond(vector<bbhbond_group> *bond,vector<ehbond> *effect);
		void gethbond(vector<bbhbond_group> *hbond,vector<ehbond> *effect, double cutoff);
		void gethbond(vector<bbhbond_group> *bond,vector< vector<ehbond> > *effect);
		void gethbond2(vector<bbhbond_group> *hbond,vector< vector<ehbond> > *effect);
		void getschbond2(vector<struct proton> *protons, vector<struct bbhbond_group> *bb, vector< vector<ehbond> > *effect, vector< vector<eschbond> > *effect_sc);
		void evulatenmrcons(vector<struct noeline> *,double);
		void evaluatenmrcons_frame(vector<struct noeline> *nmrcons, double cutoff);
		void rmsd_matrix(vector< vector<double> > *,vector<int> *,int);
		void getvector(vector<struct index_three>,vector<double> *,vector<double> *,vector<double> *);
		void do_rmsf(void);

		void get_contact(vector<int> pos, vector<int> used, vector<float> * result);
		void get_contact(float rc,float shift, vector<int> pos, vector<int> used, vector<float> * result);
		void get_contacts(vector<struct bb_group>, vector<struct index_two>, vector<int>, vector<float>*);

		void getcoor(int,int,double *,double *,double *);
		void getcoor(vector<int>,int,vector<double> *,vector<double> *,vector<double> *);
		void getcoor(vector<int>,int,vector<float> *,vector<float> *,vector<float> *);
		void getcoor(vector<int>,vector<float> *xx,vector<float> *yy,vector<float> *zz);



};

//class to process crystal data
class smtry
{
	protected:
		double matrix[3][3];
		double motion[3];
	public:
		void getdata(double x[3][3],double y[3]);
		void trans(double x[3],double y[3]);
		void print();
};


class CTraj2: public CTraj
{
private:

protected:
	vector<smtry> crystal_sym_array;
	double translation[3][3];
	double scale[3][3];
	bool btrans;
	int ncry;
	bool bsmtry;
public:
	vector<double> b;  //b factors
	CTraj2(void);
	~CTraj2(void);
	void clear();
	void process_tran(void);
	int loadcoor(string);
	int select(vector<int>);
	int unitcell(void);
	void ninecells(void);
};

#endif
