// Copyright (c) 2023 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "paddle/phi/core/kernel_registry.h"
#include "paddle/phi/kernels/elementwise_add_kernel.h"
#include "paddle/phi/kernels/softmax_kernel.h"

namespace phi {
namespace fusion {

template <typename T, typename Context>
void FusedSoftmaxMaskKernel(const Context& dev_ctx,
                            const DenseTensor& x,
                            const DenseTensor& mask,
                            DenseTensor* out) {
  auto x_dim = x.dims();
  auto mask_dim = mask.dims();
  auto query_seq_len = x_dim[2];
  auto key_seq_len = x_dim[3];

  PADDLE_ENFORCE_GT(query_seq_len,
                    1,
                    phi::errors::InvalidArgument(
                        "Input x's second last dim must be large than 1 but "
                        "received the second last dimension of x is %d",
                        query_seq_len));

  PADDLE_ENFORCE_EQ(key_seq_len >= 32 && key_seq_len < 8192,
                    true,
                    phi::errors::InvalidArgument(
                        "Input x's last dim must be between [32, 8192) "
                        "received the last dimension of x is %d",
                        key_seq_len));

  PADDLE_ENFORCE_EQ(mask_dim[1],
                    1,
                    phi::errors::InvalidArgument(
                        "Input mask's second dim must be 1 "
                        "received the second dimension of mask is %d",
                        mask_dim[1]));

  // dim of x and mask must be equal
  for (size_t idx = 0; idx < 4; ++idx) {
    if (idx == 1) continue;
    PADDLE_ENFORCE_EQ(
        x_dim[idx],
        mask_dim[idx],
        phi::errors::InvalidArgument(
            "Input x's %dth dim should be equal with input mask's %dth dim "
            "but "
            "received the %dth dimension of x and mask are not equal "
            "the %dth dim of x is %d, while the %dth dim of mask is %d.",
            idx,
            idx,
            idx,
            idx,
            x_dim[idx],
            idx,
            mask_dim[idx]));
  }
  DenseTensor t = phi::Add<T, Context>(dev_ctx, x, mask);
  SoftmaxKernel<T, Context>(dev_ctx, t, 3, out);  // axis for softmax
}

}  // namespace fusion
}  // namespace phi

PD_REGISTER_KERNEL(fused_softmax_mask,
                   CPU,
                   ALL_LAYOUT,
                   phi::fusion::FusedSoftmaxMaskKernel,
                   float,
                   double) {}
