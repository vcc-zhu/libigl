// This file is part of libigl, a simple c++ geometry processing library.
//
// Copyright (C) 2014 Daniele Panozzo <daniele.panozzo@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
#include "triangle_triangle_adjacency.h"
#include "is_edge_manifold.h"
#include "all_edges.h"
#include "unique_simplices.h"
#include "unique_edge_map.h"
#include <algorithm>

template <typename Scalar, typename Index>
IGL_INLINE void igl::triangle_triangle_adjacency_preprocess(const Eigen::PlainObjectBase<Scalar>& /*V*/,
                                   const Eigen::PlainObjectBase<Index>& F,
                                   std::vector<std::vector<int> >& TTT)
{
  for(int f=0;f<F.rows();++f)
    for (int i=0;i<F.cols();++i)
    {
      // v1 v2 f ei
      int v1 = F(f,i);
      int v2 = F(f,(i+1)%F.cols());
      if (v1 > v2) std::swap(v1,v2);
      std::vector<int> r(4);
      r[0] = v1; r[1] = v2;
      r[2] = f;  r[3] = i;
      TTT.push_back(r);
    }
  std::sort(TTT.begin(),TTT.end());
}

// Extract the face adjacencies
template <typename Index>
IGL_INLINE void igl::triangle_triangle_adjacency_extractTT(const Eigen::PlainObjectBase<Index>& F,
                                  std::vector<std::vector<int> >& TTT,
                                  Eigen::PlainObjectBase<Index>& TT)
{
  TT = Eigen::PlainObjectBase<Index>::Constant((int)(F.rows()),F.cols(),-1);

  for(int i=1;i<(int)TTT.size();++i)
  {
    std::vector<int>& r1 = TTT[i-1];
    std::vector<int>& r2 = TTT[i];
    if ((r1[0] == r2[0]) && (r1[1] == r2[1]))
    {
      TT(r1[2],r1[3]) = r2[2];
      TT(r2[2],r2[3]) = r1[2];
    }
  }
}

// Extract the face adjacencies indices (needed for fast traversal)
template <typename Index>
IGL_INLINE void igl::triangle_triangle_adjacency_extractTTi(const Eigen::PlainObjectBase<Index>& F,
                                   std::vector<std::vector<int> >& TTT,
                                   Eigen::PlainObjectBase<Index>& TTi)
{
  TTi = Eigen::PlainObjectBase<Index>::Constant((int)(F.rows()),F.cols(),-1);

  for(int i=1;i<(int)TTT.size();++i)
  {
    std::vector<int>& r1 = TTT[i-1];
    std::vector<int>& r2 = TTT[i];
    if ((r1[0] == r2[0]) && (r1[1] == r2[1]))
    {
      TTi(r1[2],r1[3]) = r2[3];
      TTi(r2[2],r2[3]) = r1[3];
    }
  }
}

// Compute triangle-triangle adjacency
template <typename Scalar, typename Index>
IGL_INLINE void igl::triangle_triangle_adjacency(const Eigen::PlainObjectBase<Scalar>& V,
                        const Eigen::PlainObjectBase<Index>& F,
                        Eigen::PlainObjectBase<Index>& TT)
{
  //assert(igl::is_edge_manifold(V,F));
  std::vector<std::vector<int> > TTT;

  triangle_triangle_adjacency_preprocess(V,F,TTT);
  triangle_triangle_adjacency_extractTT(F,TTT,TT);
}

// Compute triangle-triangle adjacency with indices
template <typename Scalar, typename Index>
IGL_INLINE void igl::triangle_triangle_adjacency(const Eigen::PlainObjectBase<Scalar>& V,
                        const Eigen::PlainObjectBase<Index>& F,
                        Eigen::PlainObjectBase<Index>& TT,
                        Eigen::PlainObjectBase<Index>& TTi)
{
  //assert(igl::is_edge_manifold(V,F));
  std::vector<std::vector<int> > TTT;

  triangle_triangle_adjacency_preprocess(V,F,TTT);
  triangle_triangle_adjacency_extractTT(F,TTT,TT);
  triangle_triangle_adjacency_extractTTi(F,TTT,TTi);
}

