#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <math.h>
#include <time.h>
using namespace std;

#include "traj.h"
using namespace ldw_math;


// ///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CTraj::clear()
{
	x.clear();
	y.clear();
	z.clear();
	nframe=0;
}




int CTraj::appendcoor(string filename)
{
	double xx,yy,zz;
	string line,part;

	ifstream fin(filename.c_str());
	
	while(getline(fin,line))
	{
		if(part=="ENDMDL" || part=="END")
		{
			if(x.size()%natom!=0)
				cout<<"In traj reading, suppose to read "<<natom<<" coors but actually read in "<<x.size()<<endl;
		}

		part=line.substr(0,6);
		if(part!="ATOM  " && part!="HETATM")
			continue;

		part=line.substr(30,8);
		xx=atof(part.c_str());
		x.push_back(xx);
		part=line.substr(38,8);
		yy=atof(part.c_str());
		y.push_back(yy);
		part=line.substr(46,8);
		zz=atof(part.c_str());
		z.push_back(zz);
	};


	nframe=x.size()/natom;
	return nframe;
}


int CTraj::loadcoor(string filename)
{
	double xx,yy,zz;
	string line,part;
	bool bend;

	bend=0;


	ifstream fin(filename.c_str());

	
	while(getline(fin,line))
	{
		if(part=="ENDMDL" || part=="END")
		{
			//finished read first molecule 
			bend=1;
			if(x.size()%natom!=0)
				cout<<"In traj reading, suppose to read "<<natom<<" coors but actually read in "<<x.size()<<endl;
		}

		part=line.substr(0,6);
		if(part!="ATOM  " && part!="HETATM")
			continue;

		part=line.substr(30,8);
		xx=atof(part.c_str());
		x.push_back(xx);
		part=line.substr(38,8);
		yy=atof(part.c_str());
		y.push_back(yy);
		part=line.substr(46,8);
		zz=atof(part.c_str());
		z.push_back(zz);
		if(bend==0)
		{
			part=line.substr(12,4);
			atomname.push_back(part);
		}
	};

	if(natom==0)
		nframe=0;
	else
		nframe=x.size()/natom;

#pragma acc enter data copyin(this[:1])
  double *tmp = x.data();
  int sz = x.size();
#pragma acc enter data copyin(tmp[:sz])
  tmp = y.data();
  sz = y.size();
#pragma acc enter data copyin(tmp[:sz])
  tmp = z.data();
  sz = z.size();
#pragma acc enter data copyin(tmp[:sz])

	return nframe;
}

int CTraj::set_range(int begin,int stop)
{
	if(begin<0)
	{
		begin=0;
		cout<<"Reset begin to "<<begin<<endl;
	}

	if(stop>nframe)
	{
		stop=nframe;
		cout<<"Reset stop to "<<stop<<endl;
	}

	if(stop==0)
		stop=nframe;

	nframe=stop-begin;


	if(stop<nframe)
	{
		x.erase(x.begin()+stop*natom,x.begin()+x.size());
		y.erase(y.begin()+stop*natom,y.begin()+y.size());
		z.erase(z.begin()+stop*natom,z.begin()+z.size());
	}
	if(begin>0)
	{
		x.erase(x.begin(),x.begin()+begin*natom);
		y.erase(y.begin(),y.begin()+begin*natom);
		z.erase(z.begin(),z.begin()+begin*natom);
	}

	cout<<"Using frames from "<<begin<<" to "<<stop<<endl;
	return nframe;
}



void CTraj::getschbond2(vector<struct proton> *protons, vector<struct bbhbond_group> *bb, vector< vector<ehbond> > *effect, vector< vector<eschbond> > *effect_sc)
{
	int i,j,k;
	int base;
	int n,h[3],c,o;
	int nh;
	int nid,cid;
	double u[3];
	float x2,x3,x4,x5;
	float y2,y3,y4,y5;
	float z2,z3,z4,z5;
	float phi,psi,d;

	int kk,good_kk;
	float min_d;

	effect->resize(nres);
	for(i=0;i<(int)effect->size();i++)
	{
		effect->at(i).resize(nframe);
		for(k=0;k<nframe;k++)
		{
			effect->at(i).at(k).n_length=0;
			effect->at(i).at(k).c_length=0;
			effect->at(i).at(k).n_phi=0;
			effect->at(i).at(k).c_phi=0;
			effect->at(i).at(k).n_psi=0;
			effect->at(i).at(k).c_psi=0;
		}
	}

	effect_sc->resize(protons->size());
	for(i=0;i<(int)effect_sc->size();i++)
	{
		effect_sc->at(i).resize(nframe);
		for(k=0;k<nframe;k++)
		{
			effect_sc->at(i).at(k).n_length=0;
			effect_sc->at(i).at(k).c_length=0;
			effect_sc->at(i).at(k).n_phi=0;
			effect_sc->at(i).at(k).c_phi=0;
			effect_sc->at(i).at(k).n_psi=0;
			effect_sc->at(i).at(k).c_psi=0;
		}
	}


	
	for(i=0;i<(int)protons->size();i++)
	{
		for(j=0;j<(int)bb->size();j++)
		{
			nh=protons->at(i).nh;
			for(k=0;k<nh;k++)
				h[k]=protons->at(i).hpos[k];
			n=protons->at(i).cpos;
			if(h[0]<=-1 || n<=-1 || (nh==2 && h[1]<0) || (nh==3 && h[2]<0) ) 
				continue;
			c=bb->at(j).cpos;
			o=bb->at(j).opos;
			if(o<=-1 || c<=-1)
				continue;
			n--;c--;o--;
			for(k=0;k<nh;k++)
				h[k]--;

			for(k=0;k<nframe;k++)
			{
				//cout<<"i,j,k is "<<i<<" "<<j<<" "<<k<<endl;
				min_d=100000.0;
				for(kk=0;kk<nh;kk++)
				{
					base=k*natom;
					u[0]=x[h[kk]+base]-x[o+base];
					u[1]=y[h[kk]+base]-y[o+base];
					u[2]=z[h[kk]+base]-z[o+base];
					d=veclength(u);
					if(d<min_d)
					{
						min_d=d;
						good_kk=kk;
					}
				}
				d=min_d;
				x2=x[n+base];y2=y[n+base];z2=z[n+base];
				x3=x[h[good_kk]+base];y3=y[h[good_kk]+base];z3=z[h[good_kk]+base];
				x4=x[o+base];y4=y[o+base];z4=z[o+base];
				x5=x[c+base];y5=y[c+base];z5=z[c+base];
				phi=coor_to_angle(x2,y2,z2,x3,y3,z3,x4,y4,z4);
				psi=coor_to_angle(x3,y3,z3,x4,y4,z4,x5,y5,z5);
				if(d<3 && phi>0.5 && psi>0.5)
				{
					d=1/(d-1);
					phi*=phi;
					psi*=psi;
					nid=protons->at(i).id-1;
					cid=bb->at(j).id-1; /* C start from 0*/
					
					//effect->at(nid).at(k).n_length=d;
					//effect->at(nid).at(k).n_phi=phi;
					//effect->at(nid).at(k).n_psi=psi;

					effect_sc->at(i).at(k).n_length=d;
					effect_sc->at(i).at(k).n_phi=phi;
					effect_sc->at(i).at(k).n_psi=psi;
					effect_sc->at(i).at(k).id=protons->at(i).id;
					effect_sc->at(i).at(k).type=protons->at(i).type;

					if(bb->at(j).type==1)
					{
						effect->at(cid).at(k).c_length=d;
						effect->at(cid).at(k).c_phi=phi;
						effect->at(cid).at(k).c_psi=psi;
					}
					
				}
			}//for k
		}//for j
	}//for i

	return;
}


//hbond type (1, bb) (12, sc OH) (13,sc NH) (22 or 23, sc CO)
void CTraj::gethbond(vector<bbhbond_group> *hbond,vector<ehbond> *effect)
{
	int i,j,k;
	int base;
	int nid,cid;
	int n,h,c,o;
	double u[3];
	double x2,x3,x4,x5,y2,y3,y4,y5,z2,z3,z4,z5;
	double d,phi,psi;

	bbhbond_group* hbond_arr = hbond->data();
	int hbond_size = hbond->size();
	effect->resize(nres);
	ehbond* effect_arr = effect->data();
	int effect_size = effect->size();
	double* x_arr = this->x.data();
	int x_size = this->x.size();
	double* y_arr = this->y.data();
	int y_size = this->y.size();
	double* z_arr = this->z.data();
	int z_size = this->z.size();

  int nframe = this->nframe;
  int natom  = this->natom;

#pragma acc data \
	copyin(hbond_arr[:hbond_size]) \
	copyout(effect_arr[:effect_size]) \
	present(x_arr[:x_size], y_arr[:y_size], z_arr[:z_size])
{

#pragma acc parallel loop gang
	for(i=0; i<hbond_size; i++)
	{
#pragma acc loop vector \
	private(k,n,h,c,o,base,u[:3],d,x2,x3,x4,x5,y2,y3,y4,y5,\
					z2,z3,z4,z5,phi,psi,nid,cid)
		for(j=0; j<hbond_size; j++)
		{
			k=j-i;
			if(k<3 && k>-3)
				continue;
			n=hbond_arr[i].npos;
			h=hbond_arr[i].hpos;
			if(h<=-1 || n<=-1) 
				continue;
			c=hbond_arr[j].cpos;
			o=hbond_arr[j].opos;
			if(o<=-1 || c<=-1)
				continue;
			n--;h--;c--;o--;
			if(h<0 || n<0 || o<0 || c<0)
				continue;

#pragma acc loop seq
			for(k=0;k<nframe;k++)
			{
				base=k*natom;
				u[0]=x_arr[h+base]-x_arr[o+base];
				u[1]=y_arr[h+base]-y_arr[o+base];
				u[2]=z_arr[h+base]-z_arr[o+base];
				d=veclength(u);
				x2=x_arr[n+base];y2=y_arr[n+base];z2=z_arr[n+base];
				x3=x_arr[h+base];y3=y_arr[h+base];z3=z_arr[h+base];
				x4=x_arr[o+base];y4=y_arr[o+base];z4=z_arr[o+base];
				x5=x_arr[c+base];y5=y_arr[c+base];z5=z_arr[c+base];
				phi=coor_to_angle(x2,y2,z2,x3,y3,z3,x4,y4,z4);
				psi=coor_to_angle(x3,y3,z3,x4,y4,z4,x5,y5,z5);
				if(d<3 && phi>0.5 && psi>0.5)
				{
					d=1/(d-1);
					phi*=phi;
					psi*=psi;
					nid=hbond_arr[i].id-1;
					cid=hbond_arr[j].id-1; /* C start from 0*/
					if(hbond_arr[i].type==1)
					{
#pragma acc atomic update
						effect_arr[nid].n_length+=d;
#pragma acc atomic update
						effect_arr[nid].n_phi+=phi;
#pragma acc atomic update
						effect_arr[nid].n_psi+=psi;
					}
					if(hbond_arr[j].type==1)
					{
#pragma acc atomic update
						effect_arr[cid].c_length+=d;
#pragma acc atomic update
						effect_arr[cid].c_phi+=phi;
#pragma acc atomic update
						effect_arr[cid].c_psi+=psi;
					}
				}
			}
		}
	}

#pragma acc parallel loop
	for(i=0; i<effect_size; i++)
	{
		effect_arr[i].n_length/=nframe;
		effect_arr[i].c_length/=nframe;
		effect_arr[i].n_phi/=nframe;
		effect_arr[i].c_phi/=nframe;
		effect_arr[i].n_psi/=nframe;
		effect_arr[i].c_psi/=nframe;
	}

} // end data

	return;
}


