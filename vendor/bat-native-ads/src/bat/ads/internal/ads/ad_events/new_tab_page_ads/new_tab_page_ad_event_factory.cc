/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_factory.h"

#include "base/check.h"
#include "bat/ads/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_clicked.h"
#include "bat/ads/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_served.h"
#include "bat/ads/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_viewed.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads::new_tab_page_ads {

std::unique_ptr<AdEventInterface<NewTabPageAdInfo>> AdEventFactory::Build(
    const mojom::NewTabPageAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  switch (event_type) {
    case mojom::NewTabPageAdEventType::kServed: {
      return std::make_unique<AdEventServed>();
    }

    case mojom::NewTabPageAdEventType::kViewed: {
      return std::make_unique<AdEventViewed>();
    }

    case mojom::NewTabPageAdEventType::kClicked: {
      return std::make_unique<AdEventClicked>();
    }
  }
}

}  // namespace ads::new_tab_page_ads
