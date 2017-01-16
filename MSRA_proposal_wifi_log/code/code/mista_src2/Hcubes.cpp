#include "debug.h"
#include "Hcubes.h"

int ERASURE_COUNT_MAX=100;
int INSERTION_COUNT_MAX=100;
int dimensions = -1;  // Global variable used by KDtree

bool operator< (const H_center& h1, const H_center& h2) 
	{ return (h1.center<h2.center) || ((h1.center==h2.center) && (h1.id<h2.id)); };

int H_store::insert(H& h_add)
{
	return insert_pt(checked_new(H(h_add)));
};

int H_store::insert_pt(H* h_pt)
{
	// Insert new H in the repository
	rectangles.push_back(h_pt);
	int new_id=(int)rectangles.size()-1;

	// Add indexes
	H_center tmp;
	switch(strategy) {
		case HOMEMADE:
			for(int i=0; i<dim; i++)
			{
				tmp.id=new_id;
				tmp.center=(h_pt->low[i]+h_pt->up[i])/2;
				index[i].insert(tmp);
			};
			break; 
		case KDTREE:
			H_KDobj k_tmp;
			k_tmp.id=new_id;
			KDObject<H_KDobj> obj(k_tmp);
			for(int i=0; i<dim; i++)
				obj[i]=(h_pt->low[i]+h_pt->up[i])/2;
			kd_index->insert(obj);
			stats_count++;
			// From time to time re-balance the KDtree...
			if((stats_count % INSERTION_COUNT_MAX)==0)
				kd_index->balance();
			break;

	};

	return new_id;
};

void H_store::erase(int h_erase) 
{
	if(h_erase>=0 && h_erase<(int)rectangles.size())
	{
		// Virtual deletion of element: pointer:=0, and indexes remain in "index"
		checked_delete(rectangles[h_erase]);
		rectangles[h_erase]=0;
		// After ERASURE_COUNT_MAX erasures, physically delete useless indexes 

		// Avoid this section if KDTREE is used
		if(strategy==HOMEMADE)
		{
			stats_count++;
			if(stats_count>ERASURE_COUNT_MAX)
			{
				for(vector< set<H_center> >::iterator h_set=index.begin(); h_set!=index.end(); ++h_set)
				{
					set<H_center> tmp;
					// tmp.reserve(h_set->size());
					for(set<H_center>::iterator it=h_set->begin(); it!=h_set->end(); it++)
						if(this->rectangles[it->id]!=0)
							tmp.insert(*it);
					h_set->swap(tmp);				
				};
				stats_count=0;
			};
		};
	} else
		launch_error("Wrong index of H to be erased!!");
};

void H_store::clear() 
{
	// Manually deallocate Hs in the storage
	for(vector<H*>::iterator it=rectangles.begin(); it!=rectangles.end(); it++)
		checked_delete(*it);
	rectangles.clear();

	// Reset indexes
	switch(strategy) {
		case HOMEMADE:
			for(vector< set<H_center> >::iterator it=index.begin(); it!=index.end(); it++)
				(*it).clear();
			break;
		case KDTREE:
			checked_delete(kd_index);
			kd_index=checked_new(KDTree<H_KDobj>);
	};
};

void H_store::fast_clear() 
{
	rectangles.clear();

	// Reset indexes
	switch(strategy) {
		case HOMEMADE:
			for(vector< set<H_center> >::iterator it=index.begin(); it!=index.end(); it++)
				(*it).clear();
			break;
		case KDTREE:
			// No clear() method is provided, so we must destroy and create again the index
			checked_delete(kd_index);
			kd_index=checked_new(KDTree<H_KDobj>);
	};
};