//hbond type (1, bb) (12, sc OH) (13,sc NH) (22 or 23, sc CO)
void CTraj::gethbond(vector<bbhbond_group> *hbond,vector<ehbond> *effect, double cutoff)
{

	int i,j,k;
	int base;
	int nid,cid;
	int n,h,c,o;
	double u[3];
	double x2,x3,x4,x5,y2,y3,y4,y5,z2,z3,z4,z5;
	double d,phi,psi;

	
	effect->resize(nres);
	for(i=0;i<(int)hbond->size();i++)
	{

		for(j=0;j<(int)hbond->size();j++)
		{
			k=j-i;
			if(k<3 && k>-3)
				continue;
			n=hbond->at(i).npos;
			h=hbond->at(i).hpos;
			if(h<=-1 || n<=-1) 
				continue;
			c=hbond->at(j).cpos;
			o=hbond->at(j).opos;
			if(o<=-1 || c<=-1)
				continue;
			n--;h--;c--;o--;
			if(h<0 || n<0 || o<0 || c<0)
				continue;



			for(k=0;k<nframe;k++)
			{
				//cout<<"i,j,k is "<<i<<" "<<j<<" "<<k<<endl;
				base=k*natom;
				u[0]=x[h+base]-x[o+base];
				u[1]=y[h+base]-y[o+base];
				u[2]=z[h+base]-z[o+base];
				d=veclength(u);
				x2=x[n+base];y2=y[n+base];z2=z[n+base];
				x3=x[h+base];y3=y[h+base];z3=z[h+base];
				x4=x[o+base];y4=y[o+base];z4=z[o+base];
				x5=x[c+base];y5=y[c+base];z5=z[c+base];
				phi=coor_to_angle(x2,y2,z2,x3,y3,z3,x4,y4,z4);
				psi=coor_to_angle(x3,y3,z3,x4,y4,z4,x5,y5,z5);
				if(d<3 && phi>0.5 && psi>0.5)
				{
					d=1/(d-1);
					phi*=phi;
					psi*=psi;
					nid=hbond->at(i).id-1;
					cid=hbond->at(j).id-1; /* C start from 0*/

					if(d>cutoff)
						d=cutoff;

					if(hbond->at(i).type==1)
					{
						effect->at(nid).n_length+=d;
						effect->at(nid).n_phi+=phi;
						effect->at(nid).n_psi+=psi;
					}
					if(hbond->at(j).type==1)
					{
						effect->at(cid).c_length+=d;
						effect->at(cid).c_phi+=phi;
						effect->at(cid).c_psi+=psi;
					}
				}
			}
		}
	}

	for(i=0;i<(int)effect->size();i++)
	{
		effect->at(i).n_length/=nframe;
		effect->at(i).c_length/=nframe;
		effect->at(i).n_phi/=nframe;
		effect->at(i).c_phi/=nframe;
		effect->at(i).n_psi/=nframe;
		effect->at(i).c_psi/=nframe;
	}
	return;
}



void CTraj::gethbond(vector<bbhbond_group> *hbond,vector< vector<ehbond> > *effect)
{

	int i,j,k;
	int base;
	int nid,cid;
	int n,h,c,o;
	double u[3];
	double x2,x3,x4,x5,y2,y3,y4,y5,z2,z3,z4,z5;
	double d,phi,psi;

	effect->resize(nres);
	for(i=0;i<(int)effect->size();i++)
	{
		effect->at(i).resize(nframe);
		for(k=0;k<nframe;k++)
		{
			effect->at(i).at(k).n_length=0;
			effect->at(i).at(k).c_length=0;
			effect->at(i).at(k).n_phi=0;
			effect->at(i).at(k).c_phi=0;
			effect->at(i).at(k).n_psi=0;
			effect->at(i).at(k).c_psi=0;
		}
	}

	for(i=0;i<(int)hbond->size();i++)
	{

		for(j=0;j<(int)hbond->size();j++)
		{
			k=j-i;
			if(k<3 && k>-3)
				continue;
			n=hbond->at(i).npos;
			h=hbond->at(i).hpos;
			if(h<0) 
				continue;
			c=hbond->at(j).cpos;
			o=hbond->at(j).opos;
			if(o<0)
				continue;
			n--;h--;c--;o--;


			for(k=0;k<nframe;k++)
			{
				//cout<<"i,j,k is "<<i<<" "<<j<<" "<<k<<endl;
				base=k*natom;
				u[0]=x[h+base]-x[o+base];
				u[1]=y[h+base]-y[o+base];
				u[2]=z[h+base]-z[o+base];
				d=veclength(u);
				x2=x[n+base];y2=y[n+base];z2=z[n+base];
				x3=x[h+base];y3=y[h+base];z3=z[h+base];
				x4=x[o+base];y4=y[o+base];z4=z[o+base];
				x5=x[c+base];y5=y[c+base];z5=z[c+base];
				phi=coor_to_angle(x2,y2,z2,x3,y3,z3,x4,y4,z4);
				psi=coor_to_angle(x3,y3,z3,x4,y4,z4,x5,y5,z5);
				if(d<3 && phi>0.5 && psi>0.5)
				{
					d=1/(d-1);
					phi*=phi;
					psi*=psi;
					nid=hbond->at(i).id-1;
					cid=hbond->at(j).id-1; /* C start from 0*/
					if(hbond->at(i).type==1)
					{
						effect->at(nid).at(k).n_length+=d;
						effect->at(nid).at(k).n_phi+=phi;
						effect->at(nid).at(k).n_psi+=psi;
					}
					if(hbond->at(j).type==1)
					{
						effect->at(cid).at(k).c_length+=d;
						effect->at(cid).at(k).c_phi+=phi;
						effect->at(cid).at(k).c_psi+=psi;
					}
				}
			}
		}
	}
	return;
}


void CTraj::gethbond2(vector<bbhbond_group> *hbond,vector< vector<ehbond> > *effect)
{

	int i,j,k;
	int base;
	int nid,cid;
	int n,h,c,o;
	double u[3];
	double x2,x3,x4,x5,y2,y3,y4,y5,z2,z3,z4,z5;
	double d,phi,psi;

	effect->resize(nres);
	for(i=0;i<(int)effect->size();i++)
	{
		effect->at(i).resize(nframe);
		for(k=0;k<nframe;k++)
		{
			effect->at(i).at(k).n_length=0;
			effect->at(i).at(k).c_length=0;
			effect->at(i).at(k).n_phi=0;
			effect->at(i).at(k).c_phi=0;
			effect->at(i).at(k).n_psi=0;
			effect->at(i).at(k).c_psi=0;
		}
	}

	for(i=0;i<(int)hbond->size();i++)
	{

		for(j=0;j<(int)hbond->size();j++)
		{
			k=j-i;
			if(k<=3 && k>=-3)
				continue;
			n=hbond->at(i).npos;
			h=hbond->at(i).hpos;
			if(h<0) 
				continue;
			c=hbond->at(j).cpos;
			o=hbond->at(j).opos;
			if(o<0)
				continue;
			n--;h--;c--;o--;


			for(k=0;k<nframe;k++)
			{
				//cout<<"i,j,k is "<<i<<" "<<j<<" "<<k<<endl;
				base=k*natom;
				u[0]=x[h+base]-x[o+base];
				u[1]=y[h+base]-y[o+base];
				u[2]=z[h+base]-z[o+base];
				d=veclength(u);
				x2=x[n+base];y2=y[n+base];z2=z[n+base];
				x3=x[h+base];y3=y[h+base];z3=z[h+base];
				x4=x[o+base];y4=y[o+base];z4=z[o+base];
				x5=x[c+base];y5=y[c+base];z5=z[c+base];
				phi=coor_to_angle(x2,y2,z2,x3,y3,z3,x4,y4,z4);
				psi=coor_to_angle(x3,y3,z3,x4,y4,z4,x5,y5,z5);
				if(d<3 && phi>0.5 && psi>0.5)
				{
					d=1/(d-1);
					phi*=phi;
					psi*=psi;
					nid=hbond->at(i).id-1;
					cid=hbond->at(j).id-1; /* C start from 0*/
					if(hbond->at(i).type==1)
					{
						effect->at(nid).at(k).n_length=d;
						effect->at(nid).at(k).n_phi=phi;
						effect->at(nid).at(k).n_psi=psi;
					}
					if(hbond->at(i).type==1)
					{
						effect->at(cid).at(k).c_length=d;
						effect->at(cid).at(k).c_phi=phi;
						effect->at(cid).at(k).c_psi=psi;
					}
				}
			}
		}
	}
	return;
}




void CTraj::dis_matrix(vector<int> *index,vector<double> *dis)
{
	int i,j,n1,n2;
	double d1,d2,d3,d;
	for(i=0;i<(int)index->size();i++)
	{
		n1=index->at(i)-1;
		for(j=0;j<(int)index->size();j++)
		{		
			n2=index->at(j)-1;
			d1=x.at(n1)-x.at(n2);
			d2=y.at(n1)-y.at(n2);
			d3=z.at(n1)-z.at(n2);
			d=sqrt(d1*d1+d2*d2+d3*d3);
			dis->push_back(d);
		}
	}
	return;
}


void CTraj::getangle(vector<struct dihe_group> *index, vector<double> * angle)
{
	int i,j;
	int x1,x2,x3,x4;
	int base;
	double t;


	for(i=0;i<nframe;i++)
	{
		base=i*natom-1;  // -1 because of C starts from 0 but not 1 !
		for(j=0;j<(int)index->size();j++)
		{
			x1=base+index->at(j).x1;
			x2=base+index->at(j).x2;
			x3=base+index->at(j).x3;
			x4=base+index->at(j).x4;
			t=coor_to_angle(x[x1],y[x1],z[x1],x[x2],y[x2],z[x2],x[x3],y[x3],z[x3]);
			angle->push_back(t);
			t=coor_to_angle(x[x2],y[x2],z[x2],x[x3],y[x3],z[x3],x[x4],y[x4],z[x4]);
			angle->push_back(t);
		}
	}
	return;
}




void CTraj::getdihe(vector<struct dihe_group> *index, vector<double> * dihe)
{	
	int i,j;
	int x1,x2,x3,x4;
	int base;
	double t;


	for(i=0;i<nframe;i++)
	{
		base=i*natom-1;  // -1 because of C starts from 0 but not 1 !
		for(j=0;j<(int)index->size();j++)
		{
			if(index->at(j).bgood==1)
			{
				x1=base+index->at(j).x1;
				x2=base+index->at(j).x2;
				x3=base+index->at(j).x3;
				x4=base+index->at(j).x4;
				t=coor_to_dihe(x[x1],y[x1],z[x1],x[x2],y[x2],z[x2],x[x3],y[x3],z[x3],x[x4],y[x4],z[x4]);
				dihe->push_back(t);
			}
			else
				dihe->push_back(-1000.0);
		}
	}
	return;
}


