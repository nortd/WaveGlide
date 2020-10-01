import 'package:flutter/material.dart';
import 'package:waveglide/widgets/value_tile.dart';

class BreathStrengthPercentageValueTile extends ValueTile {

  int intValue;
  String description;
  Color color;
  double size;

  BreathStrengthPercentageValueTile(this.intValue, this.size) {
    this.description = "Breath S. [%]";

    if(this.intValue == null) {
      this.color = Colors.grey;
    } else if(this.intValue >= 50 && this.intValue <= 100) {
      this.color = Colors.green;
    } else if(this.intValue > 15 && this.intValue < 50) {
      this.color = Colors.orange;
    } else {
      this.color = Colors.red;
    }


    this.value = this.intValue;
  }

}