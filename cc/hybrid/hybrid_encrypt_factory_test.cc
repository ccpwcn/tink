// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////

#include "cc/hybrid/hybrid_encrypt_factory.h"

#include "cc/hybrid_encrypt.h"
#include "cc/crypto_format.h"
#include "cc/keyset_handle.h"
#include "cc/util/status.h"
#include "cc/util/test_util.h"
#include "gtest/gtest.h"
#include "proto/ecies_aead_hkdf.pb.h"
#include "proto/tink.pb.h"

using crypto::tink::test::AddRawKey;
using crypto::tink::test::AddTinkKey;
using google::crypto::tink::EciesAeadHkdfKeyFormat;
using google::crypto::tink::EciesAeadHkdfPublicKey;
using google::crypto::tink::EcPointFormat;
using google::crypto::tink::EllipticCurveType;
using google::crypto::tink::HashType;
using google::crypto::tink::KeyData;
using google::crypto::tink::Keyset;
using google::crypto::tink::KeyStatusType;
using google::crypto::tink::KeyTemplate;

namespace crypto {
namespace tink {
namespace {

class HybridEncryptFactoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
  }
  void TearDown() override {
  }
};

EciesAeadHkdfPublicKey GetNewEciesPublicKey() {
  auto ecies_key = test::GetEciesAesGcmHkdfTestKey(
      EllipticCurveType::NIST_P256, EcPointFormat::UNCOMPRESSED,
      HashType::SHA256, 24);
  return ecies_key.public_key();
}

TEST_F(HybridEncryptFactoryTest, testBasic) {
  EXPECT_TRUE(HybridEncryptFactory::RegisterStandardKeyTypes().ok());
  EXPECT_TRUE(HybridEncryptFactory::RegisterLegacyKeyTypes().ok());

  Keyset keyset;
  KeysetHandle keyset_handle(keyset);
  auto hybrid_encrypt_result =
      HybridEncryptFactory::GetPrimitive(keyset_handle);
  EXPECT_FALSE(hybrid_encrypt_result.ok());
  EXPECT_EQ(util::error::INVALID_ARGUMENT,
      hybrid_encrypt_result.status().error_code());
  EXPECT_PRED_FORMAT2(testing::IsSubstring, "no primary",
      hybrid_encrypt_result.status().error_message());
}

TEST_F(HybridEncryptFactoryTest, testPrimitive) {
  // Prepare a Keyset.
  Keyset keyset;
  std::string key_type =
      "type.googleapis.com/google.crypto.tink.EciesAeadHkdfPublicKey";

  uint32_t key_id_1 = 1234543;

  AddTinkKey(key_type, key_id_1, GetNewEciesPublicKey(), KeyStatusType::ENABLED,
             KeyData::ASYMMETRIC_PUBLIC, &keyset);

  uint32_t key_id_2 = 726329;
  AddRawKey(key_type, key_id_2, GetNewEciesPublicKey(), KeyStatusType::ENABLED,
            KeyData::ASYMMETRIC_PUBLIC, &keyset);

  uint32_t key_id_3 = 7213743;
  AddTinkKey(key_type, key_id_3, GetNewEciesPublicKey(), KeyStatusType::ENABLED,
             KeyData::ASYMMETRIC_PUBLIC, &keyset);

  keyset.set_primary_key_id(key_id_3);

  // Create a KeysetHandle and use it with the factory.
  KeysetHandle keyset_handle(keyset);
  auto hybrid_encrypt_result =
      HybridEncryptFactory::GetPrimitive(keyset_handle);
  EXPECT_TRUE(hybrid_encrypt_result.ok()) << hybrid_encrypt_result.status();
  auto hybrid_encrypt = std::move(hybrid_encrypt_result.ValueOrDie());

  // Test the resulting HybridEncrypt-instance.
  std::string plaintext = "some plaintext";
  std::string context_info = "some context info";

  auto encrypt_result = hybrid_encrypt->Encrypt(plaintext, context_info);
  EXPECT_TRUE(encrypt_result.ok()) << encrypt_result.status();
}

}  // namespace
}  // namespace tink
}  // namespace crypto


int main(int ac, char* av[]) {
  testing::InitGoogleTest(&ac, av);
  return RUN_ALL_TESTS();
}
