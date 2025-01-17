/*
 This file is part of MADNESS.

 Copyright (C) 2007,2010 Oak Ridge National Laboratory

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

 For more information please contact:

 Robert J. Harrison
 Oak Ridge National Laboratory
 One Bethel Valley Road
 P.O. Box 2008, MS-6367

 email: harrisonrj@ornl.gov
 tel:   865-241-3937
 fax:   865-572-0680

 $Id$
 */

//#define WORLD_INSTANTIATE_STATIC_TEMPLATES

/*!
 \file examples/nemo.h
 \brief solve the HF equations using numerical exponential MOs

 The source is
 <a href=http://code.google.com/p/m-a-d-n-e-s-s/source/browse/local
 /trunk/src/apps/examples/nemo.h>here</a>.

 */

#ifndef NEMO_H_
#define NEMO_H_

#include <madness/mra/mra.h>
#include <madness/mra/funcplot.h>
#include <madness/mra/operator.h>
#include <madness/mra/lbdeux.h>
#include <chem/SCF.h>
#include <chem/correlationfactor.h>
#include <chem/molecular_optimizer.h>
#include <examples/nonlinsol.h>
#include <madness/mra/vmra.h>

namespace madness {

class PNO;

// this class needs to be moved to vmra.h !!

// This class is used to store information for the non-linear solver
template<typename T, std::size_t NDIM>
class vecfunc {
public:
	World& world;
	std::vector<Function<T, NDIM> > x;

	vecfunc(World& world, const std::vector<Function<T, NDIM> >& x1) :
			world(world), x(x1) {
	}

	vecfunc(const std::vector<Function<T, NDIM> >& x1) :
			world(x1[0].world()), x(x1) {
	}

	vecfunc(const vecfunc& other) :
			world(other.world), x(other.x) {
	}

	vecfunc& operator=(const vecfunc& other) {
		x = other.x;
		return *this;
	}

	vecfunc operator-(const vecfunc& b) const {
		return vecfunc(world, sub(world, x, b.x));
	}

	vecfunc operator+=(const vecfunc& b) { // Operator+= necessary
		x = add(world, x, b.x);
		return *this;
	}

	vecfunc operator*(double a) const { // Scale by a constant necessary

		PROFILE_BLOCK(Vscale);
		std::vector<Function<T,NDIM> > result(x.size());
		for (unsigned int i=0; i<x.size(); ++i) {
			result[i]=mul(a,x[i],false);
		}
		world.gop.fence();

//		scale(world, x, a);
		return result;
	}
};

/// the non-linear solver requires an inner product
template<typename T, std::size_t NDIM>
T inner(const vecfunc<T, NDIM>& a, const vecfunc<T, NDIM>& b) {
	Tensor<T> i = inner(a.world, a.x, b.x);
	return i.sum();
}

// The default constructor for functions does not initialize
// them to any value, but the solver needs functions initialized
// to zero for which we also need the world object.
template<typename T, std::size_t NDIM>
struct allocator {
	World& world;
	const int n;

	/// @param[in]	world	the world
	/// @param[in]	nn		the number of functions in a given vector
	allocator(World& world, const int nn) :
			world(world), n(nn) {
	}

	/// allocate a vector of n empty functions
	vecfunc<T, NDIM> operator()() {
		return vecfunc<T, NDIM>(world, zero_functions<T, NDIM>(world, n));
	}
};



/// The Nemo class
class Nemo: public MolecularOptimizationTargetInterface {
	typedef std::shared_ptr<real_convolution_3d> poperatorT;
	friend class PNO;

    /// struct for running a protocol of subsequently tightening precision
    struct protocol {
        protocol(const Nemo& nemo) : start_prec(1.e-4), current_prec(start_prec),
            end_prec(nemo.get_calc()->param.econv), thresh(1.e-4), econv(1.e-4),
            dconv(1.e-3) {
            if (nemo.world.rank()==0) {
                print("starting protocol");
                print("thresh",thresh,"econv ",econv,"dconv",dconv);
                print("start",start_prec,"end",end_prec);
            }
        }

        double start_prec;   ///< starting precision, typically 1.e-4
        double current_prec;   ///< current precision
        double end_prec;     ///< final precision