void H_store::change_index_strategy(indexing_strategy new_strategy)
{
	if(new_strategy!=strategy)
	{
		strategy=new_strategy;
		switch(new_strategy) {
			case HOMEMADE:
				{
					// Delete KDTree and create indexes on each dimension
					checked_delete(kd_index);
					// Init
					set<H_center> empty; 
					for(int i=0; i<dim; i++)
						index.push_back(empty);
					// Compress vector of Hs and create new indexes
					vector<H*>::iterator oldit,newit;
					int idx=-1;
					H_center tmp;
					for(newit=oldit=rectangles.begin(); oldit!=rectangles.end(); oldit++)
						if(*oldit!=0)
						{
							// Move rectangle in the vector
							*newit=*oldit;
							++newit;
							++idx;
							// Insert index
							for(int i=0; i<dim; i++)
							{
								tmp.id=idx;
								tmp.center=((*oldit)->low[i]+(*oldit)->up[i])/2;
								index[i].insert(tmp);
							};
						};
					rectangles.erase(newit,rectangles.end()); // Erase empty spaces
					stats_count=0;
				};
				break;
			case KDTREE:
				{
					// Delete indexes and insert all rectangles in KDtree
					for(int i=0; i<dim; i++)
						index[i].clear();
					// Init
					kd_index=checked_new(KDTree<H_KDobj>);
					dimensions=dim; // Set global variable...

					// Compress vector of Hs and create new indexes
					vector<H*>::iterator oldit,newit;
					int idx=-1;
					H_KDobj k_tmp;
					for(newit=oldit=rectangles.begin(); oldit!=rectangles.end(); oldit++)
						if(*oldit!=0)
						{
							// Move rectangle in the vector
							*newit=*oldit;
							++newit;
							++idx;
							// Insert index
							k_tmp.id=idx;
							KDObject<H_KDobj> obj(k_tmp);
							for(int i=0; i<dim; i++)
								obj[i]=((*oldit)->low[i]+(*oldit)->up[i])/2;
							kd_index->insert(obj);
						};
					rectangles.erase(newit,rectangles.end()); // Erase empty spaces
					stats_count=0;
					kd_index->balance();
				};
				break;
			default:
				launch_error("Unknown strategy in change_index_strategy!!");
		};
	};

	//switch(strategy) {
	//	case HOMEMADE:
	//		for(int i=0; i<dim; i++)
	//		{
	//			tmp.id=new_id;
	//			tmp.center=(h_add.low[i]+h_add.up[i])/2;
	//			index[i].insert(tmp);
	//		};
	//		break;
	//	case KDTREE:
	//		H_KDobj k_tmp;
	//		k_tmp.id=new_id;
	//		KDObject<H_KDobj> obj(k_tmp);
	//		for(int i=0; i<dim; i++)
	//			obj[i]=(h_add.low[i]+h_add.up[i])/2;
	//		kd_index->insert(obj);
	//		break;

	//};

};


set<int> *H_store::intersect_list(H& h, float tau) 
{
	switch(strategy) {
		case HOMEMADE:
			return homemade_intersect_list(h,tau);
			break;
		case KDTREE:
			return kdtree_intersect_list(h,tau);
			break;
		default:
			launch_error("Do not know which index strategy to use in intersect_list!!");
			return 0;
	};
};


//
//  VERSION 3: USE A *KD-TREE* INDEX
//
set<int> *H_store::kdtree_intersect_list(H& h, float tau) 
{ 

	float* my_center = checked_new(float[dimensions]);
	float tmp_len,max_len=0;
	for(int i=0; i<dimensions; i++)
	{
		my_center[i]=(h.low[i]+h.up[i])/2;
		if((tmp_len=(h.up[i]-h.low[i])) > max_len)
			max_len=tmp_len;
	};
	vector<KDObjDist<H_KDobj> > res=kd_index->findNear(my_center,tau+max_len/2);

	set<int> *selected=checked_new(set<int>);
	H_KDobj *iter_h;
	for(vector<KDObjDist<H_KDobj> >::iterator it=res.begin(); it!=res.end(); it++)
	{
		iter_h=&it->object;
		bool contained=true;
		for(int i=0; contained && (i<dimensions); i++)
			contained=(rectangles[iter_h->id]!=0 && rectangles[iter_h->id]->low[i]<=h.up[i] && rectangles[iter_h->id]->up[i]>=h.low[i]);
		// Candidate is OK. Now, if i>0, check if it was OK in previous dimensions
		if(contained)
		{
			selected->insert(iter_h->id);
		};
	};
	return selected;
};

