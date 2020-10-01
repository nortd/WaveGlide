import 'package:flutter/material.dart';
import 'package:waveglide/widgets/value_tile.dart';

class OxygenPulsePercentageValueTile extends ValueTile {

  int intValue;
  String description;
  Color color;
  double size;

  OxygenPulsePercentageValueTile(this.intValue, this.size) {
    this.description = "O2 Pulse [%]";

    if(this.intValue == null) {
      this.color = Colors.grey;
    } else if(this.intValue >= 0 && this.intValue <= 40) {
      this.color = Colors.green;
    } else if(this.intValue > 40 && this.intValue < 80) {
      this.color = Colors.orange;
    } else {
      this.color = Colors.red;
    }


    this.value = this.intValue;
  }

}