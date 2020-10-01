import 'package:flutter/material.dart';
import 'package:waveglide/data/data_item.dart';
import 'package:waveglide/util/time_util.dart';
import 'package:waveglide/widgets/value_tile.dart';

class RemainingOxygenValueTile extends StatefulWidget {

  Stream<dynamic> _bleDataStream;
  double size;

  RemainingOxygenValueTile(this._bleDataStream, this.size);
  @override
  State<StatefulWidget> createState() => _RemainingOxygenValueTileState(this._bleDataStream, this.size);
}

class _RemainingOxygenValueTileState extends State<RemainingOxygenValueTile> {

  final int fullBatteryChargeSeconds = 10 * 3600;
  Stream<dynamic> _bleDataStream;
  int _percentageValue;
  String _value;

  String description;
  Color color = Colors.orange;
  double size;
  OxygenUnit mode = OxygenUnit.time;

  _RemainingOxygenValueTileState(this._bleDataStream, this.size) {
    this.description = "Oxygen [h]";

  }

  @override
  void initState() {
    super.initState();

    _bleDataStream.listen((dataItem) {
      /*if (dataItem.type == DataItemType.oxygenTotal) {
        setState(() {
          _percentageValue = dataItem.value.toInt() % 100;

          if (this._percentageValue == null) {
            this.color = Colors.orange;
          } else if(this._percentageValue >= 60) {
            this.color = Colors.green;
          } else if(this._percentageValue >= 30) {
            this.color = Colors.orange;
          } else {
            this.color = Colors.red;
          }

          _updateValue();


        });
      }*/
    });
  }

  void _updateValue() {
    if(mode == OxygenUnit.percentage) {
      this.description = "Oxygen [%]";

      this._value = _percentageValue.toString();
    } else if(mode == OxygenUnit.time) {
      this.description = "Oxygen [h]";

      this._value = _percentageValue.toString();
    }  else if(mode == OxygenUnit.volume) {
      this.description = "Oxygen [L]";

      this._value = _percentageValue.toString();
    }  else if(mode == OxygenUnit.pressure) {
      this.description = "Oxygen [bar]";

      this._value = _percentageValue.toString();
    }
  }


  void _toggleMode() {
    switch(mode) {
      case OxygenUnit.time: mode = OxygenUnit.percentage; return;
      case OxygenUnit.percentage: mode = OxygenUnit.volume; return;
      case OxygenUnit.volume: mode = OxygenUnit.pressure; return;
      case OxygenUnit.pressure: mode = OxygenUnit.time; return;
    }
    setState(() {
      _updateValue();
    });
  }

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () {
        _toggleMode();
      },
      child: ValueTile(
        value: this._value,
        color: this.color,
        size: this.size,
        description: this.description,
      )
    );
  }

}

enum OxygenUnit {
  percentage,
  time,
  volume,
  pressure,
}