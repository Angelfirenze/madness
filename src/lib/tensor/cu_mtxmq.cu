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
  
  Part of the code is adopted from Nvidia CUDA sample code and NOT OPTMIZED
*/
#ifndef MADNESS_TENSOR_CU_MTXMQ_H__INCLUDED
#define MADNESS_TENSOR_CU_MTXMQ_H__INCLUDED


#include <madness_config.h>
#define ENABLE_CUBLAS 1  
#include <tensor/cu_mtxmq_kernels.cu>
#include <tensor/cu_mtxmq.h>
//namespace madness {

 

    /// Matrix = Matrix transpose * matrix ... reference implementation

    /// Does \c C=AT*B whereas mTxm does C=C+AT*B.  It also supposed
    /// to be fast which it achieves thru restrictions
    ///   * All dimensions even
    ///   * All pointers aligned
    /// \code
    ///    c(i,j) = sum(k) a(k,i)*b(k,j)  <------ does not accumulate into C
    /// \endcode

 
 template <typename aT, typename bT, typename cT>
    void cu_mTxmq(long dimi, long dimj, long dimk,
               cT* restrict c, const aT* a, const bT* b) {
        printf("gpu code");
        const aT *h_A= a;
	const bT *h_B= b;
	cT *h_C= c;
	aT *d_A;
	aT *d_odata;
	bT *d_B, *hb;
	cT *d_C, *hc;
	aT *ha; 
	dim3 threads = dim3(BLOCK_SIZE,BLOCK_SIZE);
	dim3 grid = dim3(dimj / threads.x, dimi / threads.y);
	
	//if (dimi%BLOCK_SIZE !=0 || dimj%BLOCK_SIZE!=0){
	dim3 threads_rem, grid_rem;
	  
	//  kernel = &transposeNoBankConflicts; 
	
	//}
	//unsigned int tile_size = sizeof(aT) * TILE_DIM * (TILE_DIM+1);
	//unsigned int tile_sizee = sizeof(aT) * TILE_DIM * (TILE_DIM);
	int size_i = dimi + (BLOCK_SIZE-(dimi%BLOCK_SIZE));
	int size_k = dimk + (BLOCK_SIZE-(dimk%BLOCK_SIZE));
	int size_j = dimj + (BLOCK_SIZE-(dimj%BLOCK_SIZE));
	int i,j;
	if (dimi%BLOCK_SIZE !=0 || dimj%BLOCK_SIZE!=0 || dimk%BLOCK_SIZE!=0){
	
	grid_rem = dim3(size_i / BLOCK_SIZE, size_k / BLOCK_SIZE);
	ha =(aT*) malloc(size_i*size_k*sizeof(aT));
	hb =(bT*) malloc(size_j*size_k*sizeof(bT));
	hc =(cT*) malloc(size_k*size_j*sizeof(cT));
	for ( i=0, j=0;i<dimi*dimk; i++, j++)
	{
	  if (i%dimk==0 && i!=0)
	    j=j+ (BLOCK_SIZE-(dimk%BLOCK_SIZE));
	  ha[j]=h_A[i];
//	   printf("hA[%d]=%f\t ",j,ha[j]);
	}
	
	
	for (  i=0, j=0;i<dimj*dimk; i++, j++)
	{
	  if (i%dimj==0 && j!=0)
	    j=j+ (BLOCK_SIZE-(dimj%BLOCK_SIZE));
	  hb[j]=h_B[i];
	}
	}
	
	int A_size = size_i*size_k*sizeof(aT);
	int B_size = size_k*size_j*sizeof(bT);
	int C_size = size_k*size_j*sizeof(cT);
	
	//printf("A_size=%d\n\n\n",A_size);
	
	cudaMalloc((void**)&d_A, A_size) ;
	cudaMalloc((void**)&d_B, B_size) ;
	cudaMalloc((void**)&d_C, C_size) ;
	cudaMalloc((void**)&d_odata, A_size) ;
	cudaMemcpy(d_A, ha, A_size, cudaMemcpyHostToDevice) ; 
	cudaMemcpy(d_B, hb, B_size, cudaMemcpyHostToDevice) ;
	
	//printf("tile size = %u\n",tile_size);
	if (dimi%BLOCK_SIZE !=0 || dimj%BLOCK_SIZE!=0 || dimk%BLOCK_SIZE!=0){
	transposeNoBankConflicts<aT><<<grid_rem, threads>>>(d_odata,d_A, size_k, size_i, 1);
//if ( cudaSuccess != cudaGetLastError() )
  //  printf( "Error!\n" );

//	cudaThreadSynchronize();
//	cudaMemcpy((void *)ha,(void *) d_odata, A_size, cudaMemcpyDeviceToHost) ;
	grid_rem = dim3(size_j / BLOCK_SIZE, size_k / BLOCK_SIZE);
//	  printf("ha[%d]=%f\t ",i,ha[i]);
	matrixMul_coalescing<aT,bT,cT><<< grid_rem, threads >>>(d_C, d_odata, d_B, size_i, size_j);
	
//cudaThreadSynchronize();
	}
	else
	{ transposeNoBankConflicts<aT><<<grid, threads>>>(d_A,d_A, dimk, dimi, 1);
	
	matrixMul_coalescing<aT,bT,cT><<< grid, threads >>>(d_C, d_A, d_B, dimi, dimj);
	}
	//matrixMul_coalescing_rem<aT,bT,cT><<< grid, threads>>>(d_C, d_A, d_B, dimk, dimj, dimi);
	//if (dimi%BLOCK_SIZE !=0 || dimj%BLOCK_SIZE!=0){
	  //threads_rem = dim3(dimj%BLOCK_SIZE,dimi%BLOCK_SIZE);
	//dim3 grid_rem = dim3(1);
	//matrixMul_coalescing_rem<<< grid_rem, threads_rem, tile_sizee >>>(d_C, d_A, d_B, dimk, dimj, dimi);
	//}
	// copy result from device to host
	cudaMemcpy((void *)hc,(void *) d_C, C_size, cudaMemcpyDeviceToHost) ;
       for (  i=0, j=0;i<dimj*dimk; i++, j++)
	{
	  if (i%dimj==0 && i!=0)
	    j=j+ (BLOCK_SIZE-(dimj%BLOCK_SIZE));
	  h_C[i]=hc[j];
	}

	free(ha);
	free(hb);
	free(hc);
	cudaFree(d_odata);
	cudaFree(d_A);
	cudaFree(d_B);
	cudaFree(d_C);
    }

