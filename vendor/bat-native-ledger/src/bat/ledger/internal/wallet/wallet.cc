/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet.h"

#include <map>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

#include "wally_bip39.h"  // NOLINT

namespace ledger {
namespace wallet {

Wallet::Wallet(LedgerImpl* ledger)
    : ledger_(ledger),
      create_(std::make_unique<WalletCreate>(ledger)),
      recover_(std::make_unique<WalletRecover>(ledger)),
      balance_(std::make_unique<WalletBalance>(ledger)),
      promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {}

Wallet::~Wallet() = default;

void Wallet::CreateWalletIfNecessary(ledger::ResultCallback callback) {
  create_->CreateWallet(absl::nullopt, std::move(callback));
}

std::string Wallet::GetWalletPassphrase(mojom::RewardsWalletPtr wallet) {
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  if (wallet->recovery_seed.empty()) {
    BLOG(0, "Seed is empty");
    return "";
  }

  char* words = nullptr;
  const int result =
      bip39_mnemonic_from_bytes(nullptr, &wallet->recovery_seed.front(),
                                wallet->recovery_seed.size(), &words);

  if (result != 0) {
    BLOG(0, "Bip39 failed: " << result);
    NOTREACHED();
    return "";
  }

  const std::string pass_phrase = words;
  wally_free_string(words);

  return pass_phrase;
}

void Wallet::RecoverWallet(const std::string& pass_phrase,
                           ledger::LegacyResultCallback callback) {
  recover_->Start(pass_phrase, [this, callback](const mojom::Result result) {
    if (result == mojom::Result::LEDGER_OK) {
      ledger_->database()->DeleteAllBalanceReports(
          [](const mojom::Result _) {});
      DisconnectAllWallets(callback);
      return;
    }
    callback(result);
  });
}

void Wallet::FetchBalance(ledger::FetchBalanceCallback callback) {
  balance_->Fetch(std::move(callback));
}

void Wallet::ExternalWalletAuthorization(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  CreateWalletIfNecessary(
      base::BindOnce(&Wallet::OnCreateWalletIfNecessary, base::Unretained(this),
                     std::move(callback), wallet_type, args));
}

void Wallet::OnCreateWalletIfNecessary(
    ledger::ExternalWalletAuthorizationCallback callback,
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Wallet couldn't be created");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  AuthorizeWallet(wallet_type, args, callback);
}

void Wallet::AuthorizeWallet(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  if (wallet_type == constant::kWalletUphold) {
    ledger_->uphold()->WalletAuthorization(args, callback);
    return;
  }

  if (wallet_type == constant::kWalletBitflyer) {
    ledger_->bitflyer()->WalletAuthorization(args, callback);
    return;
  }

  if (wallet_type == constant::kWalletGemini) {
    ledger_->gemini()->WalletAuthorization(args, callback);
    return;
  }

  NOTREACHED();
  callback(mojom::Result::LEDGER_ERROR, {});
}

void Wallet::DisconnectWallet(const std::string& wallet_type,
                              ledger::LegacyResultCallback callback) {
  if (wallet_type == constant::kWalletUphold) {
    if (const auto uphold_wallet = ledger_->uphold()->GetWallet()) {
      switch (uphold_wallet->status) {
        case mojom::WalletStatus::DISCONNECTED_VERIFIED:
          DCHECK(uphold_wallet->token.empty());
          DCHECK(uphold_wallet->address.empty());
          break;
        case mojom::WalletStatus::PENDING:
          DCHECK(!uphold_wallet->token.empty());
          DCHECK(uphold_wallet->address.empty());
          break;
        case mojom::WalletStatus::VERIFIED:
          DCHECK(!uphold_wallet->token.empty());
          DCHECK(!uphold_wallet->address.empty());
          break;
        default:
          BLOG(0,
               "Wallet status should have been either DISCONNECTED_VERIFIED, "
               "PENDING, or VERIFIED!");
      }
    } else {
      BLOG(0, "Uphold wallet is null!");
    }

    return promotion_server_->delete_claim()->Request(
        constant::kWalletUphold, [this, callback](const mojom::Result result) {
          if (result != mojom::Result::LEDGER_OK) {
            const auto uphold_wallet = ledger_->uphold()->GetWallet();
            if (!uphold_wallet) {
              BLOG(0, "Uphold wallet is null!");
              BLOG(0, "Wallet unlinking failed!");
              return callback(mojom::Result::LEDGER_ERROR);
            }

            if (uphold_wallet->status ==
                    mojom::WalletStatus::DISCONNECTED_VERIFIED ||
                uphold_wallet->status == mojom::WalletStatus::VERIFIED) {
              BLOG(0, "Wallet unlinking failed!");
              return callback(result);
            }
          }

          ledger_->uphold()->DisconnectWallet({});
          ledger_->state()->ResetWalletType();
          callback(mojom::Result::LEDGER_OK);
        });
  }

  if (wallet_type == constant::kWalletBitflyer) {
    promotion_server_->delete_claim()->Request(
        constant::kWalletBitflyer,
        [this, callback](const mojom::Result result) {
          if (result != mojom::Result::LEDGER_OK) {
            BLOG(0, "Wallet unlinking failed");
            callback(result);
            return;
          }
          ledger_->bitflyer()->DisconnectWallet(true);
          ledger_->state()->ResetWalletType();
          callback(mojom::Result::LEDGER_OK);
        });
    return;
  }

  if (wallet_type == constant::kWalletGemini) {
    promotion_server_->delete_claim()->Request(
        constant::kWalletGemini, [this, callback](const mojom::Result result) {
          if (result != mojom::Result::LEDGER_OK) {
            BLOG(0, "Wallet unlinking failed");
            callback(result);
            return;
          }
          ledger_->gemini()->DisconnectWallet(true);
          ledger_->state()->ResetWalletType();
          callback(mojom::Result::LEDGER_OK);
        });
    return;
  }

  NOTREACHED();
  callback(mojom::Result::LEDGER_OK);
}

void Wallet::DisconnectAllWallets(ledger::LegacyResultCallback callback) {
  DisconnectWallet(constant::kWalletUphold, [](const mojom::Result result) {});
  DisconnectWallet(constant::kWalletBitflyer,
                   [](const mojom::Result result) {});
  DisconnectWallet(constant::kWalletGemini, [](const mojom::Result result) {});
  callback(mojom::Result::LEDGER_OK);
}

mojom::RewardsWalletPtr Wallet::GetWallet(bool* corrupted) {
  DCHECK(corrupted);
  *corrupted = false;

  const std::string json =
      ledger_->ledger_client()->GetStringState(state::kWalletBrave);

  if (json.empty()) {
    return nullptr;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    BLOG(0, "Parsing of brave wallet failed");
    *corrupted = true;
    return nullptr;
  }

  auto wallet = ledger::mojom::RewardsWallet::New();

  const base::Value::Dict& dict = value->GetDict();
  const auto* payment_id = dict.FindString("payment_id");
  if (!payment_id) {
    *corrupted = true;
    return nullptr;
  }
  wallet->payment_id = *payment_id;

  const auto* seed = dict.FindString("recovery_seed");
  if (!seed) {
    *corrupted = true;
    return nullptr;
  }
  std::string decoded_seed;
  if (!base::Base64Decode(*seed, &decoded_seed)) {
    BLOG(0, "Problem decoding recovery seed");
    *corrupted = true;
    return nullptr;
  }

  std::vector<uint8_t> vector_seed;
  vector_seed.assign(decoded_seed.begin(), decoded_seed.end());
  wallet->recovery_seed = vector_seed;

  if (const auto* geo_country = dict.FindString("geo_country")) {
    wallet->geo_country = *geo_country;
  }

  return wallet;
}

mojom::RewardsWalletPtr Wallet::GetWallet() {
  bool corrupted;
  return GetWallet(&corrupted);
}

bool Wallet::SetWallet(mojom::RewardsWalletPtr wallet) {
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return false;
  }

