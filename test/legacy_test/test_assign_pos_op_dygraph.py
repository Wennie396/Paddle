#   Copyright (c) 2024 PaddlePaddle Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import unittest

import numpy as np

import paddle
from paddle.base import core
from paddle.distributed.models.moe import utils


def assign_pos(x, _cum_count):
    cum_count = np.copy(_cum_count)
    x = x.reshape(-1)
    res = np.zeros((cum_count[-1],), dtype=np.int64)
    for i, idx in enumerate(x):
        p = cum_count[idx]
        cum_count[idx] -= 1
        if p >= 1:
            res[p - 1] = i
    return res


def count(x, upper_num):
    res = np.zeros((upper_num,)).astype(int)
    for i in x.reshape(-1):
        if i >= 0 and i < len(res):
            res[i] += 1
    return res


# why defining the assert function specially?
# Because assign_pos_op is multithread-op, which can make the order of numbers
# in each counter(bin) is random. But the numbers set is certain in each counter(bin).
np_allclose = np.allclose


def assert_allclose(res, out, cum_count):
    c0 = 0
    for c in cum_count:
        if c == c0:
            continue
        data1 = np.copy(res[c0:c])
        data2 = np.copy(out[c0:c])
        data1.sort()
        data2.sort()
        assert np_allclose(data2, data1)
        c0 = c
    return True


@unittest.skipIf(
    not core.is_compiled_with_cuda(), "core is not compiled with CUDA"
)
class TestAssignPosAPI(unittest.TestCase):
    def setUp(self):
        self.x = np.random.randint(0, 16, size=(100, 2)).astype("int64")
        y = count(self.x, 16)
        self.cum_count = np.cumsum(y).astype(self.x.dtype)
        self.out = assign_pos(self.x, self.cum_count)
        self.place = paddle.CUDAPlace(0)

    def test_api_dygraph(self):
        paddle.disable_static()
        x = paddle.to_tensor(self.x)
        cum_count = paddle.to_tensor(self.cum_count).astype(x.dtype)

        out = utils._assign_pos(x, cum_count)
        assert_allclose(out.numpy(), self.out, self.cum_count)


if __name__ == '__main__':
    paddle.enable_static()
    unittest.main()
