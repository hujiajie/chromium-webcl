// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_gatt_characteristic.h"

#include "base/logging.h"
#include "device/bluetooth/bluetooth_gatt_descriptor.h"

namespace device {

BluetoothGattCharacteristic::BluetoothGattCharacteristic() {
}

BluetoothGattCharacteristic::~BluetoothGattCharacteristic() {
}

// static
BluetoothGattCharacteristic* BluetoothGattCharacteristic::Create(
    const BluetoothUUID& uuid,
    const std::vector<uint8_t>& value,
    Properties properties,
    Permissions permissions) {
  LOG(ERROR) << "Creating local GATT characteristics currently not supported.";
  return NULL;
}

std::vector<BluetoothGattDescriptor*>
BluetoothGattCharacteristic::GetDescriptorsByUUID(const BluetoothUUID& uuid) {
  std::vector<BluetoothGattDescriptor*> descriptors;
  for (BluetoothGattDescriptor* descriptor : GetDescriptors()) {
    if (descriptor->GetUUID() == uuid) {
      descriptors.push_back(descriptor);
    }
  }
  return descriptors;
}

}  // namespace device
