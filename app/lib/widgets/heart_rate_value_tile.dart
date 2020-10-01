import 'package:flutter/material.dart';
import 'package:waveglide/widgets/value_tile.dart';

class HeartRateValueTile extends ValueTile {

  int intValue;
  String description;
  Color color;
  double size;

  HeartRateValueTile(this.intValue, this.size) {
    this.description = "HR [b/m]";

    if(this.intValue == null) {
      this.color = Colors.grey;
    } else if(this.intValue >= 60 && this.intValue <= 120) {
      this.color = Colors.green;
    } else if(this.intValue > 50 && this.intValue < 60) {
      this.color = Colors.orange;
    } else if(this.intValue > 120 && this.intValue < 150) {
      this.color = Colors.orange;
    } else {
      this.color = Colors.red;
    }


    this.value = this.intValue;
  }

}