//
//	VERSION 2: USE ONLY ONE INDEX (E.G., THE SHORTEST)
//
set<int> *H_store::homemade_intersect_list(H& h, float tau) 
{ 
	set<int> *selected=checked_new(set<int>);
	set<H_center>::iterator from_h, to_h, iter_h, from_tmp, to_tmp;

	// Get a raw list of possible itersections -- seems to be faster by using 1 dim. only
	//	int j=rand()%(int)index.size();
	int j=0;
	from_h=index[j].lower_bound(H_center(-1,APPROX_FLOAT(h.low[j]-tau)));
	to_h=index[j].upper_bound(H_center(MAX_INTEGER,APPROX_FLOAT(h.up[j]+tau)));
	for(int i=1; i<(int)index.size(); i++)
	{
		from_tmp=index[i].lower_bound(H_center(-1,APPROX_FLOAT(h.low[i]-tau)));
		to_tmp=index[i].upper_bound(H_center(MAX_INTEGER,APPROX_FLOAT(h.up[i]+tau)));
		if(distance(from_tmp,to_tmp)<distance(from_h,to_h))
		{
			from_h=from_tmp;
			to_h=to_tmp;
		};
	};

	// Now filter the list
	for(iter_h=from_h; iter_h!=to_h; iter_h++)
	// Check true intersection
	{
		bool contained=true;
		for(int i=0; contained && (i<(int)index.size()); i++)
			contained=(rectangles[iter_h->id]!=0 && rectangles[iter_h->id]->low[i]<=h.up[i] && rectangles[iter_h->id]->up[i]>=h.low[i]);
		// Candidate is OK. Now, if i>0, check if it was OK in previous dimensions
		if(contained)
		{
			selected->insert(iter_h->id);
		};
	};
	return selected;
};


// 
//   VERSION 1: COMPUTE INTERSECTION OF SETS OF INDEXES
//
//set<int> *H_store::intersect_list(H h, float tau) 
//{ 
//	set<int> *selected=new set<int>;
//	set<int> *next_step=new set<int>;
//	set<H_center>::iterator from_h, to_h, iter_h;
//
//	for(int i=0; i<this->dim; i++)
//	{
//		selected->clear();
//		swap(selected,next_step);
//		// Get a raw list of possible itersections
//		from_h=index[i].lower_bound(H_center(-1,APPROX_FLOAT(h.low[i]-tau)));
//		to_h=index[i].upper_bound(H_center(MAX_INTEGER,APPROX_FLOAT(h.up[i]+tau)));
//		// Now filter the list
//		for(iter_h=from_h; iter_h!=to_h; iter_h++)
//			// Check true intersection
//			if(rectangles[iter_h->id]!=0 && rectangles[iter_h->id]->low[i]<=h.up[i] && rectangles[iter_h->id]->up[i]>=h.low[i])
//				// Candidate is OK. Now, if i>0, check if it was OK in previous dimensions
//				if(i==0 || selected->find(iter_h->id)!=selected->end())
//					next_step->insert(iter_h->id);
//	};
//
//	delete selected;
//	return next_step;
//};