template <> void cu_mTxmq(long m, long n,long k, std::complex<double> *C, const std::complex<double> *A, const double *B){}    
#if !ENABLE_CUBLAS 
      template void cu_mTxmq(long dimi, long dimj, long dimk, float*  c, const float* a, const float* b) ;
     
      template void cu_mTxmq(long m, long n,long k, double *C, const double *A, const double *B);
    
  template <> void cu_mTxmq(long m, long n,long k, std::complex<double> *C, const std::complex<double> *A, const std::complex<double> *B){}
	 
       
#else

  template<>   void cu_mTxmq(long m, long n,long k, double *C, const double *A, const double *B){


	double one=1.0;
	double zero=0.0;
	//printf(" GPU Scublas code execution");
	//sleep(100);
	int M = (int)m;
	int N = (int)n;
	int K = (int)k;
	cublasStatus stat;
	double *devPtrA, *devPtrB, *devPtrC;
	
	cublasInit();
	
	stat = cublasAlloc (M*K, sizeof(double), (void**)&devPtrA);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	stat = cublasAlloc (K*N, sizeof(double), (void**)&devPtrB);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	stat = cublasAlloc (M*N, sizeof(double), (void**)&devPtrC);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	cublasSetMatrix (M, K, sizeof(double), (void *)A, M, (void *)devPtrA, M);
	cublasSetMatrix (K, N, sizeof(double), (void *)B, K, (void *)devPtrB, K);
	//dgemm_("n","t",&nj,&ni,&nk,&one,b,&nj,a,&ni,&zero,c,&nj,1,1);
	//cublasDgemm('t','n',M,N,K,one,devPtrA,K,devPtrB,K,zero,devPtrC,M);
	cublasDgemm('n','t',N,M,K,one,devPtrB,N,devPtrA,M,zero,devPtrC,N);
	int  b=cublasGetError();
	if (b == CUBLAS_STATUS_INVALID_VALUE)
	  printf("CUBLAS_STATUS_INVALID_VALUE");
	else if (b == CUBLAS_STATUS_ARCH_MISMATCH)
	  printf("CUBLAS_STATUS_ARCH_MISMATCH");
        else if (b ==CUBLAS_STATUS_EXECUTION_FAILED )
          printf("CUBLAS_STATUS_EXECUTION_FAILED");
        else if (b ==CUBLAS_STATUS_MAPPING_ERROR )
          printf("CUBLAS_STATUS_MAPPING_ERROR");
        else if (b ==CUBLAS_STATUS_ALLOC_FAILED )
          printf("CUBLAS_STATUS_ALLOC_FAILED");
        else if (b ==CUBLAS_STATUS_NOT_INITIALIZED )
          printf("init CUBLAS_STATUS_NOT_INITIALIZED");
        else if (b ==CUBLAS_STATUS_INTERNAL_ERROR )
          printf("CUBLAS_STATUS_INTERNAL_ERROR");
	//else
	  //printf("Error=%d",b);
	//cublasGetMatrix (K, K, sizeof(double), (void *)devPtrC, K, (void *)C, K);
	//dgemm_("n","t",&nj,&ni,&nk,&one,b,&nj,a,&ni,&zero,c,&nj,1,1);
	//cublasSgemm('n','t',N,M,K,one,devPtrB,N,devPtrA,M,zero,devPtrC,N);
	cublasGetMatrix (M, N, sizeof(double), (void *)devPtrC, M, (void *)C, M);
	cublasFree (devPtrA);
	cublasFree (devPtrB);
	cublasFree (devPtrC);
	cublasShutdown();

    }


