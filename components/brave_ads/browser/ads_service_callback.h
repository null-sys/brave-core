/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_

#include <string>

#include "base/callback.h"  // IWYU pragma: keep
#include "base/values.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom-forward.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

using GetDiagnosticsCallback =
    base::OnceCallback<void(absl::optional<base::Value::List>)>;

using GetStatementOfAccountsCallback =
    base::OnceCallback<void(ads::mojom::StatementInfoPtr)>;

using MaybeServeInlineContentAdCallback =
    base::OnceCallback<void(const std::string&,
                            absl::optional<base::Value::Dict>)>;

using PurgeOrphanedAdEventsForTypeCallback =
    base::OnceCallback<void(const bool)>;

using GetHistoryCallback = base::OnceCallback<void(base::Value::List)>;

using ToggleAdThumbUpCallback = base::OnceCallback<void(base::Value::Dict)>;
using ToggleAdThumbDownCallback = base::OnceCallback<void(base::Value::Dict)>;
using ToggleAdOptInCallback = base::OnceCallback<void(const std::string&, int)>;
using ToggleAdOptOutCallback =
    base::OnceCallback<void(const std::string&, int)>;
using ToggleSavedAdCallback = base::OnceCallback<void(base::Value::Dict)>;
using ToggleFlaggedAdCallback = base::OnceCallback<void(base::Value::Dict)>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_CALLBACK_H_