  const std::string seed_string = base::Base64Encode(wallet->recovery_seed);
  std::string event_string;
  if (wallet->recovery_seed.size() > 1) {
    event_string =
        std::to_string(wallet->recovery_seed[0] + wallet->recovery_seed[1]);
  }

  base::Value::Dict new_wallet;
  new_wallet.Set("payment_id", wallet->payment_id);
  new_wallet.Set("recovery_seed", seed_string);
  new_wallet.Set("geo_country", wallet->geo_country);

  std::string json;
  base::JSONWriter::Write(new_wallet, &json);

  ledger_->ledger_client()->SetStringState(state::kWalletBrave, json);

  ledger_->database()->SaveEventLog(state::kRecoverySeed, event_string);

  if (!wallet->payment_id.empty()) {
    ledger_->database()->SaveEventLog(state::kPaymentId, wallet->payment_id);
  }

  return true;
}

void Wallet::LinkRewardsWallet(const std::string& destination_payment_id,
                               ledger::PostSuggestionsClaimCallback callback) {
  promotion_server_->post_claim_brave()->Request(
      destination_payment_id,
      base::BindOnce(&Wallet::OnClaimWallet, base::Unretained(this),
                     std::move(callback)));
}

void Wallet::OnClaimWallet(ledger::PostSuggestionsClaimCallback callback,
                           mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK &&
      result != mojom::Result::ALREADY_EXISTS) {
    std::move(callback).Run(result, "");
    return;
  }

  ledger_->promotion()->TransferTokens(std::move(callback));
}

}  // namespace wallet
}  // namespace ledger
