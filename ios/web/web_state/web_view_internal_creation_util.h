// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_WEB_STATE_WEB_VIEW_INTERNAL_CREATION_UTIL_H_
#define IOS_WEB_WEB_STATE_WEB_VIEW_INTERNAL_CREATION_UTIL_H_

#import <UIKit/UIKit.h>

@class WKWebView;
@class WKWebViewConfiguration;

// This file is a collection of functions that vend web views.
namespace web {
class BrowserState;

// Returns a new UIWebView for displaying regular web content.
// Note: Callers are responsible for releasing the returned UIWebView.
// DEPRECATED: Please use the WKWebView equivalent instead.
UIWebView* CreateWebView(CGRect frame);

// Creates a new WKWebView for displaying regular web content and registers a
// user agent for it.
//
// Preconditions for creation of a WKWebView:
// 1) |browser_state|, |configuration| are not null.
// 2) web::BrowsingDataPartition is synchronized.
// 3) The WKProcessPool of the configuration is the same as the WKProcessPool
//    of the WKWebViewConfiguration associated with |browser_state|.
//
// Note: Callers are responsible for releasing the returned WKWebView.
WKWebView* CreateWKWebView(CGRect frame,
                           WKWebViewConfiguration* configuration,
                           BrowserState* browser_state,
                           BOOL use_desktop_user_agent);

// Creates and returns a new WKWebView for displaying regular web content.
// The preconditions for the creation of a WKWebView are the same as the
// previous method.
// Note: Callers are responsible for releasing the returned WKWebView.
WKWebView* CreateWKWebView(CGRect frame,
                           WKWebViewConfiguration* configuration,
                           BrowserState* browser_state);

}  // namespace web

#endif  // IOS_WEB_WEB_STATE_WEB_VIEW_INTERNAL_CREATION_UTIL_H_
