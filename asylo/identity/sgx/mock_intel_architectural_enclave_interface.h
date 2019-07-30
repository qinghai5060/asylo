/*
 *
 * Copyright 2019 Asylo authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef ASYLO_IDENTITY_SGX_MOCK_INTEL_ARCHITECTURAL_ENCLAVE_INTERFACE_H_
#define ASYLO_IDENTITY_SGX_MOCK_INTEL_ARCHITECTURAL_ENCLAVE_INTERFACE_H_

#include <cstdint>
#include <string>

#include <gmock/gmock.h>
#include "absl/types/span.h"
#include "asylo/crypto/util/bytes.h"
#include "asylo/identity/sgx/identity_key_management_structs.h"
#include "asylo/identity/sgx/intel_architectural_enclave_interface.h"
#include "asylo/util/status.h"

namespace asylo {
namespace sgx {

class MockIntelArchitecturalEnclaveInterface
    : public IntelArchitecturalEnclaveInterface {
 public:
  MOCK_METHOD2(GetPceTargetinfo, Status(Targetinfo *, uint16_t *));
  MOCK_METHOD7(GetPceInfo,
               Status(const Report &, absl::Span<const uint8_t>, uint8_t,
                      std::string *, uint16_t *, uint16_t *, uint8_t *));
  MOCK_METHOD4(PceSignReport, Status(const Report &, uint16_t target_pce_svn,
                                     UnsafeBytes<kCpusvnSize>, std::string *));
};

}  // namespace sgx
}  // namespace asylo

#endif  // ASYLO_IDENTITY_SGX_MOCK_INTEL_ARCHITECTURAL_ENCLAVE_INTERFACE_H_
