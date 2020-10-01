import 'package:flutter/material.dart';
import 'package:waveglide/widgets/value_tile.dart';

class PulsoxySignalStrengthValueTile extends ValueTile {

  int intValue;
  String description;
  Color color;
  double size;

  PulsoxySignalStrengthValueTile(this.intValue, this.size) {
    this.description = "PO Signal [%]";
    if(this.intValue == null) {
      this.color = Colors.grey;
    } else {
      if (this.intValue >= 6) {
        this.color = Colors.green;
      } else if (this.intValue >= 3) {
        this.color = Colors.orange;
      } else {
        this.color = Colors.red;
      }
      this.value = (((this.intValue.toDouble() / 7) * 100).round());
    }

  }

}