void CTraj::getring(vector<struct ring_group> *index, vector<struct nh_group>* select, vector<struct double_five> *ring_effect)
{	
	int i,j,k,ii,jj,m;
	int base;
	int t[6];
	double p1[3],t1[3],t2[3],t3[3];
	double u[6][3];
	double sum[3];
	double ori[3];
	double e;

	ring_group* index_arr = index->data();
	int index_size = index->size();
	nh_group* select_arr = select->data();
	int select_size = select->size();
	ring_effect->resize(select_size);
	double_five* effect_arr = ring_effect->data();
	int effect_size = ring_effect->size();
	double* x_arr = this->x.data();
	int x_size = this->x.size();
	double* y_arr = this->y.data();
	int y_size = this->y.size();
	double* z_arr = this->z.data();
	int z_size = this->z.size();

	int nframe = this->nframe;
	int natom = this->natom;

#pragma acc data \
	copyin(index_arr[:index_size], select_arr[:select_size], \
  	x_arr[:x_size], y_arr[:y_size], z_arr[:z_size]) \
	copyout(effect_arr[:effect_size])
{

#pragma acc parallel loop collapse(2)
	for(i=0; i<select_size; i++)
		for(j=0; j<5; j++)
			effect_arr[i].x[j]=0.0;

	for(i=0;i<nframe;i++)
	{
		base=i*natom;

#pragma acc parallel loop gang \
	private(ii,jj,m,t[:6],u[:6][:3],sum[:3],\
	ori[:3],t1[:3],t2[:3],t3[:3])
		for(j=0; j<index_size; j++)
		{
			if(index_arr[j].x1 == 1 || index_arr[j].x1 == 2 || index_arr[j].x1 == 5){
				m=6;
			} else {
				m=5;
			}

			t[0]=index_arr[j].x2;
			t[1]=index_arr[j].x3;
			t[2]=index_arr[j].x4;
			t[3]=index_arr[j].x5;
			t[4]=index_arr[j].x6;
			t[5]=index_arr[j].x7;

			u[0][0]=x_arr[t[0]+base-1]; u[0][1]=y_arr[t[0]+base-1]; u[0][2]=z_arr[t[0]+base-1];
			u[1][0]=x_arr[t[1]+base-1]; u[1][1]=y_arr[t[1]+base-1]; u[1][2]=z_arr[t[1]+base-1];
			u[2][0]=x_arr[t[2]+base-1]; u[2][1]=y_arr[t[2]+base-1]; u[2][2]=z_arr[t[2]+base-1];
			u[3][0]=x_arr[t[3]+base-1]; u[3][1]=y_arr[t[3]+base-1]; u[3][2]=z_arr[t[3]+base-1];
			u[4][0]=x_arr[t[4]+base-1]; u[4][1]=y_arr[t[4]+base-1]; u[4][2]=z_arr[t[4]+base-1];
			if(m >= 6) {
				u[5][0]=x_arr[t[5]+base-1]; u[5][1]=y_arr[t[5]+base-1]; u[5][2]=z_arr[t[5]+base-1];
			} else {
				u[5][0]=0; u[5][1]=0; u[5][2]=0;
			}

			sum[0] = (u[0][0] + u[1][0] + u[2][0] + u[3][0] + u[4][0] + u[5][0]) / m;
			sum[1] = (u[0][1] + u[1][1] + u[2][1] + u[3][1] + u[4][1] + u[5][1]) / m;
			sum[2] = (u[0][2] + u[1][2] + u[2][2] + u[3][2] + u[4][2] + u[5][2]) / m;

			u[0][0] -= sum[0]; u[0][1] -= sum[1]; u[0][2] -= sum[2];
			u[1][0] -= sum[0]; u[1][1] -= sum[1]; u[1][2] -= sum[2];
			u[2][0] -= sum[0]; u[2][1] -= sum[1]; u[2][2] -= sum[2];
			u[3][0] -= sum[0]; u[3][1] -= sum[1]; u[3][2] -= sum[2];
			u[4][0] -= sum[0]; u[4][1] -= sum[1]; u[4][2] -= sum[2];
			u[5][0] -= sum[0]; u[5][1] -= sum[1]; u[5][2] -= sum[2];

			ring(u,m,ori);

			t1[0] = u[0][0] - u[1][0];
			t1[1] = u[0][1] - u[1][1];
			t1[2] = u[0][2] - u[1][2];
			t2[0] = u[2][0] - u[1][0];
			t2[1] = u[2][1] - u[1][1];
			t2[2] = u[2][2] - u[1][2];

			cross(t3,t1,t2);
			if(dot(t3,ori)<0)
			{
				ori[0] = -ori[0];
				ori[1] = -ori[1];
				ori[2] = -ori[2];
			}

#pragma acc loop vector private(e,p1[:3])
			for(k=0; k<select_size; k++)
			{
				if(select_arr[k].hpos>=1)
				{
					e=0;
					p1[0]=x_arr[base+select_arr[k].hpos-1]-sum[0];
					p1[1]=y_arr[base+select_arr[k].hpos-1]-sum[1];
					p1[2]=z_arr[base+select_arr[k].hpos-1]-sum[2];
					e+=effect(u,m,ori,p1); 
					e=e*10;
#pragma acc atomic update
					effect_arr[k].x[index_arr[j].x1-1]+=e;
				}
			}
		}
	}

#pragma acc parallel loop collapse(2)
	for(ii=0; ii<select_size; ii++)
	{
		for(jj=0;jj<5;jj++)
		{
			effect_arr[ii].x[jj]/=nframe;
		}
	}

} // end data

	return;
}


void CTraj::getring(vector<struct ring_group> *index, vector<struct nh_group>* select, vector< vector<struct double_five> > *ring_effect)
{	
	int i,j,ii,jj,m;
	int base;
	int t[6];
	double p1[3],t1[3],t2[3],t3[3];
	double u[6][3];
	double sum[3];
	double ori[3];
	double e;
	struct double_five temp;
	
	for(i=0;i<5;i++)
		temp.x[i]=0;
	ring_effect->resize(select->size());
	for(ii=0;ii<(int)select->size();ii++)
	{
		for(jj=0;jj<nframe;jj++)
		{
			ring_effect->at(ii).push_back(temp);
		}
	}


	for(i=0;i<nframe;i++)
	{
		base=i*natom;  
		for(j=0;j<(int)index->size();j++)
		{
			switch(index->at(j).x1)
			{
				case 1:
                case 2:
                case 5:
					m=6;
					break;
				case 4:
				case 3:
					m=5;
					break;
			}
			t[0]=index->at(j).x2;
			t[1]=index->at(j).x3;
			t[2]=index->at(j).x4;
			t[3]=index->at(j).x5;
			t[4]=index->at(j).x6;
			t[5]=index->at(j).x7;

           
            for(ii=0;ii<m;ii++)
			{
				u[ii][0]=x[t[ii]+base-1];
                u[ii][1]=y[t[ii]+base-1];
                u[ii][2]=z[t[ii]+base-1];
            }
           
            for(jj=0;jj<3;jj++)
				sum[jj]=0; 
			for(ii=0;ii<m;ii++)
            {
				for(jj=0;jj<3;jj++)
				{
					sum[jj]+=u[ii][jj];
				}
			}
			for(jj=0;jj<3;jj++)
				sum[jj]/=m; 
			for(ii=0;ii<m;ii++)
			{
				for(jj=0;jj<3;jj++)
					u[ii][jj]-=sum[jj];
			}

			ring(u,m,ori);

            for(jj=0;jj<3;jj++)
            {
                   t1[jj]=u[0][jj]-u[1][jj];
                   t2[jj]=u[2][jj]-u[1][jj];
            }
            cross(t3,t1,t2);
            if(dot(t3,ori)<0)
            {
				for(jj=0;jj<3;jj++)
					ori[jj]=-ori[jj];
			}

			for(ii=0;ii<(int)select->size();ii++)
			{
				if(select->at(ii).hpos>=1)
				{
					e=0;
					p1[0]=x[base+select->at(ii).hpos-1]-sum[0];
					p1[1]=y[base+select->at(ii).hpos-1]-sum[1];
					p1[2]=z[base+select->at(ii).hpos-1]-sum[2];
					e+=effect(u,m,ori,p1); 
					e=e*10;
					ring_effect->at(ii).at(i).x[index->at(j).x1-1]+=e;
				}
			}
		}
	}

	return;
}



void CTraj::getring(vector<struct ring_group> *index, vector<struct methyl_group>* select, vector<struct double_five> *ring_effect, enum methyl c)
{	
	int i,j,ii,jj,m,k;
	int base;
	int t[6];
	double p1[3],t1[3],t2[3],t3[3];
	double u[6][3];
	double sum[3];
	double ori[3];
	double e;
	struct double_five temp;
	
	for(i=0;i<5;i++)
		temp.x[i]=0;
	for(i=0;i<(int)select->size();i++)
		ring_effect->push_back(temp);


	for(i=0;i<nframe;i++)
	{
		base=i*natom;  
		for(j=0;j<(int)index->size();j++)
		{
			switch(index->at(j).x1)
			{
				case 1:
                case 2:
                case 5:
					m=6;
					break;
				case 4:
				case 3:
					m=5;
					break;
			}
			t[0]=index->at(j).x2;
			t[1]=index->at(j).x3;
			t[2]=index->at(j).x4;
			t[3]=index->at(j).x5;
			t[4]=index->at(j).x6;
			t[5]=index->at(j).x7;

           
            for(ii=0;ii<m;ii++)
			{
				u[ii][0]=x[t[ii]+base-1];
                u[ii][1]=y[t[ii]+base-1];
                u[ii][2]=z[t[ii]+base-1];
            }
           
            for(jj=0;jj<3;jj++)
				sum[jj]=0; 
			for(ii=0;ii<m;ii++)
            {
				for(jj=0;jj<3;jj++)
				{
					sum[jj]+=u[ii][jj];
				}
			}
			for(jj=0;jj<3;jj++)
				sum[jj]/=m; 
			for(ii=0;ii<m;ii++)
			{
				for(jj=0;jj<3;jj++)
					u[ii][jj]-=sum[jj];
			}

			ring(u,m,ori);

            for(jj=0;jj<3;jj++)
            {
                   t1[jj]=u[0][jj]-u[1][jj];
                   t2[jj]=u[2][jj]-u[1][jj];
            }
            cross(t3,t1,t2);
            if(dot(t3,ori)<0)
            {
				for(jj=0;jj<3;jj++)
					ori[jj]=-ori[jj];
			}

			for(ii=0;ii<(int)select->size();ii++)
			{
				e=0;

				switch(c)
				{
				case hydrogen:
					for(k=0;k<3;k++)
					{
						p1[0]=x[base+select->at(ii).hpos[k]-1]-sum[0];
						p1[1]=y[base+select->at(ii).hpos[k]-1]-sum[1];
						p1[2]=z[base+select->at(ii).hpos[k]-1]-sum[2];
						//cout<<"Ori is "<<ori[0]<<" "<<ori[1]<<" "<<ori[2]<<endl;
						//cout<<"X is "<<p1[0]<<" "<<p1[1]<<" "<<p1[2]<<endl;
						e+=effect(u,m,ori,p1); 
					}
					break;
				case carbon:
					p1[0]=x[base+select->at(ii).cpos-1]-sum[0];
					p1[1]=y[base+select->at(ii).cpos-1]-sum[1];
					p1[2]=z[base+select->at(ii).cpos-1]-sum[2];
					e+=effect(u,m,ori,p1); 
					break;
				}

				e=e*10;
				ring_effect->at(ii).x[index->at(j).x1-1]+=e;
			}
		}
	}

	for(ii=0;ii<(int)select->size();ii++)
	{
		for(jj=0;jj<5;jj++)
		{
			ring_effect->at(ii).x[jj]/=nframe;
		}
	}
	return;
}