void HPartitioning(H& h, H& h_prime, vector<H>& Out, vector<H>& Out_prime, H& h_intersect)
{
	h_intersect.low.clear();
	h_intersect.up.clear();
	int n=(int)h.low.size();
	H tmp;
	for(int i=0; i<n; i++)
	{
		// Left cut along i-th dimension
		if(h.low[i]<h_prime.low[i])
		{
			tmp.low.clear();
			for(int j=0; j<i; j++) tmp.low.push_back(h_intersect.low[j]);
			for(int j=i; j<n; j++) tmp.low.push_back(h.low[j]);
			tmp.up.clear();
			for(int j=0; j<i; j++) tmp.up.push_back(h_intersect.up[j]);
			tmp.up.push_back(h_prime.low[i]);
			for(int j=i+1; j<n; j++) tmp.up.push_back(h.up[j]);
			Out.push_back(tmp);			
		} else 
			if(h.low[i]>h_prime.low[i])
			{
				tmp.low.clear();
				for(int j=0; j<i; j++) tmp.low.push_back(h_intersect.low[j]);
				for(int j=i; j<n; j++) tmp.low.push_back(h_prime.low[j]);
				tmp.up.clear();
				for(int j=0; j<i; j++) tmp.up.push_back(h_intersect.up[j]);
				tmp.up.push_back(h.low[i]);
				for(int j=i+1; j<n; j++) tmp.up.push_back(h_prime.up[j]);
				Out_prime.push_back(tmp);			
			};

		// Right cut along i-th dimension
		if(h.up[i]<h_prime.up[i])
		{
			tmp.low.clear();
			for(int j=0; j<i; j++) tmp.low.push_back(h_intersect.low[j]);
			tmp.low.push_back(h.up[i]);
			for(int j=i+1; j<n; j++) tmp.low.push_back(h_prime.low[j]);
			tmp.up.clear();
			for(int j=0; j<i; j++) tmp.up.push_back(h_intersect.up[j]);
			for(int j=i; j<n; j++) tmp.up.push_back(h_prime.up[j]);
			Out_prime.push_back(tmp);			
		} else 
			if(h.up[i]>h_prime.up[i])
			{
				tmp.low.clear();
				for(int j=0; j<i; j++) tmp.low.push_back(h_intersect.low[j]);
				tmp.low.push_back(h_prime.up[i]);
				for(int j=i+1; j<n; j++) tmp.low.push_back(h.low[j]);
				tmp.up.clear();
				for(int j=0; j<i; j++) tmp.up.push_back(h_intersect.up[j]);
				for(int j=i; j<n; j++) tmp.up.push_back(h.up[j]);
				Out.push_back(tmp);			
			};
		h_intersect.low.push_back(max(h.low[i],h_prime.low[i]));
		h_intersect.up.push_back(min(h.up[i],h_prime.up[i]));
	};
};

bool H_intersection(H& h1, H& h2)
{
	bool flag=true;
	int n=(int)h1.low.size();
	for(int i=0; i<n && flag; i++)
		flag = flag && (h1.low[i]<h2.up[i]) && (h2.low[i]<h1.up[i]);
	return flag;
};

