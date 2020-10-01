import 'dart:convert';

class WaveglideDevice {
  String deviceId;
  String deviceName;

  WaveglideDevice(this.deviceId, this.deviceName);

  String getShortId() {
    if(deviceId != null) {
      return deviceId.substring(0, 18);
    }
    return '-';
  }

  Map<String, dynamic> toJson() => {
    'deviceId': deviceId,
    'deviceName': deviceName,
  };

  WaveglideDevice.fromJson(Map<String, dynamic> json)
      : deviceId = json['deviceId'],
        deviceName = json['deviceName'];

  String toJsonString() {
    return json.encode(toJson());
  }

  WaveglideDevice.fromJsonString(String jsonString) : this.fromJson(json.decode(jsonString));
}