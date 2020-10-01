import 'package:flutter/material.dart';
import 'package:waveglide/widgets/value_tile.dart';

class SpO2ValueTile extends ValueTile {

  int intValue;
  String description;
  Color color;
  double size;

  SpO2ValueTile(this.intValue, this.size) {
    this.description = "SpO2 [%]";
    if (this.intValue == null) {
      this.color = Colors.orange;
    } else if(this.intValue >= 90) {
      this.color = Colors.green;
    } else if(this.intValue >= 80) {
      this.color = Colors.orange;
    } else {
      this.color = Colors.red;
    }


    this.value = this.intValue;
  }

}