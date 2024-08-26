//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#include "ozz/base/maths/soa_math_archive.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/maths/soa_float.h"
#include "ozz/base/maths/soa_float4x4.h"
#include "ozz/base/maths/soa_quaternion.h"
#include "ozz/base/maths/soa_transform.h"

namespace ozz {
namespace io {
void Extern<math::SoaFloat2>::Save(OArchive& _archive,
                                   const math::SoaFloat2* _values,
                                   size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
                        2 * 4 * _count);
}
void Extern<math::SoaFloat2>::Load(IArchive& _archive, math::SoaFloat2* _values,
                                   size_t _count, uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->x), 2 * 4 * _count);
}

void Extern<math::SoaFloat3>::Save(OArchive& _archive,
                                   const math::SoaFloat3* _values,
                                   size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
                        3 * 4 * _count);
}
void Extern<math::SoaFloat3>::Load(IArchive& _archive, math::SoaFloat3* _values,
                                   size_t _count, uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->x), 3 * 4 * _count);
}

void Extern<math::SoaFloat4>::Save(OArchive& _archive,
                                   const math::SoaFloat4* _values,
                                   size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
                        4 * 4 * _count);
}
void Extern<math::SoaFloat4>::Load(IArchive& _archive, math::SoaFloat4* _values,
                                   size_t _count, uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->x), 4 * 4 * _count);
}

void Extern<math::SoaQuaternion>::Save(OArchive& _archive,
                                       const math::SoaQuaternion* _values,
                                       size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
                        4 * 4 * _count);
}
void Extern<math::SoaQuaternion>::Load(IArchive& _archive,
                                       math::SoaQuaternion* _values,
                                       size_t _count, uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->x), 4 * 4 * _count);
}

void Extern<math::SoaFloat4x4>::Save(OArchive& _archive,
                                     const math::SoaFloat4x4* _values,
                                     size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->cols[0].x),
                        4 * 4 * 4 * _count);
}
void Extern<math::SoaFloat4x4>::Load(IArchive& _archive,
                                     math::SoaFloat4x4* _values, size_t _count,
                                     uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->cols[0].x),
                        4 * 4 * 4 * _count);
}

void Extern<math::SoaTransform>::Save(OArchive& _archive,
                                      const math::SoaTransform* _values,
                                      size_t _count) {
  _archive << MakeArray(reinterpret_cast<const float*>(&_values->translation.x),
                        10 * 4 * _count);
}
void Extern<math::SoaTransform>::Load(IArchive& _archive,
                                      math::SoaTransform* _values,
                                      size_t _count, uint32_t _version) {
  (void)_version;
  _archive >> MakeArray(reinterpret_cast<float*>(&_values->translation.x),
                        10 * 4 * _count);
}
}  // namespace io
}  // namespace ozz