/*	
//cudaSetDevice(3);
	double one=1.0;
	double zero=0.0;
	printf(" GPU cublas code execution m=%d, n%d,k=%d",m,n,k);
	//sleep(100);
	int M = (int)m;
	int N = (int)n;
	int K = (int)k;
	cublasStatus stat,b;
	double *devPtrA, *devPtrB, *devPtrC;
 		
	do{
	stat=cublasInit();
	}while(	stat!= CUBLAS_STATUS_SUCCESS);
    b=cublasGetError();
        if (b == CUBLAS_STATUS_INVALID_VALUE)
          printf("CUBLAS_STATUS_INVALID_VALUE");
        else if (b == CUBLAS_STATUS_ARCH_MISMATCH)
          printf("CUBLAS_STATUS_ARCH_MISMATCH");
else if (b ==CUBLAS_STATUS_EXECUTION_FAILED )
          printf("CUBLAS_STATUS_EXECUTION_FAILED");
else if (b ==CUBLAS_STATUS_MAPPING_ERROR )
          printf("CUBLAS_STATUS_MAPPING_ERROR");
else if (b ==CUBLAS_STATUS_ALLOC_FAILED )
          printf("CUBLAS_STATUS_ALLOC_FAILED");
else if (b ==CUBLAS_STATUS_NOT_INITIALIZED )
          printf("init CUBLAS_STATUS_NOT_INITIALIZED");
else if (b ==CUBLAS_STATUS_INTERNAL_ERROR )
          printf("CUBLAS_STATUS_INTERNAL_ERROR");

        else
          printf("CUBLAS_STATUS_SUCCESS");
	
	stat = cublasAlloc (M*K, sizeof(double), (void**)&devPtrA);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	stat = cublasAlloc (K*N, sizeof(double), (void**)&devPtrB);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	stat = cublasAlloc (M*N*4, sizeof(double), (void**)&devPtrC);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	cublasSetMatrix (M, K, sizeof(double), (void *)A, M, (void *)devPtrA, M);
	cublasSetMatrix (K, N, sizeof(double), (void *)B, K, (void *)devPtrB, K);

//do{	
//	cublasFree (devPtrC);
//	stat = cublasAlloc (M*N, sizeof(double), (void**)&devPtrC);
  //      if (stat != CUBLAS_STATUS_SUCCESS) {
    //    printf ("device memory allocation failed");
      //  return ;
       // }

	cublasDgemm('t','n',M,N,K,one,devPtrA,M,devPtrB,K,zero,devPtrC,K);
//cudaThreadSynchronize();

	//cublasDgemm('n','t',N,M,K,one,devPtrB,N,devPtrA,M,zero,devPtrC,N);
	 b=cublasGetError();
//}while(b != CUBLAS_STATUS_SUCCESS);
	if (b == CUBLAS_STATUS_INVALID_VALUE)
	  printf("CUBLAS_STATUS_INVALID_VALUE");
	else if (b == CUBLAS_STATUS_ARCH_MISMATCH)
	  printf("CUBLAS_STATUS_ARCH_MISMATCH");
else if (b ==CUBLAS_STATUS_EXECUTION_FAILED )
          printf("CUBLAS_STATUS_EXECUTION_FAILED");
else if (b ==CUBLAS_STATUS_MAPPING_ERROR )
          printf("CUBLAS_STATUS_MAPPING_ERROR");
else if (b ==CUBLAS_STATUS_ALLOC_FAILED )
          printf("CUBLAS_STATUS_ALLOC_FAILED");
else if (b ==CUBLAS_STATUS_NOT_INITIALIZED )
          printf("CUBLAS_STATUS_NOT_INITIALIZED");
else if (b ==CUBLAS_STATUS_INTERNAL_ERROR )
          printf("CUBLAS_STATUS_INTERNAL_ERROR");

	else
	  printf("CUBLAS_STATUS_SUCCESS");

// make sure Dgemm is finished
        cudaError_t cudaErr = cudaThreadSynchronize();
        if( cudaErr != cudaSuccess ) {
    printf( "Dgemm failed on invocation \n" );
        }
	//cublasGetMatrix (K, K, sizeof(double), (void *)devPtrC, K, (void *)C, K);
	//dgemm_("n","t",&nj,&ni,&nk,&one,b,&nj,a,&ni,&zero,c,&nj,1,1);
	//cublasDgemm('n','t',N,M,K,one,devPtrB,N,devPtrA,M,zero,devPtrC,N);
	cublasGetMatrix (M, N, sizeof(double), (void *)devPtrC, M, (void *)C, M);
	cublasFree (devPtrA);
	cublasFree (devPtrB);
	cublasFree (devPtrC);
	cublasShutdown();
    }
    
  */  
  template<>   void cu_mTxmq(long m, long n,long k, std::complex<double> *C, const std::complex<double> *A, const std::complex<double> *B){
	cuDoubleComplex one;
	one.x=1.0;
	one.y=0.0;
	cuDoubleComplex zero;
	zero.x=0.0;
	zero.y=0.0;
	printf(" complx GPU code execution");
	//sleep(100);
	int M = (int)m;
	int N = (int)n;
	int K = (int)k;
	
	cublasStatus stat;
	cuDoubleComplex *devPtrA, *devPtrB, *devPtrC;
	cuDoubleComplex *A1=(cuDoubleComplex *)A;
	cuDoubleComplex *B1=(cuDoubleComplex *)B;
	//cuDoubleComplex *C1=(cuDoubleComplex *)C;
	cublasInit();
	
	stat = cublasAlloc (M*K, sizeof(cuDoubleComplex), (void**)&devPtrA);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	stat = cublasAlloc (K*N, sizeof(cuDoubleComplex), (void**)&devPtrB);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	stat = cublasAlloc (M*N, sizeof(cuDoubleComplex), (void**)&devPtrC);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	cublasSetMatrix (M, K, sizeof(cuDoubleComplex), (void *)A1, M, (void *)devPtrA, M);
	cublasSetMatrix (K, N, sizeof(cuDoubleComplex), (void *)B1, K, (void *)devPtrB, K);
	
	//cublasZgemm('n','t',N,M,K,one,devPtrB,N,devPtrA,M,zero,devPtrC,K);
	cublasZgemm('n','t',N,M,K,one,devPtrB,N,devPtrA,M,zero,devPtrC,N);
	cublasGetMatrix (N, M, sizeof(*C), (void *)devPtrC, N, (void *)C, N);
	cublasFree (devPtrA);
	cublasFree (devPtrB);
	cublasFree (devPtrC);
	cublasShutdown();

    }


