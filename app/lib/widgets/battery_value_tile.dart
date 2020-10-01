import 'package:flutter/material.dart';
import 'package:waveglide/data/data_item.dart';
import 'package:waveglide/util/time_util.dart';
import 'package:waveglide/util/toast_util.dart';
import 'package:waveglide/widgets/value_tile.dart';

class BatteryValueTile extends StatefulWidget {

  Stream<dynamic> _bleDataStream;
  double size;

  BatteryValueTile(this._bleDataStream, this.size);
  @override
  State<StatefulWidget> createState() => _BatteryValueTileState(this._bleDataStream, this.size);
}

class _BatteryValueTileState extends State<BatteryValueTile> {

  final int fullBatteryChargeSeconds = 10 * 3600;
  Stream<dynamic> _bleDataStream;
  int _percentageValue;
  String _value;

  String description;
  Color color = Colors.orange;
  double size;
  bool percentageMode = true;

  _BatteryValueTileState(this._bleDataStream, this.size) {
    this.description = "Battery [%]";

  }

  @override
  void initState() {
    super.initState();

    _bleDataStream.listen((dataItem) {
      if (dataItem.type == DataItemType.batteryPercentage) {
        if(!mounted) {
          return;
        }
        setState(() {
          _percentageValue = dataItem.value.toInt();

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
      }
    });
  }

  void _updateValue() {
    if(percentageMode) {
      this.description = "Battery [%]";

      this._value = _percentageValue.toString();
    } else {
      this.description = "Battery [h]";

      int remainingBatterySeconds = (fullBatteryChargeSeconds * (_percentageValue.toDouble() / 100.0)).round();

      this._value = TimeUtil.secondsToHM(remainingBatterySeconds);
    }
  }



  void _toggleMode() {
    percentageMode = !percentageMode;
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
      onLongPress: () {
        ToastUtil.showSimpleToast("Shows remaining battery capacity in percentage or time", long: true);
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