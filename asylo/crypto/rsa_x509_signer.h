/*
 * Copyright 2020 Asylo authors
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
 */

#ifndef ASYLO_CRYPTO_RSA_2048_SIGNING_KEY_H_
#define ASYLO_CRYPTO_RSA_2048_SIGNING_KEY_H_

#include <openssl/base.h>
#include <openssl/rsa.h>

#include <memory>

#include "asylo/crypto/algorithms.pb.h"
#include "asylo/crypto/util/byte_container_view.h"
#include "asylo/crypto/x509_signer.h"
#include "asylo/util/cleansing_types.h"

namespace asylo {

// An implementation of the X509Signer interface that uses RSA 2048-bit
// keys for signing and SHA256 for message hashing.
class RsaX509Signer : public X509Signer {
 public:
  // Creates an RSA 2048-bit X509Signer from the given PEM-encoded
  // |serialized_private_key|.
  static StatusOr<std::unique_ptr<RsaX509Signer>> CreateFromPem(
      ByteContainerView serialized_private_key, const EVP_MD* hash);

  int KeySizeInBits() const;

  // From X509Signer.
  StatusOr<CleansingVector<char>> SerializeToPem() const override;

  Status SignX509(X509* x509) const override;

 private:
  explicit RsaX509Signer(bssl::UniquePtr<RSA> private_key, const EVP_MD* hash);

  bssl::UniquePtr<RSA> private_key_;
  const EVP_MD* hash_;
};

}  // namespace asylo

#endif  // ASYLO_CRYPTO_RSA_2048_SIGNING_KEY_H_
