import 'package:flutter/material.dart';
import 'package:waveglide/widgets/value_tile.dart';

class BreathCycleTimeValueTile extends ValueTile {

  int intValue;
  String description;
  Color color;
  double size;

  BreathCycleTimeValueTile(this.intValue, this.size) {
    this.description = "Breath C. [ms]";

    if(this.intValue == null) {
      this.color = Colors.grey;
    } else if(this.intValue >= 1500 && this.intValue <= 4000) {
      this.color = Colors.green;
    } else if(this.intValue > 1000 && this.intValue < 1500) {
      this.color = Colors.orange;
    } else {
      this.color = Colors.red;
    }


    this.value = this.intValue;
  }

}