        double thresh;  ///< numerical precision of representing functions
        double econv;   ///< energy convergence of SCF calculations
        double dconv;   ///< density convergence of SCF calculations

        bool finished() const {
            return (current_prec<end_prec*0.9999);   // account for noise
        }

        /// go to the next level
        protocol& operator++() {
            // ending criterion
            bool finish=(approx(current_prec,end_prec));
            current_prec*=0.1;

            if (not finish) {
                if (current_prec<end_prec) current_prec=end_prec;
                print("setting current_prec to", current_prec);
                infer_thresholds(current_prec);
            }
            return *this;
        }

        /// infer thresholds starting from a target precision
        void infer_thresholds(const double prec) {
            econv=prec;
            thresh=econv;
            dconv=std::min(1.e-3,sqrt(econv)*0.1);
        }

        /// compare two positive doubles to be equal
        bool approx(const double a, const double b) const {
            return (std::abs(a/b-1.0)<1.e-12);
        }
    };

public:

	/// ctor

	/// @param[in]	world1	the world
	/// @param[in]	calc	the SCF
	Nemo(World& world1, std::shared_ptr<SCF> calc) :
			world(world1), calc(calc), ttt(0.0), sss(0.0), coords_sum(-1.0) {}

	void construct_nuclear_correlation_factor() {
		// construct the nuclear potential
		// Make the nuclear potential, initial orbitals, etc.
		calc->make_nuclear_potential(world);
		calc->potentialmanager->vnuclear().print_size("vnuc");
		calc->project_ao_basis(world);
//		save_function(calc->potentialmanager->vnuclear(),"vnuc");
	    // construct the nuclear correlation factor:
	    nuclear_correlation=create_nuclear_correlation_factor(world,*calc);
	    R = nuclear_correlation->function();
	    R.set_thresh(FunctionDefaults<3>::get_thresh());
	    R_square = nuclear_correlation->square();
	    R_square.set_thresh(FunctionDefaults<3>::get_thresh());
	}

	double value() {return value(calc->molecule.get_all_coords());}

	double value(const Tensor<double>& x);

	/// compute the nuclear gradients
	Tensor<double> gradient(const Tensor<double>& x);

	bool provides_gradient() const {return true;}

	/// returns the molecular hessian matrix at structure x
	Tensor<double> hessian(const Tensor<double>& x);

	/// solve the CPHF equations for the nuclear displacements

	/// this function computes that part of the orbital response that is
	/// orthogonal to the occupied space. If no NCF's are used this
	/// corresponds to the normal response. If NCF's are used the part
	/// parallel to the occupied space must be added!
	/// \f[
	///     F^X = F^\perp + F^\parallel
	/// \f]
	/// cf parallel_CPHF()
	/// @param[in]  iatom   the atom A to be moved
	/// @param[in]  iaxis   the coordinate X of iatom to be moved
	/// @return     \ket{i^X} or \ket{F^\perp}
	vecfuncT cphf(const int iatom, const int iaxis, const Tensor<double> fock,
	        const vecfuncT& guess, const protocol& p) const;

	/// solve the CPHF equation for all displacements

	/// this function computes the nemo response F^X
    /// \f[
    ///     F^X = F^\perp + F^\parallel
    /// \f]
	/// To reconstruct the unregularized orbital response (not recommended):
	/// \f[
	///   i^X   = R^X F + R F^X
	/// \f]
	/// The orbital response i^X constructed in this way is automatically
	/// orthogonal to the occupied space because of the parallel term F^\parallel
	/// @return a vector of the nemo response F^X for all displacements
	std::vector<vecfuncT> compute_all_cphf();

    /// this function computes that part of the orbital response that is
    /// parallel to the occupied space.
    /// \f[
    ///     F^X = F^\perp + F^\parallel
    /// \f]
    /// If no NCF's are used F^\parallel vanishes.
    /// If NCF's are used this term does not vanish because the derivatives of
    /// the NCF does not vanish, and it is given by
    /// \f[
    ///  F_i^\parallel = -\frac{1}{2}\sum_k|F_k ><F_k | (R^2)^X | F_i>
    /// \f]
    vecfuncT parallel_CPHF(const vecfuncT& nemo, const int iatom,
            const int iaxis) const;

