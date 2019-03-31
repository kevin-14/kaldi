// tensor/tensor-pattern.h

//  Copyright      2019  Johns Hopkins University (author: Daniel Povey)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#ifndef KALDI_TENSOR_TENSOR_PATTERN_H_
#define KALDI_TENSOR_TENSOR_PATTERN_H_ 1

#include "tensor/tensor-common.h"
#include <limits>

/**
   This is some notes on plans for kaldi10 tensor stuff, nothing is fully fleshed out.
*/

namespace kaldi {
namespace tensor {



/*
  This struct stores the dimension and strides of a Tensor.

   *SHIFTING TO THE RIGHT*

  The main thing to watch out for is that the dimensions of 'dims' and 'strides'
  to look at is not 0 ... num_axes, but KALDI_TENSOR_MAX_DIM - num_axes
  ... KALDI_TENSOR_MAX_DIM - 1.  The last dimension is always located at
  KALDI_TENSOR_MAX_DIM - 1, i.e. the dims and strides are always
  right-justified.  In addition, for unused axes, we always maintain dim=1 and
  stride=0. This happens to be quite convenient for implementation if we adopt
  the standard broadcasting rules in things like PyTorch, whereby the
  highest-numbered axes always line up.

  Below we describe the the properties that a TensorPattern is required to have.

  These properties are stricter than some other frameworks, such as PyTorch,
  which allow the users to manually add dimensions with stride 0 and dim > 1 so
  that a lower-dimensional quantity can masquerade as one with a higher
  dimension.  We require that it never be possible to access the same memory
  location using two different tuples of indexes.  We also don't allow zero dims
  (i.e. a Tensor that is initialized must not have num_elemnts==0).  If you want
  an empty Tensor, just use a null pointer.  In addition, we require that the
  stride equal zero for any axis that has dim = 1.

  Our requirements on a TensorPattern are:

    0 <= num_axes <= KALDI_TENSOR_MAX_DIM.

    for 0 <= i < num_axes:
       dims[i] > 0
       if dims[i] == 1, then strides[i] = 0.
       if dims[i] != 1, then strides[i] != 0

    for num_axes <= i < KALDI_TENSOR_MAX_DIM:
       dims[i] == 1
       strides[i] == 0

    ... plus the uniqueness property.

  Note: in the public interface of class Tensor, if you ask for
  dim(i) it will return pattern.dims[num_axes - i].

  The uniqueness property requires that we must not be able to access the same
  memory location via two different tuples of indexes).  Recause testing this
  property exactly would be difficult in general without bringing in concepts
  from number theory, we test a slightly stronger version of it that covers all
  cases we are likely to encounter.  This is that, if we take all the axes with
  dim != 1 and sort them from greatest to least stride, then for each i,
  abs(strides[i]) >= dims[i+1] * abs(strides[i+1]).
*/
struct TensorPattern {
  int32 num_axes;
  int32 dims[KALDI_TENSOR_MAX_DIM];     // the dims in reversed order, indexed
                                        // by 'raxis' (reversed axis)
  int32 strides[KALDI_TENSOR_MAX_DIM];  // the strides in reversed order,
                                        // indexed by 'raxis' (reversed axis)
  int32 code;  // pattern code; see ComputePatternCode() in tensor-pattern-utils.h
               // for details.  It is the responsibility of the user to keep
               // this updated (i.e. don't change dims or strides without updating
               // 'code').

  // Returns true if the TensorPattern is valid, I.e. that it satifies all the
  // properties mentioned above.
  //
  //  @param [in] check_code   If true, the check includes verifying that the
  //                        'code' has the value it should (c.f. GetPatternCode()).
  //  @return     Returns true if valid, false if not valid.
  bool IsValid(bool check_code = true);
};


// We may later get rid of this struct and just have functions to get
// these properties.
struct TensorPatternProperties {
  // Below are cached properties that are derived from the underlying data in
  // struct TensorPattern.

  // The number of elements in the Tensor, which equals the product
  // of dims[0] .. dims[num_axes - 1].  Will always be >0.
  int64 num_elements;


  // Binary code describing the pattern, see GetPatternCode() in
  // tensor-pattern-utils.h.
  int32 code;

  // is_contiguous means that the data form a contiguous block in memory; it is
  // not the same as PyTorch's is_contiguous which is a stronger condition,
  // implying also that the strides are as for a `C-style` array.
  bool is_contiguous;

  // has_c_strides means that the strides of all axes i with dim[i] != 1,
  // equal the product of all later-numbered dims, i.e.
  // \f$ strides[i] = \prod_{j>i} dim[j] \f$, or `strides[i] = 0` if
  // dim[i] == 1 (since we use the convention that axes with dim=1 always
  // have stride=0.
  // has_c_strides is the equivalent of PyTorch's is_contiguous.
  // this->has_c_strides implies this->is_contiguous.
  bool has_c_strides;

  // Sets the members of *this to be the properties of pattern 'pattern'.
  // Ignores the previously existing values of *this.
  void UpdateProperties(const TensorPattern &pattern);
};


}  // namespace tensor
}  // namespace kaldi


#endif  // KALDI_TENSOR_TENSOR_PATTERN_H_
