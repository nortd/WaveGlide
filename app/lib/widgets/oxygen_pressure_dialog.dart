import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

class OxygenPressureDialog extends StatefulWidget {

  const OxygenPressureDialog({Key key}) : super(key: key);

  @override
  _OxygenPressureDialogState createState() => _OxygenPressureDialogState();
}

class _OxygenPressureDialogState extends State<OxygenPressureDialog> {
  double _oxygenBottleRemainingPressure;
  SharedPreferences _preferences;

  @override
  void initState() {
    super.initState();
    _getData();
  }

  void _getData() async {
    _preferences = await SharedPreferences.getInstance();

    _oxygenBottleRemainingPressure = _preferences.getDouble("oxygenBottleRemainingPressure") ?? 200;

    setState((){});
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: Text("Select current pressure of oxygen bottle"),
      content: _preferences == null ? Text('Loading...', style: TextStyle(color: Colors.white)) : Container(
        height: 100,
        child: Column(

          children: <Widget>[
            Text(_oxygenBottleRemainingPressure.round().toString() + " bar"),
            Slider(
              value: _oxygenBottleRemainingPressure,
              min: 0,
              max: 200,
              onChanged: (value) {
                setState(() {
                  _oxygenBottleRemainingPressure = value;
                });
              },
            ),
          ],
        )
      ),
      actions: <Widget>[ FlatButton(
        child: Text("Save"),
        onPressed: () {
          _preferences.setDouble("oxygenBottleRemainingPressure", _oxygenBottleRemainingPressure);

          Navigator.pop(context, _oxygenBottleRemainingPressure.round().toDouble());
        },
      ),
      ],
    );
  }
}