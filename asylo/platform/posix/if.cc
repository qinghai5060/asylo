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

#include <net/if.h>

#include "asylo/platform/host_call/trusted/host_calls.h"

extern "C" {

unsigned int if_nametoindex(const char *ifname) {
  return enc_untrusted_if_nametoindex(ifname);
}

char *if_indextoname(unsigned int ifindex, char *ifname) {
  return enc_untrusted_if_indextoname(ifindex, ifname);
}

}  // extern "C"