template<>  void cu_mTxmq(long m, long n,long k,float *C, const float *A, const float *B){
	float one=1.0;
	float zero=0.0;
	printf(" GPU Scublas code execution");
	//sleep(100);
	int M = (int)m;
	int N = (int)n;
	int K = (int)k;
	cublasStatus stat;
	float *devPtrA, *devPtrB, *devPtrC;
	
	cublasInit();
	
	stat = cublasAlloc (M*K, sizeof(float), (void**)&devPtrA);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	stat = cublasAlloc (K*N, sizeof(float), (void**)&devPtrB);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	stat = cublasAlloc (M*N, sizeof(float), (void**)&devPtrC);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	cublasSetMatrix (M, K, sizeof(float), (void *)A, M, (void *)devPtrA, M);
	cublasSetMatrix (K, N, sizeof(float), (void *)B, K, (void *)devPtrB, K);
	//dgemm_("n","t",&nj,&ni,&nk,&one,b,&nj,a,&ni,&zero,c,&nj,1,1);
	//cublasDgemm('t','n',M,N,K,one,devPtrA,K,devPtrB,K,zero,devPtrC,M);
	cublasSgemm('n','t',N,M,K,one,devPtrB,N,devPtrA,M,zero,devPtrC,N);
	int  b=cublasGetError();
	if (b == CUBLAS_STATUS_INVALID_VALUE)
	  printf("CUBLAS_STATUS_INVALID_VALUE");
	else if (b == CUBLAS_STATUS_ARCH_MISMATCH)
	  printf("CUBLAS_STATUS_ARCH_MISMATCH");
	else
	  printf("Error=%d",b);
	//cublasGetMatrix (K, K, sizeof(float), (void *)devPtrC, K, (void *)C, K);
	//dgemm_("n","t",&nj,&ni,&nk,&one,b,&nj,a,&ni,&zero,c,&nj,1,1);
	//cublasSgemm('n','t',N,M,K,one,devPtrB,N,devPtrA,M,zero,devPtrC,N);
	cublasGetMatrix (M, N, sizeof(float), (void *)devPtrC, M, (void *)C, M);
	cublasFree (devPtrA);
	cublasFree (devPtrB);
	cublasFree (devPtrC);
	cublasShutdown();

    }
    
    
  template<>   void cu_mTxmq(long m, long n,long k, std::complex<float> *C, const std::complex<float> *A, const std::complex<float> *B){
	cuComplex one;
	one.x=1.0;
	one.y=0.0;
	cuComplex zero;
	zero.x=0.0;
	zero.y=0.0;
	printf(" complx GPU code execution");
	//sleep(100);
	int M = (int)m;
	int N = (int)n;
	int K = (int)k;
	
	cublasStatus stat;
	cuComplex *devPtrA, *devPtrB, *devPtrC;
	cuComplex *A1=(cuComplex *)A;
	cuComplex *B1=(cuComplex *)B;
	//cuDoubleComplex *C1=(cuDoubleComplex *)C;
	cublasInit();
	
	stat = cublasAlloc (M*K, sizeof(cuComplex), (void**)&devPtrA);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	stat = cublasAlloc (K*N, sizeof(cuComplex), (void**)&devPtrB);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	stat = cublasAlloc (M*N, sizeof(cuComplex), (void**)&devPtrC);
	if (stat != CUBLAS_STATUS_SUCCESS) {
	printf ("device memory allocation failed");
	return ;
	}
	
	cublasSetMatrix (M, K, sizeof(cuComplex), (void *)A1, M, (void *)devPtrA, M);
	cublasSetMatrix (K, N, sizeof(cuComplex), (void *)B1, K, (void *)devPtrB, K);
	
	//cublasCgemm('n','t',N,M,K,one,devPtrB,N,devPtrA,M,zero,devPtrC,K);
	cublasCgemm('n','t',N,M,K,one,devPtrB,N,devPtrA,M,zero,devPtrC,N);
	cublasGetMatrix (N, M, sizeof(*C), (void *)devPtrC, N, (void *)C, N);
	cublasFree (devPtrA);
	cublasFree (devPtrB);
	cublasFree (devPtrC);
	cublasShutdown();

    }
#endif

//}
#endif // MADNESS_TENSOR_CU_MTXMQ_H__INCLUDED
