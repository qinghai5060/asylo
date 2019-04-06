/*
 *
 * Copyright 2017 Asylo authors
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

#include "asylo/platform/primitives/sim/untrusted_sim.h"

#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <utility>

#include "absl/base/call_once.h"
#include "absl/debugging/leak_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "asylo/platform/primitives/extent.h"
#include "asylo/platform/primitives/primitive_status.h"
#include "asylo/platform/primitives/primitives.h"
#include "asylo/platform/primitives/sim/shared_sim.h"
#include "asylo/platform/primitives/untrusted_primitives.h"
#include "asylo/platform/primitives/util/status_conversions.h"
#include "asylo/util/error_codes.h"
#include "asylo/util/status.h"
#include "asylo/util/statusor.h"

namespace asylo {
namespace primitives {

namespace {

PrimitiveStatus sim_asylo_exit_call(uint64_t untrusted_selector, void *params) {
  return Client::ExitCallback(
      untrusted_selector, reinterpret_cast<UntrustedParameterStack *>(params));
}

void *sim_asylo_local_alloc_handler(size_t size) { return malloc(size); }

void sim_asylo_local_free_handler(void *ptr) { return free(ptr); }

inline size_t RoundUpToPageBoundary(size_t size) {
  const size_t kPageSize = getpagesize();
  return ((size + kPageSize - 1) / kPageSize) * kPageSize;
}

absl::once_flag init_trampoline_once;

void InitTrampolineOnce() {
  static SimTrampoline *sim_trampoline = nullptr;
  if (sim_trampoline != nullptr) {
    return;
  }
  sim_trampoline = static_cast<SimTrampoline *>(mmap(
      /*addr=*/reinterpret_cast<void *>(sim_trampoline_address),
      /*len=*/RoundUpToPageBoundary(sizeof(SimTrampoline)),
      /*prot=*/PROT_EXEC | PROT_READ | PROT_WRITE,
      /*flags=*/MAP_ANONYMOUS | MAP_PRIVATE,
      /*fd=*/-1,
      /*offset=*/0));
  CHECK_EQ(sim_trampoline, GetSimTrampoline())
      << "Failed to map sim_trampoline, errno=" << strerror(errno);
  GetSimTrampoline()->magic_number = kTrampolineMagicNumber;
  GetSimTrampoline()->version = kTrampolineVersion;
  GetSimTrampoline()->asylo_exit_call = &sim_asylo_exit_call;
  GetSimTrampoline()->asylo_local_alloc_handler =
      &sim_asylo_local_alloc_handler;
  GetSimTrampoline()->asylo_local_free_handler = &sim_asylo_local_free_handler;
}

}  // namespace

SimEnclaveClient::~SimEnclaveClient() {
  if (dl_handle_) {
    if (enclave_call_) {
      UntrustedParameterStack fini_params;
      enclave_call_(kSelectorAsyloFini, &fini_params);
    }
    dlclose(dl_handle_);
  }
}

StatusOr<std::shared_ptr<Client>> SimBackend::Load(
    const std::string &path,
    std::unique_ptr<Client::ExitCallProvider> exit_call_provider) {
  // Initialize trampoline once. absl::call_once guarantees that initialization
  // will run exactly once across all threads, and all other threads will not
  // run it, but will instead wait for the first one to finish running.
  absl::call_once(init_trampoline_once, &InitTrampolineOnce);

  std::shared_ptr<SimEnclaveClient> client(
      new SimEnclaveClient(std::move(exit_call_provider)));

  // Open the enclave shared object file.
  {
    // Make client reference available as thread-local for the time it loads
    // the enclave binary, in order to enable exit calls by the enclave
    // initialization.
    Client::ScopedCurrentClient scoped_client(client.get());

    // dlopen may allocate resources which are not disposed by dlclose.
    absl::LeakCheckDisabler disabler;
    client->dl_handle_ = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
  }
  if (!client->dl_handle_) {
    return Status{
        error::GoogleError::NOT_FOUND,
        absl::StrCat("dlopen of ", path, " failed with: ", dlerror())};
  }

  // Resolve and set the enclave entry point trampoline.
  void *asylo_enclave_call = dlsym(client->dl_handle_, "asylo_enclave_call");
  if (!asylo_enclave_call) {
    return Status{error::GoogleError::NOT_FOUND,
                  "Could not resolve enclave entry handler: "
                  "asylo_enclave_call"};
  }
  client->enclave_call_ = reinterpret_cast<EnclaveCallPtr>(asylo_enclave_call);

  return client;
}

Status SimEnclaveClient::Destroy() {
  if (dl_handle_) {
    dlclose(dl_handle_);
    dl_handle_ = nullptr;
  }
  return Status::OkStatus();
}

Status SimEnclaveClient::EnclaveCallInternal(uint64_t selector,
                                             UntrustedParameterStack *params) {
  PrimitiveStatus primitive_status;

  // Ensure client is properly initialized.
  if (!dl_handle_ || !enclave_call_) {
    return Status{error::GoogleError::FAILED_PRECONDITION,
                  "Enclave client closed or uninitialized."};
  }

  const auto status = enclave_call_(selector, params);
  return MakeStatus(status);
}

bool SimEnclaveClient::IsClosed() const { return dl_handle_ == nullptr; }

}  // namespace primitives
}  // namespace asylo