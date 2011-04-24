
/// \file testSepRep.cc
/// \brief test the SeparatedRepresentation (SepRep) for representing matrices

#define WORLD_INSTANTIATE_STATIC_TEMPLATES
#include "mra/seprep.h"
#include "mra/mra.h"

using namespace madness;

bool is_small(const double& val, const double& eps) {
	return (val<eps);
}

std::string ok(const bool b) {if (b) return "ok   "; return "fail ";};

bool is_large(const double& val, const double& eps) {
	return (val>eps);
}

int testGenTensor_ctor(const long& k, const long& dim, const double& eps, const TensorType& tt) {

	print("entering ctor");
	Tensor<double> t0=Tensor<double>(3,3,3,3,3,3);
	t0.fillrandom();
//	t0.fillindex();
	double norm=0.0;
	int nerror=0;

	// default ctor
//	GenTensor<double> t1;

	{
		// ctor with rhs=Tensor, deep
		GenTensor<double> t2(t0,eps,tt);
		norm=(t0-t2.full_tensor_copy()).normf();
		print(ok(is_small(norm, eps)), "ctor with rhs=Tensor/1 ",t2.what_am_i(),t2.rank(),norm);
		if (!is_small(norm,eps)) nerror++;

		// test deepness
		t0.scale(2.0);
		norm=(t0-t2.full_tensor_copy()).normf();
		print(ok(is_large(norm, eps)), "ctor with rhs=Tensor/2 ",t2.what_am_i(),t2.rank(),norm);
		if (!is_large(norm,eps)) nerror++;
	}

	{
		// ctor with rhs=GenTensor, shallow
		GenTensor<double> t3(t0,eps,tt);
		GenTensor<double> t4(t3);
		norm=(t3.full_tensor_copy()-t4.full_tensor_copy()).normf();
		print(ok(is_small(norm,eps)),"ctor with rhs=GenTensor/1",t4.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

		// test deepness
		t3.scale(2.0);
		norm=(t3.full_tensor_copy()-t4.full_tensor_copy()).normf();
		print(ok(is_small(norm, eps)), "ctor with rhs=GenTensor/2 ",t4.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;
	}

	{
		// deep ctor using copy()
		GenTensor<double> t3(t0,eps,tt);
		GenTensor<double> t4(copy(t3));
		norm=(t3.full_tensor_copy()-t4.full_tensor_copy()).normf();
		print(ok(is_small(norm,eps)),"ctor with rhs=GenTensor, using copy()",t4.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

		// test deepness
		t3.scale(3.0);
		norm=(t3.full_tensor_copy()-t4.full_tensor_copy()).normf();
		print(ok(is_large(norm, eps)), "ctor with rhs=GenTensor, using copy()",t4.what_am_i(),norm);
		if (!is_large(norm,eps)) nerror++;
	}


	print("all done\n");
	return nerror;
}

int testGenTensor_assignment(const long& k, const long& dim, const double& eps, const TensorType& tt) {


	print("entering assignment");
	Tensor<double> t0=Tensor<double>(3,3,3,3,3,3);
	Tensor<double> t1=Tensor<double>(3,3,3,3,3,3);
	std::vector<Slice> s(dim,Slice(0,1));
	t0.fillrandom();
	t1.fillindex();

//	t0.fillindex();
	double norm=0.0;
	int nerror=0;

	// default ctor
	const GenTensor<double> g0(t0,eps,tt);
	{
		GenTensor<double> g1(copy(g0));
		g1.scale(2.0);
		norm=(g0.full_tensor_copy()-g1.full_tensor_copy()).normf();
		print(ok(is_large(norm,eps)),"pre-assignment check",g1.what_am_i(),norm);
		if (!is_large(norm,eps)) nerror++;

	}


	// regular assignment: g1=g0
	{

		GenTensor<double> g1(t1,eps,tt);
		g1=g0;
		norm=(g0.full_tensor_copy()-g1.full_tensor_copy()).normf();
		print(ok(is_small(norm,eps)),"assignment with rhs=GenTensor/1",g1.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

		// test deepness
		g1.scale(2.0);
		norm=(g0.full_tensor_copy()-g1.full_tensor_copy()).normf();
		print(ok(is_small(norm,eps)),"assignment with rhs=GenTensor/2",g1.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}

	// regular assignment w/ copy: g1=copy(g0)
	{

		GenTensor<double> g1(t1,eps,tt);
		g1=copy(g0);
		norm=(g0.full_tensor_copy()-g1.full_tensor_copy()).normf();
		print(ok(is_small(norm,eps)),"copy assignment with rhs=GenTensor/1",g1.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

		// test deepness
		g1.scale(2.0);
		norm=(g0.full_tensor_copy()-g1.full_tensor_copy()).normf();
		print(ok(is_large(norm,eps)),"copy assignment with rhs=GenTensor/2",g1.what_am_i(),norm);
		if (!is_large(norm,eps)) nerror++;

	}

	// regular assignment: g1=number
	{

	}


	// sliced assignment: g1=g0(s)
	{
		GenTensor<double> g1(t1,eps,tt);
		g1=g0;
		g1.scale(34.0);
		g1=g0(s);
		norm=(g0.full_tensor_copy()(s)-g1.full_tensor_copy()).normf();
		print(ok(is_small(norm,eps)),"sliced assignment with rhs=GenTensor/1",g1.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;


		// test deepness
		g1.scale(2.0);
		norm=(g0.full_tensor_copy()(s)-g1.full_tensor_copy()).normf();
		print(ok(is_large(norm,eps)),"sliced assignment with rhs=GenTensor/2",g1.what_am_i(),norm,
				"SHOULD FAIL FOR FULLRANK");
//		if (!is_large(norm,eps)) nerror++;

	}

	// sliced assignment: g1(s)=g0
	{

	}

	// sliced assignment: g1(s)=g0(s)
	{

	}

	// sliced assignment: g1(s)=number
	{
		GenTensor<double> g1(t1,eps,tt);
		Tensor<double> t2=copy(t1);
		g1(s)=0.0;
		t2(s)=0.0;
		norm=(t2-g1.full_tensor_copy()).normf();
		print(ok(is_small(norm,eps)),"sliced assignment with rhs=0.0",g1.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}
	print("all done\n");
	return nerror;

}

int testGenTensor_algebra(const long& k, const long& dim, const double& eps, const TensorType& tt) {

	print("entering algebra");
	Tensor<double> t0=Tensor<double>(k,k,k,k,k,k);
	Tensor<double> t1=Tensor<double>(k,k,k,k,k,k);

	std::vector<Slice> s(dim,Slice(0,k/2-1));
	std::vector<Slice> s2(dim,Slice(k/2,k-1));
//	print(s,s2);
	t0.fillrandom();
	t1.fillindex();

	Tensor<double> t2=copy(t0(s));

	double norm=0.0;
	int nerror=0;

	// default ctor
	GenTensor<double> g0(t0,eps,tt);
	GenTensor<double> g1(t1,eps,tt);

	// test inplace add: g0+=g1
	{
		GenTensor<double> g0(t0,eps,tt);
		g0+=g1;
		t0+=t1;
		norm=(g0.full_tensor_copy()-t0).normf();
		print(ok(is_small(norm,eps)),"algebra g0+=g1      ",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}


	// test inplace add: g0+=g1(s)
	{
		GenTensor<double> g0(t2,eps,tt);
		g0+=g1(s);
		t2+=t1(s);
		norm=(g0.full_tensor_copy()-t2).normf();
		print(ok(is_small(norm,eps)),"algebra g0+=g1(s)   ",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}


	// test inplace add: g0(s)+=g1(s)
	{
		GenTensor<double> g0(t0,eps,tt);
		g0(s)+=g1(s);
		t0(s)+=t1(s);
		norm=(g0.full_tensor_copy()-t0).normf();
		print(ok(is_small(norm,eps)),"algebra g0(s)+=g1(s)",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}
	// test inplace add: g0(s)+=g1(s)
	{
		GenTensor<double> g0(t0,eps,tt);
		g0(s2)+=g1(s);
		t0(s2)+=t1(s);
		norm=(g0.full_tensor_copy()-t0).normf();
		print(ok(is_small(norm,eps)),"algebra g0(s)+=g1(s)",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}

	// test inplace scale: g=g0*=fac
	{
		GenTensor<double> g0(t0,eps,tt);
		GenTensor<double> g2=g0.scale(2.0);
		Tensor<double> t2=t0.scale(2.0);
		norm=(g0.full_tensor_copy()-t0).normf();
		print(ok(is_small(norm,eps)),"algebra scale",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}



	print("all done\n");
	return nerror;
}


int testGenTensor_update(const long& k, const long& dim, const double& eps, const TensorType& tt) {

	print("entering update");
	Tensor<double> t0=Tensor<double>(k,k,k,k,k,k);
	Tensor<double> t1=Tensor<double>(k,k,k,k,k,k);

	t0.fillindex();
	t1=2.0;

	double norm=0.0;
	int nerror=0;

	// test rank-0 + rank-1
	{
		GenTensor<double> g1(t1,eps,tt);
		GenTensor<double> g0(Tensor<double>(k,k,k,k,k,k),eps,tt);

		g0.update_by(g1);
		g0.finalize_accumulate();
		t0=g1.full_tensor_copy();

		norm=(g0.full_tensor_copy()-t0).normf();
		print(ok(is_small(norm,eps)),"update g0+=g1       ",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}

	// test rank-1 + rank-1
	{
		t0=2.0;
		GenTensor<double> g0(t0,eps,tt);
		GenTensor<double> g1(t1,eps,tt);

		g0.update_by(g1);
		g0.finalize_accumulate();
		t0+=g1.full_tensor_copy();

		norm=(g0.full_tensor_copy()-t0).normf();
		print(ok(is_small(norm,eps)),"update g0+=g1       ",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}

	// test rank-2 + rank-1
	{
		t0.fillindex();
		GenTensor<double> g0(t0,eps,tt);

		GenTensor<double> g1(t1,eps,tt);
		g1.fillrandom();
		g1.scale(2.34e4);

		g0.update_by(g1);
		g0.finalize_accumulate();
		t0+=g1.full_tensor_copy();

		norm=(g0.full_tensor_copy()-t0).normf();
		print(ok(is_small(norm,eps)),"update g0+=g1       ",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}

	// test rank-2 + rank-1 + rank-1
	{
		GenTensor<double> g1(t1,eps,tt);
		g1.fillrandom();
		g1.scale(2.34e4);
		GenTensor<double> g0(t0,eps,tt);

		g0.update_by(g1);
		t0+=g1.full_tensor_copy();
		g0.finalize_accumulate();

		g1.fillrandom();
		g1.scale(8.34e2);
		g0.update_by(g1);
		t0+=g1.full_tensor_copy();
		g0.finalize_accumulate();

		norm=(g0.full_tensor_copy()-t0).normf();
		print(ok(is_small(norm,eps)),"update g0+=g1+=g1   ",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}

	// test rank-1 + rank-2
	{
		t0.fillindex();
		GenTensor<double> g0(t0,eps,tt);
		GenTensor<double> g1(t1,eps,tt);
		g1.fillrandom();
		g1.scale(2.34e2);

		g0.update_by(g1);
		t0+=g1.full_tensor_copy();
		g0.finalize_accumulate();

		norm=(g0.full_tensor_copy()-t0).normf();
		print(ok(is_small(norm,eps)),"update rank-1 + rank-2   ",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}

	print("all done\n");
	return nerror;
}


int testGenTensor_rankreduce(const long& k, const long& dim, const double& eps, const TensorType& tt) {

	print("entering rank reduce");
	Tensor<double> t0=Tensor<double>(k,k,k,k,k,k);
	Tensor<double> t1=Tensor<double>(k,k,k,k,k,k);

	t0.fillindex();
	t1=2.0;

	double norm=0.0;
	int nerror=0;

	{
		//					tt	 k d
		SepRep<double> sr1(TT_2D,4,2);
		sr1.fillrandom(2);
	//    sr1.config().weights_[1]*=3.0;
	//    sr1.config().vector_[0](1,2)*=4.e2;
		Tensor<double> t=sr1.reconstructTensor();

		SepRep<double> sr2(TT_2D,4,2);
		sr2.fillrandom();

		sr2.config().vector_[0].fillrandom()*2.0;
		sr1+=sr2;
		t+=sr2.reconstructTensor();

		sr2.config().vector_[0].fillrandom();
		sr1+=sr2;
		t+=sr2.reconstructTensor();

		sr2.config().vector_[0].fillrandom();
		sr1+=sr2;
		t+=sr2.reconstructTensor();

		sr1.config().ortho3();

		Tensor<double> t3=sr1.reconstructTensor();
//		print("t,t3");
//		print(t);
//		print(t3);
		print("norm",(t-t3).normf());
		norm=(t-t3).normf();
		print(ok(is_small(norm,eps)),"sophisticated rank reduce   ",norm);
	}


	print("all done\n");
	return nerror;
}



int testGenTensor_transform(const long& k, const long& dim, const double& eps, const TensorType& tt) {

	print("entering transform");
	Tensor<double> t0=Tensor<double>(k,k,k,k,k,k);
	Tensor<double> c=Tensor<double> (k,k);
	Tensor<double> cc[dim];
	for (unsigned int idim=0; idim<dim; idim++) {
		cc[idim]=Tensor<double>(k,k);
		cc[idim].fillrandom();
	}

	t0.fillrandom();
	c.fillindex();
	c.scale(1.0/c.normf());

	double norm=0.0;
	int nerror=0;

	// default ctor
	GenTensor<double> g0(t0,eps,tt);

	// test transform_dir
	{
		const long ndim=t0.ndim();

		for (long idim=0; idim<ndim; idim++) {
//		for (long idim=0; idim<1; idim++) {
			GenTensor<double> g1=transform_dir(g0,c,idim);
			Tensor<double> t1=transform_dir(t0,c,idim);
			norm=(g1.full_tensor_copy()-t1).normf();
			print(ok(is_small(norm,eps)),"transform_dir",idim,g0.what_am_i(),norm);
			if (!is_small(norm,eps)) nerror++;
		}
	}

	// test transform with tensor
	{
		GenTensor<double> g1=transform(g0,c);
		Tensor<double> t1=transform(t0,c);
		norm=(g1.full_tensor_copy()-t1).normf();
		print(ok(is_small(norm,eps)),"transform.scale",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}


	// test general_transform
	{
		GenTensor<double> g1=general_transform(g0,cc);
		Tensor<double> t1=general_transform(t0,cc);
		norm=(g1.full_tensor_copy()-t1).normf();
		print(ok(is_small(norm,eps)),"general_transform",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}



	// test general_transform with scale
	{
		GenTensor<double> g1=general_transform(g0,cc).scale(1.9);
		Tensor<double> t1=general_transform(t0,cc).scale(1.9);
		norm=(g1.full_tensor_copy()-t1).normf();
		print(ok(is_small(norm,eps)),"general_transform.scale",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;

	}


	print("all done\n");
	return nerror;

}

int testGenTensor_reconstruct(const long& k, const long& dim, const double& eps, const TensorType& tt) {

	print("entering reconstruct");
	Tensor<double> t0=Tensor<double>(k,k,k,k,k,k);
	Tensor<double> t1=Tensor<double>(k,k,k,k,k,k);
	Tensor<double> t2=Tensor<double>(k,k,k,k,k,k);


	t1.fillrandom();
	t2.fillindex();

	GenTensor<double> g0(t0,eps,tt);
	GenTensor<double> g1(t1,eps,tt);
	GenTensor<double> g2(t2,eps,tt);

	double norm=0.0;
	int nerror=0;


	// reconstruct empty tensor
	{
		Tensor<double> t=g0.full_tensor_copy();
		norm=(t-t0).normf();
		print(ok(is_small(norm,eps)),"reconstruct/1",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;
	}

	// reconstruct empty tensor
	{
		Tensor<double> t=t0;
		g0.accumulate_into(t,-1.0);
		norm=(t).normf();
		print(ok(is_small(norm,eps)),"reconstruct/2",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;
	}


	// reconstruct high rank tensor
	{
		Tensor<double> t=g1.full_tensor_copy();
		norm=(t-t1).normf();
		print(ok(is_small(norm,eps)),"reconstruct/3",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;
	}

	// reconstruct high rank tensor
	{
		Tensor<double> t=t1;
		g1.accumulate_into(t,-1.0);
		norm=(t).normf();
		print(ok(is_small(norm,eps)),"reconstruct/4",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;
	}


	// reconstruct lowish rank tensor
	{
		Tensor<double> t=g2.full_tensor_copy();
		norm=(t-t2).normf();
		print(ok(is_small(norm,eps)),"reconstruct/5",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;
	}

	// reconstruct lowish rank tensor
	{
		Tensor<double> t=t2;
		g2.accumulate_into(t,-1.0);
		norm=(t).normf();
		print(ok(is_small(norm,eps)),"reconstruct/6",g0.what_am_i(),norm);
		if (!is_small(norm,eps)) nerror++;
	}


	print("all done\n");
	return nerror;

}




int main(int argc, char**argv) {

    initialize(argc,argv);
    World world(MPI::COMM_WORLD);
    srand(time(NULL));

    // the parameters
    const long k=6;
    const unsigned int dim=6;
    double eps=1.e-3;

#if 0

    //					tt	 k d
    SepRep<double> sr1(TT_2D,4,2);
    sr1.fillrandom(2);
//    sr1.config().weights_[1]*=3.0;
//    sr1.config().vector_[0](1,2)*=4.e2;
    Tensor<double> t=sr1.reconstructTensor();

    SepRep<double> sr2(TT_2D,4,2);
    sr2.fillrandom();

    sr2.config().vector_[0].fillrandom()*2.0;
    sr1+=sr2;
    t+=sr2.reconstructTensor();

    sr2.config().vector_[0].fillrandom();
    sr1+=sr2;
    t+=sr2.reconstructTensor();

    sr2.config().vector_[0].fillrandom();
    sr1+=sr2;
    t+=sr2.reconstructTensor();

    sr1.config().ortho3();

    Tensor<double> t3=sr1.reconstructTensor();
    print("t,t3");
    print(t);
    print(t3);
    print("norm",(t-t3).normf());

    return 0;

#endif


#if 0

    // do some benchmarking

	Tensor<double> t0=Tensor<double>(k,k,k,k,k,k);
	Tensor<double> t1=Tensor<double>(k,k,k,k,k,k);

	t0.fillindex();
	t0=2.5;
	t1=2.3;
	long nloop=1.e5;
	GenTensor<double> g1(t1,eps,TT_2D);
	g1.fillrandom();
	double tim=wall_time();

    if(world.rank() == 0) print("starting at time", wall_time()-tim);
    for (unsigned int i=0; i<nloop; i++) {
//		GenTensor<double> g0(t0,eps,TT_2D);
    	g1.fillrandom();
    }
    if(world.rank() == 0) print("baseline at time  ", wall_time()-tim);
    tim=wall_time();

	GenTensor<double> g0(t0,eps,TT_2D);
    for (unsigned int i=0; i<nloop; i++) {
    	g1.fillrandom();
		g0.update_by(g1);
//		g0.finalize_accumulate();
    }
    if(world.rank() == 0) print("update_by at time  ", wall_time()-tim);
    tim=wall_time();

    for (unsigned int i=0; i<nloop; i++) {
    	g1.fillrandom();
		g1.accumulate_into(t0,1.0);
    }
    if(world.rank() == 0) print("accumulate_into at time  ", wall_time()-tim);
    tim=wall_time();

//    GenTensor<double> g2(t0,eps,TT_2D);
//    GenTensor<double> g3(t1,eps,TT_2D);
//    for (unsigned int i=0; i<nloop; i++) {
//    	g1.fillrandom();
//    	g2+=g3;
//    }
//    if(world.rank() == 0) print("inplace_add at time  ", wall_time()-tim);
//    tim=wall_time();

//    MADNESS_EXCEPTION("end benchmark",0);
#endif

    int error=0;
    print("hello world");

//    error+=testGenTensor_ctor(k,dim,eps,TT_FULL);
//    error+=testGenTensor_ctor(k,dim,eps,TT_3D);
//    error+=testGenTensor_ctor(k,dim,eps,TT_2D);
//
//    error+=testGenTensor_assignment(k,dim,eps,TT_FULL);
//    error+=testGenTensor_assignment(k,dim,eps,TT_3D);
//    error+=testGenTensor_assignment(k,dim,eps,TT_2D);
//
    error+=testGenTensor_update(k,dim,eps,TT_2D);
    error+=testGenTensor_rankreduce(k,dim,eps,TT_2D);

//
//    error+=testGenTensor_algebra(k,dim,eps,TT_FULL);
//    error+=testGenTensor_algebra(k,dim,eps,TT_3D);
//    error+=testGenTensor_algebra(k,dim,eps,TT_2D);
//
//    error+=testGenTensor_transform(k,dim,eps,TT_FULL);
//    error+=testGenTensor_transform(k,dim,eps,TT_3D);
//    error+=testGenTensor_transform(k,dim,eps,TT_2D);
//
//    error+=testGenTensor_reconstruct(k,dim,eps,TT_FULL);
//    error+=testGenTensor_reconstruct(k,dim,eps,TT_2D);

    print(ok(error==0),error,"finished test suite\n");


    world.gop.fence();
    finalize();

    return 0;
}