void CTraj::getring(vector<struct ring_group> *index, vector<struct proton>* select, vector<struct double_five> *ring_effect)
{	
	int i,j,ii,jj,m,k;
	int base;
	int t[6];
	double p1[3],t1[3],t2[3],t3[3];
	double u[6][3];
	double sum[3];
	double ori[3];
	double e;

	ring_group* index_arr = index->data();
	int index_size = index->size();
	proton* select_arr = select->data();
	int select_size = select->size();
	ring_effect->resize(select_size);
	double_five* effect_arr = ring_effect->data();
	int effect_size = ring_effect->size();
	double* x_arr = this->x.data();
	int x_size = this->x.size();
	double* y_arr = this->y.data();
	int y_size = this->y.size();
	double* z_arr = this->z.data();
	int z_size = this->z.size();

	int nframe = this->nframe;
	int natom = this->natom;

#pragma acc data \
	copyin(index_arr[:index_size], select_arr[:select_size], \
		x_arr[:x_size], y_arr[:y_size], z_arr[:z_size]) \
	copyout(effect_arr[:effect_size])
{

#pragma acc parallel loop collapse(2)
	for(i=0; i<select_size; i++)
		for(j=0; j<5; j++)
			effect_arr[i].x[j]=0.0;

	for(i=0;i<nframe;i++)
	{
		base=i*natom;  

#pragma acc parallel loop gang \
	private(m,t[:6],u[:6][:3],sum[:3],ori[:3],t1[:3],t2[:3],t3[:3])
		for(j=0; j<index_size; j++)
		{
			if(index_arr[j].x1 == 1 || index_arr[j].x1 == 2 || index_arr[j].x1 == 5){
				m=6;
			} else {
				m=5;
			}

			t[0]=index_arr[j].x2;
			t[1]=index_arr[j].x3;
			t[2]=index_arr[j].x4;
			t[3]=index_arr[j].x5;
			t[4]=index_arr[j].x6;
			t[5]=index_arr[j].x7;

			u[0][0]=x_arr[t[0]+base-1]; u[0][1]=y_arr[t[0]+base-1]; u[0][2]=z_arr[t[0]+base-1];
			u[1][0]=x_arr[t[1]+base-1]; u[1][1]=y_arr[t[1]+base-1]; u[1][2]=z_arr[t[1]+base-1];
			u[2][0]=x_arr[t[2]+base-1]; u[2][1]=y_arr[t[2]+base-1]; u[2][2]=z_arr[t[2]+base-1];
			u[3][0]=x_arr[t[3]+base-1]; u[3][1]=y_arr[t[3]+base-1]; u[3][2]=z_arr[t[3]+base-1];
			u[4][0]=x_arr[t[4]+base-1]; u[4][1]=y_arr[t[4]+base-1]; u[4][2]=z_arr[t[4]+base-1];
			if(m >= 6) {
				u[5][0]=x_arr[t[5]+base-1]; u[5][1]=y_arr[t[5]+base-1]; u[5][2]=z_arr[t[5]+base-1];
			} else {
				u[5][0]=0; u[5][1]=0; u[5][2]=0;
			}

			sum[0] = (u[0][0] + u[1][0] + u[2][0] + u[3][0] + u[4][0] + u[5][0]) / m;
			sum[1] = (u[0][1] + u[1][1] + u[2][1] + u[3][1] + u[4][1] + u[5][1]) / m;
			sum[2] = (u[0][2] + u[1][2] + u[2][2] + u[3][2] + u[4][2] + u[5][2]) / m;

			u[0][0] -= sum[0]; u[0][1] -= sum[1]; u[0][2] -= sum[2];
			u[1][0] -= sum[0]; u[1][1] -= sum[1]; u[1][2] -= sum[2];
			u[2][0] -= sum[0]; u[2][1] -= sum[1]; u[2][2] -= sum[2];
			u[3][0] -= sum[0]; u[3][1] -= sum[1]; u[3][2] -= sum[2];
			u[4][0] -= sum[0]; u[4][1] -= sum[1]; u[4][2] -= sum[2];
			u[5][0] -= sum[0]; u[5][1] -= sum[1]; u[5][2] -= sum[2];

			ring(u,m,ori);

			t1[0] = u[0][0] - u[1][0];
			t1[1] = u[0][1] - u[1][1];
			t1[2] = u[0][2] - u[1][2];
			t2[0] = u[2][0] - u[1][0];
			t2[1] = u[2][1] - u[1][1];
			t2[2] = u[2][2] - u[1][2];

			cross(t3,t1,t2);
			if(dot(t3,ori)<0)
			{
				ori[0] = -ori[0];
				ori[1] = -ori[1];
				ori[2] = -ori[2];
			}

#pragma acc loop vector private(k,e,p1[:3])
			for(ii=0; ii<select_size; ii++)
			{
				e=0;
#pragma acc loop seq
				for(k=0; k<select_arr[ii].nh; k++)
				{
					if(select_arr[ii].hpos[k] >= 0) {
						p1[0]=x_arr[base+select_arr[ii].hpos[k]-1]-sum[0];
						p1[1]=y_arr[base+select_arr[ii].hpos[k]-1]-sum[1];
						p1[2]=z_arr[base+select_arr[ii].hpos[k]-1]-sum[2];
					}
					e+=effect(u,m,ori,p1); 
				}
				e=e*10*3/select_arr[ii].nh;
#pragma acc atomic update
				effect_arr[ii].x[index_arr[j].x1-1]+=e;
			}
		}
	}

#pragma acc parallel loop collapse(2)
	for(ii=0; ii<select_size; ii++)
	{
		for(jj=0;jj<5;jj++)
		{
			effect_arr[ii].x[jj]/=nframe;
		}
	}

} // end data

	return;
}



void CTraj::getring(vector<struct ring_group> *index, vector<struct proton>* select, vector< vector<struct double_five> > *ring_effect)
{	
	int i,j,ii,jj,m,k;
	int base;
	int t[6];
	double p1[3],t1[3],t2[3],t3[3];
	double u[6][3];
	double sum[3];
	double ori[3];
	double e;
	struct double_five temp;
	
	for(i=0;i<5;i++)
		temp.x[i]=0;
	ring_effect->resize(select->size());
	for(ii=0;ii<(int)select->size();ii++)
	{
		for(jj=0;jj<nframe;jj++)
		{
			ring_effect->at(ii).push_back(temp);
		}
	}


	for(i=0;i<nframe;i++)
	{
		base=i*natom;  
		for(j=0;j<(int)index->size();j++)
		{
			switch(index->at(j).x1)
			{
				case 1:
                case 2:
                case 5:
					m=6;
					break;
				case 4:
				case 3:
					m=5;
					break;
			}
			t[0]=index->at(j).x2;
			t[1]=index->at(j).x3;
			t[2]=index->at(j).x4;
			t[3]=index->at(j).x5;
			t[4]=index->at(j).x6;
			t[5]=index->at(j).x7;

           
            for(ii=0;ii<m;ii++)
			{
				u[ii][0]=x[t[ii]+base-1];
                u[ii][1]=y[t[ii]+base-1];
                u[ii][2]=z[t[ii]+base-1];
            }
           
            for(jj=0;jj<3;jj++)
				sum[jj]=0; 
			for(ii=0;ii<m;ii++)
            {
				for(jj=0;jj<3;jj++)
				{
					sum[jj]+=u[ii][jj];
				}
			}
			for(jj=0;jj<3;jj++)
				sum[jj]/=m; 
			for(ii=0;ii<m;ii++)
			{
				for(jj=0;jj<3;jj++)
					u[ii][jj]-=sum[jj];
			}

			ring(u,m,ori);

            for(jj=0;jj<3;jj++)
            {
                   t1[jj]=u[0][jj]-u[1][jj];
                   t2[jj]=u[2][jj]-u[1][jj];
            }
            cross(t3,t1,t2);
            if(dot(t3,ori)<0)
            {
				for(jj=0;jj<3;jj++)
					ori[jj]=-ori[jj];
			}

			for(ii=0;ii<(int)select->size();ii++)
			{
				e=0;	
				for(k=0;k<(int)select->at(ii).nh;k++)
				{
					p1[0]=x[base+select->at(ii).hpos[k]-1]-sum[0];
					p1[1]=y[base+select->at(ii).hpos[k]-1]-sum[1];
					p1[2]=z[base+select->at(ii).hpos[k]-1]-sum[2];
					e+=effect(u,m,ori,p1); 
				}
				e=e*10*3/select->at(ii).nh;
				ring_effect->at(ii).at(i).x[index->at(j).x1-1]+=e;
			}
		}
	}
	return;
}


void CTraj::getring_bb(vector<struct ring_group> *index, vector<struct bb_group>* select, vector<struct double_five> *ring_effect, enum bb_carbon c)
{	
	int i,j,ii,jj,m;
	int base;
	int t[6];
	double p1[3],t1[3],t2[3],t3[3];
	double u[6][3];
	double sum[3];
	double ori[3];
	double e;
	struct double_five temp;
	
	for(i=0;i<5;i++)
		temp.x[i]=0;
	for(i=0;i<(int)select->size();i++)
		ring_effect->push_back(temp);


	for(i=0;i<nframe;i++)
	{
		base=i*natom;  
		for(j=0;j<(int)index->size();j++)
		{
			switch(index->at(j).x1)
			{
				case 1:
                case 2:
                case 5:
					m=6;
					break;
				case 4:
				case 3:
					m=5;
					break;
			}
			t[0]=index->at(j).x2;
			t[1]=index->at(j).x3;
			t[2]=index->at(j).x4;
			t[3]=index->at(j).x5;
			t[4]=index->at(j).x6;
			t[5]=index->at(j).x7;

           
            for(ii=0;ii<m;ii++)
			{
				u[ii][0]=x[t[ii]+base-1];
                u[ii][1]=y[t[ii]+base-1];
                u[ii][2]=z[t[ii]+base-1];
            }
           
            for(jj=0;jj<3;jj++)
				sum[jj]=0; 
			for(ii=0;ii<m;ii++)
            {
				for(jj=0;jj<3;jj++)
				{
					sum[jj]+=u[ii][jj];
				}
			}
			for(jj=0;jj<3;jj++)
				sum[jj]/=m; 
			for(ii=0;ii<m;ii++)
			{
				for(jj=0;jj<3;jj++)
					u[ii][jj]-=sum[jj];
			}

			ring(u,m,ori);

            for(jj=0;jj<3;jj++)
            {
                   t1[jj]=u[0][jj]-u[1][jj];
                   t2[jj]=u[2][jj]-u[1][jj];
            }
            cross(t3,t1,t2);
            if(dot(t3,ori)<0)
            {
				for(jj=0;jj<3;jj++)
					ori[jj]=-ori[jj];
			}

			for(ii=0;ii<(int)select->size();ii++)
			{
				e=0;

				if((select->at(ii).capos>=1 && c==bb_ca) || (select->at(ii).cbpos>=1 && c==bb_cb)||(select->at(ii).copos>=1 && c==bb_co))
				switch (c)
				{
				case bb_ca:
					p1[0]=x[base+select->at(ii).capos-1]-sum[0];
					p1[1]=y[base+select->at(ii).capos-1]-sum[1];
					p1[2]=z[base+select->at(ii).capos-1]-sum[2];
					break;
				case bb_cb:
					p1[0]=x[base+select->at(ii).cbpos-1]-sum[0];
					p1[1]=y[base+select->at(ii).cbpos-1]-sum[1];
					p1[2]=z[base+select->at(ii).cbpos-1]-sum[2];
					break;
				case bb_co:
					p1[0]=x[base+select->at(ii).copos-1]-sum[0];
					p1[1]=y[base+select->at(ii).copos-1]-sum[1];
					p1[2]=z[base+select->at(ii).copos-1]-sum[2];
					break;
				}
				e+=effect(u,m,ori,p1); 
				e=e*10;
				ring_effect->at(ii).x[index->at(j).x1-1]+=e;
			}
		}
	}

	for(ii=0;ii<(int)select->size();ii++)
	{
		for(jj=0;jj<5;jj++)
		{
			ring_effect->at(ii).x[jj]/=nframe;
		}
	}
	return;
}



