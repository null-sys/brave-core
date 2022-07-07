/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_H_

#include <memory>
#include <string>

#include "base/observer_list.h"
#include "bat/ads/ads_callback.h"
#include "bat/ads/internal/ads/serving/inline_content_ad_serving_observer.h"
#include "bat/ads/internal/segments/segments_aliases.h"

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace resource {
class AntiTargeting;
}  // namespace resource

struct InlineContentAdInfo;

namespace inline_content_ads {

class EligibleAdsBase;

class Serving final {
 public:
  Serving(geographic::SubdivisionTargeting* subdivision_targeting,
          resource::AntiTargeting* anti_targeting_resource);
  ~Serving();
  Serving(const Serving&) = delete;
  Serving& operator=(const Serving&) = delete;

  void AddObserver(ServingObserver* observer);
  void RemoveObserver(ServingObserver* observer);

  void MaybeServeAd(const std::string& dimensions,
                    MaybeServeInlineContentAdCallback callback);

 private:
  bool IsSupported() const;

  bool ServeAd(const InlineContentAdInfo& ad,
               MaybeServeInlineContentAdCallback callback) const;
  void FailedToServeAd(const std::string& dimensions,
                       MaybeServeInlineContentAdCallback callback);
  void ServedAd(const InlineContentAdInfo& ad);

  void NotifyOpportunityAroseToServeInlineContentAd(
      const SegmentList& segments) const;
  void NotifyDidServeInlineContentAd(const InlineContentAdInfo& ad) const;
  void NotifyFailedToServeInlineContentAd() const;

  base::ObserverList<ServingObserver> observers_;

  std::unique_ptr<EligibleAdsBase> eligible_ads_;
};

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_H_