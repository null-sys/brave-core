/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 import '../settings_shared.css.js';
 import '../settings_vars.css.js';

 import {html, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
 import {WebUIListenerMixin} from 'chrome://resources/js/web_ui_listener_mixin.js';
 import {SettingsPinShortcutPageBrowserProxyImpl} from './pin_shortcut_page_browser_proxy.m.js';

 const SettingsPinShortcutPageBase = WebUIListenerMixin(PolymerElement)

 class SettingsPinShortcutPage extends SettingsPinShortcutPageBase {
   static get is() {
     return 'settings-pin-shortcut-page'
   }

   static get template() {
     return html`{__html_template__}`
   }

   static get properties() {
     return {
        pinned_: {
         readOnly: false,
         type: Boolean
       }
     }
   }

   browserProxy_ = SettingsPinShortcutPageBrowserProxyImpl.getInstance()

   ready() {
     super.ready()
     this.pinned_ = false

     this.browserProxy_.checkShortcutPinState()
     this.addWebUIListener('shortcut-pin-state-changed', pinned => this.set('pinned_', pinned))
   }

   onPinShortcutTap_() {
     this.browserProxy_.pinShortcut()
   }
 }

 customElements.define(SettingsPinShortcutPage.is, SettingsPinShortcutPage);