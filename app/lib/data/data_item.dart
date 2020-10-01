class DataItem {
  DataItemType type;
  double value;

  DataItem(this.value, this.type);

  DataItem.asInt(int intValue, this.type) {
    this.value = intValue.toDouble();
  }
}

enum DataItemType {
  breathStrengthPercentage,
  respirationPeriodMs,
  oxygenPercentage,
  oxygenTotal,
  flightLevel,
  batteryPercentage,
  pulsoxySignalStrength,
  pulsoxyHeartrate,
  pulsoxySpO2,
  pulsoxyStatus,
  status
}