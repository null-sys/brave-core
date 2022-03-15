/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/url/url_utils.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/sys_string_conversions.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#import "net/base/mac/url_conversions.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Implementation

@implementation NSURL(Utilities)

- (nullable NSString*)brave_domainAndRegistry {
  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
        net::GURLWithNSURL(self), net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  return base::SysUTF8ToNSString(domain);
}

- (nullable NSString*)brave_baseDomainExcludingPrivateRegistries {
  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
        net::GURLWithNSURL(self), net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
  return base::SysUTF8ToNSString(domain);
}

- (bool)brave_isHostIPAddress {
  return net::GURLWithNSURL(self).HostIsIPAddress();
}

- (NSString*)brave_spec {
  return base::SysUTF8ToNSString(net::GURLWithNSURL(self).spec());
}

- (NSURL*)addingQueryParameter:(NSString*)key value:(NSString*)value {
  GURL gurl_ = net::AppendQueryParameter(net::GURLWithNSURL(self),
                   base::SysNSStringToUTF8(key),
                       base::SysNSStringToUTF8(value));
  return net::NSURLWithGURL(gurl_);
}

- (NSURL*)replacingQueryParameter:(NSString*)key value:(NSString*)value {
  GURL gurl_ = net::AppendOrReplaceQueryParameter(net::GURLWithNSURL(self),
                   base::SysNSStringToUTF8(key),
                       base::SysNSStringToUTF8(value));
  return net::NSURLWithGURL(gurl_);
}

- (nullable NSString*)valueForQueryParameter:(NSString*)key {
  std::string result;
  bool success = net::GetValueForKeyInQuery(net::GURLWithNSURL(self),
                     base::SysNSStringToUTF8(key),
                         &result);
  if (success) {
    return base::SysUTF8ToNSString(result);
  }
  return nullptr;
}

- (bool)hasScheme:(NSString*)scheme {
  return net::GURLWithNSURL(self).SchemeIs(base::SysNSStringToUTF8([scheme lowercaseString]));
}
@end