void CTraj::getani(vector<struct ani_group> *index, vector<struct methyl_group>* select, vector<struct double_four> *ani_effect, enum methyl c)
{
	int i,j,ii,jj,k;
	int i1,i2,i3;
	int base;
	double center[3];
	double v1[3];
	double v2[3];
	double ori[3];
	double cosa;
	double length;
	double e;


	struct double_four temp;
	
	for(i=0;i<4;i++)
		temp.x[i]=0;
	for(i=0;i<(int)select->size();i++)
		ani_effect->push_back(temp);
	
	for(i=0;i<nframe;i++)
	{
		base=i*natom;
		for(j=0;j<(int)index->size();j++)
		{
			i1=index->at(j).pos[0]+base-1;
			i2=index->at(j).pos[1]+base-1;
			i3=index->at(j).pos[2]+base-1;
			center[0]=(x[i1]+x[i2]+x[i3])/3;
			center[1]=(y[i1]+y[i2]+y[i3])/3;
			center[2]=(z[i1]+z[i2]+z[i3])/3;

			v1[0]=x[i1]-x[i2];
			v1[1]=y[i1]-y[i2];
			v1[2]=z[i1]-z[i2];

			v2[0]=x[i3]-x[i2];
			v2[1]=y[i3]-y[i2];
			v2[2]=z[i3]-z[i2];

			cross(ori,v1,v2);

			for(jj=0;jj<(int)select->size();jj++)
			{
				e=0;				
				switch(c)
				{
				case hydrogen:
					for(k=0;k<3;k++)
					{
						i1=base+select->at(jj).cpos-1+k+1;
						v1[0]=center[0]-x[i1];
						v1[1]=center[1]-y[i1];
						v1[2]=center[2]-z[i1];
						length=v1[0]*v1[0]+v1[1]*v1[1]+v1[2]*v1[2];
						cosa=dot(v1,ori);
						cosa/=sqrt(ori[0]*ori[0]+ori[1]*ori[1]+ori[2]*ori[2]);
						cosa/=sqrt(length);					 
						e+=(1-3*cosa*cosa)/(length*sqrt(length));
					}
					ani_effect->at(jj).x[index->at(j).type-1]+=e/3.0*1000;
					break;

				case carbon:
					i1=base+select->at(jj).cpos-1;
					v1[0]=center[0]-x[i1];
					v1[1]=center[1]-y[i1];
					v1[2]=center[2]-z[i1];
					length=v1[0]*v1[0]+v1[1]*v1[1]+v1[2]*v1[2];
					cosa=dot(v1,ori);
					cosa/=sqrt(ori[0]*ori[0]+ori[1]*ori[1]+ori[2]*ori[2]);
					cosa/=sqrt(length);					 
					e+=(1-3*cosa*cosa)/(length*sqrt(length));
					ani_effect->at(jj).x[index->at(j).type-1]+=e*1000;
					break;
				}
			}
		}
	}

	for(ii=0;ii<(int)select->size();ii++)
	{
		for(jj=0;jj<4;jj++)
		{
			ani_effect->at(ii).x[jj]/=nframe;
		}
	}
	return;
}


void CTraj::getani(vector<struct ani_group> *index, vector<struct proton>* select, vector<struct double_four> *ani_effect)
{
	int i,j,ii,jj,k;
	int i1,i2,i3,i4;
	int base;
	double center[3];
	double v1[3];
	double v2[3];
	double v3[3];
	double ori[3];
	double cosa;
	double length;
	double e;

	ani_group* index_arr = index->data();
	int index_size = index->size();
	proton* select_arr = select->data();
	int select_size = select->size();
	ani_effect->resize(select_size);
	double_four* effect_arr = ani_effect->data();
	int effect_size = ani_effect->size();
	double* x_arr = this->x.data();
	int x_size = this->x.size();
	double* y_arr = this->y.data();
	int y_size = this->y.size();
	double* z_arr = this->z.data();
	int z_size = this->z.size();

	int nframe = this->nframe;
	int natom = this->natom;

#pragma acc data \
	copyin(index_arr[:index_size], select_arr[:select_size], \
		x_arr[:x_size], y_arr[:y_size], z_arr[:z_size]) \
	copyout(effect_arr[:effect_size])
{

#pragma acc parallel loop collapse(2)
	for(i=0;i<select_size;i++)
		for(j=0;j<4;j++)
			effect_arr[i].x[j]=0.0;
	
	for(i=0;i<nframe;i++)
	{
		base=i*natom;
#pragma acc parallel loop gang private(i1,i2,i3,center[:3], \
	v1[:3],v2[:3],ori[:3])
		for(j=0; j<index_size; j++)
		{
			i1=index_arr[j].pos[0]+base-1;
			i2=index_arr[j].pos[1]+base-1;
			i3=index_arr[j].pos[2]+base-1;
			center[0]=(x_arr[i1]+x_arr[i2]+x_arr[i3])/3;
			center[1]=(y_arr[i1]+y_arr[i2]+y_arr[i3])/3;
			center[2]=(z_arr[i1]+z_arr[i2]+z_arr[i3])/3;

			v1[0]=x_arr[i1]-x_arr[i2];
			v1[1]=y_arr[i1]-y_arr[i2];
			v1[2]=z_arr[i1]-z_arr[i2];

			v2[0]=x_arr[i3]-x_arr[i2];
			v2[1]=y_arr[i3]-y_arr[i2];
			v2[2]=z_arr[i3]-z_arr[i2];

			cross(ori,v1,v2);

#pragma acc loop vector private(k,e,i4,v3[:3],length,cosa)
			for(jj=0; jj<select_size; jj++)
			{
				e=0;		
#pragma acc loop seq		
				for(k=0; k<select_arr[jj].nh; k++)
				{
					i4=base+select_arr[jj].hpos[k]-1;
					v3[0]=center[0]-x_arr[i4];
					v3[1]=center[1]-y_arr[i4];
					v3[2]=center[2]-z_arr[i4];
					length=v3[0]*v3[0]+v3[1]*v3[1]+v3[2]*v3[2];
					cosa=dot(v3,ori);
					cosa/=sqrt(ori[0]*ori[0]+ori[1]*ori[1]+ori[2]*ori[2]);
					cosa/=sqrt(length);		 
					e+=(1-3*cosa*cosa)/(length*sqrt(length));
				}
#pragma acc atomic update
				effect_arr[jj].x[index_arr[j].type-1]+=e/select_arr[jj].nh*1000;
			}
		}
	}

#pragma acc parallel loop collapse(2)
	for(ii=0; ii<select_size; ii++)
	{
		for(jj=0; jj<4; jj++)
		{
			effect_arr[ii].x[jj]/=nframe;
		}
	}

} // end data

	return;
}


void CTraj::getani(vector<struct ani_group> *index, vector<struct proton>* select, vector< vector<struct double_four>  > *ani_effect)
{
	int i,j,ii,jj,k;
	int i1,i2,i3;
	int base;
	double center[3];
	double v1[3];
	double v2[3];
	double ori[3];
	double cosa;
	double length;
	double e;


	struct double_four temp;
	
	for(i=0;i<4;i++)
		temp.x[i]=0;
	ani_effect->resize(select->size());
	for(ii=0;ii<(int)select->size();ii++)
	{
		for(jj=0;jj<nframe;jj++)
		{
			ani_effect->at(ii).push_back(temp);
		}
	}

	
	for(i=0;i<nframe;i++)
	{
		base=i*natom;
		for(j=0;j<(int)index->size();j++)
		{
			i1=index->at(j).pos[0]+base-1;
			i2=index->at(j).pos[1]+base-1;
			i3=index->at(j).pos[2]+base-1;
			center[0]=(x[i1]+x[i2]+x[i3])/3;
			center[1]=(y[i1]+y[i2]+y[i3])/3;
			center[2]=(z[i1]+z[i2]+z[i3])/3;

			v1[0]=x[i1]-x[i2];
			v1[1]=y[i1]-y[i2];
			v1[2]=z[i1]-z[i2];

			v2[0]=x[i3]-x[i2];
			v2[1]=y[i3]-y[i2];
			v2[2]=z[i3]-z[i2];

			cross(ori,v1,v2);

			for(jj=0;jj<(int)select->size();jj++)
			{
				e=0;				
				for(k=0;k<(int)select->at(jj).nh;k++)
				{
					i1=base+select->at(jj).hpos[k]-1;
					v1[0]=center[0]-x[i1];
					v1[1]=center[1]-y[i1];
					v1[2]=center[2]-z[i1];
					length=v1[0]*v1[0]+v1[1]*v1[1]+v1[2]*v1[2];
					cosa=dot(v1,ori);
					cosa/=sqrt(ori[0]*ori[0]+ori[1]*ori[1]+ori[2]*ori[2]);
					cosa/=sqrt(length);					 
					e+=(1-3*cosa*cosa)/(length*sqrt(length));
				}
				ani_effect->at(jj).at(i).x[index->at(j).type-1]+=e/select->at(jj).nh*1000;
			}
		}
	}


	return;
}