	/// returns the vibrational frequencies

	/// @param[in]  hessian the hessian matrix
    /// @param[in]  project whether to project out translation and rotation
    /// @param[in]  print_hessian   whether to print the hessian matrix
	/// @return the frequencies in atomic units
	Tensor<double> compute_frequencies(const Tensor<double>& hessian,
	        const bool project, const bool print_hessian) const;

	/// compute the mass-weight the hessian matrix

	/// use as: hessian.emul(massweights);
	/// @param[in]  molecule    for getting access to the atomic masses
	/// @return the mass-weighting matrix for the hessian
	Tensor<double> massweights(const Molecule& molecule) const;

	std::shared_ptr<SCF> get_calc() const {return calc;}

	/// compute the Fock matrix from scratch
	tensorT compute_fock_matrix(const vecfuncT& nemo, const tensorT& occ) const;

	/// return a reference to the molecule
	Molecule& molecule() {return calc->molecule;}

    /// return a reference to the molecule
    Molecule& molecule() const {
        return calc->molecule;
    }

    /// make the density (alpha or beta)
    real_function_3d make_density(const Tensor<double>& occ,
            const vecfuncT& nemo) const;

    /// make the density using different bra and ket vectors

    /// e.g. for computing the perturbed density \sum_i \phi_i \phi_i^X
    /// or when using nemos: \sum_i R2nemo_i nemo_i
    real_function_3d make_density(const tensorT & occ,
            const vecfuncT& bra, const vecfuncT& ket) const;

private:

	/// the world
	World& world;

	std::shared_ptr<SCF> calc;

	mutable double ttt, sss;
	void START_TIMER(World& world) const {
	    world.gop.fence(); ttt=wall_time(); sss=cpu_time();
	}

	void END_TIMER(World& world, const std::string msg) const {
	    END_TIMER(world,msg.c_str());
	}

	void END_TIMER(World& world, const char* msg) const {
	    ttt=wall_time()-ttt; sss=cpu_time()-sss;
	    if (world.rank()==0) printf("timer: %20.20s %8.2fs %8.2fs\n", msg, sss, ttt);
	}

public:

	/// the nuclear correlation factor
	std::shared_ptr<NuclearCorrelationFactor> nuclear_correlation;

	/// the nuclear correlation factor
	real_function_3d R;

    /// the square of the nuclear correlation factor
    real_function_3d R_square;

private:

	/// sum of square of coords at last solved geometry
	mutable double coords_sum;

	/// a poisson solver
	std::shared_ptr<real_convolution_3d> poisson;

	void print_nuclear_corrfac() const;

	/// adapt the thresholds consistently to a common value
    void set_protocol(const double thresh) {

        calc->set_protocol<3>(world,thresh);

        // (re) construct nuclear potential and correlation factors
        construct_nuclear_correlation_factor();

        // (re) construct the Poisson solver
        poisson = std::shared_ptr<real_convolution_3d>(
                CoulombOperatorPtr(world, calc->param.lo, FunctionDefaults<3>::get_thresh()));

        // set thresholds for the MOs
        set_thresh(world,calc->amo,thresh);
        set_thresh(world,calc->bmo,thresh);

    }

	/// solve the HF equations
	double solve(const protocol& proto);

	/// given nemos, compute the HF energy
	double compute_energy(const vecfuncT& psi, const vecfuncT& Jpsi,
			const vecfuncT& Kpsi) const;

    /// given nemos, compute the HF energy using the regularized expressions for T and V
    double compute_energy_regularized(const vecfuncT& nemo, const vecfuncT& Jnemo,
            const vecfuncT& Knemo, const vecfuncT& Unemo) const;

	/// compute the reconstructed orbitals, and all potentials applied on nemo

