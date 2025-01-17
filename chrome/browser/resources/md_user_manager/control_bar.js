// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'control-bar' is the horizontal bar at the bottom of the user
 * manager screen.
 */
 Polymer({
  is: 'control-bar',

  properties: {
    /**
     * True if 'Browse as Guest' button is displayed.
     * @type {boolean}
     */
    showGuest: {
      type: Boolean,
      value: false
    },

    /**
     * True if 'Add Person' button is displayed.
     * @type {boolean}
     */
    showAddPerson: {
      type: Boolean,
      value: false
    },

    /** @private {!signin.ProfileBrowserProxy} */
    browserProxy_: Object
  },

  /** @override */
  created: function() {
    this.browserProxy_ = signin.ProfileBrowserProxyImpl.getInstance();
  },

  /**
   * Handler for 'Browse as Guest' button click event.
   * @param {!Event} event
   * @private
   */
  onLaunchGuestTap_: function(event) {
    this.browserProxy_.launchGuestUser();
  },

  /**
   * Handler for 'Add Person' button click event.
   * @param {!Event} event
   * @private
   */
  onAddUserTap_: function(event) {
    // Event is caught by user-manager-pages.
    this.fire('change-page', {page: 'create-user-page'});
  }
});