void CTraj::getani(vector<struct ani_group> *index, vector<struct nh_group>* select, vector<struct double_four> *ani_effect)
{
	int i,j,ii,k;
	int i1,i2,i3,i4;
	int base;
	double center[3];
	double v1[3];
	double v2[3];
	double v3[3];
	double ori[3];
	double cosa;
	double length;
	double e;

	ani_group* index_arr = index->data();
	int index_size = index->size();
	nh_group* select_arr = select->data();
	int select_size = select->size();
	ani_effect->resize(select_size);
	double_four* effect_arr = ani_effect->data();
	int effect_size = ani_effect->size();
	double* x_arr = this->x.data();
	int x_size = this->x.size();
	double* y_arr = this->y.data();
	int y_size = this->y.size();
	double* z_arr = this->z.data();
	int z_size = this->z.size();

	int nframe = this->nframe;
	int natom = this->natom;

#pragma acc data \
	copyin(index_arr[:index_size], select_arr[:select_size], \
		x_arr[:x_size], y_arr[:y_size], z_arr[:z_size]) \
	copyout(effect_arr[:effect_size])
{

#pragma acc parallel loop collapse(2)
	for(i=0;i<select_size;i++)
		for(j=0;j<4;j++)
			effect_arr[i].x[j]=0.0;

	for(i=0;i<nframe;i++)
	{
		base=i*natom;
#pragma acc parallel loop gang \
	private(i1,i2,i3,center[:3],v1[:3],v2[:3],ori[:3])
		for(j=0; j<index_size; j++)
		{
			i1=index_arr[j].pos[0]+base-1;
			i2=index_arr[j].pos[1]+base-1;
			i3=index_arr[j].pos[2]+base-1;
			center[0]=(x_arr[i1]+x_arr[i2]+x_arr[i3])/3;
			center[1]=(y_arr[i1]+y_arr[i2]+y_arr[i3])/3;
			center[2]=(z_arr[i1]+z_arr[i2]+z_arr[i3])/3;

			v1[0]=x_arr[i1]-x_arr[i2];
			v1[1]=y_arr[i1]-y_arr[i2];
			v1[2]=z_arr[i1]-z_arr[i2];

			v2[0]=x_arr[i3]-x_arr[i2];
			v2[1]=y_arr[i3]-y_arr[i2];
			v2[2]=z_arr[i3]-z_arr[i2];

			cross(ori,v1,v2);

#pragma acc loop vector private(e,i4,v3[:3],length,cosa)
			for(k=0; k<select_size; k++)
			{
				if(select_arr[k].hpos>=1)
				{
					e=0;				
					i4=base+select_arr[k].hpos-1;
					v3[0]=center[0]-x_arr[i4];
					v3[1]=center[1]-y_arr[i4];
					v3[2]=center[2]-z_arr[i4];
					length=v3[0]*v3[0]+v3[1]*v3[1]+v3[2]*v3[2];
					cosa=dot(v3,ori);
					cosa/=sqrt(ori[0]*ori[0]+ori[1]*ori[1]+ori[2]*ori[2]);
					cosa/=sqrt(length);					 
					e+=(1-3*cosa*cosa)/(length*sqrt(length));
#pragma acc atomic update
					effect_arr[k].x[index_arr[j].type-1]+=e*1000;
				}
			}
		}
	}

#pragma acc parallel loop collapse(2)
	for(i=0; i<select_size; i++)
		for(j=0; j<4; j++)
			effect_arr[i].x[j]/=nframe;

} // end data

	return;
}


void CTraj::do_rmsf(void)
{
	int i,j;
	int jump;
	double meanx,meany,meanz;
	double rms;
	double t,tt;

	rmsf.clear();
	for(i=0;i<natom;i++)
	{
		meanx=meany=meanz=0.0;
		for(j=0;j<nframe;j++)
		{
			jump=j*natom+i;
			meanx+=x.at(jump);
			meany+=y.at(jump);
			meanz+=z.at(jump);
		}
		meanx/=nframe;
		meany/=nframe;
		meanz/=nframe;

		rms=0.0;
		for(j=0;j<nframe;j++)
		{
			jump=j*nframe+i;
			t=x.at(jump)-meanx;tt=t*t;
			t=y.at(jump)-meany;tt+=t*t;
			t=z.at(jump)-meanz;tt+=t*t;
			rms+=tt;
		}
		rms=sqrt(rms/nframe);
		rmsf.push_back(rms);
	}

	return;
}



void CTraj::getani(vector<struct ani_group> *index, vector<struct nh_group>* select, vector< vector<struct double_four>  > *ani_effect)
{
	int i,j,ii,jj;
	int i1,i2,i3;
	int base;
	double center[3];
	double v1[3];
	double v2[3];
	double ori[3];
	double cosa;
	double length;
	double e;


	struct double_four temp;
	
	for(i=0;i<4;i++)
		temp.x[i]=0;
	ani_effect->resize(select->size());
	for(ii=0;ii<(int)select->size();ii++)
	{
		for(jj=0;jj<nframe;jj++)
		{
			ani_effect->at(ii).push_back(temp);
		}
	}
	
	for(i=0;i<nframe;i++)
	{
		base=i*natom;
		for(j=0;j<(int)index->size();j++)
		{
			i1=index->at(j).pos[0]+base-1;
			i2=index->at(j).pos[1]+base-1;
			i3=index->at(j).pos[2]+base-1;
			center[0]=(x[i1]+x[i2]+x[i3])/3;
			center[1]=(y[i1]+y[i2]+y[i3])/3;
			center[2]=(z[i1]+z[i2]+z[i3])/3;

			v1[0]=x[i1]-x[i2];
			v1[1]=y[i1]-y[i2];
			v1[2]=z[i1]-z[i2];

			v2[0]=x[i3]-x[i2];
			v2[1]=y[i3]-y[i2];
			v2[2]=z[i3]-z[i2];

			cross(ori,v1,v2);

			for(jj=0;jj<(int)select->size();jj++)
			{
				if(select->at(jj).hpos>=1)
				{
					e=0;				
					i1=base+select->at(jj).hpos-1;
					v1[0]=center[0]-x[i1];
					v1[1]=center[1]-y[i1];
					v1[2]=center[2]-z[i1];
					length=v1[0]*v1[0]+v1[1]*v1[1]+v1[2]*v1[2];
					cosa=dot(v1,ori);
					cosa/=sqrt(ori[0]*ori[0]+ori[1]*ori[1]+ori[2]*ori[2]);
					cosa/=sqrt(length);					 
					e+=(1-3*cosa*cosa)/(length*sqrt(length));
					ani_effect->at(jj).at(i).x[index->at(j).type-1]+=e*1000;
				}
			}
		}
	}

	return;
}

double CTraj::noedistance_frame(vector<int> *att1, vector<int> *att2,int j)
{

	double x0,y0,z0;
	double r2,r6,sum,sumsum;
	int i1,j1;
	int n1,n2;
	int jump;

	sumsum=0;
	for(i1=0;i1<(int)att1->size();i1++)
	for(j1=0;j1<(int)att2->size();j1++)
	{
		n1=att1->at(i1)-1;
		n2=att2->at(j1)-1;
		
		jump=j*natom;
		x0=x.at(jump+n1)-x.at(jump+n2);
		y0=y.at(jump+n1)-y.at(jump+n2);
		z0=z.at(jump+n1)-z.at(jump+n2);
		r2=x0*x0+y0*y0+z0*z0;
		r6=r2*r2*r2;
		sum=1.0/r6;
		sumsum+=sum;
	}
	sumsum/=i1;
	sumsum/=j1;
	r2=pow(sumsum,-1/6.0);
	return r2;
}

double CTraj::noedistance(vector<int> *att1, vector<int> *att2)
{
	double x0,y0,z0;
	double r2,r6,sum,sumsum;
	int i1,j1;
	int n1,n2;
	int j;
	int jump;

	sumsum=0;
	for(i1=0;i1<(int)att1->size();i1++)
	for(j1=0;j1<(int)att2->size();j1++)
	{
		n1=att1->at(i1)-1;
		n2=att2->at(j1)-1;

		sum=0.0;
		for(j=0;j<nframe;j++)
		{
			jump=j*natom;
			x0=x.at(jump+n1)-x.at(jump+n2);
			y0=y.at(jump+n1)-y.at(jump+n2);
			z0=z.at(jump+n1)-z.at(jump+n2);
			r2=x0*x0+y0*y0+z0*z0;
			r6=r2*r2*r2;
			sum+=1.0/r6;
		}
		sum/=nframe;
		sumsum+=sum;
	}
	sumsum/=i1;
	sumsum/=j1;
	r2=pow(sumsum,-1/6.0);
	return r2;
}


void CTraj::evaluatenmrcons_frame(vector<struct noeline> *nmrcons, double cutoff)
{
	int ii,i,j;
	int pos1,pos2;
	int n1,n2;
	vector<int> att1,att2;
	double d;
	ofstream fout("noe_frame.dat");


	for(ii=0;ii<nframe;ii++)
	{
		for(i=0;i<(int)nmrcons->size();i++)
		{	

			if(fabs((float)(nmrcons->at(i).resid1-nmrcons->at(i).resid2))<=2)
				continue;

			nmrcons->at(i).obs.clear();
			nmrcons->at(i).pos1.clear();
			nmrcons->at(i).pos2.clear();
			for(n1=0;n1<(int)nmrcons->at(i).index1.atoms.size();n1++)
			for(n2=0;n2<(int)nmrcons->at(i).index2.atoms.size();n2++)
			{
				att1=nmrcons->at(i).index1.atoms.at(n1);
				att2=nmrcons->at(i).index2.atoms.at(n2);
				nmrcons->at(i).obs.push_back(noedistance_frame(&att1,&att2,ii));
				nmrcons->at(i).pos1.push_back(att1.at(0));
				nmrcons->at(i).pos2.push_back(att2.at(0));
			}

			d=100000.0;
			pos1=pos2=0;
			for(j=0;j<nmrcons->at(i).obs.size();j++)
			{
				if(nmrcons->at(i).obs.at(j)<d)
				{
					d=nmrcons->at(i).obs.at(j);
					pos1=nmrcons->at(i).pos1.at(j);
					pos2=nmrcons->at(i).pos2.at(j);
				}
			}
			nmrcons->at(i).bvio=0;
			nmrcons->at(i).d=d;
			
			if(nmrcons->at(i).a+cutoff<d)
				fout<<d<<" ";
			else
				fout<<d<<" ";
		}
		fout<<endl;

	}
	fout.close();

	ofstream fout2("noe_frame_head.dat");
	for(i=0;i<(int)nmrcons->size();i++)
	{
		if(fabs((float)(nmrcons->at(i).resid1-nmrcons->at(i).resid2))<=2)
			continue;

		fout2<<nmrcons->at(i).resid1<<" ";
		fout2<<nmrcons->at(i).resname1<<" ";
		fout2<<nmrcons->at(i).atomname1<<" ";
		fout2<<nmrcons->at(i).resid2<<" ";
		fout2<<nmrcons->at(i).resname2<<" ";
		fout2<<nmrcons->at(i).atomname2<<" ";
		fout2<<nmrcons->at(i).b<<" ";
		fout2<<nmrcons->at(i).c<<" ";
		fout2<<nmrcons->at(i).a<<endl;
	}
	fout2.close();
	return;
}

