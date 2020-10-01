import 'package:flutter_blue/flutter_blue.dart';

class SerialServiceConfig {

  final Guid serviceId;
  final Guid txCharacteristicId;
  final Guid rxCharacteristicId;

  final int lengthOfCharacteristic;

  SerialServiceConfig(this.serviceId, this.txCharacteristicId, this.rxCharacteristicId,
      [this.lengthOfCharacteristic = 20]);
}

/// Configuration for the Nordic UART Service
class NordicSerialServiceConfig extends SerialServiceConfig {
  NordicSerialServiceConfig()
      : super(
      Guid('6E400001-B5A3-F393­E0A9­E50E24DCCA9E'),
      Guid('6E400002-B5A3-F393­E0A9­E50E24DCCA9E'),
      Guid('6E400003-B5A3-F393-E0A9­E50E24DCCA9E'),
      20);
}