void combine_rectangle_set(vector<H>& I, H_store *O, bool add_freq, float tau)
{
//	H_store *O = new H_store(n);
	vector<H> *H_I = checked_new(vector<H>);
	vector<H> *H_next = checked_new(vector<H>);

	// Heuristic memory reservation
	//H_I->reserve(2*(O->rectangles.size()+I.size()));
	//H_next->reserve(2*(O->rectangles.size()+I.size()));

	// Output variables used in HPartitioning's
	vector<H> H_set;
	vector<H> H_set_prime;
	H h_int;


	for(vector<H>::iterator h=I.begin(); h!=I.end(); h++)
	{
		H_I->clear();
		H_I->push_back(*h);
		set<int> *intersect = O->intersect_list(*h, tau);
//cerr << "Found " << intersect->size() << " intersecting Hs: ";for(set<int>::iterator h_prime=intersect->begin(); h_prime!=intersect->end(); h_prime++) cerr << *h_prime << "=[" << O->at(*h_prime) << "] "; cerr << endl;
	
		for(set<int>::iterator h_prime=intersect->begin(); h_prime!=intersect->end(); h_prime++)
		{
//int ii=0;cerr << "h_prime=" << *h_prime << endl;for(vector<H*>::iterator rect=O->rectangles.begin(); rect!=O->rectangles.end(); ++rect, ++ii)cerr << ii << ":[" << *rect << "] ";cerr << endl;
			if(add_freq)
			{
				H_set.clear();
				H_set_prime.clear();
				// Cut away the new "h" from existing "h_prime"
				HPartitioning(*h, *O->at(*h_prime), H_set, H_set_prime, h_int);
				// Non-intersected parts keep old density
				for(vector<H>::iterator it=H_set_prime.begin(); it!=H_set_prime.end(); it++)
				{
					(*it).density=O->at(*h_prime)->density;
					O->insert(*it);
				};
			};
			// Now check intersections with the parts composing "h"
			H_next->clear();
			for(vector<H>::iterator h2=H_I->begin(); h2!=H_I->end(); h2++)
				if(H_intersection(*h2,*O->at(*h_prime)))
				{
					// They intersect ==> compute intersections
					H_set.clear();
					H_set_prime.clear();
					HPartitioning(*h2, *O->at(*h_prime), H_set, H_set_prime, h_int);
					if(add_freq)
					{
						// Intersected part increments density (single increment), if "add_freq" is set
						h_int.density=O->at(*h_prime)->density+1;
						O->insert(h_int);
					};
					// NOTICE: If !add_freq => keep old O --> simulates the "H_I = h2 - O" difference
					// Remaining parts of the new contributing H (h2) are kept apart for next iteration
					for(vector<H>::iterator it=H_set.begin(); it!=H_set.end(); it++)
						H_next->push_back(*it);				
				} else
				{
					// No intersection: *h2 is copied as it is in the "residuals" repository (H_next)
					H_next->push_back(*h2);				
				};
			if(add_freq)
				// Existing h_set_prime has been replaced by its parts => we can (virtually) delete it
				O->erase(*h_prime);
			// The remaining pieces of "h" (main cycle) are collected for next iteration
			H_I->clear();
			swap(H_I,H_next);
		};
		checked_delete(intersect);
		// The pieces of "h" that survived do not intersect any rectangle => add with density=1
		for(vector<H>::iterator it=H_I->begin(); it!=H_I->end(); it++)
		{
			(*it).density=1;
			O->insert(*it);
		};
	};
	checked_delete(H_I);
	checked_delete(H_next);

	//// Filter out virtually removed Hs from store
	//vector<H*> *filtered_O = new vector<H*>;
	//for(vector<H*>::iterator it=O->rectangles.begin(); it!=O->rectangles.end(); it++)
	//	if(*it != 0)
	//		filtered_O->push_back(*it);
	//delete O;  // Notice: do not call O->erase(), because allocated H are kept!

	//return filtered_O;
};


ostream& operator << (ostream& s, H h)
{
	vector<float>::iterator it_up = h.up.begin();
	int count = 0;
	
	for(vector<float>::iterator it=h.low.begin(); it!=h.low.end(); it++, it_up++, count++){
		if (count)
			s << "\t\t\t[";
		else 
			s << "[";
		
		if (*it >= 0)
			s << *it;
		else
			s << 0;
		s << ", ";
		if (*it_up >= 0)
			s << *it_up;
		else
			s << 0;
		s << "]";
		
		
	}
	s << endl;
	/*
	for(vector<double>::iterator it=h.up.begin(); it!=h.up.end(); it++)
		s << " " << *it;
	s << " ]" << endl;
	*/
	return s;

	
	/*
	vector<float>::iterator it_up=h.up.begin();
	s << "Low coords:";
	for(vector<float>::iterator it=h.low.begin(); it!=h.low.end(); it++;it_up++){
		s << " " << *it;
	
	}
	s << ", Up coords:";
	for(vector<float>::iterator it=h.up.begin(); it!=h.up.end(); it++){
		s << " " << *it;
	}
	s << ", Density: " << h.density << endl;
	return s;
	*/
};

ostream& operator << (ostream& s, H_center h)
{
	s << "(" << h.id << "," << h.center << ")";
	return s;
};

ostream& operator << (ostream& s, vector<H> v)
{
	s << "############ Begin H vector ######\n";
	for(vector<H>::iterator it=v.begin(); it!=v.end(); it++)
		s << *it << "-------------------\n";
	s << "############ End   H vector ######\n";
	return s;
};