void CTraj::evulatenmrcons(vector<struct noeline> *nmrcons, double cutoff)
{
	int i,j;
	int pos1,pos2;
	int n1,n2;
	vector<int> att1,att2;
	double d;
	ofstream fout("violation.dat");
	ofstream fout2("fullfilled.dat");
	ofstream fout3("grouped.dat");
	ofstream fchimera("for_chimera.dat");
	ofstream fchimera2("for_chimera2.dat");


	
	for(i=0;i<(int)nmrcons->size();i++)
	{	
		for(n1=0;n1<(int)nmrcons->at(i).index1.atoms.size();n1++)
		for(n2=0;n2<(int)nmrcons->at(i).index2.atoms.size();n2++)
		{
			att1=nmrcons->at(i).index1.atoms.at(n1);
			att2=nmrcons->at(i).index2.atoms.at(n2);
			nmrcons->at(i).obs.push_back(noedistance(&att1,&att2));
			nmrcons->at(i).pos1.push_back(att1.at(0));
			nmrcons->at(i).pos2.push_back(att2.at(0));
		}
	}

	for(i=0;i<(int)nmrcons->size();i++)
	{
		d=100000.0;
		pos1=pos2=0;
		for(j=0;j<nmrcons->at(i).obs.size();j++)
		{
			if(nmrcons->at(i).obs.at(j)<d)
			{
				d=nmrcons->at(i).obs.at(j);
				pos1=nmrcons->at(i).pos1.at(j);
				pos2=nmrcons->at(i).pos2.at(j);
			}
		}
		nmrcons->at(i).bvio=0;
		nmrcons->at(i).d=d;

		if(nmrcons->at(i).a+cutoff<d)
		{
			nmrcons->at(i).bvio=1;
			fout<<nmrcons->at(i).group<<" ";
			fout<<i<<" "<<nmrcons->at(i).resid1<<" "<<nmrcons->at(i).resname1<<" "<<nmrcons->at(i).atomname1<<" ";
			fout<<nmrcons->at(i).resid2<<" "<<nmrcons->at(i).resname2<<" "<<nmrcons->at(i).atomname2<<" ";
			fout<<d<<" "<<nmrcons->at(i).c<<" "<<nmrcons->at(i).a<<endl;
			if(abs(nmrcons->at(i).resid1-nmrcons->at(i).resid2)>2 && d-nmrcons->at(i).a>cutoff)
			{
				fchimera<<"distance @/serialNumber="<<pos1<<" @/serialNumber="<<pos2<<endl;
				fchimera<<"display :"<<nmrcons->at(i).resid1<<endl;
				fchimera<<"display :"<<nmrcons->at(i).resid2<<endl;
				fchimera2<<"distance :"<<nmrcons->at(i).resid1<<"@CA :"<<nmrcons->at(i).resid2<<"@CA"<<endl;
			}
		}
		else
		{		
			fout2<<nmrcons->at(i).group<<" ";
			fout2<<i<<" "<<nmrcons->at(i).resid1<<" "<<nmrcons->at(i).resname1<<" "<<nmrcons->at(i).atomname1<<" ";
			fout2<<nmrcons->at(i).resid2<<" "<<nmrcons->at(i).resname2<<" "<<nmrcons->at(i).atomname2<<" ";
			fout2<<d<<" "<<nmrcons->at(i).c<<" "<<nmrcons->at(i).a<<endl;
		}
	}

	int ngroup=nmrcons->at(i-1).group;
	int i1=0;
	int i2=0;


	for(i=0;i<=ngroup;i++)
	{
		for(;i1<nmrcons->size() && nmrcons->at(i1).group<=i;i1++)
		{
			if(nmrcons->at(i1).a+cutoff<nmrcons->at(i1).d)
			{
				fout3<<nmrcons->at(i1).group<<" ";
				fout3<<i1<<" "<<nmrcons->at(i1).resid1<<" "<<nmrcons->at(i1).resname1<<" "<<nmrcons->at(i1).atomname1<<" ";
				fout3<<nmrcons->at(i1).resid2<<" "<<nmrcons->at(i1).resname2<<" "<<nmrcons->at(i1).atomname2<<" ";
				fout3<<nmrcons->at(i1).d<<" "<<nmrcons->at(i1).c<<" "<<nmrcons->at(i1).a<<" VIOLATED "<<endl;
			}
		}
		

		for(;i2<nmrcons->size() && nmrcons->at(i2).group<=i;i2++)
		{
			if(nmrcons->at(i2).a+cutoff>=nmrcons->at(i2).d)
			{
				fout3<<nmrcons->at(i2).group<<" ";
				fout3<<i2<<" "<<nmrcons->at(i2).resid1<<" "<<nmrcons->at(i2).resname1<<" "<<nmrcons->at(i2).atomname1<<" ";
				fout3<<nmrcons->at(i2).resid2<<" "<<nmrcons->at(i2).resname2<<" "<<nmrcons->at(i2).atomname2<<" ";
				fout3<<nmrcons->at(i2).d<<" "<<nmrcons->at(i2).c<<" "<<nmrcons->at(i2).a<<endl;
			}
		}
		fout3<<endl;
	}
	
	fout.close();
	fout2.close();
	fout3.close();
	fchimera.close();
	fchimera2.close();


	return;
}


void CTraj::rmsd_matrix(vector< vector<double> > *rmsd,vector<int> *ca, int skip)
{
	float *x1,*y1,*z1,*x2,*y2,*z2;
	int nca,jumpi,jumpj;
	int i,j,k;
	vector< double> t;
	class CRmsd rmsdf;

	nca=ca->size();
	
	x1=new float[nframe*nca];
	x2=new float[nframe*nca];
	y1=new float[nframe*nca];
	y2=new float[nframe*nca];
	z1=new float[nframe*nca];
	z2=new float[nframe*nca];

	for(i=0;i<nframe;i++)
	{
		if((i+1)%skip==0)
		{
			
			t.clear();
			jumpi=i*natom-1;
			for(j=0;j<i;j++)
				t.push_back(rmsd->at(j).at(i));
			t.push_back(0.0);
			for(j=i+1;j<nframe;j++)
			{
				if((j+1)%skip==0)
				{
					jumpj=j*natom-1;
					for(k=0;k<nca;k++)
					{
						x1[k]=x[jumpi+ca->at(k)];
						y1[k]=y[jumpi+ca->at(k)];
						z1[k]=z[jumpi+ca->at(k)];

						x2[k]=x[jumpj+ca->at(k)];
						y2[k]=y[jumpj+ca->at(k)];
						z2[k]=z[jumpj+ca->at(k)];
					}

					t.push_back(rmsdf.calculate_rotation_rmsd(x1,y1,z1,x2,y2,z2,nca));
				}
			}
			rmsd->push_back(t);
		}
	}
	return;
}



void CTraj::getvector(vector<struct index_three> nh,vector<double> *xx,vector<double> *yy,vector<double> *zz)
{
	int i,m,j;
	double r;
	double x1,y1,z1;
	for(i=0;i<nframe;i++)
	{	
		m=i*natom-1;		
		for(j=0;j<nh.size();j++)
		{
			x1=x.at(m+nh.at(j).x3)-x.at(m+nh.at(j).x2);
			y1=y.at(m+nh.at(j).x3)-y.at(m+nh.at(j).x2);
			z1=z.at(m+nh.at(j).x3)-z.at(m+nh.at(j).x2);
			r=sqrt(x1*x1+y1*y1+z1*z1);
			if(r>0)
			{
				x1/=r;y1/=r;z1/=r;
			}	
			else
			{
				x1=1.0;y1=z1=0.0;
			}	
			xx->push_back(x1);
			yy->push_back(y1);
			zz->push_back(z1);		
		}
	}
}


void CTraj:: getcoor(int ipos,int iframe,double *xx,double *yy,double *zz)
{
	ipos+=iframe*natom-1;
	*xx=x.at(ipos);
	*yy=y.at(ipos);
	*zz=z.at(ipos);
}


void CTraj::getcoor(vector<int> pos,int iframe,vector<double> *xx,vector<double> *yy,vector<double> *zz)
{
	int j;
	int adj;
	unsigned int i;

	adj=iframe*natom-1;
	for(i=0;i<pos.size();i++)
	{
		j=pos.at(i)+adj;
		xx->push_back(x.at(j));
		yy->push_back(y.at(j));
		zz->push_back(z.at(j));
	}
	return;
}


void CTraj::getcoor(vector<int> pos,vector<float> *xx,vector<float> *yy,vector<float> *zz)
{
	int j;
	int adj;
	unsigned int i,iframe;

	for(iframe=0;iframe<nframe;iframe++)
	{
		adj=iframe*natom-1;
		for(i=0;i<pos.size();i++)
		{
			j=pos.at(i)+adj;
			xx->push_back(x.at(j));
			yy->push_back(y.at(j));
			zz->push_back(z.at(j));
		}
	}
	return;
}

void CTraj::getcoor(vector<int> pos,int iframe,vector<float> *xx,vector<float> *yy,vector<float> *zz)
{
	int j;
	int adj;
	unsigned int i;

	adj=iframe*natom-1;
	for(i=0;i<pos.size();i++)
	{
		j=pos.at(i)+adj;
		xx->push_back(x.at(j));
		yy->push_back(y.at(j));
		zz->push_back(z.at(j));
	}
	return;
}

void CTraj::get_contact(float rc,float shift, vector<int> pos, vector<int> used, vector<float> * result)
{
	int i,j;
	int ii,jj;
	float contact;
	float x0,y0,z0;
	float rr;

	for(i=0;i<(int)pos.size();i++)
	{
		contact=0.0;
		ii=pos.at(i);
		if(ii<1)
		{
			result->push_back(-1.0);
			continue;
		}
		ii--;
		x0=x.at(ii);
		y0=y.at(ii);
		z0=z.at(ii);
		for(j=0;j<(int)used.size();j++)
		{
			jj=used.at(j);
			if(jj<0)
				continue;
			jj--;
			rr=(x.at(jj)-x0)*(x.at(jj)-x0)+(y.at(jj)-y0)*(y.at(jj)-y0)+(z.at(jj)-z0)*(z.at(jj)-z0);
			rr=sqrt(rr)-shift;
			contact+=exp(-rr/rc);				
		}
		result->push_back(contact);
	}

	return;
}


void CTraj::get_contact(vector<int> pos, vector<int> used, vector<float> * result)
{
	int i,j;
	int ii,jj;
	float contact;
	float x0,y0,z0;
	float rr;

	for(i=0;i<(int)pos.size();i++)
	{
		contact=0.0;
		ii=pos.at(i);
		if(ii<0)
		{
			result->push_back(-1.0);
			continue;
		}
		ii--;
		x0=x.at(ii);
		y0=y.at(ii);
		z0=z.at(ii);
		for(j=0;j<(int)used.size();j++)
		{
			jj=used.at(j);
			if(jj<0)
				continue;
			jj--;
			rr=(x.at(jj)-x0)*(x.at(jj)-x0)+(y.at(jj)-y0)*(y.at(jj)-y0)+(z.at(jj)-z0)*(z.at(jj)-z0);
			rr=sqrt(rr);
			contact+=exp(-rr/3.0);				
		}
		result->push_back(contact);
	}

	return;
}

