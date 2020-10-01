import 'package:flutter/material.dart';
import 'package:waveglide/data/data_item.dart';
import 'package:waveglide/util/time_util.dart';
import 'package:waveglide/util/toast_util.dart';
import 'package:waveglide/widgets/value_tile.dart';

class AltitudeValueTile extends StatefulWidget {

  Stream<dynamic> _bleDataStream;
  double size;

  AltitudeValueTile(this._bleDataStream, this.size);
  @override
  State<StatefulWidget> createState() => _AltitudeValueTileState(this._bleDataStream, this.size);
}

class _AltitudeValueTileState extends State<AltitudeValueTile> {

  Stream<dynamic> _bleDataStream;
  int _flValue;
  String _value;

  String description;
  Color color = Colors.orange;
  double size;
  bool flMode = true;

  _AltitudeValueTileState(this._bleDataStream, this.size) {
    this.description = "Altitude [FL]";

  }

  @override
  void initState() {
    super.initState();

    _bleDataStream.listen((dataItem) {
      if (dataItem.type == DataItemType.flightLevel) {
        setState(() {
          _flValue = dataItem.value.toInt();

          if (this._flValue == null) {
            this.color = Colors.grey;
          } else if(this._flValue <= 130) {
            this.color = Colors.green;
          } else if(this._flValue <= 180) {
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
    if(flMode) {
      this.description = "Altitude [FL]";

      this._value = _flValue.toString();
    } else {
      this.description = "Altitude [m]";

      this._value = ((_flValue * 100).toDouble() * 0.3048).round().toString();
    }
  }



  void _toggleMode() {
    flMode = !flMode;
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