	/// to use these potentials in the fock matrix computation they must
	/// be multiplied by the nuclear correlation factor
	/// @param[in]	nemo	the nemo orbitals
	/// @param[out]	psi		the reconstructed, full orbitals
	/// @param[out]	Jnemo	Coulomb operator applied on the nemos
	/// @param[out]	Knemo	exchange operator applied on the nemos
	/// @param[out]	Vnemo	nuclear potential applied on the nemos
	/// @param[out]	Unemo	regularized nuclear potential applied on the nemos
	void compute_nemo_potentials(const vecfuncT& nemo, vecfuncT& psi,
			vecfuncT& Jnemo, vecfuncT& Knemo, vecfuncT& Vnemo,
			vecfuncT& Unemo) const;

	/// normalize the nemos
	void normalize(vecfuncT& nemo) const;

    /// orthonormalize the vectors

    /// @param[in,out]	nemo	the vectors to be orthonormalized
    void orthonormalize(vecfuncT& nemo) const;

	/// return the Coulomb potential
	real_function_3d get_coulomb_potential(const vecfuncT& psi) const;

	bool is_dft() const {return calc->xc.is_dft();}

	/// localize the nemo orbitals
	vecfuncT localize(const vecfuncT& nemo) const;

	/// return the threshold for vanishing elements in orbital rotations
    double trantol() const {
        return calc->vtol / std::min(30.0, double(get_calc()->amo.size()));
    }

	template<typename solverT>
	void rotate_subspace(World& world, const tensorT& U, solverT& solver,
			int lo, int nfunc) const;

    tensorT Q2(const tensorT& s) const {
        tensorT Q = -0.5*s;
        for (int i=0; i<s.dim(0); ++i) Q(i,i) += 1.5;
        return Q;
    }

    /// save a function
    template<typename T, size_t NDIM>
    void save_function(const Function<T,NDIM>& f, const std::string name) const;

    /// load a function
    template<typename T, size_t NDIM>
    void load_function(Function<T,NDIM>& f, const std::string name) const;

    /// save a function
    template<typename T, size_t NDIM>
    void save_function(const std::vector<Function<T,NDIM> >& f, const std::string name) const;

    /// load a function
    template<typename T, size_t NDIM>
    void load_function(std::vector<Function<T,NDIM> >& f, const std::string name) const;

};

/// rotate the KAIN subspace (cf. SCF.cc)
template<typename solverT>
void Nemo::rotate_subspace(World& world, const tensorT& U, solverT& solver,
        int lo, int nfunc) const {
    std::vector < vecfunc<double, 3> > &ulist = solver.get_ulist();
    std::vector < vecfunc<double, 3> > &rlist = solver.get_rlist();
    for (unsigned int iter = 0; iter < ulist.size(); ++iter) {
        vecfuncT& v = ulist[iter].x;
        vecfuncT& r = rlist[iter].x;
        vecfuncT vnew = transform(world, vecfuncT(&v[lo], &v[lo + nfunc]), U,
                trantol(), false);
        vecfuncT rnew = transform(world, vecfuncT(&r[lo], &r[lo + nfunc]), U,
                trantol(), true);

        world.gop.fence();
        for (int i=0; i<nfunc; i++) {
            v[i] = vnew[i];
            r[i] = rnew[i];
        }
    }
    world.gop.fence();
}

/// save a function
template<typename T, size_t NDIM>
void Nemo::save_function(const Function<T,NDIM>& f, const std::string name) const {
    if (world.rank()==0) print("saving function",name);
    f.print_size(name);
    archive::ParallelOutputArchive ar(world, name.c_str(), 1);
    ar & f;
}

/// save a function
template<typename T, size_t NDIM>
void Nemo::save_function(const std::vector<Function<T,NDIM> >& f, const std::string name) const {
    if (world.rank()==0) print("saving vector of functions",name);
    archive::ParallelOutputArchive ar(world, name.c_str(), 1);
    ar & f.size();
    for (const Function<T,NDIM>& ff:f)  ar & ff;
}

/// save a function
template<typename T, size_t NDIM>
void Nemo::load_function(std::vector<Function<T,NDIM> >& f, const std::string name) const {
    if (world.rank()==0) print("loading vector of functions",name);
    archive::ParallelInputArchive ar(world, name.c_str(), 1);
    std::size_t fsize=0;
    ar & fsize;
    f.resize(fsize);
    for (std::size_t i=0; i<fsize; ++i) ar & f[i];
}

}

#endif /* NEMO_H_ */