void CTraj::get_contacts(vector<struct bb_group> bb, vector<struct index_two> index, vector<int> c2, vector<float> *results)
{
	int i,j,p;
	int ii,jj;
	float contact;
	float x0,y0,z0;
	float rr;

	bb_group* bb_arr = bb.data();
	int bb_size = bb.size();
	index_two* index_arr = index.data();
	int index_size = index.size();
	int* c2_arr = c2.data();
	int c2_size = c2.size();
	results->resize(index_size*3);
	float* results_arr = results->data();
	int results_size = results->size();
	double* x_arr = this->x.data();
	int x_size = this->x.size();
	double* y_arr = this->y.data();
	int y_size = this->y.size();
	double* z_arr = this->z.data();
	int z_size = this->z.size();

#pragma acc data copyin(bb_arr[:bb_size], index_arr[:index_size], \
 c2_arr[:c2_size], x_arr[:x_size], y_arr[:y_size], z_arr[:z_size]) \
 copyout(results_arr[:results_size])
{

#pragma acc parallel loop gang collapse(2) private(ii,x0,y0,z0,contact)
	for(i=1; i<index_size-1; i++)
	{
  	for(p=0; p<3; p++)
		{
			if(p==0) {
					ii=bb_arr[index_arr[i].x1-1].capos;
			} else if(p==1) {
					ii=bb_arr[index_arr[i].x1-1].cbpos;
			} else {
					ii=bb_arr[index_arr[i].x1-1].copos;
			}

			if(ii < 1 || ii > x_size) {
				results_arr[i*3+p]=-1.0;
				continue;
			}

			contact=0.0;

			ii--;
			x0=x_arr[ii];
			y0=y_arr[ii];
			z0=z_arr[ii];

#pragma acc loop vector private(jj,rr) reduction(+:contact)
			for(j=0;j<c2_size;j++)
			{
				jj=c2_arr[j];
				if(jj<=1 || jj>x_size) continue;

				jj--;
				rr=(x_arr[jj]-x0)*(x_arr[jj]-x0)+(y_arr[jj]-y0)*(y_arr[jj]-y0)+(z_arr[jj]-z0)*(z_arr[jj]-z0);
				rr=sqrt(rr);
				contact+=exp(-rr/3.0);			
			}

			results_arr[i*3+p]=contact;

		}
	}

} // end data

	return;

}	



CTraj::CTraj()
{
	natom=0;
	nres=0;
	nframe=0;
};

CTraj::~CTraj()
{
  double *tmp = x.data();
#pragma acc exit data delete(tmp)
  tmp = y.data();
#pragma acc exit data delete(tmp)
  tmp = z.data();
#pragma acc exit data delete(tmp)
#pragma acc exit data delete(this)
};







void smtry::getdata(double x[3][3],double y[3])
{
	int i,j;
	for(i=0;i<3;i++)
	for(j=0;j<3;j++)
		matrix[i][j]=x[i][j];
	for(i=0;i<3;i++)
		motion[i]=y[i];
	return;
}

void smtry::trans(double x[3],double y[3])
{
	for(int i=0;i<3;i++)
		y[i]=x[0]*matrix[i][0]+x[1]*matrix[i][1]+x[2]*matrix[i][2]+motion[i];
	return;
}

void smtry::print()
{
	int j;
	for(int i=0;i<3;i++)
	{
		for(j=0;j<3;j++)
			cout<<matrix[i][j]<<" ";
		cout<<motion[i]<<endl;
	}
	return ;
}





void CTraj2::clear()
{
	x.clear();
	y.clear();
	z.clear();
	b.clear();
	crystal_sym_array.clear();
	nframe=0;
	natom=0;
	nres=0;
	memset(translation,0,sizeof(double)*9);
	memset(scale,0,sizeof(double)*9);
	btrans=0;
	bsmtry=0;
}

int CTraj2::select(vector<int> t)
{	
	int i,j;
	int nmiss=0;
	vector<double> xx,yy,zz,bb;
	for(i=0;i<(int)t.size();i++)
	{
		j=t.at(i)-1;
		if(j>=0)
		{
			xx.push_back(x.at(j));
			yy.push_back(y.at(j));
			zz.push_back(z.at(j));
			bb.push_back(b.at(j));
		}
		else
		{
			nmiss++;
		}
	}
	x=xx;
	y=yy;
	z=zz;
	b=bb;
	natom=x.size();
	return nmiss;
}


int CTraj2::loadcoor(string filename)
{
	double xx,yy,zz,bb;
	string line,part,part1,part2;
	double cx[3][3];
	double cy[3];
	class smtry crystal_sym;
	crystal_sym_array.clear();


	ifstream fin(filename.c_str());
	while(getline(fin,line))
	{
		part1.clear();
		part2.clear();

		if(line.size()>18)
		{
			part1=line.substr(0,10);
			part2=line.substr(13,5);
		}
		if(part1=="REMARK 290" && part2=="SMTRY")
		{
			bsmtry=1;
			cx[0][0]=atof(line.substr(24,9).c_str());
			cx[0][1]=atof(line.substr(34,9).c_str());
			cx[0][2]=atof(line.substr(44,9).c_str());
			cy[0]=atof(line.substr(59,9).c_str());
			getline(fin,line);
			cx[1][0]=atof(line.substr(24,9).c_str());
			cx[1][1]=atof(line.substr(34,9).c_str());
			cx[1][2]=atof(line.substr(44,9).c_str());
			cy[1]=atof(line.substr(59,9).c_str());
			getline(fin,line);
			cx[2][0]=atof(line.substr(24,9).c_str());
			cx[2][1]=atof(line.substr(34,9).c_str());
			cx[2][2]=atof(line.substr(44,9).c_str());
			cy[2]=atof(line.substr(59,9).c_str());
			
			if(!(cx[0][0]==1 && cx[1][1]==1 && cx[2][2]==1 && cx[0][1]==0 && cx[1][0]==0 && cx[0][2]==0 && cx[2][0]==0
						  && cx[1][2]==0 && cx[2][1]==0 && cy[0]==0 && cy[1]==0 && cy[2]==0))
			{
				crystal_sym.getdata(cx,cy);crystal_sym_array.push_back(crystal_sym);
			}
		}

		if(line.size()>5)
			part1=line.substr(0,5);
		else
			part1.clear();
		if(part1=="SCALE")
		{
			scale[0][0]=atof(line.substr(10,10).c_str());
			scale[0][1]=atof(line.substr(20,10).c_str());
			scale[0][2]=atof(line.substr(30,10).c_str());
			getline(fin,line);
			scale[1][0]=atof(line.substr(10,10).c_str());
			scale[1][1]=atof(line.substr(20,10).c_str());
			scale[1][2]=atof(line.substr(30,10).c_str());
			getline(fin,line);
			scale[2][0]=atof(line.substr(10,10).c_str());
			scale[2][1]=atof(line.substr(20,10).c_str());
			scale[2][2]=atof(line.substr(30,10).c_str());
			process_tran();
			btrans=1;
		}
		if(part1=="CRYST")
		{
			part1=line.substr(67,4);
			ncry=atoi(part1.c_str());
		}

		if(line.size()>6)
			part=line.substr(0,6);
		else
			part.clear();
		if(part=="ENDMDL" || part=="END")
		{
			//finished read first molecule 
			if(x.size()%natom!=0)
				cout<<"In traj reading, suppose to read "<<natom<<" coors but actually read in "<<x.size()<<endl;
		}


		part=line.substr(0,6);
		if(part!="ATOM  " && part!="HETATM")
			continue;

		part=line.substr(17,3);
		if(part=="HOH" || part=="WAT")
			continue;

		part=line.substr(30,8);
		xx=atof(part.c_str());
		x.push_back(xx);
		part=line.substr(38,8);
		yy=atof(part.c_str());
		y.push_back(yy);
		part=line.substr(46,8);
		zz=atof(part.c_str());
		z.push_back(zz);
		part=line.substr(60,6);
		bb=atof(part.c_str());
		b.push_back(bb);
		part=line.substr(12,4);
		atomname.push_back(part);
	};

	if(natom==0)
		nframe=0;
	else
		nframe=x.size()/natom;
	return nframe;
}

void CTraj2::process_tran()
{
	if(scale[1][0]!=0 || scale[2][0]!=0 || scale[2][1]!=0)
		return;
		
	translation[0][0]=1/scale[0][0];
	translation[0][1]=-scale[0][1]/(scale[1][1]*scale[0][0]);
	translation[0][2]=(scale[0][1]*scale[1][2]/scale[1][1]-scale[0][2])/(scale[0][0]*scale[2][2]);

	translation[1][0]=0;
	translation[1][1]=1/scale[1][1];
	translation[1][2]=-scale[1][2]/(scale[1][1]*scale[2][2]);

	translation[2][0]=0;
	translation[2][1]=0;
	translation[2][2]=1/scale[2][2];
	
	return;
}



int CTraj2::unitcell(void)
{
	class smtry crystal_sym;
	double xmean,ymean,zmean;
	double xscale,yscale,zscale;
	double xtran,ytran,ztran;
	double yold[3];
	double ynew[3];
	int n;
	int i,j;


	n=x.size();
	for(i=0;i<(int)crystal_sym_array.size();i++)
	{
		xmean=ymean=zmean=0;
		crystal_sym=crystal_sym_array.at(i);			
		for(j=0;j<n;j++)
		{
			yold[0]=x.at(j);
			yold[1]=y.at(j);
			yold[2]=z.at(j);
			crystal_sym.trans(yold,ynew);
			x.push_back(ynew[0]);
			y.push_back(ynew[1]);
			z.push_back(ynew[2]);
			xmean+=ynew[0];
			ymean+=ynew[1];
			zmean+=ynew[2];				
		}
		xmean/=n;
		ymean/=n;
		zmean/=n;

		xscale=scale[0][0]*xmean+scale[0][1]*ymean+scale[0][2]*zmean;
		yscale=scale[1][0]*xmean+scale[1][1]*ymean+scale[1][2]*zmean;
		zscale=scale[2][0]*xmean+scale[2][1]*ymean+scale[2][2]*zmean;
		
		xscale= xscale>=0 ? -((int)xscale) : -((int)(xscale-0.9999));
		yscale= yscale>=0 ? -((int)yscale) : -((int)(yscale-0.9999));
		zscale= zscale>=0 ? -((int)zscale) : -((int)(zscale-0.9999));	
		
		
		xtran=xscale*translation[0][0]+yscale*translation[0][1]+zscale*translation[0][2];
		ytran=yscale*translation[1][1]+zscale*translation[1][2];
		ztran=zscale*translation[2][2];
		
		for(j=0;j<natom;j++)
		{
			x.at(j+natom*(i+1))+=xtran;
			y.at(j+natom*(i+1))+=ytran;
			z.at(j+natom*(i+1))+=ztran;
		}			
	}

	return i;
}

void CTraj2::ninecells(void)
{
	int i,m,k,n;
	double xtran,ytran,ztran;
	int nnow;

	nnow=x.size();

	for(m=-1;m<=1;m++)
	for(n=-1;n<=1;n++)
	for(k=-1;k<=1;k++)
	{
		if(m==0 && n==0 && k==0)
			continue;
		xtran=m*translation[0][0]+n*translation[0][1]+k*translation[0][2];
		ytran=n*translation[1][1]+k*translation[1][2];
		ztran=k*translation[2][2];
		for(i=0;i<nnow;i++)
		{
			x.push_back(x.at(i)+xtran);
			y.push_back(y.at(i)+ytran);
			z.push_back(z.at(i)+ztran);
		}
	}
	return;
}


CTraj2::CTraj2()
{
	x.clear();
	y.clear();
	z.clear();
	b.clear();
	crystal_sym_array.clear();
	nframe=0;
	natom=0;
	nres=0;
	memset(translation,0,sizeof(double)*9);
	memset(scale,0,sizeof(double)*9);
	btrans=0;
	bsmtry=0;
};

CTraj2::~CTraj2()
{

};