template <
  typename DerivedF, 
  typename TTIndex, 
  typename TTiIndex>
  IGL_INLINE void igl::triangle_triangle_adjacency(
    const Eigen::PlainObjectBase<DerivedF> & F,
    std::vector<std::vector<std::vector<TTIndex> > > & TT,
    std::vector<std::vector<std::vector<TTiIndex> > > & TTi)
{
  return triangle_triangle_adjacency(F,true,TT,TTi);
}

template <
  typename DerivedF, 
  typename TTIndex>
  IGL_INLINE void igl::triangle_triangle_adjacency(
    const Eigen::PlainObjectBase<DerivedF> & F,
    std::vector<std::vector<std::vector<TTIndex> > > & TT)
{
  std::vector<std::vector<std::vector<TTIndex> > > not_used;
  return triangle_triangle_adjacency(F,false,TT,not_used);
}

template <
  typename DerivedF, 
  typename TTIndex, 
  typename TTiIndex>
  IGL_INLINE void igl::triangle_triangle_adjacency(
    const Eigen::PlainObjectBase<DerivedF> & F,
    const bool construct_TTi,
    std::vector<std::vector<std::vector<TTIndex> > > & TT,
    std::vector<std::vector<std::vector<TTiIndex> > > & TTi)
{
  using namespace Eigen;
  using namespace std;
  assert(F.cols() == 3 && "Faces must be triangles");
  // number of faces
  const int m = F.rows();
  typedef typename DerivedF::Index Index;
  typedef Matrix<typename DerivedF::Scalar,Dynamic,2> MatrixX2I;
  typedef Matrix<typename DerivedF::Index,Dynamic,1> VectorXI;
  MatrixX2I E,uE;
  VectorXI EMAP;
  vector<vector<Index> > uE2E;
  unique_edge_map(F,E,uE,EMAP,uE2E);
  return triangle_triangle_adjacency(E,EMAP,uE2E,construct_TTi,TT,TTi);
}

template <
  typename DerivedE, 
  typename DerivedEMAP,
  typename uE2EType,
  typename TTIndex, 
  typename TTiIndex>
  IGL_INLINE void igl::triangle_triangle_adjacency(
    const Eigen::PlainObjectBase<DerivedE> & E,
    const Eigen::PlainObjectBase<DerivedEMAP> & EMAP,
    const std::vector<std::vector<uE2EType> > & uE2E,
    const bool construct_TTi,
    std::vector<std::vector<std::vector<TTIndex> > > & TT,
    std::vector<std::vector<std::vector<TTiIndex> > > & TTi)
{
  using namespace std;
  using namespace Eigen;
  typedef typename DerivedE::Index Index;
  const size_t m = E.rows()/3;
  assert(E.rows() == m*3 && "E should come from list of triangles.");
  // E2E[i] --> {j,k,...} means face edge i corresponds to other faces edges j
  // and k
  TT.resize (m,vector<vector<TTIndex> >(3));
  if(construct_TTi)
  {
    TTi.resize(m,vector<vector<TTiIndex> >(3));
  }

  // No race conditions because TT*[f][c]'s are in bijection with e's
  // Minimum number of iterms per openmp thread
  const size_t ne = E.rows();
# ifndef IGL_OMP_MIN_VALUE
#   define IGL_OMP_MIN_VALUE 1000
# endif
# pragma omp parallel for if (ne>IGL_OMP_MIN_VALUE)
  // Slightly better memory access than loop over E
  for(Index f = 0;f<m;f++)
  {
    for(Index c = 0;c<3;c++)
    {
      const Index e = f + m*c;
      const Index i = E(e,0);
      const Index j = E(e,1);
      //const Index f = e%m;
      //const Index c = e/m;
      const vector<Index> & N = uE2E[EMAP(e)];
      for(const auto & ne : N)
      {
        const Index nf = ne%m;
        TT[f][c].push_back(nf);
        if(construct_TTi)
        {
          const Index nc = ne/m;
          TTi[f][c].push_back(nc);
        }
      }
    }
  }
}

#ifdef IGL_STATIC_LIBRARY
// Explicit template specialization
#endif
